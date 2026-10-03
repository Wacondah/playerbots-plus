/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_STALE_QUEST_ITEM_H
#define PLAYERBOTS_PLUS_STALE_QUEST_ITEM_H

#include <cstdint>

// Pure rule of the quest item housekeeping.
namespace PlayerbotsPlus
{
struct StaleFacts
{
    bool isQuestItem = false;     // item class "quest"
    uint32_t sellPrice = 0;       // the vendor buys it: the sell errand handles it
    uint32_t quests = 0;          // quests that mention the item (objective, given item, starter)
    uint32_t questsRewarded = 0;  // of those, the ones this bot has already turned in
    bool neededByGroup = false;   // an open quest of another bot needs it
    bool neededByMaster = false;  // an open quest of the master needs it
};

// Useless and unsellable: every quest that mentions it is done, nobody still needs it.
// An item no quest mentions is kept: its purpose is unknown.
bool IsStaleQuestItem(StaleFacts const& facts);
}  // namespace PlayerbotsPlus

#endif
