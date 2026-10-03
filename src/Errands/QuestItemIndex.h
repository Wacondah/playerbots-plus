/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_QUEST_ITEM_INDEX_H
#define PLAYERBOTS_PLUS_QUEST_ITEM_INDEX_H

#include "Define.h"

#include <vector>

// Which quests mention an item: as an objective, a given item or a starter item.
namespace PlayerbotsPlus
{
namespace QuestItemIndex
{
void Build();  // at startup, once the quest and item templates are loaded
std::vector<uint32> const& QuestsOf(uint32 itemEntry);
}  // namespace QuestItemIndex
}  // namespace PlayerbotsPlus

#endif
