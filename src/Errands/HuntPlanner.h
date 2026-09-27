/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_HUNT_PLANNER_H
#define PLAYERBOTS_PLUS_HUNT_PLANNER_H

#include "ErrandPlanner.h"

// Pure decision logic for the "errands hunt" strategy.
namespace PlayerbotsPlus
{
struct GroupBot
{
    uint64_t guid = 0;
    bool isTank = false;
    bool hasHunt = false;
};

// The one bot allowed to pull: first tank with the strategy, else the lowest
// GUID with it. 0 when no bot has the strategy.
uint64_t ElectPuller(std::vector<GroupBot> const& bots);

struct Mob
{
    uint64_t id = 0;
    Vec3 pos;
    uint32_t level = 0;
    bool elite = false;
    bool inCombat = false;
    bool tappedByOther = false;
    bool needed = false;          // some group bot needs its kill or its quest loot
    uint32_t hostilesNearby = 0;  // other hostiles within PackRadius
};

struct HuntSnapshot
{
    bool errandsIdle = false;  // this tick's errands decision is "nothing to do"
    bool isPuller = false;
    bool groupReady = false;
    uint32_t minGroupLevel = 0;
    Vec3 masterPos;
    std::vector<Mob> mobs;
};

struct HuntConfig
{
    float radius = 45.f;  // wider than errands: mobs aggro inside ~20 yd before the master is idle
    uint32_t maxLevelAbove = 2;
    uint32_t timeoutMs = 20000;
    uint32_t blacklistMs = 60000;
};

struct HuntState
{
    uint64_t target = 0;
    uint32_t startedAt = 0;
    std::unordered_map<uint64_t, uint32_t> blacklistedAt;
    std::string lastReason;
};

// Start/Continue: attack decision.target. Abandon: gave up on the target.
Decision PlanHunt(HuntSnapshot const& snap, HuntState& state, HuntConfig const& cfg, uint32_t now);
}  // namespace PlayerbotsPlus

#endif
