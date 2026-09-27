/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "BagPlanner.h"

#include <utility>

namespace PlayerbotsPlus
{
namespace
{
template <class V>
V Lookup(std::unordered_map<uint32_t, V> const& map, uint32_t key)
{
    auto const it = map.find(key);
    return it == map.end() ? V{} : it->second;
}

bool Candidate(BagStack const& s)
{
    return s.movable && s.maxStack > 1 && s.count > 0;
}

// A receiver never ranks the item lower than the giver: share would send it back.
bool TierOk(BagMate const& mate, BagStack const& s)
{
    return Lookup(mate.tier, s.entry) >= s.tier;
}

// Best (stack, receiver) pair: smallest stack, then highest score, then lowest GUID.
struct Pick
{
    BagStack const* stack = nullptr;
    BagMate const* mate = nullptr;
    uint32_t score = 0;

    void Offer(BagStack const& s, BagMate const& m, uint32_t sc)
    {
        bool const better = !stack || s.count < stack->count ||
                            (s.count == stack->count &&
                             (sc > score || (sc == score && m.guid < mate->guid)));
        if (better)
        {
            stack = &s;
            mate = &m;
            score = sc;
        }
    }
};

BagMove Result(BagState& state, uint64_t item, uint64_t receiver, bool merge, std::string reason)
{
    state.lastReason = reason;
    return BagMove{item, receiver, merge, false, std::move(reason)};
}
}  // namespace

BagMove PlanBags(BagSnapshot const& snap, BagState& state, BagConfig const& cfg)
{
    if (snap.freeSlots >= cfg.minFreeSlots)
    {
        state.warned = false;
        return Result(state, 0, 0, false, "enough space");
    }

    // 1. Merge into partial stacks: frees a slot at no cost for the receiver.
    Pick merge;
    for (BagStack const& s : snap.stacks)
        if (Candidate(s))
            for (BagMate const& m : snap.mates)
            {
                uint32_t const room = Lookup(m.room, s.entry);
                if (room >= s.count && TierOk(m, s))
                    merge.Offer(s, m, room);
            }
    if (merge.stack)
        return Result(state, merge.stack->id, merge.mate->guid, true, "merge");

    // 2. Whole stack into a free slot, receiver staying at or above the threshold.
    Pick slot;
    for (BagStack const& s : snap.stacks)
        if (Candidate(s))
            for (BagMate const& m : snap.mates)
                if (m.freeSlots >= cfg.minFreeSlots + 1 && TierOk(m, s))
                    slot.Offer(s, m, m.freeSlots);
    if (slot.stack)
        return Result(state, slot.stack->id, slot.mate->guid, false, "move to free slot");

    BagMove none = Result(state, 0, 0, false, "bags full, nothing to rebalance");
    none.warn = !state.warned;
    state.warned = true;
    return none;
}
}  // namespace PlayerbotsPlus
