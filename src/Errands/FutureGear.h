/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_FUTURE_GEAR_H
#define PLAYERBOTS_PLUS_FUTURE_GEAR_H

#include <cstdint>
#include <set>
#include <vector>

// Pure rules for "future gear": green+ pieces only the bot's level keeps it from wearing.
namespace PlayerbotsPlus
{
struct FutureItem
{
    uint64_t id = 0;
    uint32_t group = 0;     // slot group (legs, finger, trinket...)
    uint32_t capacity = 1;  // pieces worn at once in that group (2 rings, 2 trinkets...)
    float score = 0.f;
    float worn = 0.f;       // score of the weakest piece worn in that group (0: empty)
};

// Pieces better than what is worn, at most `capacity` per group, the best first.
std::set<uint64_t> KeepFutureGear(std::vector<FutureItem> const& items);

// The piece would be kept next to the ones already held.
bool WouldKeep(FutureItem const& candidate, std::vector<FutureItem> held);

// Required level within reach (maxAhead 0: no limit).
bool WithinLevelAhead(uint32_t level, uint32_t requiredLevel, uint32_t maxAhead);
}  // namespace PlayerbotsPlus

#endif
