/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "SharePlanner.h"

namespace PlayerbotsPlus
{
namespace
{
bool WantsToEquip(ShareUsage usage)
{
    return usage == ShareUsage::Equip || usage == ShareUsage::Replace;
}

void ExpireBlacklist(ShareState& state, ShareConfig const& cfg, uint32_t now)
{
    for (auto it = state.blacklistedAt.begin(); it != state.blacklistedAt.end();)
    {
        if (Elapsed(now, it->second, cfg.blacklistMs))
            it = state.blacklistedAt.erase(it);
        else
            ++it;
    }
}

// Gear: the receiver gaining the most. Materials: highest tier, most held, lowest GUID.
ShareReceiver const* BestReceiver(ShareItem const& item, ShareState const& state)
{
    if (item.usage == ShareUsage::Quest)
        return nullptr;

    ShareReceiver const* best = nullptr;
    auto blacklisted = [&](ShareReceiver const& r) { return state.blacklistedAt.count({item.id, r.guid}) > 0; };

    if (!WantsToEquip(item.usage))
        for (ShareReceiver const& r : item.receivers)
            if (WantsToEquip(r.usage) && !blacklisted(r) &&
                (!best || r.gain > best->gain || (r.gain == best->gain && r.guid < best->guid)))
                best = &r;
    if (best)
        return best;

    for (ShareReceiver const& r : item.receivers)
    {
        if (r.tier <= item.tier || blacklisted(r))
            continue;
        if (!best || r.tier > best->tier || (r.tier == best->tier && r.held > best->held) ||
            (r.tier == best->tier && r.held == best->held && r.guid < best->guid))
            best = &r;
    }
    return best;
}

// The master comes first for an upgrade the holder does not want itself.
bool ForMaster(ShareItem const& item)
{
    return item.usage != ShareUsage::Quest && !WantsToEquip(item.usage) && item.masterGain > 0.f &&
           !item.masterDeclined;
}

ShareDecision Result(ShareState& state, uint64_t item, uint64_t receiver, std::string reason, bool toMaster = false)
{
    state.lastReason = reason;
    return ShareDecision{item, receiver, toMaster, std::move(reason)};
}
}  // namespace

uint8_t TierFor(uint32_t usedBy, uint32_t known)
{
    uint32_t const both = usedBy & known;
    if (both & ProfessionBit::Primary)
        return 2;
    if (both & ProfessionBit::Secondary)
        return 1;
    return 0;
}

ShareDecision PlanShare(ShareSnapshot const& snap, ShareState& state, ShareConfig const& cfg, uint32_t now)
{
    ExpireBlacklist(state, cfg, now);

    if (!snap.errandsIdle)
        return Result(state, 0, 0, "errands first");

    for (ShareItem const& item : snap.items)
    {
        if (snap.master && ForMaster(item))
            return Result(state, item.id, snap.master, "offer to master", true);
        if (ShareReceiver const* r = BestReceiver(item, state))
            return Result(state, item.id, r->guid, "give");
        // Nobody uses it: an enchanter turns it into dust and essences.
        if (item.disenchanter && !item.holderCanDisenchant && item.usage != ShareUsage::Quest &&
            !WantsToEquip(item.usage) && !state.blacklistedAt.count({item.id, item.disenchanter}))
            return Result(state, item.id, item.disenchanter, "give to disenchant");
    }

    return Result(state, 0, 0, "nothing to share");
}

bool WantedByGroup(ShareItem const& item)
{
    if (item.usage == ShareUsage::Quest || ForMaster(item))
        return true;
    for (ShareReceiver const& r : item.receivers)
        if ((!WantsToEquip(item.usage) && WantsToEquip(r.usage)) || r.tier > item.tier)
            return true;
    return false;
}

void MarkShareFailed(ShareState& state, uint64_t item, uint64_t receiver, uint32_t now)
{
    state.blacklistedAt[{item, receiver}] = now;
    state.lastReason = "receiver bags full";
}
}  // namespace PlayerbotsPlus
