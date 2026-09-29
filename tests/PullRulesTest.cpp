#include "PullRules.h"

#include <gtest/gtest.h>

using namespace PlayerbotsPlus;

namespace
{
HoldFacts Hold(PullStep step, bool puller = false, bool tank = false, bool attacked = false, bool near = false)
{
    HoldFacts f;
    f.step = step;
    f.puller = puller;
    f.tank = tank;
    f.attacked = attacked;
    f.mobNearTank = near;
    return f;
}
}  // namespace

TEST(PullHold, NothingRunningOrWaitingForForce)
{
    EXPECT_TRUE(HoldAllows(Hold(PullStep::None), ActionKind::Attack));
    EXPECT_TRUE(HoldAllows(Hold(PullStep::AwaitingForce), ActionKind::Move));
}

TEST(PullHold, MembersFrozenButHealAndDefend)
{
    EXPECT_FALSE(HoldAllows(Hold(PullStep::Moving), ActionKind::Move));
    EXPECT_FALSE(HoldAllows(Hold(PullStep::Returning), ActionKind::Attack));
    EXPECT_FALSE(HoldAllows(Hold(PullStep::Waiting), ActionKind::OffensiveSpell));
    EXPECT_TRUE(HoldAllows(Hold(PullStep::Moving), ActionKind::Other));  // heals, buffs
    EXPECT_TRUE(HoldAllows(Hold(PullStep::Moving, false, false, true), ActionKind::Attack));
}

TEST(PullHold, PullerFreeUntilHidden)
{
    EXPECT_TRUE(HoldAllows(Hold(PullStep::Moving, true), ActionKind::Move));
    EXPECT_TRUE(HoldAllows(Hold(PullStep::Returning, true), ActionKind::OffensiveSpell));
    EXPECT_FALSE(HoldAllows(Hold(PullStep::Waiting, true), ActionKind::Move));
}

TEST(PullHold, TankWaitsForTheMob)
{
    EXPECT_FALSE(HoldAllows(Hold(PullStep::Returning, false, true), ActionKind::Move));
    EXPECT_TRUE(HoldAllows(Hold(PullStep::Returning, false, true, false, true), ActionKind::Attack));
    EXPECT_TRUE(HoldAllows(Hold(PullStep::Waiting, false, true, false, true), ActionKind::Move));
}

TEST(PullEndRules, ForceWindow)
{
    EndFacts f;
    f.step = PullStep::AwaitingForce;
    f.msSinceStart = PullForceWaitMs - 1;
    EXPECT_EQ(CheckEnd(f), PullEnd::Continue);
    f.msSinceStart = PullForceWaitMs;
    EXPECT_EQ(CheckEnd(f), PullEnd::Cancel);
}

TEST(PullEndRules, CancelWhileMoving)
{
    EndFacts f;
    f.step = PullStep::Moving;
    EXPECT_EQ(CheckEnd(f), PullEnd::Continue);
    f.pullerAlive = false;
    EXPECT_EQ(CheckEnd(f), PullEnd::Cancel);
    f.pullerAlive = true;
    f.targetAlive = false;
    EXPECT_EQ(CheckEnd(f), PullEnd::Cancel);
    f.targetAlive = true;
    f.masterOnOtherTarget = true;
    EXPECT_EQ(CheckEnd(f), PullEnd::Cancel);
    f.masterOnOtherTarget = false;
    f.msSinceStart = PullReachTimeoutMs;
    EXPECT_EQ(CheckEnd(f), PullEnd::Cancel);
}

TEST(PullEndRules, DoneAfterTheShot)
{
    EndFacts f;
    f.step = PullStep::Returning;
    f.anyAttacker = true;
    EXPECT_EQ(CheckEnd(f), PullEnd::Continue);
    f.attackersAllOnTank = true;
    EXPECT_EQ(CheckEnd(f), PullEnd::Done);
    f.attackersAllOnTank = false;
    f.msSinceShot = PullAfterShotMs;
    EXPECT_EQ(CheckEnd(f), PullEnd::Done);
    f.msSinceShot = 0;
    f.anyAttacker = false;
    f.targetAlive = false;
    EXPECT_EQ(CheckEnd(f), PullEnd::Done);
    f.targetAlive = true;
    f.pullerAlive = false;
    EXPECT_EQ(CheckEnd(f), PullEnd::Cancel);
}

TEST(PullBoardTest, PutGetErase)
{
    EXPECT_FALSE(PullBoard::Get(42).has_value());
    PullRun run;
    run.step = PullStep::Moving;
    run.puller = 7;
    PullBoard::Put(42, run);
    ASSERT_TRUE(PullBoard::Get(42).has_value());
    EXPECT_EQ(PullBoard::Get(42)->puller, 7u);
    PullBoard::Erase(42);
    EXPECT_FALSE(PullBoard::Get(42).has_value());
    PullBoard::SetLastSkull(42, 99);
    EXPECT_EQ(PullBoard::LastSkull(42), 99u);
    EXPECT_EQ(PullBoard::LastSkull(43), 0u);
}
