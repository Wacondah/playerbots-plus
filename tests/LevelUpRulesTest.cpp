#include "LevelUpRules.h"

#include <gtest/gtest.h>

using namespace PlayerbotsPlus;

TEST(TalentSource, NothingWithoutFreePoints)
{
    EXPECT_EQ(PickTalentSource(0, true, 10), TalentSource::None);
}

TEST(TalentSource, StoredSpecFirstThenCurrentTreeThenAsk)
{
    EXPECT_EQ(PickTalentSource(1, true, 0), TalentSource::Stored);
    EXPECT_EQ(PickTalentSource(1, true, 12), TalentSource::Stored);
    EXPECT_EQ(PickTalentSource(1, false, 12), TalentSource::Current);
    EXPECT_EQ(PickTalentSource(1, false, 0), TalentSource::Ask);
}

TEST(QuestShare, OnlyToABotThatCanTakeIt)
{
    QuestShareCheck ok;
    ok.sharable = true;
    ok.targetIsBot = true;
    ok.inRange = true;
    ok.targetCanTake = true;
    EXPECT_TRUE(ShouldShareQuest(ok));

    for (bool QuestShareCheck::*field : {&QuestShareCheck::sharable, &QuestShareCheck::targetIsBot,
                                         &QuestShareCheck::inRange, &QuestShareCheck::targetCanTake})
    {
        QuestShareCheck c = ok;
        c.*field = false;
        EXPECT_FALSE(ShouldShareQuest(c));
    }
}

TEST(LootRoll, NeedForGearItWouldWear)
{
    LootFacts f;
    f.gearUpgrade = true;
    EXPECT_TRUE(ShouldRollNeed(f));
    f.uniqueHeld = true;
    EXPECT_FALSE(ShouldRollNeed(f));
}

TEST(LootRoll, NeedForMaterialsOfItsOwnProfessions)
{
    LootFacts f;
    f.ownMaterial = true;
    EXPECT_TRUE(ShouldRollNeed(f));
    EXPECT_FALSE(ShouldRollNeed(LootFacts{}));  // anything else: mod-playerbots decides
}

TEST(Release, OnlyWhenNobodyCanResurrect)
{
    ReleaseFacts f;
    f.dead = true;
    f.calmMs = 10000;
    EXPECT_TRUE(ShouldRelease(f, 10000));
    f.someoneCanResurrect = true;
    EXPECT_FALSE(ShouldRelease(f, 10000));
}

TEST(Release, WaitsForCalmAndNeverInDungeonsOrAsGhost)
{
    ReleaseFacts f;
    f.dead = true;
    f.calmMs = 9999;
    EXPECT_FALSE(ShouldRelease(f, 10000));
    f.calmMs = 10000;
    for (bool ReleaseFacts::*field : {&ReleaseFacts::ghost, &ReleaseFacts::inDungeon})
    {
        ReleaseFacts c = f;
        c.*field = true;
        EXPECT_FALSE(ShouldRelease(c, 10000));
    }
    f.dead = false;
    EXPECT_FALSE(ShouldRelease(f, 10000));
}
