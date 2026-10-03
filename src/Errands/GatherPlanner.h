/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_GATHER_PLANNER_H
#define PLAYERBOTS_PLUS_GATHER_PLANNER_H

#include "ErrandPlanner.h"

// Pure decision logic of the gathering detour (mining and herb nodes near an idle master).
namespace PlayerbotsPlus
{
struct GatherNode
{
    uint64_t id = 0;
    Vec3 pos;
    bool gatherable = false;  // the bot's skill (and pick) opens it
    bool contested = false;   // another bot goes for it, or the master stands at it
    bool guarded = false;     // a hostile next to it
};

struct GatherSnapshot
{
    bool idle = false;  // the master is idle (every errands gate but the bot's distance)
    bool bagsFull = false;
    Vec3 masterPos;
    Vec3 botPos;
    std::vector<GatherNode> nodes;
};

struct GatherConfig
{
    float radius = 50.f;  // from the master; 0 disables
    uint32_t timeoutMs = 20000;
    uint32_t blacklistMs = 60000;
};

struct GatherState
{
    uint64_t target = 0;
    uint32_t startedAt = 0;
    std::unordered_map<uint64_t, uint32_t> blacklistedAt;
    std::string lastReason;
};

// Start/Continue: go and open decision.target. Abandon: gave up on it.
Decision PlanGather(GatherSnapshot const& snap, GatherState& state, GatherConfig const& cfg, uint32_t now);

// The node could not be opened on arrival: skip it for a while.
void MarkGatherFailed(GatherState& state, uint32_t now);

// Mining (with a pick) or herbalism at or above the node's required skill.
bool CanGather(uint32_t lockSkill, uint32_t skillValue, uint32_t requiredSkill, bool hasMiningPick);
}  // namespace PlayerbotsPlus

#endif
