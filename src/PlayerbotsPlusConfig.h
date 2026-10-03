/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_CONFIG_H
#define PLAYERBOTS_PLUS_CONFIG_H

#include "ErrandPlanner.h"
#include "HuntPlanner.h"
#include "GatherPlanner.h"

#include <cstdint>

namespace PlayerbotsPlus
{
struct ModuleConfig
{
    bool enabled = true;
    PlannerConfig planner;
    uint32_t repairThreshold = 30;
    bool sellWhite = false;
    uint32_t maxSellQuality = 3;  // 0 grey .. 4 epic
    uint32_t maxDisenchantQuality = 3;
    bool sellFood = true;
    uint32_t huntMaxLevelAbove = 2;
    float huntRadius = 45.f;
    float huntPackRadius = 8.f;
    float gatherRadius = 50.f;  // gathering detour around the idle master (0: off)
    uint32_t bagsMinFreeSlots = 1;
    uint32_t shoppingReservePer10Levels = 10000;
    uint32_t shoppingMaxCopper = 5000;
    uint32_t shoppingMaxCrafts = 20;
    uint32_t gearMaxLevelAhead = 0;  // future gear kept up to this many levels ahead (0: no limit)
    bool cityAuto = true;
    uint32_t cityTimeoutMs = 180000;
    uint32_t cityCooldownMs = 600000;

    void Load();
};

ModuleConfig& Config();
HuntConfig HuntSettings();
GatherConfig GatherSettings();
}  // namespace PlayerbotsPlus

#endif
