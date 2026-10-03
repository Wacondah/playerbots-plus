/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "QuestItemIndex.h"

#include "ObjectMgr.h"

#include <algorithm>
#include <unordered_map>

namespace PlayerbotsPlus
{
namespace
{
std::unordered_map<uint32, std::vector<uint32>>& Index()
{
    static std::unordered_map<uint32, std::vector<uint32>> index;
    return index;
}

void Add(uint32 item, uint32 quest)
{
    if (!item)
        return;
    std::vector<uint32>& quests = Index()[item];
    if (std::find(quests.begin(), quests.end(), quest) == quests.end())
        quests.push_back(quest);
}
}  // namespace

void QuestItemIndex::Build()
{
    Index().clear();
    for (auto const& [id, quest] : sObjectMgr->GetQuestTemplates())
    {
        for (uint8 i = 0; i < QUEST_ITEM_OBJECTIVES_COUNT; ++i)
            Add(quest->RequiredItemId[i], id);
        for (uint8 i = 0; i < QUEST_SOURCE_ITEM_IDS_COUNT; ++i)
            Add(quest->ItemDrop[i], id);
        Add(quest->StartItem, id);  // given when the quest is accepted
    }
    // An item that starts a quest (the quest the item names).
    for (auto const& [entry, proto] : *sObjectMgr->GetItemTemplateStore())
        if (proto.StartQuest)
            Add(entry, proto.StartQuest);
}

std::vector<uint32> const& QuestItemIndex::QuestsOf(uint32 itemEntry)
{
    static std::vector<uint32> const none;
    auto const it = Index().find(itemEntry);
    return it == Index().end() ? none : it->second;
}
}  // namespace PlayerbotsPlus
