/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_SHOPPING_H
#define PLAYERBOTS_PLUS_SHOPPING_H

#include "Define.h"
#include "ShoppingPlanner.h"

class Creature;
class Player;

// Core side of the craft shopping list: vendors that sell it, and buying.
namespace PlayerbotsPlus
{
// The vendor sells an item of the list without a supply limit.
bool SellsShopping(Creature* npc, ShoppingList const& list);
bool SellsShoppingEntry(uint32 entry, ShoppingList const& list);  // from the vendor template

// Buys the list's lots this vendor sells, a stack at a time, keeping `reserve` money.
// True if anything was bought.
bool BuyShoppingAt(Player* bot, Creature* npc, ShoppingList const& list, uint32 reserve);
}  // namespace PlayerbotsPlus

#endif
