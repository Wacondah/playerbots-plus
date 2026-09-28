/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_QUEST_ITEMS_H
#define PLAYERBOTS_PLUS_QUEST_ITEMS_H

#include <cstdint>
#include <vector>

// Pure rules for items quests require (even those also used by a profession, like
// Goretusk Liver): what each bot keeps, and where a surplus goes.
namespace PlayerbotsPlus
{
struct QuestNeed
{
    uint64_t guid = 0;
    uint32_t held = 0;    // in its bags
    uint32_t needed = 0;  // required by its incomplete quests (0: none)
};

struct QuestTransfer
{
    uint64_t to = 0;
    uint32_t count = 0;

    bool Acts() const { return to && count; }
};

// The holder's surplus (what its quests do not need) to the group bot missing the most,
// no more than it misses. `group` includes the holder.
QuestTransfer PlanQuestTransfer(uint64_t holder, std::vector<QuestNeed> const& group);
}  // namespace PlayerbotsPlus

#endif
