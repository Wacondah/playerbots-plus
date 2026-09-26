/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_CONFIG_H
#define PLAYERBOTS_PLUS_CONFIG_H

#include "ErrandPlanner.h"

#include <cstdint>

namespace PlayerbotsPlus
{
struct ModuleConfig
{
    bool enabled = true;
    PlannerConfig planner;
    uint32_t repairThreshold = 30;
    bool sellWhite = false;

    void Load();
};

ModuleConfig& Config();
}  // namespace PlayerbotsPlus

#endif
