/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "ShoppingPlanner.h"

#include <algorithm>

namespace PlayerbotsPlus
{
namespace
{
uint32_t CeilDiv(uint32_t a, uint32_t b)
{
    return (a + b - 1) / b;
}

// Purchases for `crafts` crafts; `slots` receives the bag slots they take.
ShoppingList ListFor(RecipeOption const& recipe, uint32_t crafts, uint32_t& slots)
{
    ShoppingList list;
    list.crafts = crafts;
    slots = 0;
    for (ReagentNeed const& r : recipe.reagents)
    {
        uint32_t const need = crafts * r.perCraft;
        if (!r.vendor || r.held >= need)
            continue;
        uint32_t const lotSize = std::max<uint32_t>(r.lotSize, 1);
        Purchase p;
        p.item = r.item;
        p.lots = CeilDiv(need - r.held, lotSize);
        p.count = p.lots * lotSize;
        p.cost = p.lots * r.lotPrice;
        list.cost += p.cost;
        slots += CeilDiv(p.count, std::max<uint32_t>(r.maxStack, 1));
        list.purchases.push_back(p);
    }
    return list;
}
}  // namespace

uint32_t ShoppingReserve(uint32_t level, uint32_t perTenLevels)
{
    return level / 10 * perTenLevels;
}

ShoppingList PlanShopping(RecipeOption const& recipe, ShoppingBudget const& budget)
{
    uint32_t limit = recipe.gear ? 1 : budget.maxCrafts;
    for (ReagentNeed const& r : recipe.reagents)
        if (!r.vendor && r.perCraft)
            limit = std::min(limit, r.held / r.perCraft);
    ShoppingList none;
    if (!limit)
    {
        none.reason = "missing materials";
        return none;
    }

    uint32_t const spendable =
        budget.money > budget.reserve ? std::min(budget.cap, budget.money - budget.reserve) : 0;
    bool tooExpensive = false;
    for (uint32_t crafts = limit; crafts > 0; --crafts)
    {
        uint32_t slots = 0;
        ShoppingList list = ListFor(recipe, crafts, slots);
        if (list.purchases.empty())
        {
            none.reason = "nothing to buy";
            return none;
        }
        tooExpensive = list.cost > spendable;
        if (!tooExpensive && slots <= budget.freeSlots)
        {
            list.reason = "shopping";
            return list;
        }
    }
    none.reason = tooExpensive ? "no budget" : "bags full";
    return none;
}
}  // namespace PlayerbotsPlus
