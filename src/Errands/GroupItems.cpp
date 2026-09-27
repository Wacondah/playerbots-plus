/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "GroupItems.h"

#include "Group.h"
#include "ItemUsageValue.h"
#include "Playerbots.h"
#include "PlayerbotsPlusConfig.h"
#include "ReagentIndex.h"
#include "StatsWeightCalculator.h"

#include <algorithm>

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
}  // namespace

std::vector<Player*> GroupBots(Player* bot, float maxDistance)
{
    std::vector<Player*> bots;
    Group* group = bot->GetGroup();
    if (!group)
        return bots;
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || member == bot || !member->IsAlive() || !GET_PLAYERBOT_AI(member) ||
            member->GetMapId() != bot->GetMapId())
            continue;
        if (maxDistance > 0.f && bot->GetDistance(member) > maxDistance)
            continue;
        bots.push_back(member);
    }
    return bots;
}

ShareItem DescribeForGroup(PlayerbotAI* holderAI, Item* item, std::vector<Player*> const& others)
{
    uint32 const itemId = item->GetEntry();
    uint32 const usedBy = ReagentIndex::UsedBy(itemId);

    ShareItem s;
    s.id = item->GetGUID().GetRawValue();
    s.usage = ToShareUsage(UsageFor(holderAI->GetBot(), itemId));
    s.tier = TierFor(usedBy, ReagentIndex::Known(holderAI->GetBot()));
    s.masterGain = MasterGain(RealMaster(holderAI), item->GetTemplate(), item->GetItemRandomPropertyId());
    s.holderCanDisenchant = CanDisenchant(holderAI->GetBot(), item->GetTemplate());
    s.groupCanDisenchant = s.holderCanDisenchant;
    uint32 bestEnchanting = 0;
    for (Player* other : others)
    {
        if (CanDisenchant(other, item->GetTemplate()))
        {
            s.groupCanDisenchant = true;
            if (other->GetSkillValue(SKILL_ENCHANTING) > bestEnchanting)
            {
                bestEnchanting = other->GetSkillValue(SKILL_ENCHANTING);
                s.disenchanter = other->GetGUID().GetRawValue();
            }
        }
        ShareReceiver r;
        r.guid = other->GetGUID().GetRawValue();
        r.usage = ToShareUsage(UsageFor(other, itemId));
        r.tier = TierFor(usedBy, ReagentIndex::Known(other));
        r.gain = (r.usage == ShareUsage::Equip || r.usage == ShareUsage::Replace) ? GainFor(other, item) : 0.f;
        r.held = other->GetItemCount(itemId, true);
        s.receivers.push_back(r);
    }
    return s;
}

float MasterGain(Player* master, ItemTemplate const* proto, int32 randomProperty)
{
    if (!master || !proto || master->CanUseItem(proto) != EQUIP_ERR_OK)
        return 0.f;

    if (proto->Class == ITEM_CLASS_CONTAINER)
    {
        if (proto->SubClass != ITEM_SUBCLASS_CONTAINER)
            return 0.f;
        uint32 smallest = proto->ContainerSlots;
        for (uint8 bagSlot = INVENTORY_SLOT_BAG_START; bagSlot < INVENTORY_SLOT_BAG_END; ++bagSlot)
        {
            Bag* bag = master->GetBagByPos(bagSlot);
            uint32 const size = bag ? bag->GetBagSize() : 0;  // an empty bag slot counts as size 0
            smallest = std::min(smallest, size);
        }
        return float(proto->ContainerSlots - smallest);
    }

    if (proto->Class != ITEM_CLASS_ARMOR && proto->Class != ITEM_CLASS_WEAPON)
        return 0.f;
    uint8 const slot = master->FindEquipSlot(proto, NULL_SLOT, true);
    if (slot == NULL_SLOT)
        return 0.f;
    StatsWeightCalculator calc(master);
    Item* current = master->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
    float const currentScore =
        current ? calc.CalculateItem(current->GetEntry(), current->GetItemRandomPropertyId()) : 0.f;
    return std::max(0.f, calc.CalculateItem(proto->ItemId, randomProperty) - currentScore);
}

bool CanDisenchant(Player* player, ItemTemplate const* proto)
{
    PlayerbotAI* ai = GET_PLAYERBOT_AI(player);
    return ai && proto && proto->DisenchantID && proto->Quality >= ITEM_QUALITY_UNCOMMON &&
           proto->Quality <= Config().maxDisenchantQuality &&
           (proto->Class == ITEM_CLASS_ARMOR || proto->Class == ITEM_CLASS_WEAPON) &&
           player->GetSkillValue(SKILL_ENCHANTING) >= proto->RequiredDisenchantSkill &&
           player->HasSkill(SKILL_ENCHANTING) && ai->HasStrategy("errands craft", BotState::BOT_STATE_NON_COMBAT);
}

Player* RealMaster(PlayerbotAI* botAI)
{
    Player* master = botAI->GetMaster();
    return master && master->IsInWorld() && !GET_PLAYERBOT_AI(master) ? master : nullptr;
}

uint32 CountInBags(std::vector<Player*> const& players, uint32 entry)
{
    uint32 count = 0;
    for (Player* player : players)
        ForEachBagItem(player,
                       [&](Item* item)
                       {
                           if (item->GetEntry() == entry)
                               count += item->GetCount();
                       });
    return count;
}

uint32 FreeSlots(Player* player)
{
    uint32 free = 0;
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
        if (!player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            ++free;
    for (uint8 bagSlot = INVENTORY_SLOT_BAG_START; bagSlot < INVENTORY_SLOT_BAG_END; ++bagSlot)
        if (Bag* bag = player->GetBagByPos(bagSlot))
            if (bag->GetTemplate()->SubClass == ITEM_SUBCLASS_CONTAINER)
                free += bag->GetFreeSlots();
    return free;
}

bool GiveItemTo(Player* giver, Item* item, Player* receiver)
{
    ItemPosCountVec dest;
    if (receiver->CanStoreItem(NULL_BAG, NULL_SLOT, dest, item, false) != EQUIP_ERR_OK)
        return false;
    giver->MoveItemFromInventory(item->GetBagSlot(), item->GetSlot(), true);
    item->SetOwnerGUID(receiver->GetGUID());
    receiver->MoveItemToInventory(dest, item, true);
    return true;
}
}  // namespace PlayerbotsPlus
