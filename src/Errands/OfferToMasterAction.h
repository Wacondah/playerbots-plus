/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_OFFER_TO_MASTER_ACTION_H
#define PLAYERBOTS_PLUS_OFFER_TO_MASTER_ACTION_H

#include "Action.h"
#include "ErrandsValues.h"

namespace PlayerbotsPlus
{
// The game's trade distance.
constexpr float OfferDistance = 10.0f;
// A crafted item not found in the bags this long after the cast is dropped.
constexpr uint32 CraftedOfferWaitMs = 15000;

// Starts an offer to the master (share and craft use it).
void StartOffer(ErrandsData& data, Item* item, uint32 now);

// Drives the pending offer: trade request, item placed, then success or decline.
// Upstream trade handling accepts on the bot side once the master accepts.
class OfferToMasterAction : public Action
{
public:
    OfferToMasterAction(PlayerbotAI* botAI) : Action(botAI, "offer to master") {}

    bool isUseful() override;
    bool Execute(Event event) override;

private:
    Item* FindEntry(uint32 entry);
    void Decline(ErrandsData& data, uint32 now, char const* why);
};
}  // namespace PlayerbotsPlus

#endif
