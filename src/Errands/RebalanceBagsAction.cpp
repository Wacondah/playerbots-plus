/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "RebalanceBagsAction.h"

#include "ErrandsCommon.h"
#include "GroupItems.h"
#include "ItemUsageValue.h"
#include "ObjectAccessor.h"
#include "Playerbots.h"
#include "PlayerbotsPlusConfig.h"
#include "ReagentIndex.h"

#include <unordered_set>

namespace PlayerbotsPlus
{
bool RebalanceBagsAction::isUseful()
{
    ErrandsData& data = AI_VALUE(ErrandsData&, "errands data");
    uint32 const now = getMSTime();
    if (bot->IsInCombat() || !Elapsed(now, data.lastBagsAt, BagsIntervalMs))
        return false;
    data.lastBagsAt = now;

    data.bagMove = PlanBags(BuildSnapshot(), data.bags, BagConfig{Config().bagsMinFreeSlots});
    if (data.bagMove.warn)
        botAI->TellMaster(data.bagMove.reason);
    return data.bagMove.Acts();
}

bool RebalanceBagsAction::Execute(Event /*event*/)
{
    ErrandsData& data = AI_VALUE(ErrandsData&, "errands data");
    BagMove const move = data.bagMove;
    Player* receiver = ObjectAccessor::FindPlayer(ObjectGuid(move.receiver));
    Item* item = FindBagItem(bot, move.item);
    if (!item || !receiver)
        return false;

    // Formatted before the move: a merged stack no longer exists afterwards.
    std::string const what = chat->FormatItem(item->GetTemplate(), item->GetCount());
    if (!GiveItemTo(bot, item, receiver))
        return false;
    DebugErrands(botAI, "bags: " + move.reason + " " + what + " -> " + receiver->GetName());
    return true;
}

// Tradeable, and not something the holder relies on (quest, food/water/potions, ammo).
bool RebalanceBagsAction::Movable(Item* item)
{
    if (!item->CanBeTraded())
        return false;
    if (QuestItemNeed(bot, item->GetEntry()))  // its quests need it: a stack cannot be split here
        return false;
    switch (AI_VALUE2(ItemUsage, "item usage", int32(item->GetEntry())))
    {
        case ITEM_USAGE_QUEST:
        case ITEM_USAGE_USE:
        case ITEM_USAGE_KEEP:
        case ITEM_USAGE_AMMO:
            return false;
        default:
            return true;
    }
}

BagSnapshot RebalanceBagsAction::BuildSnapshot()
{
    BagSnapshot snap;
    snap.freeSlots = FreeSlots(bot);
    if (snap.freeSlots >= Config().bagsMinFreeSlots)
        return snap;  // the planner only needs the count

    uint32 const known = ReagentIndex::Known(bot);
    std::unordered_set<uint32> entries;
    ForEachBagItem(bot,
                   [&](Item* item)
                   {
                       uint32 const maxStack = item->GetMaxStackCount();
                       if (maxStack <= 1)
                           return;
                       uint32 const entry = item->GetEntry();
                       snap.stacks.push_back({item->GetGUID().GetRawValue(), entry, item->GetCount(), maxStack,
                                              Movable(item), TierFor(ReagentIndex::UsedBy(entry), known)});
                       entries.insert(entry);
                   });

    for (Player* mate : GroupBots(bot, BagsDistance))
    {
        if (mate->IsInCombat())
            continue;
        BagMate m;
        m.guid = mate->GetGUID().GetRawValue();
        m.freeSlots = FreeSlots(mate);
        uint32 const mateKnown = ReagentIndex::Known(mate);
        for (uint32 entry : entries)
            m.tier[entry] = TierFor(ReagentIndex::UsedBy(entry), mateKnown);
        ForEachBagItem(mate,
                       [&](Item* item)
                       {
                           if (entries.count(item->GetEntry()))
                               m.room[item->GetEntry()] += item->GetMaxStackCount() - item->GetCount();
                       });
        snap.mates.push_back(std::move(m));
    }
    return snap;
}
}  // namespace PlayerbotsPlus
