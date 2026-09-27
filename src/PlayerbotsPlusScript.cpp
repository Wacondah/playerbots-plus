/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "ChatCommands.h"
#include "Group.h"
#include "LevelUpAction.h"
#include "Log.h"
#include "PlayerbotRepository.h"
#include "Playerbots.h"
#include "PlayerbotsPlusConfig.h"
#include "PlayerbotsPlusRegistry.h"
#include "ReagentIndex.h"
#include "ScriptMgr.h"

using namespace PlayerbotsPlus;

class PlayerbotsPlusWorldScript : public WorldScript
{
public:
    PlayerbotsPlusWorldScript()
        : WorldScript("PlayerbotsPlusWorldScript", {WORLDHOOK_ON_AFTER_CONFIG_LOAD, WORLDHOOK_ON_STARTUP})
    {
    }

    void OnAfterConfigLoad(bool /*reload*/) override { Config().Load(); }

    void OnStartup() override
    {
        if (!Config().enabled)
        {
            LOG_INFO("server.loading", ">> playerbots-plus: disabled by config");
            return;
        }
        ReagentIndex::Build();
        if (EnsureRegistered())
            LOG_INFO("server.loading", ">> playerbots-plus: errands registered");
        else
            LOG_ERROR("server.loading", "playerbots-plus: mod-playerbots contexts are empty, module inactive");
    }
};

// "errands on" / "errands off" from a master, whispered to one bot or in party chat.
// Handled here rather than by a bot strategy, so it also reaches bots with no errands
// strategy yet. The message still goes through (true), as any chat line.
class PlayerbotsPlusPlayerScript : public PlayerScript
{
public:
    PlayerbotsPlusPlayerScript()
        : PlayerScript("PlayerbotsPlusPlayerScript",
                       {PLAYERHOOK_CAN_PLAYER_USE_PRIVATE_CHAT, PLAYERHOOK_CAN_PLAYER_USE_GROUP_CHAT})
    {
    }

    using PlayerScript::OnPlayerCanUseChat;  // keep the other overloads visible

    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 /*lang*/, std::string& msg, Player* receiver) override
    {
        if (type == CHAT_MSG_WHISPER && Config().enabled)
        {
            Apply(player, receiver, ParseErrandsSwitch(msg));
            RememberSpec(player, receiver, ParseTalentsSpec(msg));
        }
        return true;
    }

    bool OnPlayerCanUseChat(Player* player, uint32 /*type*/, uint32 /*lang*/, std::string& msg, Group* group) override
    {
        ErrandsSwitch const command = ParseErrandsSwitch(msg);
        std::string const spec = ParseTalentsSpec(msg);
        if ((command == ErrandsSwitch::None && spec.empty()) || !group || !Config().enabled)
            return true;
        for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
        {
            Apply(player, ref->GetSource(), command);
            RememberSpec(player, ref->GetSource(), spec);
        }
        return true;
    }

private:
    // "talents spec <name>": mod-playerbots applies it now, the module keeps it for level-ups.
    static void RememberSpec(Player* master, Player* bot, std::string const& spec)
    {
        PlayerbotAI* botAI = bot ? GET_PLAYERBOT_AI(bot) : nullptr;
        if (spec.empty() || !botAI || botAI->GetMaster() != master || PremadeSpecNo(bot, spec) < 0)
            return;
        botAI->GetAiObjectContext()->GetValue<std::string&>("chosen spec")->Get() = spec;
        PlayerbotRepository::instance().Save(botAI);
    }

    static void Apply(Player* master, Player* bot, ErrandsSwitch command)
    {
        PlayerbotAI* botAI = bot ? GET_PLAYERBOT_AI(bot) : nullptr;
        if (command == ErrandsSwitch::None || !botAI || botAI->GetMaster() != master)
            return;
        bool const on = command == ErrandsSwitch::On;
        botAI->ChangeStrategy(ErrandsStrategies(on), BOT_STATE_NON_COMBAT);
        botAI->ChangeStrategy(ErrandsDeadStrategies(on), BOT_STATE_DEAD);
        PlayerbotRepository::instance().Save(botAI);
        botAI->TellMaster(on ? "errands: on (hunt, share, bags, craft, levelup, quests, loot, revive)" : "errands: off");
    }
};

void AddPlayerbotsPlusScripts()
{
    new PlayerbotsPlusWorldScript();
    new PlayerbotsPlusPlayerScript();
}
