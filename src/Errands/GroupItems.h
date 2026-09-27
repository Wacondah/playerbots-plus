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

// Other living bots of the bot's group on its map; maxDistance 0 means any distance.
std::vector<Player*> GroupBots(Player* bot, float maxDistance);

// What the item means to its holder and to each of `others` (usage, tier, gain, held).
ShareItem DescribeForGroup(PlayerbotAI* holderAI, Item* item, std::vector<Player*> const& others);

// Empty slots in the backpack and regular bags (profession bags excluded).
uint32 FreeSlots(Player* player);

// Moves the whole item into the receiver's bags, merging into partial stacks
// (mirrors mod-playerbots' GiveItemAction). False, nothing moved, if it does not fit.
bool GiveItemTo(Player* giver, Item* item, Player* receiver);
}  // namespace PlayerbotsPlus

#endif
