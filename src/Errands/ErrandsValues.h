/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_ERRANDS_VALUES_H
#define PLAYERBOTS_PLUS_ERRANDS_VALUES_H

#include "BagPlanner.h"
#include "ErrandPlanner.h"
#include "HuntPlanner.h"
#include "SharePlanner.h"
#include "Value.h"

namespace PlayerbotsPlus
{
// The errands decision is trusted by hunt/share only if computed this recently.
constexpr uint32 FreshDecisionMs = 2000;

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
