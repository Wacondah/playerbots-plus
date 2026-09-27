#include "SellRules.h"

#include <gtest/gtest.h>

using namespace PlayerbotsPlus;

namespace
{
FoodItem Bread()
{
    FoodItem f;
    f.isFood = true;
    f.hasSellPrice = true;
    f.quality = 1;
    return f;
}
}  // namespace

TEST(SellFood, UncraftedFoodIsSoldWithFoodCheat)
{
    EXPECT_TRUE(SellableFood(Bread(), true, true, 3));
}

TEST(SellFood, CookedFoodIsKept)
{
    FoodItem f = Bread();
    f.crafted = true;
    EXPECT_FALSE(SellableFood(f, true, true, 3));
}

TEST(SellFood, NeverWithoutFoodCheatOrWhenDisabled)
{
    EXPECT_FALSE(SellableFood(Bread(), false, true, 3));
    EXPECT_FALSE(SellableFood(Bread(), true, false, 3));
}

TEST(SellFood, QualityCapPriceQuestAndNonFood)
{
    FoodItem green = Bread();
    green.quality = 2;
    EXPECT_FALSE(SellableFood(green, true, true, 1));

    FoodItem free = Bread();
    free.hasSellPrice = false;
    EXPECT_FALSE(SellableFood(free, true, true, 3));

    FoodItem quest = Bread();
    quest.quest = true;
    EXPECT_FALSE(SellableFood(quest, true, true, 3));

    FoodItem potion = Bread();
    potion.isFood = false;
    EXPECT_FALSE(SellableFood(potion, true, true, 3));
}
