/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "ErrandPlanner.h"

#include <cmath>
#include <utility>

namespace PlayerbotsPlus
{
namespace
{
// Below this, a master position change is jitter, not movement.
constexpr float MasterMoveEpsilon = 0.5f;

bool Elapsed(uint32_t now, uint32_t since, uint32_t duration)
{
    return static_cast<uint32_t>(now - since) >= duration;
}

Decision Idle(ErrandState& state, std::string reason)
{
    state.lastReason = reason;
    return Decision{DecisionType::Idle, 0, ErrandKind::None, std::move(reason)};
}

Decision Abandon(ErrandState& state, std::string reason)
{
    Decision decision{DecisionType::Abandon, state.active.target, state.active.kind, reason};
    state.active = ActiveErrand{};
    state.lastReason = std::move(reason);
    return decision;
}

void TrackMaster(Snapshot const& snap, ErrandState& state, uint32_t now)
{
    if (!state.masterPosKnown || snap.masterMoving ||
        Distance(snap.masterPos, state.lastMasterPos) > MasterMoveEpsilon)
    {
        state.masterPosKnown = true;
        state.lastMasterPos = snap.masterPos;
        state.masterStillSince = now;
    }
}

// Why the bot may not run errands right now, or nullptr if it may.
char const* Blocker(Snapshot const& snap, ErrandState const& state, PlannerConfig const& cfg, uint32_t now)
{
    if (!snap.hasMaster)
        return "no master";
    if (!snap.masterSameMap)
        return "master on another map";
    if (snap.inInstance && !cfg.inInstances)
        return "in instance";
    if (snap.botHasStayOrGuard)
        return "stay or guard order";
    if (snap.botInCombat || snap.masterInCombat)
        return "combat";
    if (snap.botNeedsRest)
        return "resting";
    if (snap.masterMounted || snap.masterOnTaxi)
        return "master mounted";
    if (Distance(snap.botPos, snap.masterPos) > cfg.radius + cfg.margin)
        return "too far from master";
    if (!Elapsed(now, state.masterStillSince, cfg.idleDelayMs))
        return "master moving";
    return nullptr;
}
}  // namespace

float Distance(Vec3 const& a, Vec3 const& b)
{
    float const dx = a.x - b.x;
    float const dy = a.y - b.y;
    float const dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

char const* ToString(ErrandKind kind)
{
    switch (kind)
    {
        case ErrandKind::TurnIn: return "turn in";
        case ErrandKind::Accept: return "accept";
        case ErrandKind::Repair: return "repair";
        case ErrandKind::Sell: return "sell";
        default: return "none";
    }
}

Decision Plan(Snapshot const& snap, ErrandState& state, PlannerConfig const& cfg, uint32_t now)
{
    if (snap.hasMaster && snap.masterSameMap)
        TrackMaster(snap, state, now);

    if (char const* blocker = Blocker(snap, state, cfg, now))
        return state.active.IsActive() ? Abandon(state, blocker) : Idle(state, blocker);

    return Idle(state, "nothing to do");
}

void MarkDone(ErrandState& state, uint64_t fingerprint, uint32_t now)
{
    (void)state;
    (void)fingerprint;
    (void)now;
}

void MarkFailed(ErrandState& state, uint32_t now)
{
    (void)state;
    (void)now;
}
}  // namespace PlayerbotsPlus
