/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "ChatCommands.h"
#include "Event.h"
#include "Group.h"
#include "LevelUpAction.h"
#include "Log.h"
#include "PlayerbotRepository.h"
#include "Playerbots.h"
#include "PlayerbotsPlusConfig.h"
#include "PlayerbotsPlusRegistry.h"
#include "PlayerbotsPlusSchema.h"
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

class PlayerbotsPlusDatabaseScript : public DatabaseScript
{
public:
    PlayerbotsPlusDatabaseScript()
        : DatabaseScript("PlayerbotsPlusDatabaseScript", {DATABASEHOOK_ON_AFTER_DATABASES_LOADED})
    {
    }

    void OnAfterDatabasesLoaded(uint32 /*updateFlags*/) override { EnsureDbStoreValueIsText(); }
};

// "errands on" / "errands off" from a master, whispered to one bot or in party chat.
// Handled here rather than by a bot strategy, so it also reaches bots with no errands
// strategy yet. The message still goes through (true), as any chat line, except "pull"
// commands the module handles (swallowed so mod-playerbots does not pull too).
class PlayerbotsPlusPlayerScript : public PlayerScript
{
public:
    PlayerbotsPlusPlayerScript()
        : PlayerScript("PlayerbotsPlusPlayerScript",
                       {PLAYERHOOK_CAN_PLAYER_USE_PRIVATE_CHAT, PLAYERHOOK_CAN_PLAYER_USE_GROUP_CHAT})
    {
    }

    using PlayerScript::OnPlayerCanUseChat;  // keep the other overloads visible

    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 lang, std::string& msg, Player* receiver) override
    {
        if (type == CHAT_MSG_WHISPER && lang == LANG_ADDON)
        {
            AnswerQuestLog(player, receiver, msg);
            return true;
        }
        if (type == CHAT_MSG_WHISPER && Config().enabled)
        {
            PullCommand const pull = ParsePullCommand(msg);
            if (pull != PullCommand::None && DispatchPull(player, receiver, pull, false))
                return false;
            Apply(player, receiver, ParseErrandsSwitch(msg));
            RememberSpec(player, receiver, ParseTalentsSpec(msg));
        }
        return true;
    }

    bool OnPlayerCanUseChat(Player* player, uint32 /*type*/, uint32 /*lang*/, std::string& msg, Group* group) override
    {
        PullCommand const pull = ParsePullCommand(msg);
        if (pull != PullCommand::None && group && Config().enabled)
        {
            bool handled = false;
            for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
                handled = DispatchPull(player, ref->GetSource(), pull, true) || handled;
            if (handled)
                return false;
        }
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
    // "pull", "pull force", "pull cancel": handled by the module's tank instead of
    // mod-playerbots' own pull (the chat line is swallowed so both do not run).
    static bool DispatchPull(Player* master, Player* bot, PullCommand command, bool party)
    {
        PlayerbotAI* botAI = bot ? GET_PLAYERBOT_AI(bot) : nullptr;
        if (!botAI || botAI->GetMaster() != master || !botAI->HasStrategy("errands pull", BOT_STATE_NON_COMBAT))
            return false;
        if (party && !PlayerbotAI::IsTank(bot))
            return false;
        char const* action = command == PullCommand::Force    ? "errands pull force"
                             : command == PullCommand::Cancel ? "errands pull cancel"
                                                              : "errands pull request";
        botAI->DoSpecificAction(action, Event("pull", party ? "party" : "", master), true);
        return true;
    }

    // The PlayerbotsPlusQuests addon asks with "BOT\t#a questlog": answered here, the
    // addon message never reaching mod-playerbots' command parsing.
    static void AnswerQuestLog(Player* master, Player* bot, std::string const& msg)
    {
        PlayerbotAI* botAI = bot ? GET_PLAYERBOT_AI(bot) : nullptr;
        if (!Config().enabled || !botAI || botAI->GetMaster() != master || msg != "BOT\t#a questlog")
            return;
        botAI->DoSpecificAction("questlog", Event("addon", "", master), true);
    }

    // "talents spec <name>"): mod-playerbots applies it now, the module keeps it for level-ups.
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
        botAI->ChangeStrategy(ErrandsCombatStrategies(on), BOT_STATE_COMBAT);
        PlayerbotRepository::instance().Save(botAI);
        botAI->TellMaster(on ? "errands: on (hunt, share, bags, craft, levelup, quests, loot, revive, pull)" : "errands: off");
    }
};

void AddPlayerbotsPlusScripts()
{
    new PlayerbotsPlusDatabaseScript();
    new PlayerbotsPlusWorldScript();
    new PlayerbotsPlusPlayerScript();
}
