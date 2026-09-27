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

void ExpireBlacklist(ErrandState& state, PlannerConfig const& cfg, uint32_t now)
{
    for (auto it = state.blacklistedAt.begin(); it != state.blacklistedAt.end();)
    {
        if (Elapsed(now, it->second, cfg.blacklistMs))
            it = state.blacklistedAt.erase(it);
        else
            ++it;
    }
}

ErrandKind BestKind(Candidate const& c, Snapshot const& snap, ErrandState const& state, PlannerConfig const& cfg,
                    uint32_t now)
{
    auto const visit = state.visits.find(c.id);
    bool const visited = visit != state.visits.end();
    // A quest giver is worth revisiting only once the bot's quest state changed.
    bool const questsChanged = !visited || visit->second.fingerprint != snap.questFingerprint;
    // A vendor rests after a visit, so an item it refuses cannot loop the bot.
    bool const vendorRested = !visited || Elapsed(now, visit->second.at, cfg.blacklistMs);

    if (questsChanged && c.canTurnIn)
        return ErrandKind::TurnIn;
    if (questsChanged && c.canAccept)
        return ErrandKind::Accept;
    if (vendorRested && snap.needsRepair && c.canRepair)
        return ErrandKind::Repair;
    if (vendorRested && snap.hasJunk && c.canSell)
        return ErrandKind::Sell;
    if (vendorRested && c.canTrain)
        return ErrandKind::Train;
    if (vendorRested && c.canTrainClass)
        return ErrandKind::TrainClass;
    if (vendorRested && c.canSellReagent)
        return ErrandKind::BuyReagents;
    if (vendorRested && c.canSellTool)
        return ErrandKind::BuyTool;
    return ErrandKind::None;
}

Decision Pick(Snapshot const& snap, ErrandState& state, PlannerConfig const& cfg, uint32_t now)
{
    Candidate const* best = nullptr;
    ErrandKind bestKind = ErrandKind::None;
    float bestDistance = 0.f;

    for (Candidate const& c : snap.candidates)
    {
        if (Distance(c.pos, snap.masterPos) > cfg.radius || state.blacklistedAt.count(c.id))
            continue;

        ErrandKind const kind = BestKind(c, snap, state, cfg, now);
        if (kind == ErrandKind::None)
            continue;

        float const distance = Distance(c.pos, snap.botPos);
        if (!best || kind < bestKind || (kind == bestKind && distance < bestDistance))
        {
            best = &c;
            bestKind = kind;
            bestDistance = distance;
        }
    }

    if (!best)
        return Idle(state, "nothing to do");

    state.active = ActiveErrand{best->id, bestKind, now};
    std::string reason = std::string("start ") + ToString(bestKind);
    state.lastReason = reason;
    return Decision{DecisionType::Start, best->id, bestKind, std::move(reason)};
}

Decision Continue(Snapshot const& snap, ErrandState& state, PlannerConfig const& cfg, uint32_t now)
{
    // Also covers unreachable targets: movement keeps failing until the deadline.
    if (Elapsed(now, state.active.startedAt, cfg.timeoutMs))
    {
        state.blacklistedAt[state.active.target] = now;
        return Abandon(state, "timeout");
    }

    for (Candidate const& c : snap.candidates)
    {
        if (c.id != state.active.target)
            continue;
        if (Distance(c.pos, snap.masterPos) > cfg.radius)
            break;
        state.lastReason = "on errand";
        return Decision{DecisionType::Continue, c.id, state.active.kind, "on errand"};
    }

    return Abandon(state, "target gone");
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
        case ErrandKind::Train: return "train";
        case ErrandKind::TrainClass: return "train class";
        case ErrandKind::BuyReagents: return "buy reagents";
        case ErrandKind::BuyTool: return "buy tool";
        default: return "none";
    }
}

Decision Plan(Snapshot const& snap, ErrandState& state, PlannerConfig const& cfg, uint32_t now)
{
    if (snap.hasMaster && snap.masterSameMap)
        TrackMaster(snap, state, now);
    ExpireBlacklist(state, cfg, now);

    if (char const* blocker = Blocker(snap, state, cfg, now))
        return state.active.IsActive() ? Abandon(state, blocker) : Idle(state, blocker);

    if (state.active.IsActive())
        return Continue(snap, state, cfg, now);

    return Pick(snap, state, cfg, now);
}

void MarkDone(ErrandState& state, uint64_t fingerprint, uint32_t now)
{
    if (!state.active.IsActive())
        return;
    state.visits[state.active.target] = Visit{fingerprint, now};
    state.active = ActiveErrand{};
    state.lastReason = "done";
}

void MarkFailed(ErrandState& state, uint32_t now)
{
    if (!state.active.IsActive())
        return;
    state.blacklistedAt[state.active.target] = now;
    state.active = ActiveErrand{};
    state.lastReason = "failed";
}
}  // namespace PlayerbotsPlus
