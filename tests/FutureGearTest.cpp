#include "FutureGear.h"

#include <gtest/gtest.h>

using namespace PlayerbotsPlus;

namespace
{
constexpr uint32_t Legs = 7, Finger = 11;
}  // namespace

TEST(FutureGear, KeepsOnlyUpgradesOverWhatIsWorn)
{
    std::vector<FutureItem> items = {{1, Legs, 1, 30.f, 20.f}, {2, Legs, 1, 15.f, 20.f}};
    EXPECT_EQ(KeepFutureGear(items), (std::set<uint64_t>{1}));
}

TEST(FutureGear, CompetingPiecesKeepTheBest)
{
    std::vector<FutureItem> items = {{1, Legs, 1, 30.f, 20.f}, {2, Legs, 1, 40.f, 20.f}, {3, Legs, 1, 25.f, 20.f}};
    EXPECT_EQ(KeepFutureGear(items), (std::set<uint64_t>{2}));
}

TEST(FutureGear, TwoSlotGroupsKeepTwo)
{
    std::vector<FutureItem> items = {
        {1, Finger, 2, 10.f, 5.f}, {2, Finger, 2, 12.f, 5.f}, {3, Finger, 2, 8.f, 5.f}, {4, Legs, 1, 30.f, 0.f}};
    EXPECT_EQ(KeepFutureGear(items), (std::set<uint64_t>{1, 2, 4}));
}

TEST(FutureGear, TieGoesToTheLowestId)
{
    std::vector<FutureItem> items = {{5, Legs, 1, 30.f, 20.f}, {4, Legs, 1, 30.f, 20.f}};
    EXPECT_EQ(KeepFutureGear(items), (std::set<uint64_t>{4}));
}

TEST(FutureGear, WouldKeepANewPiece)
{
    std::vector<FutureItem> held = {{1, Legs, 1, 30.f, 20.f}};
    EXPECT_TRUE(WouldKeep({9, Legs, 1, 35.f, 20.f}, held));
    EXPECT_FALSE(WouldKeep({9, Legs, 1, 28.f, 20.f}, held));   // the held one is better
    EXPECT_FALSE(WouldKeep({9, Legs, 1, 18.f, 20.f}, {}));     // not better than what is worn
    EXPECT_TRUE(WouldKeep({9, Finger, 2, 9.f, 5.f}, {{1, Finger, 2, 10.f, 5.f}}));  // second ring slot
}

TEST(FutureGear, LevelWindow)
{
    EXPECT_TRUE(WithinLevelAhead(13, 40, 0));  // 0: no limit
    EXPECT_TRUE(WithinLevelAhead(13, 18, 5));
    EXPECT_FALSE(WithinLevelAhead(13, 19, 5));
}
