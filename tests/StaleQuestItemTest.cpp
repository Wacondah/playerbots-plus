#include "StaleQuestItem.h"

#include <gtest/gtest.h>

using namespace PlayerbotsPlus;

namespace
{
StaleFacts Done()
{
    StaleFacts f;
    f.isQuestItem = true;
    f.quests = 2;
    f.questsRewarded = 2;
    return f;
}
}  // namespace

TEST(StaleQuestItem, DestroyedWhenEveryQuestIsRewarded)
{
    EXPECT_TRUE(IsStaleQuestItem(Done()));
}

TEST(StaleQuestItem, KeptWhileAQuestIsNotRewarded)
{
    StaleFacts f = Done();
    f.questsRewarded = 1;  // the second quest of the chain still needs it
    EXPECT_FALSE(IsStaleQuestItem(f));
}

TEST(StaleQuestItem, UnknownPurposeIsKept)
{
    StaleFacts f = Done();
    f.quests = f.questsRewarded = 0;  // a key, a spell component: no quest mentions it
    EXPECT_FALSE(IsStaleQuestItem(f));
}

TEST(StaleQuestItem, SellableAndNeededItemsAreKept)
{
    StaleFacts f = Done();
    f.sellPrice = 8;  // the sell errand handles it
    EXPECT_FALSE(IsStaleQuestItem(f));
    f = Done();
    f.neededByGroup = true;
    EXPECT_FALSE(IsStaleQuestItem(f));
    f = Done();
    f.neededByMaster = true;
    EXPECT_FALSE(IsStaleQuestItem(f));
    f = Done();
    f.isQuestItem = false;
    EXPECT_FALSE(IsStaleQuestItem(f));
}
