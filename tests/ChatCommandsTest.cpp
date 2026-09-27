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
                                       "+errands levelup,+errands quests");
    EXPECT_EQ(ErrandsStrategies(false), "-errands,-errands hunt,-errands share,-errands bags,-errands craft,"
                                        "-errands levelup,-errands quests");
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
