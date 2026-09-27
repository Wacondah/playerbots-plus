/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_ERRANDS_VALUES_H
#define PLAYERBOTS_PLUS_ERRANDS_VALUES_H

#include "BagPlanner.h"
#include "CraftPlanner.h"
#include "ErrandPlanner.h"
#include "HuntPlanner.h"
#include "SharePlanner.h"
#include "ShoppingPlanner.h"
#include "Value.h"

#include <unordered_map>

namespace PlayerbotsPlus
{
// The errands decision is trusted by hunt/share only if computed this recently.
constexpr uint32 FreshDecisionMs = 2000;
// An offer to the master: trade request unanswered this long counts as declined,
// and a declined item entry is not offered again for this long.
constexpr uint32 OfferRequestMs = 30000;
constexpr uint32 OfferDeclineMs = 30 * 60 * 1000;

// One item offered to the master through a trade window.
struct MasterOffer
{
    uint64 item = 0;  // item instance GUID
    uint32 entry = 0;
    uint32 startedAt = 0;
    bool requested = false;  // trade request sent
    bool placed = false;     // item put in the trade window

    bool Active() const { return item != 0; }
};

struct ErrandsData
{
    ErrandState state;
    std::vector<Candidate> candidates;
    uint32 lastScanAt = 0;
    bool scanned = false;
    Decision decision;
    uint32 decidedAt = 0;  // when `decision` was computed
    HuntState hunt;
    Decision huntDecision;
    ShareState share;
    ShareDecision shareDecision;
    uint32 lastShareScanAt = 0;
    bool hasJunk = false;  // cached: evaluating junk asks every group bot about every item
    bool junkChecked = false;
    uint32 junkCheckedAt = 0;
    BagState bags;
    BagMove bagMove;
    uint32 lastBagsAt = 0;
    MasterOffer offer;
    std::unordered_map<uint32, uint32> offerDeclinedAt;  // item entry -> when the master declined
    CraftState craft;
    CraftDecision craftDecision;
    uint32 lastCraftScanAt = 0;
    uint32 pendingOfferProduct = 0;  // crafted for the master, offered once it is in the bags
    uint32 pendingOfferSince = 0;
    ShoppingList shopping;  // vendor reagents for the recipe craft picked
    uint32 shoppingSpell = 0;
    uint32 shoppingProduct = 0;
    uint32 shoppingToldSpell = 0;  // "I need a vendor for" said once per approved recipe

    bool OfferBusy() const { return offer.Active() || pendingOfferProduct; }
    bool OfferDeclined(uint32 entry, uint32 now) const
    {
        auto const it = offerDeclinedAt.find(entry);
        return it != offerDeclinedAt.end() && !Elapsed(now, it->second, OfferDeclineMs);
    }
};

// This tick's errands decision is "nothing to do": the leash holds and no errand is left.
inline bool ErrandsIdle(ErrandsData const& data, uint32 now)
{
    return data.decision.type == DecisionType::Idle && data.decision.reason == "nothing to do" &&
           !Elapsed(now, data.decidedAt, FreshDecisionMs);
}

class ErrandsDataValue : public ManualSetValue<ErrandsData&>
{
public:
    ErrandsDataValue(PlayerbotAI* botAI) : ManualSetValue<ErrandsData&>(botAI, data, "errands data") {}

private:
    ErrandsData data;
};
}  // namespace PlayerbotsPlus

#endif
