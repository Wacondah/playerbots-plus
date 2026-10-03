/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "GroupItems.h"

#include "Group.h"
#include "ItemUsageValue.h"
#include "ObjectMgr.h"
#include "Playerbots.h"
#include "PlayerbotsPlusConfig.h"
#include "RandomItemMgr.h"
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
    ItemUsage const usage =
        ai ? ai->GetAiObjectContext()->GetValue<ItemUsage>("item usage", int32(itemId))->Get() : ITEM_USAGE_NONE;
    return FittingUsage(player, sObjectMgr->GetItemTemplate(itemId), usage);
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

// mod-playerbots' class rule (the bots' own "item usage" applies it): the class's main
// armor type at that level (mail for a paladin before 40, plate after), its weapon types.
bool FitsClass(Player* player, ItemTemplate const* proto, uint32 level)
{
    if (proto->Class == ITEM_CLASS_ARMOR)
        return sRandomItemMgr.CanEquipArmor(proto, player->getClass(), level);
    if (proto->Class == ITEM_CLASS_WEAPON)
        return sRandomItemMgr.CanEquipWeapon(proto, player->getClass());
    return true;
}

ItemUsage FittingUsage(Player* player, ItemTemplate const* proto, ItemUsage usage)
{
    bool const wear = usage == ITEM_USAGE_EQUIP || usage == ITEM_USAGE_REPLACE || usage == ITEM_USAGE_BAD_EQUIP;
    return wear && proto && !FitsClass(player, proto, player->GetLevel()) ? ITEM_USAGE_NONE : usage;
}

Item* FindBagItem(Player* bot, uint64 guid)
{
    Item* found = nullptr;
    ForEachBagItem(bot,
                   [&](Item* item)
                   {
                       if (!found && item->GetGUID().GetRawValue() == guid)
                           found = item;
                   });
    return found;
}

bool IsGear(ItemTemplate const* proto)
{
    return proto->Class == ITEM_CLASS_ARMOR || proto->Class == ITEM_CLASS_WEAPON ||
           proto->Class == ITEM_CLASS_CONTAINER;
}

namespace
{
// Equipment slots a piece of that inventory type goes to (two for rings, trinkets and
// one-hand weapons of a dual wielder); empty for anything else.
std::vector<uint8> SlotsFor(Player* bot, ItemTemplate const* proto)
{
    switch (proto->InventoryType)
    {
        case INVTYPE_HEAD: return {EQUIPMENT_SLOT_HEAD};
        case INVTYPE_NECK: return {EQUIPMENT_SLOT_NECK};
        case INVTYPE_SHOULDERS: return {EQUIPMENT_SLOT_SHOULDERS};
        case INVTYPE_CHEST:
        case INVTYPE_ROBE: return {EQUIPMENT_SLOT_CHEST};
        case INVTYPE_WAIST: return {EQUIPMENT_SLOT_WAIST};
        case INVTYPE_LEGS: return {EQUIPMENT_SLOT_LEGS};
        case INVTYPE_FEET: return {EQUIPMENT_SLOT_FEET};
        case INVTYPE_WRISTS: return {EQUIPMENT_SLOT_WRISTS};
        case INVTYPE_HANDS: return {EQUIPMENT_SLOT_HANDS};
        case INVTYPE_FINGER: return {EQUIPMENT_SLOT_FINGER1, EQUIPMENT_SLOT_FINGER2};
        case INVTYPE_TRINKET: return {EQUIPMENT_SLOT_TRINKET1, EQUIPMENT_SLOT_TRINKET2};
        case INVTYPE_CLOAK: return {EQUIPMENT_SLOT_BACK};
        case INVTYPE_WEAPON:
            if (bot->CanDualWield())
                return {EQUIPMENT_SLOT_MAINHAND, EQUIPMENT_SLOT_OFFHAND};
            return {EQUIPMENT_SLOT_MAINHAND};
        case INVTYPE_2HWEAPON:
        case INVTYPE_WEAPONMAINHAND: return {EQUIPMENT_SLOT_MAINHAND};
        case INVTYPE_SHIELD:
        case INVTYPE_WEAPONOFFHAND:
        case INVTYPE_HOLDABLE: return {EQUIPMENT_SLOT_OFFHAND};
        case INVTYPE_RANGED:
        case INVTYPE_RANGEDRIGHT:
        case INVTYPE_THROWN:
        case INVTYPE_RELIC: return {EQUIPMENT_SLOT_RANGED};
        default: return {};
    }
}
}  // namespace

