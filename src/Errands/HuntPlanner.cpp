/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "HuntPlanner.h"

#include <utility>

namespace PlayerbotsPlus
{
namespace
{
Decision Result(HuntState& state, DecisionType type, uint64_t target, std::string reason)
{
    state.lastReason = reason;
    return Decision{type, target, ErrandKind::None, std::move(reason)};
}

void ExpireBlacklist(HuntState& state, HuntConfig const& cfg, uint32_t now)
{
    for (auto it = state.blacklistedAt.begin(); it != state.blacklistedAt.end();)
    {
        if (Elapsed(now, it->second, cfg.blacklistMs))
            it = state.blacklistedAt.erase(it);
        else
            ++it;
    }
}

// Every rule except the pack veto, which gets its own diagnostic.
bool Huntable(Mob const& m, HuntSnapshot const& snap, HuntState const& state, HuntConfig const& cfg)
{
    return m.needed && !m.elite && !m.inCombat && !m.tappedByOther &&
           m.level <= snap.minGroupLevel + cfg.maxLevelAbove && Distance(m.pos, snap.masterPos) <= cfg.radius &&
           !state.blacklistedAt.count(m.id);
}

char const* Gate(HuntSnapshot const& snap)
{
    if (!snap.errandsIdle)
        return "errands first";
    if (!snap.isPuller)
        return "not puller";
    if (!snap.groupReady)
        return "group not ready";
    return nullptr;
}
}  // namespace

uint64_t ElectPuller(std::vector<GroupBot> const& bots)
{
    uint64_t tank = 0;
    uint64_t lowest = 0;
    for (GroupBot const& b : bots)
    {
        if (!b.hasHunt)
            continue;
        if (b.isTank && (!tank || b.guid < tank))
            tank = b.guid;
        if (!lowest || b.guid < lowest)
            lowest = b.guid;
    }
    return tank ? tank : lowest;
}

Decision PlanHunt(HuntSnapshot const& snap, HuntState& state, HuntConfig const& cfg, uint32_t now)
{
    ExpireBlacklist(state, cfg, now);

    if (char const* gate = Gate(snap))
    {
        if (!state.target)
            return Result(state, DecisionType::Idle, 0, gate);
        uint64_t const target = state.target;
        state.target = 0;
        return Result(state, DecisionType::Abandon, target, gate);
    }

    if (state.target)
    {
        if (Elapsed(now, state.startedAt, cfg.timeoutMs))
        {
            uint64_t const target = state.target;
            state.blacklistedAt[target] = now;
            state.target = 0;
            return Result(state, DecisionType::Abandon, target, "timeout");
        }
        for (Mob const& m : snap.mobs)
            if (m.id == state.target && Huntable(m, snap, state, cfg) && !m.hostilesNearby)
                return Result(state, DecisionType::Continue, m.id, "hunting");
        // Dead, engaged or no longer needed: pick the next one.
        state.target = 0;
    }

    Mob const* best = nullptr;
    bool packVeto = false;
    for (Mob const& m : snap.mobs)
    {
        if (!Huntable(m, snap, state, cfg))
            continue;
        if (m.hostilesNearby)
        {
            packVeto = true;
            continue;
        }
        if (!best || Distance(m.pos, snap.masterPos) < Distance(best->pos, snap.masterPos))
            best = &m;
    }

    if (!best)
        return Result(state, DecisionType::Idle, 0, packVeto ? "pack nearby" : "no quest mob");

    state.target = best->id;
    state.startedAt = now;
    return Result(state, DecisionType::Start, best->id, "hunt start");
}
}  // namespace PlayerbotsPlus
