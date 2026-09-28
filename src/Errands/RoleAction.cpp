/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "RoleAction.h"

#include "AiFactory.h"
#include "Event.h"
#include "LevelUpAction.h"
#include "PlayerbotAIConfig.h"
#include "PlayerbotFactory.h"
#include "PlayerbotRepository.h"
#include "Playerbots.h"
#include "RoleRules.h"

#include <algorithm>

namespace PlayerbotsPlus
{
namespace
{
// Premade specs 0..2 are the PvE builds of trees 0..2, 3..5 their PvP builds.
constexpr int32 PremadeTrees = 3;

// Tree with the most talent points, -1 when none is spent.
int32 CurrentTab(Player* bot)
{
    std::map<uint8, uint32> tabs = AiFactory::GetPlayerSpecTabs(bot);
    if (!tabs[0] && !tabs[1] && !tabs[2])
        return -1;
    return AiFactory::GetPlayerSpecTab(bot);
}
}  // namespace

bool RoleAction::Execute(Event event)
{
    std::string const param = event.getParam();
    if (param.empty())
    {
        botAI->TellMaster(Status());
        return true;
    }

    size_t const space = param.find(' ');
    Role const role = ParseRole(param.substr(0, space));
    std::string const specName = space == std::string::npos ? "" : param.substr(space + 1);
    if (role == Role::None)
    {
        botAI->TellMaster(
            "role: whisper 'role tank', 'role heal' or 'role dps' (optionally a spec, e.g. 'role dps combat')");
        return false;
    }

    uint8 const cls = bot->getClass();
    int32 specNo = -1;
    if (!specName.empty())
    {
        specNo = PremadeSpecNo(bot, specName);
        if (specNo < 0)
            specNo = PremadeSpecNo(bot, specName + " pve");
        if (specNo < 0)
        {
            botAI->TellMaster("role: unknown spec '" + specName + "', whisper 'talents spec list'");
            return false;
        }
        if (!TabFitsRole(cls, role, specNo % PremadeTrees))
        {
            botAI->TellMaster("role: " + specName + " is not a " + RoleName(role) + " spec");
            return false;
        }
    }
    else
    {
        specNo = TabForRole(cls, role, CurrentTab(bot));
        if (specNo < 0 || sPlayerbotAIConfig.premadeSpecName[cls][specNo].empty())
        {
            botAI->TellMaster(std::string("role: my class cannot ") + RoleName(role));
            return false;
        }
    }

    std::vector<std::string> const nonCombat = botAI->GetStrategies(BOT_STATE_NON_COMBAT);
    std::vector<std::string> const dead = botAI->GetStrategies(BOT_STATE_DEAD);
    std::vector<std::string> const combat = botAI->GetStrategies(BOT_STATE_COMBAT);

    PlayerbotFactory::InitTalentsBySpecNo(bot, specNo, true);
    PlayerbotFactory(bot, bot->GetLevel()).InitPetTalents();
    botAI->ResetStrategies();  // combat strategies of the new tree
    Restore(BOT_STATE_NON_COMBAT, nonCombat);
    Restore(BOT_STATE_DEAD, dead);
    botAI->ChangeStrategy(role == Role::Dps ? "+threat" : "-threat", BOT_STATE_COMBAT);
    for (std::string const& name : combat)  // the module's own combat strategies (errands loot)
        if (name.rfind("errands", 0) == 0)
            botAI->ChangeStrategy("+" + name, BOT_STATE_COMBAT);

    std::string const& spec = sPlayerbotAIConfig.premadeSpecName[cls][specNo];
    AI_VALUE(std::string&, "chosen spec") = spec;
    PlayerbotRepository::instance().Save(botAI);
    botAI->TellMaster(std::string("role: ") + RoleName(role) + " (" + spec + ")");
    return true;
}

// Back to exactly the saved list: ResetStrategies put the defaults in place of it.
void RoleAction::Restore(BotState state, std::vector<std::string> const& saved)
{
    std::string change;
    for (std::string const& name : botAI->GetStrategies(state))
        if (std::find(saved.begin(), saved.end(), name) == saved.end())
            change += (change.empty() ? "-" : ",-") + name;
    for (std::string const& name : saved)
        change += (change.empty() ? "+" : ",+") + name;
    if (!change.empty())
        botAI->ChangeStrategy(change, state);
}

std::string RoleAction::Status()
{
    std::string role = "dps";
    if (botAI->IsTank(bot))
        role = "tank";
    else if (botAI->IsHeal(bot))
        role = "heal";
    std::string const& spec = AI_VALUE(std::string&, "chosen spec");
    return "role: " + role + (spec.empty() ? "" : " (" + spec + ")") +
           (botAI->HasStrategy("threat", BOT_STATE_COMBAT) ? ", threat" : "");
}
}  // namespace PlayerbotsPlus
