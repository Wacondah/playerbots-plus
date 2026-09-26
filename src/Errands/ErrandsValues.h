/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_ERRANDS_VALUES_H
#define PLAYERBOTS_PLUS_ERRANDS_VALUES_H

#include "ErrandPlanner.h"
#include "HuntPlanner.h"
#include "Value.h"

namespace PlayerbotsPlus
{
struct ErrandsData
{
    ErrandState state;
    std::vector<Candidate> candidates;
    uint32 lastScanAt = 0;
    bool scanned = false;
    Decision decision;
    uint32 decidedAt = 0;  // when `decision` was computed; hunting trusts it only if fresh
    HuntState hunt;
    Decision huntDecision;
};

class ErrandsDataValue : public ManualSetValue<ErrandsData&>
{
public:
    ErrandsDataValue(PlayerbotAI* botAI) : ManualSetValue<ErrandsData&>(botAI, data, "errands data") {}

private:
    ErrandsData data;
};
}  // namespace PlayerbotsPlus

#endif
