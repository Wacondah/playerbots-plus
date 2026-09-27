#include "BagPlanner.h"

#include <gtest/gtest.h>

using namespace PlayerbotsPlus;

namespace
{
BagStack Stack(uint64_t id, uint32_t entry, uint32_t count, uint8_t tier = 0)
{
    return BagStack{id, entry, count, 20, true, tier};
}

BagMate Mate(uint64_t guid, uint32_t freeSlots)
{
    BagMate m;
    m.guid = guid;
    m.freeSlots = freeSlots;
    return m;
}

BagSnapshot Full()
{
    BagSnapshot snap;
    snap.freeSlots = 0;
    return snap;
}
}  // namespace

TEST(Bags, EnoughSpaceDoesNothing)
{
    BagSnapshot snap = Full();
    snap.freeSlots = 1;
    snap.stacks = {Stack(1, 100, 3)};
    snap.mates = {Mate(10, 5)};
    BagState state;
    BagMove m = PlanBags(snap, state, BagConfig{});
    EXPECT_FALSE(m.Acts());
    EXPECT_EQ(m.reason, "enough space");
}

TEST(Bags, MergePreferredOverSlot)
{
    BagSnapshot snap = Full();
    snap.stacks = {Stack(1, 100, 3), Stack(2, 200, 1)};
    BagMate withRoom = Mate(10, 0);
    withRoom.room[100] = 5;
    snap.mates = {Mate(20, 8), withRoom};
    BagState state;
    BagMove m = PlanBags(snap, state, BagConfig{});
    EXPECT_EQ(m.item, 1u);
    EXPECT_EQ(m.receiver, 10u);
    EXPECT_TRUE(m.merge);
}

TEST(Bags, MergeNeedsRoomForWholeStack)
{
    BagSnapshot snap = Full();
    snap.stacks = {Stack(1, 100, 6)};
    BagMate m1 = Mate(10, 0);
    m1.room[100] = 5;
    snap.mates = {m1};
    BagState state;
    EXPECT_FALSE(PlanBags(snap, state, BagConfig{}).Acts());
}

TEST(Bags, SmallestMergeableStackFirst)
{
    BagSnapshot snap = Full();
    snap.stacks = {Stack(1, 100, 4), Stack(2, 200, 2)};
    BagMate m1 = Mate(10, 0);
    m1.room[100] = 10;
    m1.room[200] = 10;
    snap.mates = {m1};
    BagState state;
    EXPECT_EQ(PlanBags(snap, state, BagConfig{}).item, 2u);
}

TEST(Bags, SlotMoveKeepsReceiverAboveThreshold)
{
    BagSnapshot snap = Full();
    snap.stacks = {Stack(1, 100, 3)};
    snap.mates = {Mate(10, 1), Mate(20, 2), Mate(30, 4)};
    BagState state;
    BagMove m = PlanBags(snap, state, BagConfig{});
    EXPECT_EQ(m.receiver, 30u);
    EXPECT_FALSE(m.merge);

    snap.mates = {Mate(10, 1)};  // would drop to 0
    BagState s2;
    EXPECT_FALSE(PlanBags(snap, s2, BagConfig{}).Acts());
}

TEST(Bags, LowerTierReceiverRefused)
{
    BagSnapshot snap = Full();
    snap.stacks = {Stack(1, 100, 3, 2)};
    BagMate lower = Mate(10, 5);
    lower.room[100] = 10;
    lower.tier[100] = 1;
    BagMate same = Mate(20, 5);
    same.tier[100] = 2;
    snap.mates = {lower, same};
    BagState state;
    BagMove m = PlanBags(snap, state, BagConfig{});
    EXPECT_EQ(m.receiver, 20u);
    EXPECT_FALSE(m.merge);
}

TEST(Bags, NonMovableAndSingleItemsSkipped)
{
    BagSnapshot snap = Full();
    BagStack locked = Stack(1, 100, 3);
    locked.movable = false;
    BagStack single = Stack(2, 200, 1);
    single.maxStack = 1;
    snap.stacks = {locked, single};
    snap.mates = {Mate(10, 5)};
    BagState state;
    EXPECT_FALSE(PlanBags(snap, state, BagConfig{}).Acts());
}

TEST(Bags, WarnOnceThenRearmAfterRecovery)
{
    BagSnapshot snap = Full();
    snap.stacks = {Stack(1, 100, 3)};
    BagState state;
    BagMove m1 = PlanBags(snap, state, BagConfig{});
    EXPECT_TRUE(m1.warn);
    EXPECT_EQ(m1.reason, "bags full, nothing to rebalance");
    EXPECT_FALSE(PlanBags(snap, state, BagConfig{}).warn);

    snap.freeSlots = 1;
    PlanBags(snap, state, BagConfig{});
    snap.freeSlots = 0;
    EXPECT_TRUE(PlanBags(snap, state, BagConfig{}).warn);
}
