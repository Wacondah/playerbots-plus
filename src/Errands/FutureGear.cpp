/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "FutureGear.h"

#include <algorithm>
#include <map>

namespace PlayerbotsPlus
{
std::set<uint64_t> KeepFutureGear(std::vector<FutureItem> const& items)
{
    std::map<uint32_t, std::vector<FutureItem const*>> groups;
    for (FutureItem const& item : items)
        if (item.score > item.worn)
            groups[item.group].push_back(&item);

    std::set<uint64_t> kept;
    for (auto& [group, pieces] : groups)
    {
        std::sort(pieces.begin(), pieces.end(), [](FutureItem const* a, FutureItem const* b)
                  { return a->score != b->score ? a->score > b->score : a->id < b->id; });
        uint32_t const capacity = std::max<uint32_t>(pieces.front()->capacity, 1);
        for (size_t i = 0; i < pieces.size() && i < capacity; ++i)
            kept.insert(pieces[i]->id);
    }
    return kept;
}

bool WouldKeep(FutureItem const& candidate, std::vector<FutureItem> held)
{
    held.push_back(candidate);
    return KeepFutureGear(held).count(candidate.id) > 0;
}

bool WithinLevelAhead(uint32_t level, uint32_t requiredLevel, uint32_t maxAhead)
{
    return !maxAhead || requiredLevel <= level + maxAhead;
}
}  // namespace PlayerbotsPlus
