/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "ShareItemAction.h"

#include "ErrandsCommon.h"
#include "GroupItems.h"
#include "ObjectAccessor.h"
#include "OfferToMasterAction.h"
#include "Playerbots.h"
#include "PlayerbotsPlusConfig.h"
#include "ReagentIndex.h"

namespace PlayerbotsPlus
{
bool ShareItemAction::isUseful()
{
    ErrandsData& data = AI_VALUE(ErrandsData&, "errands data");
    uint32 const now = getMSTime();
    if (!ErrandsIdle(data, now) || data.OfferBusy() || !Elapsed(now, data.lastShareScanAt, ShareIntervalMs))
        return false;
    data.lastShareScanAt = now;

    ShareSnapshot const snap = BuildSnapshot(data, now);
    data.shareDecision = PlanShare(snap, data.share, ShareConfig{Config().planner.blacklistMs}, now);
    if (data.shareDecision.Acts())
        DebugErrands(botAI, "share: " + data.shareDecision.reason);
    return data.shareDecision.Acts();
}

bool ShareItemAction::Execute(Event /*event*/)
{
    ErrandsData& data = AI_VALUE(ErrandsData&, "errands data");
    ShareDecision const decision = data.shareDecision;
    Item* item = FindItem(decision.item);
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
    if (!GiveItemTo(bot, item, receiver))
    {
        MarkShareFailed(data.share, decision.item, decision.receiver, getMSTime());
        DebugErrands(botAI, std::string("share: ") + receiver->GetName() + " bags full");
        return false;
    }

    botAI->TellMasterNoFacing("Gave " + what + " to " + receiver->GetName());
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

    ForEachBagItem(bot,
                   [&](Item* item)
                   {
                       ItemTemplate const* proto = item->GetTemplate();
                       bool const gear = proto->Class == ITEM_CLASS_ARMOR || proto->Class == ITEM_CLASS_WEAPON ||
                                         proto->Class == ITEM_CLASS_CONTAINER;
                       if (!item->CanBeTraded() || (!gear && !ReagentIndex::UsedBy(proto->ItemId)))
                           return;
                       ShareItem described = DescribeForGroup(botAI, item, receivers);
                       described.masterDeclined = data.OfferDeclined(proto->ItemId, now);
                       snap.items.push_back(std::move(described));
                   });
    return snap;
}

Item* ShareItemAction::FindItem(uint64_t guid)
{
    Item* found = nullptr;
    ForEachBagItem(bot,
                   [&](Item* item)
                   {
                       if (item->GetGUID().GetRawValue() == guid)
                           found = item;
                   });
    return found;
}
}  // namespace PlayerbotsPlus
