/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_SHARE_ITEM_ACTION_H
#define PLAYERBOTS_PLUS_SHARE_ITEM_ACTION_H

#include "Action.h"
#include "ErrandsValues.h"

namespace PlayerbotsPlus
{
// Bags are rescanned at most this often per bot: item usage is not free.
constexpr uint32 ShareIntervalMs = 5000;
// The game's trade distance.
constexpr float ShareDistance = 10.0f;

// Adapter between the core and PlanShare; the move mirrors GiveItemAction.
class ShareItemAction : public Action
{
public:
    ShareItemAction(PlayerbotAI* botAI) : Action(botAI, "share item") {}

    bool isUseful() override;
    bool Execute(Event event) override;

private:
    ShareSnapshot BuildSnapshot(ErrandsData& data, uint32 now);
    bool PlanQuestItems(ErrandsData& data);
    bool GiveQuestItems(ErrandsData& data);
};
}  // namespace PlayerbotsPlus

#endif
