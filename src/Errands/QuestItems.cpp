/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "QuestItems.h"

#include <algorithm>

namespace PlayerbotsPlus
{
QuestTransfer PlanQuestTransfer(uint64_t holder, std::vector<QuestNeed> const& group)
{
    QuestTransfer none;
    auto const self =
        std::find_if(group.begin(), group.end(), [holder](QuestNeed const& n) { return n.guid == holder; });
    if (self == group.end() || self->held <= self->needed)
        return none;
    uint32_t const surplus = self->held - self->needed;

    QuestNeed const* best = nullptr;
    for (QuestNeed const& n : group)
    {
        if (n.guid == holder || n.held >= n.needed)
            continue;
        uint32_t const missing = n.needed - n.held;
        uint32_t const bestMissing = best ? best->needed - best->held : 0;
        if (!best || missing > bestMissing || (missing == bestMissing && n.guid < best->guid))
            best = &n;
    }
    if (!best)
        return none;

    QuestTransfer t;
    t.to = best->guid;
    t.count = std::min(surplus, best->needed - best->held);
    return t;
}
}  // namespace PlayerbotsPlus
