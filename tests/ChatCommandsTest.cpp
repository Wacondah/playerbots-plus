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
    EXPECT_EQ(ErrandsStrategies(true), "+errands,+errands hunt,+errands share,+errands bags,+errands craft");
    EXPECT_EQ(ErrandsStrategies(false), "-errands,-errands hunt,-errands share,-errands bags,-errands craft");
}
