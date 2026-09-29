/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_PULL_PLANNER_H
#define PLAYERBOTS_PLUS_PULL_PLANNER_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Pure planning of a group pull: where to shoot from, where to hide, who wakes up.
namespace PlayerbotsPlus
{
struct PullPoint
{
    float x = 0.f, y = 0.f, z = 0.f;
};
using PullPath = std::vector<PullPoint>;

struct PullMob
{
    uint64_t id = 0;
    PullPoint pos;
    float radius = 0.f;        // aggro radius toward the puller
    bool unavoidable = false;  // formation or assistance: comes anyway
};

struct FiringOption
{
    PullPoint pos;
    bool lineOfSight = false;
    PullPath path;  // from the puller; empty = unreachable
};

struct ReturnOption
{
    size_t firing = 0;  // index in the firing options
    PullPoint pos;
    bool hidden = false;  // out of the target's line of sight
    PullPath path;        // from the firing spot; empty = unreachable
};

struct PullRoute
{
    bool found = false;
    size_t firing = 0;
    PullPoint firingPos;
    PullPoint hidePos;
    bool hidden = false;
    std::vector<uint64_t> woken;        // avoidable mobs the route wakes
    std::vector<uint64_t> unavoidable;  // come anyway
};

constexpr float PullMargin = 3.0f;
constexpr size_t PullReturnCandidates = 5;

float PathLength(PullPath const& path);

// Avoidable mobs whose aggro radius + margin the path enters (2D), sorted.
std::vector<uint64_t> WokenBy(PullPath const& path, std::vector<PullMob> const& mobs, float margin);

// Reachable firing options with line of sight, best first: fewest woken, then shortest.
std::vector<size_t> RankFiring(std::vector<FiringOption> const& firing, std::vector<PullMob> const& mobs, float margin);

// Best go + return: fewest woken, then hidden, then shortest.
PullRoute ChooseRoute(std::vector<FiringOption> const& firing, std::vector<ReturnOption> const& returns,
                      std::vector<PullMob> const& mobs, float margin);
}  // namespace PlayerbotsPlus

#endif
