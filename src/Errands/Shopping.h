/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_SHOPPING_H
#define PLAYERBOTS_PLUS_SHOPPING_H

#include "ShoppingPlanner.h"

class Creature;
class Player;

// Core side of the craft shopping list: vendors that sell it, and buying.
namespace PlayerbotsPlus
{
// The vendor sells an item of the list without a supply limit.
bool SellsShopping(Creature* npc, ShoppingList const& list);

// Buys the list's lots this vendor sells, a stack at a time. True if anything was bought.
bool BuyShoppingAt(Player* bot, Creature* npc, ShoppingList const& list);
}  // namespace PlayerbotsPlus

#endif
