/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_SHOPPING_PLANNER_H
#define PLAYERBOTS_PLUS_SHOPPING_PLANNER_H

#include "CraftPlanner.h"

#include <cstdint>
#include <string>
#include <vector>

// Pure decision logic: which vendor reagents to buy for a buyable recipe.
namespace PlayerbotsPlus
{
struct ShoppingBudget
{
    uint32_t money = 0;
    uint32_t reserve = 0;  // never spent (repairs, training)
    uint32_t cap = 0;      // spending cap per vendor visit
    uint32_t maxCrafts = 20;
    uint32_t freeSlots = 0;
};

struct Purchase
{
    uint32_t item = 0;
    uint32_t lots = 0;   // vendor lots (BuyCount items each)
    uint32_t count = 0;  // items
    uint32_t cost = 0;
};

struct ShoppingList
{
    uint32_t crafts = 0;
    std::vector<Purchase> purchases;
    uint32_t cost = 0;
    std::string reason;

    bool Any() const { return crafts > 0 && !purchases.empty(); }
};

// Money kept aside: floor(level / 10) × perTenLevels.
uint32_t ShoppingReserve(uint32_t level, uint32_t perTenLevels);

// As many crafts as the held materials allow (one for gear), lowered until the
// purchases fit the budget and the free bag slots.
ShoppingList PlanShopping(RecipeOption const& recipe, ShoppingBudget const& budget);
}  // namespace PlayerbotsPlus

#endif
