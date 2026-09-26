/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "Log.h"
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

void AddPlayerbotsPlusScripts()
{
    new PlayerbotsPlusWorldScript();
}
