/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_GROUP_ITEMS_H
#define PLAYERBOTS_PLUS_GROUP_ITEMS_H

#include "Bag.h"
#include "Player.h"
#include "SharePlanner.h"

#include <vector>

class PlayerbotAI;
struct ItemTemplate;

namespace PlayerbotsPlus
{
// Every item in the bot's bags (never equipped ones).
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

// The item with this instance GUID in the bot's bags, nullptr if it is gone.
Item* FindBagItem(Player* bot, uint64 guid);

// Armor, weapon or bag: equipped rather than consumed.
bool IsGear(ItemTemplate const* proto);

// How many of the item the player's incomplete quests require (0: none), whatever
// mod-playerbots' item usage says (Goretusk Liver is also a cooking reagent).
uint32 QuestItemNeed(Player* player, uint32 entry);

// The bot or another bot of its group needs the item for a quest.
bool QuestNeededByGroup(Player* bot, uint32 entry);

// Other living bots of the bot's group on its map; maxDistance 0 means any distance.
std::vector<Player*> GroupBots(Player* bot, float maxDistance);

// What the item means to its holder and to each of `others` (usage, tier, gain, held).
ShareItem DescribeForGroup(PlayerbotAI* holderAI, Item* item, std::vector<Player*> const& others);

// How much better the item would be for a real player: extra slots for a bag larger
// than their smallest one, score gain for gear. 0 for consumables or anything unusable.
float MasterGain(Player* master, ItemTemplate const* proto, int32 randomProperty);

// A bot with "errands craft" and enough enchanting that would disenchant this item
// (green or better up to MaxDisenchantQuality, weapon or armor).
bool CanDisenchant(Player* player, ItemTemplate const* proto);

// The bot's master when it is a real player, else nullptr.
Player* RealMaster(PlayerbotAI* botAI);

// Copies of the item in the bags (not equipped) of these players.
uint32 CountInBags(std::vector<Player*> const& players, uint32 entry);

// Empty slots in the backpack and regular bags (profession bags excluded).
uint32 FreeSlots(Player* player);

// Moves the whole item into the receiver's bags, merging into partial stacks
// (mirrors mod-playerbots' GiveItemAction). False, nothing moved, if it does not fit.
bool GiveItemTo(Player* giver, Item* item, Player* receiver);
}  // namespace PlayerbotsPlus

#endif
