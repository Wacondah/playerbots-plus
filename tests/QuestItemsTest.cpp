#include "QuestItems.h"

#include <gtest/gtest.h>

using namespace PlayerbotsPlus;

TEST(QuestItems, SurplusGoesToTheBiggestShortage)
{
    // Brogan holds 11 livers for 8, Kyra has 5 for 8, Ashyra 7 for 8.
    std::vector<QuestNeed> group = {{1, 11, 8}, {2, 5, 8}, {3, 7, 8}};
    QuestTransfer t = PlanQuestTransfer(1, group);
    EXPECT_EQ(t.to, 2u);
    EXPECT_EQ(t.count, 3u);
}

TEST(QuestItems, NoQuestMeansAllIsSurplus)
{
    std::vector<QuestNeed> group = {{1, 4, 0}, {2, 0, 8}};
    QuestTransfer t = PlanQuestTransfer(1, group);
    EXPECT_EQ(t.to, 2u);
    EXPECT_EQ(t.count, 4u);
}

TEST(QuestItems, HolderKeepsWhatItsQuestNeeds)
{
    std::vector<QuestNeed> group = {{1, 8, 8}, {2, 0, 8}};
    EXPECT_FALSE(PlanQuestTransfer(1, group).Acts());
    std::vector<QuestNeed> shortToo = {{1, 3, 8}, {2, 0, 8}};
    EXPECT_FALSE(PlanQuestTransfer(1, shortToo).Acts());
}

TEST(QuestItems, NobodyShortNothingToDo)
{
    std::vector<QuestNeed> group = {{1, 11, 8}, {2, 8, 8}, {3, 0, 0}};
    EXPECT_FALSE(PlanQuestTransfer(1, group).Acts());
}

TEST(QuestItems, TieGoesToTheLowestGuid)
{
    std::vector<QuestNeed> group = {{1, 6, 0}, {3, 0, 2}, {2, 0, 2}};
    QuestTransfer t = PlanQuestTransfer(1, group);
    EXPECT_EQ(t.to, 2u);
    EXPECT_EQ(t.count, 2u);
}
