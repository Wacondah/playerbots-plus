#include "ChatCommands.h"

#include <gtest/gtest.h>

using namespace PlayerbotsPlus;

TEST(ErrandsSwitch, OnOffCaseAndSpaces)
{
    EXPECT_EQ(ParseErrandsSwitch("errands on"), ErrandsSwitch::On);
    EXPECT_EQ(ParseErrandsSwitch("  Errands OFF "), ErrandsSwitch::Off);
}

TEST(ErrandsSwitch, AnythingElseIsNone)
{
    EXPECT_EQ(ParseErrandsSwitch("errands"), ErrandsSwitch::None);
    EXPECT_EQ(ParseErrandsSwitch("errands onward"), ErrandsSwitch::None);
    EXPECT_EQ(ParseErrandsSwitch("nc +errands"), ErrandsSwitch::None);
}

TEST(ErrandsSwitch, StrategyLists)
{
    EXPECT_EQ(ErrandsStrategies(true), "+errands,+errands hunt,+errands share,+errands bags,+errands craft,"
                                       "+errands levelup,+errands quests,+errands loot,+errands pull");
    EXPECT_EQ(ErrandsStrategies(false), "-errands,-errands hunt,-errands share,-errands bags,-errands craft,"
                                        "-errands levelup,-errands quests,-errands loot,-errands pull");
    EXPECT_EQ(ErrandsDeadStrategies(true), "+errands revive");
    EXPECT_EQ(ErrandsDeadStrategies(false), "-errands revive");
    EXPECT_EQ(ErrandsCombatStrategies(true), "+errands loot,+errands pull");
    EXPECT_EQ(ErrandsCombatStrategies(false), "-errands loot,-errands pull");
}

TEST(PullCommand, Parse)
{
    EXPECT_EQ(ParsePullCommand("pull"), PullCommand::Pull);
    EXPECT_EQ(ParsePullCommand("  Pull "), PullCommand::Pull);
    EXPECT_EQ(ParsePullCommand("pull force"), PullCommand::Force);
    EXPECT_EQ(ParsePullCommand("PULL cancel"), PullCommand::Cancel);
    EXPECT_EQ(ParsePullCommand("pull rti"), PullCommand::None);  // mod-playerbots' own variant
    EXPECT_EQ(ParsePullCommand("pulling"), PullCommand::None);
    EXPECT_EQ(ParsePullCommand(""), PullCommand::None);
}

TEST(TalentsSpec, NameAfterTheCommand)
{
    EXPECT_EQ(ParseTalentsSpec("talents spec resto pve"), "resto pve");
    EXPECT_EQ(ParseTalentsSpec("  Talents Spec holy pve "), "holy pve");  // name kept as typed
}

TEST(TalentsSpec, OtherCommandsGiveNothing)
{
    EXPECT_EQ(ParseTalentsSpec("talents spec list"), "");
    EXPECT_EQ(ParseTalentsSpec("talents spec "), "");
    EXPECT_EQ(ParseTalentsSpec("talents autopick"), "");
    EXPECT_EQ(ParseTalentsSpec("errands on"), "");
}
