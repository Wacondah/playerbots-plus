/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "ShareItemAction.h"

#include "ErrandsCommon.h"
#include "GroupItems.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "OfferToMasterAction.h"
#include "Playerbots.h"
#include "PlayerbotsPlusConfig.h"
#include "ReagentIndex.h"

#include <set>

namespace PlayerbotsPlus
{
bool ShareItemAction::isUseful()
{
    ErrandsData& data = AI_VALUE(ErrandsData&, "errands data");
    uint32 const now = getMSTime();
    if (!ErrandsIdle(data, now) || data.OfferBusy() || !Elapsed(now, data.lastShareScanAt, ShareIntervalMs))
        return false;
    data.lastShareScanAt = now;

    // Quest items first: a surplus the holder's quests do not need goes to a short alt.
    if (PlanQuestItems(data))
        return true;

    ShareSnapshot const snap = BuildSnapshot(data, now);
    data.shareDecision = PlanShare(snap, data.share, ShareConfig{Config().planner.blacklistMs}, now);
    if (data.shareDecision.Acts())
        DebugErrands(botAI, "share: " + data.shareDecision.reason);
    return data.shareDecision.Acts();
}

bool ShareItemAction::Execute(Event /*event*/)
{
    ErrandsData& data = AI_VALUE(ErrandsData&, "errands data");
    if (data.questTransfer.Acts())
        return GiveQuestItems(data);
    ShareDecision const decision = data.shareDecision;
    Item* item = FindBagItem(bot, decision.item);
    if (item && decision.toMaster)
    {
        StartOffer(data, item, getMSTime());  // through a trade window the master accepts
        return true;
    }
    Player* receiver = ObjectAccessor::FindPlayer(ObjectGuid(decision.receiver));
    if (!item || !receiver)
        return false;

    // Formatted before the move: a merged stack no longer exists afterwards.
    std::string const what = chat->FormatItem(item->GetTemplate(), item->GetCount());
    bool const gear = IsGear(item->GetTemplate());
    if (!GiveItemTo(bot, item, receiver))
    {
        MarkShareFailed(data.share, decision.item, decision.receiver, getMSTime());
        DebugErrands(botAI, std::string("share: ") + receiver->GetName() + " bags full");
        return false;
    }

    botAI->TellMasterNoFacing("Gave " + what + " to " + receiver->GetName());
    // Upstream equips on an item push packet, which a direct move does not send.
    if (PlayerbotAI* receiverAI = gear ? GET_PLAYERBOT_AI(receiver) : nullptr)
        receiverAI->DoSpecificAction("equip upgrade", Event("share item"), true);
    return true;
}

ShareSnapshot ShareItemAction::BuildSnapshot(ErrandsData& data, uint32 now)
{
    ShareSnapshot snap;
    snap.errandsIdle = ErrandsIdle(data, now);
    std::vector<Player*> const receivers = GroupBots(bot, ShareDistance);
    Player* master = RealMaster(botAI);
    if (receivers.empty() && !master)
        return snap;
    snap.master = master ? master->GetGUID().GetRawValue() : 0;
    std::set<uint64> const future = KeptFutureGearIds(bot);  // kept for when the level allows

    ForEachBagItem(bot,
                   [&](Item* item)
                   {
                       ItemTemplate const* proto = item->GetTemplate();
                       if (!item->CanBeTraded() || (!IsGear(proto) && !ReagentIndex::UsedBy(proto->ItemId)) ||
                           QuestNeededByGroup(bot, proto->ItemId) ||  // handled by PlanQuestItems
                           future.count(item->GetGUID().GetRawValue()))
                           return;
                       ShareItem described = DescribeForGroup(botAI, item, receivers);
                       described.masterDeclined = data.OfferDeclined(proto->ItemId, now);
                       snap.items.push_back(std::move(described));
                   });
    return snap;
}

bool ShareItemAction::PlanQuestItems(ErrandsData& data)
{
    data.questTransfer = QuestTransfer{};
    std::vector<Player*> const mates = GroupBots(bot, ShareDistance);
    if (mates.empty())
        return false;

    std::set<uint32> seen;
    bool found = false;
    ForEachBagItem(bot,
                   [&](Item* item)
                   {
                       uint32 const entry = item->GetEntry();
                       if (found || !seen.insert(entry).second)
                           return;
                       std::vector<QuestNeed> group = {{bot->GetGUID().GetRawValue(), bot->GetItemCount(entry, false),
                                                        QuestItemNeed(bot, entry)}};
                       for (Player* mate : mates)
                           group.push_back({mate->GetGUID().GetRawValue(), mate->GetItemCount(entry, false),
                                            QuestItemNeed(mate, entry)});
                       QuestTransfer const t = PlanQuestTransfer(group.front().guid, group);
                       if (!t.Acts())
                           return;
                       data.questItemEntry = entry;
                       data.questTransfer = t;
                       found = true;
                   });
    return found;
}

bool ShareItemAction::GiveQuestItems(ErrandsData& data)
{
    QuestTransfer const t = data.questTransfer;
    uint32 const entry = data.questItemEntry;
    data.questTransfer = QuestTransfer{};
    Player* receiver = ObjectAccessor::FindPlayer(ObjectGuid(t.to));
    ItemTemplate const* proto = sObjectMgr->GetItemTemplate(entry);
    if (!receiver || !proto || bot->GetItemCount(entry, false) < t.count)
        return false;
    if (!receiver->StoreNewItemInBestSlots(entry, t.count))  // also counts toward its quest
        return false;
    bot->DestroyItemCount(entry, t.count, true);
    botAI->TellMasterNoFacing("Gave " + chat->FormatItem(proto, t.count) + " to " + receiver->GetName() + " (quest)");
    return true;
}
}  // namespace PlayerbotsPlus
