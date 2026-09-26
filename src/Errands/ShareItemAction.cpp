/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "ShareItemAction.h"

#include "Bag.h"
#include "ErrandsCommon.h"
#include "Group.h"
#include "ItemUsageValue.h"
#include "ObjectAccessor.h"
#include "Playerbots.h"
#include "PlayerbotsPlusConfig.h"
#include "ReagentIndex.h"
#include "StatsWeightCalculator.h"

namespace PlayerbotsPlus
{
namespace
{
ShareUsage ToShareUsage(ItemUsage usage)
{
    switch (usage)
    {
        case ITEM_USAGE_EQUIP: return ShareUsage::Equip;
        case ITEM_USAGE_REPLACE: return ShareUsage::Replace;
        case ITEM_USAGE_QUEST: return ShareUsage::Quest;
        default: return ShareUsage::Other;
    }
}

ItemUsage UsageFor(Player* player, uint32 itemId)
{
    PlayerbotAI* ai = GET_PLAYERBOT_AI(player);
    return ai ? ai->GetAiObjectContext()->GetValue<ItemUsage>("item usage", int32(itemId))->Get() : ITEM_USAGE_NONE;
}

float GainFor(Player* receiver, Item* item)
{
    StatsWeightCalculator calc(receiver);
    float const score = calc.CalculateItem(item->GetEntry(), item->GetItemRandomPropertyId());
    uint8 const slot = receiver->FindEquipSlot(item->GetTemplate(), NULL_SLOT, true);
    Item* current = slot != NULL_SLOT ? receiver->GetItemByPos(INVENTORY_SLOT_BAG_0, slot) : nullptr;
    float const currentScore =
        current ? calc.CalculateItem(current->GetEntry(), current->GetItemRandomPropertyId()) : 0.f;
    return score - currentScore;
}

template <class Fn>
void ForEachBagItem(Player* bot, Fn fn)
{
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
        if (Item* item = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            fn(item);
    for (uint8 bagSlot = INVENTORY_SLOT_BAG_START; bagSlot < INVENTORY_SLOT_BAG_END; ++bagSlot)
        if (Bag* bag = bot->GetBagByPos(bagSlot))
            for (uint32 i = 0; i < bag->GetBagSize(); ++i)
                if (Item* item = bag->GetItemByPos(i))
                    fn(item);
}
}  // namespace

bool ShareItemAction::isUseful()
{
    ErrandsData& data = AI_VALUE(ErrandsData&, "errands data");
    uint32 const now = getMSTime();
    if (!ErrandsIdle(data, now) || !Elapsed(now, data.lastShareScanAt, ShareIntervalMs))
        return false;
    data.lastShareScanAt = now;

    ShareSnapshot const snap = BuildSnapshot(data, now);
    data.shareDecision = PlanShare(snap, data.share, ShareConfig{Config().planner.blacklistMs}, now);
    if (data.shareDecision.Acts())
        DebugErrands(botAI, "share: give");
    return data.shareDecision.Acts();
}

bool ShareItemAction::Execute(Event /*event*/)
{
    ErrandsData& data = AI_VALUE(ErrandsData&, "errands data");
    ShareDecision const decision = data.shareDecision;
    Item* item = FindItem(decision.item);
    Player* receiver = ObjectAccessor::FindPlayer(ObjectGuid(decision.receiver));
    if (!item || !receiver)
        return false;

    ItemPosCountVec dest;
    if (receiver->CanStoreItem(NULL_BAG, NULL_SLOT, dest, item, false) != EQUIP_ERR_OK)
    {
        MarkShareFailed(data.share, decision.item, decision.receiver, getMSTime());
        DebugErrands(botAI, std::string("share: ") + receiver->GetName() + " bags full");
        return false;
    }

    ItemTemplate const* proto = item->GetTemplate();
    uint32 const count = item->GetCount();
    bot->MoveItemFromInventory(item->GetBagSlot(), item->GetSlot(), true);
    item->SetOwnerGUID(receiver->GetGUID());
    receiver->MoveItemToInventory(dest, item, true);

    botAI->TellMasterNoFacing("Gave " + chat->FormatItem(proto, count) + " to " + receiver->GetName());
    return true;
}

ShareSnapshot ShareItemAction::BuildSnapshot(ErrandsData& data, uint32 now)
{
    ShareSnapshot snap;
    snap.errandsIdle = ErrandsIdle(data, now);
    std::vector<Player*> const receivers = Receivers();
    if (receivers.empty())
        return snap;

    uint32 const giverKnown = ReagentIndex::Known(bot);
    ForEachBagItem(bot,
                   [&](Item* item)
                   {
                       ItemTemplate const* proto = item->GetTemplate();
                       bool const gear = proto->Class == ITEM_CLASS_ARMOR || proto->Class == ITEM_CLASS_WEAPON;
                       if (!item->CanBeTraded() || (!gear && !ReagentIndex::UsedBy(proto->ItemId)))
                           return;
                       snap.items.push_back(Describe(item, receivers, giverKnown));
                   });
    return snap;
}

std::vector<Player*> ShareItemAction::Receivers()
{
    std::vector<Player*> receivers;
    Group* group = bot->GetGroup();
    if (!group)
        return receivers;
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (member && member != bot && member->IsAlive() && GET_PLAYERBOT_AI(member) &&
            bot->GetDistance(member) <= ShareDistance)
            receivers.push_back(member);
    }
    return receivers;
}

ShareItem ShareItemAction::Describe(Item* item, std::vector<Player*> const& receivers, uint32 giverKnown)
{
    uint32 const itemId = item->GetEntry();
    uint32 const usedBy = ReagentIndex::UsedBy(itemId);

    ShareItem s;
    s.id = item->GetGUID().GetRawValue();
    s.usage = ToShareUsage(AI_VALUE2(ItemUsage, "item usage", int32(itemId)));
    s.tier = TierFor(usedBy, giverKnown);
    for (Player* receiver : receivers)
    {
        ShareReceiver r;
        r.guid = receiver->GetGUID().GetRawValue();
        r.usage = ToShareUsage(UsageFor(receiver, itemId));
        r.tier = TierFor(usedBy, ReagentIndex::Known(receiver));
        r.gain = (r.usage == ShareUsage::Equip || r.usage == ShareUsage::Replace) ? GainFor(receiver, item) : 0.f;
        r.held = receiver->GetItemCount(itemId, true);
        s.receivers.push_back(r);
    }
    return s;
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
