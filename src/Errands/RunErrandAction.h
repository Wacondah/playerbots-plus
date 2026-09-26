/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_RUN_ERRAND_ACTION_H
#define PLAYERBOTS_PLUS_RUN_ERRAND_ACTION_H

#include "ErrandsValues.h"
#include "NewRpgBaseAction.h"

namespace PlayerbotsPlus
{
// Candidate NPCs are rescanned at most this often per bot.
constexpr uint32 ScanIntervalMs = 1000;
// Junk (which asks every group bot about every item) is re-evaluated at most this often.
constexpr uint32 JunkIntervalMs = 5000;

// Adapter between the core and ErrandPlanner. isUseful() runs the planner,
// Execute() carries out its decision.
class RunErrandAction : public NewRpgBaseAction
{
public:
    RunErrandAction(PlayerbotAI* botAI) : NewRpgBaseAction(botAI, "run errand") {}

    bool isUseful() override;
    bool Execute(Event event) override;

private:
    ErrandsData& Data();
    Snapshot BuildSnapshot(ErrandsData& data, uint32 now);
    void Scan(ErrandsData& data, Player* master, uint32 now);
    Candidate Describe(WorldObject* object);
    void VisitTarget(WorldObject* object);
    bool NeedsRepair();
    bool HasJunk(ErrandsData& data, uint32 now);
    bool IsBasicJunk(Item* item);
    std::vector<Item*> ExtraJunk();
    uint64_t QuestFingerprint();
};
}  // namespace PlayerbotsPlus

#endif
