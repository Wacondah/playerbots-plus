#include "ShoppingPlanner.h"

#include <gtest/gtest.h>

using namespace PlayerbotsPlus;

namespace
{
constexpr uint32_t Linen = 2996, Thread = 2320;

// Bolt of linen style recipe: 2 linen + 1 coarse thread (lots of 1 at 10 c).
RecipeOption Bag(uint32_t linen, uint32_t thread)
{
    RecipeOption r;
    r.spell = 1;
    r.product = 2;
    r.buyable = true;
    r.reagents = {{Linen, 2, linen, false, 0, 1, 20}, {Thread, 1, thread, true, 10, 1, 20}};
    return r;
}

ShoppingBudget Rich()
{
    ShoppingBudget b;
    b.money = 100000;
    b.cap = 5000;
    b.freeSlots = 10;
    return b;
}
}  // namespace

TEST(Shopping, Reserve)
{
    EXPECT_EQ(ShoppingReserve(9, 10000), 0u);
    EXPECT_EQ(ShoppingReserve(12, 10000), 10000u);
    EXPECT_EQ(ShoppingReserve(80, 10000), 80000u);
}

TEST(Shopping, CraftsFromHeldMaterials)
{
    ShoppingList l = PlanShopping(Bag(10, 1), Rich());
    EXPECT_EQ(l.crafts, 5u);
    ASSERT_EQ(l.purchases.size(), 1u);
    EXPECT_EQ(l.purchases[0].item, Thread);
    EXPECT_EQ(l.purchases[0].count, 4u);
    EXPECT_EQ(l.cost, 40u);
    EXPECT_TRUE(l.Any());
}

TEST(Shopping, MaxCraftsAndGearCap)
{
    ShoppingBudget b = Rich();
    b.maxCrafts = 3;
    EXPECT_EQ(PlanShopping(Bag(100, 0), b).crafts, 3u);
    RecipeOption gear = Bag(100, 0);
    gear.gear = true;
    EXPECT_EQ(PlanShopping(gear, Rich()).crafts, 1u);
}

TEST(Shopping, RoundsUpToVendorLots)
{
    RecipeOption r = Bag(10, 0);
    r.reagents[1].lotSize = 5;
    r.reagents[1].lotPrice = 50;
    ShoppingList l = PlanShopping(r, Rich());
    EXPECT_EQ(l.crafts, 5u);
    EXPECT_EQ(l.purchases[0].lots, 1u);
    EXPECT_EQ(l.purchases[0].count, 5u);
    EXPECT_EQ(l.cost, 50u);
}

TEST(Shopping, BudgetLowersCrafts)
{
    ShoppingBudget b = Rich();
    b.cap = 25;  // 2 threads
    EXPECT_EQ(PlanShopping(Bag(10, 0), b).crafts, 2u);
    b.cap = 5000;
    b.money = 10030;
    b.reserve = 10000;  // 30 c spendable
    EXPECT_EQ(PlanShopping(Bag(10, 0), b).crafts, 3u);
}

TEST(Shopping, NothingWhenOneCraftDoesNotFit)
{
    ShoppingBudget b = Rich();
    b.money = 10005;
    b.reserve = 10000;
    ShoppingList l = PlanShopping(Bag(10, 0), b);
    EXPECT_FALSE(l.Any());
    EXPECT_EQ(l.reason, "no budget");
    b = Rich();
    b.freeSlots = 0;
    EXPECT_EQ(PlanShopping(Bag(10, 0), b).reason, "bags full");
}

TEST(Shopping, MissingMaterials)
{
    EXPECT_EQ(PlanShopping(Bag(1, 0), Rich()).reason, "missing materials");
}

TEST(Shopping, BagSlotsCountStacks)
{
    RecipeOption r = Bag(100, 0);
    r.reagents[1].maxStack = 10;
    ShoppingBudget b = Rich();
    b.freeSlots = 1;  // 10 threads at most
    EXPECT_EQ(PlanShopping(r, b).crafts, 10u);
}

TEST(Shopping, AffordableLotsKeepTheReserve)
{
    EXPECT_EQ(AffordableLots(10100, 10000, 25, 10), 4u);  // 100 c above the reserve
    EXPECT_EQ(AffordableLots(20000, 10000, 25, 10), 10u);
    EXPECT_EQ(AffordableLots(9000, 10000, 25, 10), 0u);  // already under the reserve
    EXPECT_EQ(AffordableLots(500, 0, 0, 7), 7u);          // free lots
}
