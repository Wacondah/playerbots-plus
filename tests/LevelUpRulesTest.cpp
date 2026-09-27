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
