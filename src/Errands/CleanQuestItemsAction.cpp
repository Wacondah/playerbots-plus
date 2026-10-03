/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "CleanQuestItemsAction.h"

#include "ErrandsCommon.h"
#include "ErrandsValues.h"
#include "GroupItems.h"
#include "Playerbots.h"
#include "QuestItemIndex.h"
#include "StaleQuestItem.h"

namespace PlayerbotsPlus
{
bool CleanQuestItemsAction::isUseful()
{
    ErrandsData& data = AI_VALUE(ErrandsData&, "errands data");
    uint32 const now = getMSTime();
    if (bot->IsInCombat() || !Elapsed(now, data.lastQuestCleanAt, CleanQuestItemsIntervalMs))
        return false;
    data.lastQuestCleanAt = now;
    return FindStale() != nullptr;
}

bool CleanQuestItemsAction::Execute(Event /*event*/)
{
    Item* item = FindStale();
    if (!item)
        return false;
    std::string const what = chat->FormatItem(item->GetTemplate(), item->GetCount());
    bot->DestroyItemCount(item->GetEntry(), item->GetCount(), true);
    DebugErrands(botAI, "quests: dropped " + what);
    return true;
}

// The first quest item of the bags that no quest of the bot, its group or its master needs.
Item* CleanQuestItemsAction::FindStale()
{
    Player* master = RealMaster(botAI);
    Item* found = nullptr;
    ForEachBagItem(bot,
                   [&](Item* item)
                   {
                       ItemTemplate const* proto = item->GetTemplate();
                       if (found || proto->Class != ITEM_CLASS_QUEST)
                           return;
                       StaleFacts facts;
                       facts.isQuestItem = true;
                       facts.sellPrice = proto->SellPrice;
                       for (uint32 quest : QuestItemIndex::QuestsOf(proto->ItemId))
                       {
                           ++facts.quests;
                           if (bot->GetQuestRewardStatus(quest))
                               ++facts.questsRewarded;
                       }
                       facts.neededByGroup = QuestNeededByGroup(bot, proto->ItemId);
                       facts.neededByMaster = master && QuestItemNeed(master, proto->ItemId);
                       if (IsStaleQuestItem(facts))
                           found = item;
                   });
    return found;
}
}  // namespace PlayerbotsPlus
