/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_CITY_PLANNER_H
#define PLAYERBOTS_PLUS_CITY_PLANNER_H

#include "CraftPlanner.h"
#include "ErrandPlanner.h"

#include <set>
#include <string>
#include <vector>

// Pure decision logic for trips across a capital (the leash is released meanwhile).
namespace PlayerbotsPlus
{
namespace CityNeed
{
constexpr uint32_t ClassTraining = 1u << 0;
constexpr uint32_t Profession = 1u << 1;
constexpr uint32_t Tool = 1u << 2;
constexpr uint32_t Reagents = 1u << 3;
constexpr uint32_t Forge = 1u << 4;
constexpr uint32_t Anvil = 1u << 5;
}  // namespace CityNeed

// Forges and anvils are game objects: their stop ids are tagged so they never collide with
// creature spawn ids in a trip.
constexpr uint64_t CityStationTag = 1ull << 63;
inline uint64_t StationStopId(uint64_t spawnId) { return spawnId | CityStationTag; }
inline bool IsStationStop(uint64_t id) { return (id & CityStationTag) != 0; }
inline uint64_t StationSpawnId(uint64_t id) { return id & ~CityStationTag; }

// The city need a spell focus answers (1 anvil, 3 forge), 0 for any other focus.
uint32_t FocusNeed(uint32_t focus);

// Stations worth a walk: a recipe only missing its focus that the craft rules would pick
// there (raw ore smelting, the master's approved recipe, group, cooldown, skill-up).
uint32_t StationNeeds(CraftSnapshot const& snap, CraftState const& state);

// An NPC spawn of the capital and the needs of this bot it covers.
struct CityStop
{
    uint64_t id = 0;
    Vec3 pos;
    uint32_t covers = 0;
};

struct CitySnapshot
{
    bool masterInCapital = false;
    uint32_t zone = 0;
    bool masterMoving = false;
    bool masterInCombat = false;
    bool masterMounted = false;
    bool masterOnTaxi = false;
    bool inInstance = false;
    bool botInCombat = false;
    bool autoEnabled = true;
    bool requested = false;  // "errands city" since the last plan
    Vec3 botPos;
    uint32_t needs = 0;  // CityNeed mask the stops can cover
    std::vector<CityStop> stops;
    bool atStop = false;       // within interaction range of the current stop's NPC
    bool stopMissing = false;  // near the current stop's spawn, NPC absent or dead
};

struct CityConfig
{
    uint32_t idleDelayMs = 3000;
    uint32_t timeoutMs = 180000;
    uint32_t cooldownMs = 600000;
    uint32_t reachMs = 60000;
    uint32_t workMs = 120000;  // longest stay at one forge or anvil
};

struct CityState
{
    bool active = false;
    uint32_t zone = 0;
    uint32_t startedAt = 0;
    uint64_t current = 0;
    uint32_t currentSince = 0;
    std::set<uint64_t> done;  // visited or skipped this trip
    uint32_t masterMovedAt = 0;
    bool ended = false;
    uint32_t endedAt = 0;
    uint32_t endedNeeds = 0;
    uint32_t stopsPlanned = 0;
    uint32_t visitSince = 0;  // working at the current station since (0: not)
    uint32_t workedMs = 0;    // time spent at stations this trip: not counted in the timeout
    std::string lastReason;

    bool Active() const { return active; }
};

enum class CityStep : uint8_t
{
    None,
    GoTo,
    Visit,
    End,
    Abort
};

struct CityDecision
{
    CityStep step = CityStep::None;
    uint64_t stop = 0;
    Vec3 pos;
    std::string reason;
};

// Start when the master idles in a capital (auto or on command, not again with unchanged
// needs before the cooldown), then the nearest stop covering a need, until nothing is
// left, the timeout, or the master leaves. Moving inside the capital does not abort.
CityDecision PlanCity(CitySnapshot const& snap, CityState& state, CityConfig const& cfg, uint32_t now);

// The visit at the current stop is over (whatever it achieved).
// `now` closes the work time at a station (0 when the stop is a creature).
void MarkCityVisited(CityState& state, uint32_t now = 0);

// Stops covering the bot's needs, for the "errands city" reply.
uint32_t CountCityStops(CitySnapshot const& snap);
}  // namespace PlayerbotsPlus

#endif
