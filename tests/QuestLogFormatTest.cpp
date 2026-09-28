#include "QuestLogFormat.h"

#include <gtest/gtest.h>

using namespace PlayerbotsPlus;

namespace
{
QuestLine Bandanas()
{
    QuestLine l;
    l.id = 153;
    l.status = 1;
    l.title = "Red Leather Bandanas";
    l.objectives = {{15, 15, "Red Leather Bandana"}};
    return l;
}
}  // namespace

TEST(QuestLog, PlainLine)
{
    EXPECT_EQ(FormatQuestLine(Bandanas()), "Q\t153\t1\tRed Leather Bandanas\t15/15:Red Leather Bandana");
}

TEST(QuestLog, SeveralOrNoObjectives)
{
    QuestLine l = Bandanas();
    l.objectives.push_back({3, 8, "Goretusk Liver"});
    EXPECT_EQ(FormatQuestLine(l), "Q\t153\t1\tRed Leather Bandanas\t15/15:Red Leather Bandana|3/8:Goretusk Liver");
    l.objectives.clear();
    EXPECT_EQ(FormatQuestLine(l), "Q\t153\t1\tRed Leather Bandanas\t");
}

TEST(QuestLog, SeparatorsAreCleaned)
{
    EXPECT_EQ(CleanField("a\tb|c"), "a b c");
    QuestLine l = Bandanas();
    l.title = "Evil|Title";
    l.objectives = {{0, 1, "Name\twith tab"}};
    EXPECT_EQ(FormatQuestLine(l), "Q\t153\t1\tEvil Title\t0/1:Name with tab");
}

TEST(QuestLog, NeverOverTheLimit)
{
    QuestLine l = Bandanas();
    l.objectives.clear();
    for (int i = 0; i < 10; ++i)
        l.objectives.push_back({1, 10, std::string(40, 'x')});
    std::string const out = FormatQuestLine(l, 120);
    EXPECT_LE(out.size(), 120u);
    EXPECT_EQ(out.rfind("Q\t153\t1\tRed Leather Bandanas\t1/10:xxxxxxxx", 0), 0u);  // names cut first
}
