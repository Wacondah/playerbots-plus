/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "GatherPlanner.h"

#include <algorithm>
#include <utility>

namespace PlayerbotsPlus
{
namespace
{
constexpr uint32_t SkillMining = 186;
constexpr uint32_t SkillHerbalism = 182;

Decision Result(GatherState& state, DecisionType type, uint64_t target, std::string reason)
{
    state.lastReason = reason;
    return Decision{type, target, ErrandKind::None, std::move(reason)};
}

char const* Gate(GatherSnapshot const& snap, GatherConfig const& cfg)
{
    if (cfg.radius <= 0.f)
        return "disabled";
    if (!snap.idle)
        return "master busy";
    if (snap.bagsFull)
        return "bags full";
    return nullptr;
}

bool Eligible(GatherNode const& n, GatherSnapshot const& snap, GatherState const& state, GatherConfig const& cfg)
{
    return n.gatherable && !n.contested && !n.guarded && Distance(n.pos, snap.masterPos) <= cfg.radius &&
           !state.blacklistedAt.count(n.id);
}
}  // namespace

Decision PlanGather(GatherSnapshot const& snap, GatherState& state, GatherConfig const& cfg, uint32_t now)
{
    for (auto it = state.blacklistedAt.begin(); it != state.blacklistedAt.end();)
        it = Elapsed(now, it->second, cfg.blacklistMs) ? state.blacklistedAt.erase(it) : std::next(it);

    if (char const* gate = Gate(snap, cfg))
    {
        uint64_t const target = std::exchange(state.target, 0);
        return Result(state, target ? DecisionType::Abandon : DecisionType::Idle, target, gate);
    }

    if (state.target)
    {
        if (Elapsed(now, state.startedAt, cfg.timeoutMs))
        {
            uint64_t const target = std::exchange(state.target, 0);
            state.blacklistedAt[target] = now;
            return Result(state, DecisionType::Abandon, target, "timeout");
        }
        for (GatherNode const& n : snap.nodes)
            if (n.id == state.target && Eligible(n, snap, state, cfg))
                return Result(state, DecisionType::Continue, n.id, "gathering");
        state.target = 0;  // gathered, despawned or taken: next one
    }

    GatherNode const* best = nullptr;
    for (GatherNode const& n : snap.nodes)
        if (Eligible(n, snap, state, cfg) &&
            (!best || Distance(n.pos, snap.botPos) < Distance(best->pos, snap.botPos)))
            best = &n;
    if (!best)
        return Result(state, DecisionType::Idle, 0, "no node");

    state.target = best->id;
    state.startedAt = now;
    return Result(state, DecisionType::Start, best->id, "gather start");
}

void MarkGatherFailed(GatherState& state, uint32_t now)
{
    if (!state.target)
        return;
    state.blacklistedAt[std::exchange(state.target, 0)] = now;
    state.lastReason = "failed";
}

bool CanGather(uint32_t lockSkill, uint32_t skillValue, uint32_t requiredSkill, bool hasMiningPick)
{
    if (lockSkill != SkillMining && lockSkill != SkillHerbalism)
        return false;
    if (lockSkill == SkillMining && !hasMiningPick)
        return false;
    return skillValue >= std::max<uint32_t>(1, requiredSkill);
}
}  // namespace PlayerbotsPlus
