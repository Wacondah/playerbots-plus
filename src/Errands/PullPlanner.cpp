/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "PullPlanner.h"

#include <algorithm>
#include <cmath>
#include <tuple>

namespace PlayerbotsPlus
{
namespace
{
float Distance2d(PullPoint const& a, PullPoint const& b) { return std::hypot(a.x - b.x, a.y - b.y); }

float DistanceToSegment(PullPoint const& p, PullPoint const& a, PullPoint const& b)
{
    float const dx = b.x - a.x, dy = b.y - a.y;
    float const lengthSq = dx * dx + dy * dy;
    if (lengthSq <= 0.f)
        return Distance2d(p, a);
    float const t = std::clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) / lengthSq, 0.f, 1.f);
    return Distance2d(p, {a.x + t * dx, a.y + t * dy, 0.f});
}

bool Enters(PullPath const& path, PullMob const& mob, float margin)
{
    float const limit = mob.radius + margin;
    if (path.size() == 1)
        return Distance2d(path.front(), mob.pos) < limit;
    for (size_t i = 1; i < path.size(); ++i)
        if (DistanceToSegment(mob.pos, path[i - 1], path[i]) < limit)
            return true;
    return false;
}

std::vector<uint64_t> Unavoidable(std::vector<PullMob> const& mobs)
{
    std::vector<uint64_t> ids;
    for (PullMob const& mob : mobs)
        if (mob.unavoidable)
            ids.push_back(mob.id);
    return ids;
}
}  // namespace

float PathLength(PullPath const& path)
{
    float length = 0.f;
    for (size_t i = 1; i < path.size(); ++i)
        length += Distance2d(path[i - 1], path[i]);
    return length;
}

std::vector<uint64_t> WokenBy(PullPath const& path, std::vector<PullMob> const& mobs, float margin)
{
    std::vector<uint64_t> ids;
    if (path.empty())
        return ids;
    for (PullMob const& mob : mobs)
        if (!mob.unavoidable && Enters(path, mob, margin))
            ids.push_back(mob.id);
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    return ids;
}

std::vector<size_t> RankFiring(std::vector<FiringOption> const& firing, std::vector<PullMob> const& mobs, float margin)
{
    std::vector<std::tuple<size_t, float, size_t>> scored;  // woken, length, index
    for (size_t i = 0; i < firing.size(); ++i)
        if (firing[i].lineOfSight && !firing[i].path.empty())
            scored.emplace_back(WokenBy(firing[i].path, mobs, margin).size(), PathLength(firing[i].path), i);
    std::sort(scored.begin(), scored.end());
    std::vector<size_t> ranked;
    for (auto const& [woken, length, index] : scored)
        ranked.push_back(index);
    return ranked;
}

PullRoute ChooseRoute(std::vector<FiringOption> const& firing, std::vector<ReturnOption> const& returns,
                      std::vector<PullMob> const& mobs, float margin)
{
    PullRoute best;
    std::tuple<size_t, bool, float> bestScore{};  // woken, not hidden, length
    for (ReturnOption const& back : returns)
    {
        if (back.firing >= firing.size() || back.path.empty())
            continue;
        FiringOption const& shot = firing[back.firing];
        if (!shot.lineOfSight || shot.path.empty())
            continue;
        PullPath route = shot.path;
        route.insert(route.end(), back.path.begin(), back.path.end());
        std::vector<uint64_t> woken = WokenBy(route, mobs, margin);
        std::tuple<size_t, bool, float> const score{woken.size(), !back.hidden, PathLength(route)};
        if (best.found && !(score < bestScore))
            continue;
        best.found = true;
        best.firing = back.firing;
        best.firingPos = shot.pos;
        best.hidePos = back.pos;
        best.hidden = back.hidden;
        best.woken = std::move(woken);
        bestScore = score;
    }
    if (best.found)
        best.unavoidable = Unavoidable(mobs);
    return best;
}
}  // namespace PlayerbotsPlus
