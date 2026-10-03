/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_GATHER_NODE_ACTION_H
#define PLAYERBOTS_PLUS_GATHER_NODE_ACTION_H

#include "ErrandsValues.h"
#include "GatherPlanner.h"
#include "MovementActions.h"

namespace PlayerbotsPlus
{
// Rescan nodes at most this often while the master is idle.
constexpr uint32 GatherScanIntervalMs = 1000;
// A hostile this close to a node keeps the bot away from it.
constexpr float GatherGuardRadius = 10.f;

// Walks to a mining or herb node near the idle master and opens it through mod-playerbots'
// own loot actions ("open loot", then "store loot" on the loot response).
class GatherNodeAction : public MovementAction
{
public:
    GatherNodeAction(PlayerbotAI* botAI) : MovementAction(botAI, "gather node") {}

    bool isUseful() override;
    bool Execute(Event event) override;

private:
    std::vector<GatherNode> ScanNodes(Player* master);
    bool ClaimedByOther(Player* master, GameObject* go, uint32 skill);
};
}  // namespace PlayerbotsPlus

#endif
