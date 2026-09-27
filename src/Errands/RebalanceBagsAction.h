/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_REBALANCE_BAGS_ACTION_H
#define PLAYERBOTS_PLUS_REBALANCE_BAGS_ACTION_H

#include "Action.h"
#include "ErrandsValues.h"

namespace PlayerbotsPlus
{
// Own bags are checked at most this often.
constexpr uint32 BagsIntervalMs = 5000;
// The game's trade distance.
constexpr float BagsDistance = 10.0f;

// Adapter between the core and PlanBags.
class RebalanceBagsAction : public Action
{
public:
    RebalanceBagsAction(PlayerbotAI* botAI) : Action(botAI, "rebalance bags") {}

    bool isUseful() override;
    bool Execute(Event event) override;

private:
    BagSnapshot BuildSnapshot();
    bool Movable(Item* item);
};
}  // namespace PlayerbotsPlus

#endif
