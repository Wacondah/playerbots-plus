/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_CITY_ERRAND_ACTION_H
#define PLAYERBOTS_PLUS_CITY_ERRAND_ACTION_H

#include "CityIndex.h"
#include "RunErrandAction.h"

class Creature;

namespace PlayerbotsPlus
{
// A bot's stops in a capital are recomputed at most this often (trainer spell scans).
constexpr uint32 CityStopsIntervalMs = 5000;
// A stop not reached within this delay is skipped for the trip.
constexpr uint32 CityReachMs = 60000;

// Adapter between the core and PlanCity: trips across a capital, leash released.
class CityErrandAction : public RunErrandAction
{
public:
    CityErrandAction(PlayerbotAI* botAI) : RunErrandAction(botAI, "city errand") {}

    bool isUseful() override;
    bool Execute(Event event) override;

    // Stops covering this bot's needs in the master's capital, for the "errands city"
    // reply; -1 when the master is not in a capital.
    int32 CountStops();

private:
    CitySnapshot BuildCitySnapshot(ErrandsData& data, uint32 now);
    void RefreshStops(ErrandsData& data, Player* master, uint32 now);
    CityIndex::Spawn const* SpawnOf(uint32 zone, uint64 spawnId);
};
}  // namespace PlayerbotsPlus

#endif
