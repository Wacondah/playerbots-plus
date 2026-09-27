/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "PlayerbotsPlusConfig.h"

#include "Config.h"
#include "Define.h"

#include <algorithm>

namespace PlayerbotsPlus
{
void ModuleConfig::Load()
{
    enabled = sConfigMgr->GetOption<bool>("PlayerbotsPlus.Enable", true);
    planner.radius = sConfigMgr->GetOption<float>("PlayerbotsPlus.Errands.Radius", 20.0f);
    planner.idleDelayMs = sConfigMgr->GetOption<uint32>("PlayerbotsPlus.Errands.IdleDelay", 3000);
    planner.timeoutMs = sConfigMgr->GetOption<uint32>("PlayerbotsPlus.Errands.Timeout", 20000);
    planner.blacklistMs = sConfigMgr->GetOption<uint32>("PlayerbotsPlus.Errands.Blacklist", 60000);
    planner.inInstances = sConfigMgr->GetOption<bool>("PlayerbotsPlus.Errands.InInstances", false);
    repairThreshold = sConfigMgr->GetOption<uint32>("PlayerbotsPlus.Errands.RepairThreshold", 30);
    sellWhite = sConfigMgr->GetOption<bool>("PlayerbotsPlus.Errands.SellWhite", false);
    maxSellQuality = std::min<uint32>(sConfigMgr->GetOption<uint32>("PlayerbotsPlus.Errands.MaxSellQuality", 3), 4);
    huntMaxLevelAbove = sConfigMgr->GetOption<uint32>("PlayerbotsPlus.Hunt.MaxLevelAbove", 2);
    huntRadius = sConfigMgr->GetOption<float>("PlayerbotsPlus.Hunt.Radius", 45.0f);
    huntPackRadius = sConfigMgr->GetOption<float>("PlayerbotsPlus.Hunt.PackRadius", 8.0f);
}

ModuleConfig& Config()
{
    static ModuleConfig config;
    return config;
}

HuntConfig HuntSettings()
{
    ModuleConfig const& c = Config();
    return HuntConfig{c.huntRadius, c.huntMaxLevelAbove, c.planner.timeoutMs, c.planner.blacklistMs};
}
}  // namespace PlayerbotsPlus
