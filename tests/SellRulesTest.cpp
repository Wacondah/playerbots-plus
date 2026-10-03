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

TEST(OwnSkill, KeptWhenAKnownProfessionConsumesIt)
{
    EXPECT_TRUE(KeptForOwnSkill(ProfessionBit::Alchemy, ProfessionBit::Alchemy, 0, false));
    EXPECT_TRUE(KeptForOwnSkill(ProfessionBit::Engineering, ProfessionBit::Engineering, 0, true));
}

TEST(OwnSkill, CraftedPartsOfAnotherProfessionAreNotKept)
{
    // Copper tube held by an alchemist-herbalist.
    uint32_t const feeds = GatherFeeds(false, true, false);
    EXPECT_FALSE(KeptForOwnSkill(ProfessionBit::Engineering, ProfessionBit::Alchemy, feeds, true));
    // Copper bolts held by a miner: crafted, so no longer a raw material.
    EXPECT_FALSE(KeptForOwnSkill(ProfessionBit::Engineering, 0, GatherFeeds(true, false, false), true));
}

TEST(OwnSkill, GathererKeepsRawMaterialsOfItsCrafts)
{
    EXPECT_TRUE(KeptForOwnSkill(ProfessionBit::Blacksmithing, 0, GatherFeeds(true, false, false), false));
    EXPECT_TRUE(KeptForOwnSkill(ProfessionBit::Alchemy, 0, GatherFeeds(false, true, false), false));
    EXPECT_TRUE(KeptForOwnSkill(ProfessionBit::Leatherworking, 0, GatherFeeds(false, false, true), false));
    EXPECT_FALSE(KeptForOwnSkill(ProfessionBit::Tailoring, 0, GatherFeeds(true, true, true), false));
}

TEST(OwnSkill, MinerKeepsItsOre)
{
    uint32_t const feeds = GatherFeeds(true, false, false);
    // Copper ore: only smelting uses it.
    EXPECT_TRUE(KeptForOwnSkill(ProfessionBit::Mining, ProfessionBit::Mining, feeds, false));
    // Iron ore: smelting and an enchantment.
    EXPECT_TRUE(KeptForOwnSkill(ProfessionBit::Mining | ProfessionBit::Enchanting, ProfessionBit::Mining, feeds, false));
    // A smith without mining has no use for ore.
    EXPECT_FALSE(KeptForOwnSkill(ProfessionBit::Mining, ProfessionBit::Blacksmithing, 0, false));
    // Silver bar on a pure miner: still a raw material of its gathering.
    uint32_t const bar = ProfessionBit::Blacksmithing | ProfessionBit::Engineering | ProfessionBit::Jewelcrafting;
    EXPECT_TRUE(KeptForOwnSkill(bar, ProfessionBit::Mining, feeds, false));
}

TEST(OwnSkill, GatherFeedsMirrorsUpstream)
{
    EXPECT_EQ(GatherFeeds(true, false, false),
              ProfessionBit::Blacksmithing | ProfessionBit::Jewelcrafting | ProfessionBit::Engineering);
    EXPECT_EQ(GatherFeeds(false, true, false), ProfessionBit::Alchemy | ProfessionBit::Inscription);
    EXPECT_EQ(GatherFeeds(false, false, true), ProfessionBit::Leatherworking);
    EXPECT_EQ(GatherFeeds(false, false, false), 0u);
}