bool DescribeFutureGear(Player* bot, ItemTemplate const* proto, int32 randomProperty, uint64 id, FutureItem& out)
{
    if (!proto || proto->Quality < ITEM_QUALITY_UNCOMMON ||
        (proto->Class != ITEM_CLASS_ARMOR && proto->Class != ITEM_CLASS_WEAPON) ||
        bot->CanUseItem(proto) != EQUIP_ERR_CANT_EQUIP_LEVEL_I ||  // the level check comes last
        !WithinLevelAhead(bot->GetLevel(), proto->RequiredLevel, Config().gearMaxLevelAhead) ||
        !FitsClass(bot, proto, std::max<uint32>(bot->GetLevel(), proto->RequiredLevel)))
        return false;
    std::vector<uint8> const slots = SlotsFor(bot, proto);
    if (slots.empty())
        return false;

    StatsWeightCalculator calc(bot);
    out.id = id;
    out.group = slots.front();
    out.capacity = uint32(slots.size());
    out.score = calc.CalculateItem(proto->ItemId, randomProperty);
    out.worn = -1.f;
    for (uint8 slot : slots)
    {
        Item* worn = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        float const score = worn ? calc.CalculateItem(worn->GetEntry(), worn->GetItemRandomPropertyId()) : 0.f;
        out.worn = out.worn < 0.f ? score : std::min(out.worn, score);
    }
    return true;
}

std::vector<FutureItem> KeptFutureGear(Player* bot)
{
    std::vector<FutureItem> all;
    ForEachBagItem(bot,
                   [&](Item* item)
                   {
                       FutureItem f;
                       if (DescribeFutureGear(bot, item->GetTemplate(), item->GetItemRandomPropertyId(),
                                              item->GetGUID().GetRawValue(), f))
                           all.push_back(f);
                   });
    std::set<uint64_t> const kept = KeepFutureGear(all);
    std::vector<FutureItem> out;
    for (FutureItem const& f : all)
        if (kept.count(f.id))
            out.push_back(f);
    return out;
}

std::set<uint64> KeptFutureGearIds(Player* bot)
{
    std::set<uint64> ids;
    for (FutureItem const& f : KeptFutureGear(bot))
        ids.insert(f.id);
    return ids;
}

uint32 QuestItemNeed(Player* player, uint32 entry)
{
    uint32 needed = 0;
    for (auto const& [questId, status] : player->getQuestStatusMap())
    {
        // Complete but not turned in: the items are taken at turn-in, so they are still needed.
        bool const open = status.Status == QUEST_STATUS_INCOMPLETE || status.Status == QUEST_STATUS_COMPLETE;
        Quest const* quest = open ? sObjectMgr->GetQuestTemplate(questId) : nullptr;
        for (uint8 i = 0; quest && i < QUEST_ITEM_OBJECTIVES_COUNT; ++i)
            if (quest->RequiredItemId[i] == entry)
                needed += quest->RequiredItemCount[i];
    }
    return needed;
}

bool QuestNeededByGroup(Player* bot, uint32 entry)
{
    if (QuestItemNeed(bot, entry))
        return true;
    for (Player* mate : GroupBots(bot, 0.f))
        if (QuestItemNeed(mate, entry))
            return true;
    return false;
}

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

    if ((proto->Class != ITEM_CLASS_ARMOR && proto->Class != ITEM_CLASS_WEAPON) ||
        !FitsClass(master, proto, master->GetLevel()))
        return 0.f;
    uint8 const slot = master->FindEquipSlot(proto, NULL_SLOT, true);
    if (slot == NULL_SLOT)
        return 0.f;
    StatsWeightCalculator calc(master);
    Item* current = master->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
    float const currentScore =
        current ? calc.CalculateItem(current->GetEntry(), current->GetItemRandomPropertyId()) : 0.f;
    float const score = calc.CalculateItem(proto->ItemId, randomProperty);
    // Same margin as the bots' own upgrades: a marginal gain is not worth a trade.
    if (current && score <= currentScore * sPlayerbotAIConfig.equipUpgradeThreshold)
        return 0.f;
    return std::max(0.f, score - currentScore);
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
