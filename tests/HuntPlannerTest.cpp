#include "HuntPlanner.h"

#include <gtest/gtest.h>

using namespace PlayerbotsPlus;

namespace
{
constexpr uint32_t T0 = 1000;

HuntSnapshot Ready()
{
    HuntSnapshot snap;
    snap.errandsIdle = true;
    snap.isPuller = true;
    snap.groupReady = true;
    snap.minGroupLevel = 5;
    snap.masterPos = {0.f, 0.f, 0.f};
    return snap;
}

Mob QuestMob(uint64_t id, float x)
{
    Mob m;
    m.id = id;
    m.pos = {x, 0.f, 0.f};
    m.level = 5;
    m.needed = true;
    return m;
}
}  // namespace

TEST(Election, TankWithStrategyFirst)
{
    EXPECT_EQ(ElectPuller({{10, false, true}, {30, true, true}, {20, true, false}}), 30u);
}

TEST(Election, LowestGuidWithStrategyWithoutTank)
{
    EXPECT_EQ(ElectPuller({{30, false, true}, {10, false, false}, {20, false, true}}), 20u);
}

TEST(Election, NobodyWithStrategy)
{
    EXPECT_EQ(ElectPuller({{10, true, false}}), 0u);
}

TEST(HuntGate, EachGateBlocks)
{
    struct Case
    {
        void (*apply)(HuntSnapshot&);
        char const* reason;
    };
    Case const cases[] = {
        {[](HuntSnapshot& s) { s.errandsIdle = false; }, "errands first"},
        {[](HuntSnapshot& s) { s.isPuller = false; }, "not puller"},
        {[](HuntSnapshot& s) { s.groupReady = false; }, "group not ready"},
    };
    for (Case const& c : cases)
    {
        HuntSnapshot snap = Ready();
        snap.mobs = {QuestMob(1, 5.f)};
        c.apply(snap);
        HuntState state;
        Decision d = PlanHunt(snap, state, HuntConfig{}, T0);
        EXPECT_EQ(d.type, DecisionType::Idle) << c.reason;
        EXPECT_EQ(d.reason, c.reason);
        EXPECT_EQ(state.lastReason, c.reason);
    }
}

TEST(HuntPick, StartsOnNearestEligibleToMaster)
{
    HuntSnapshot snap = Ready();
    snap.mobs = {QuestMob(1, 12.f), QuestMob(2, -6.f)};
    HuntState state;
    Decision d = PlanHunt(snap, state, HuntConfig{}, T0);
    EXPECT_EQ(d.type, DecisionType::Start);
    EXPECT_EQ(d.target, 2u);
    EXPECT_EQ(state.target, 2u);
}

TEST(HuntPick, EachFilterRejects)
{
    struct Case
    {
        void (*apply)(Mob&);
        char const* what;
    };
    Case const cases[] = {
        {[](Mob& m) { m.needed = false; }, "not needed"},
        {[](Mob& m) { m.elite = true; }, "elite"},
        {[](Mob& m) { m.inCombat = true; }, "in combat"},
        {[](Mob& m) { m.tappedByOther = true; }, "tapped"},
        {[](Mob& m) { m.level = 8; }, "level 5+3"},
        {[](Mob& m) { m.pos = {46.f, 0.f, 0.f}; }, "outside radius"},
    };
    for (Case const& c : cases)
    {
        HuntSnapshot snap = Ready();
        Mob m = QuestMob(1, 5.f);
        c.apply(m);
        snap.mobs = {m};
        HuntState state;
        EXPECT_EQ(PlanHunt(snap, state, HuntConfig{}, T0).reason, "no quest mob") << c.what;
    }
}

TEST(HuntPick, LevelAtLimitIsAllowed)
{
    HuntSnapshot snap = Ready();
    Mob m = QuestMob(1, 5.f);
    m.level = 7;
    snap.mobs = {m};
    HuntState state;
    EXPECT_EQ(PlanHunt(snap, state, HuntConfig{}, T0).type, DecisionType::Start);
}

TEST(HuntPick, PackVetoHasItsOwnReason)
{
    HuntSnapshot snap = Ready();
    Mob m = QuestMob(1, 5.f);
    m.hostilesNearby = 1;
    snap.mobs = {m};
    HuntState state;
    EXPECT_EQ(PlanHunt(snap, state, HuntConfig{}, T0).reason, "pack nearby");
}

TEST(HuntLifecycle, ContinuesWhileTargetStillEligible)
{
    HuntSnapshot snap = Ready();
    snap.mobs = {QuestMob(1, 5.f)};
    HuntState state;
    PlanHunt(snap, state, HuntConfig{}, T0);
    Decision d = PlanHunt(snap, state, HuntConfig{}, T0 + 100);
    EXPECT_EQ(d.type, DecisionType::Continue);
    EXPECT_EQ(d.target, 1u);
}

TEST(HuntLifecycle, TimeoutBlacklists)
{
    HuntSnapshot snap = Ready();
    snap.mobs = {QuestMob(1, 5.f)};
    HuntState state;
    HuntConfig cfg;
    PlanHunt(snap, state, cfg, T0);
    Decision d = PlanHunt(snap, state, cfg, T0 + cfg.timeoutMs);
    EXPECT_EQ(d.type, DecisionType::Abandon);
    EXPECT_EQ(d.reason, "timeout");
    EXPECT_EQ(state.target, 0u);
    EXPECT_EQ(state.blacklistedAt.count(1), 1u);
    EXPECT_EQ(PlanHunt(snap, state, cfg, T0 + cfg.timeoutMs + 1).reason, "no quest mob");
    EXPECT_EQ(PlanHunt(snap, state, cfg, T0 + cfg.timeoutMs + cfg.blacklistMs).type, DecisionType::Start);
}

TEST(HuntLifecycle, DeadTargetMovesToNextMob)
{
    HuntSnapshot snap = Ready();
    snap.mobs = {QuestMob(1, 5.f)};
    HuntState state;
    PlanHunt(snap, state, HuntConfig{}, T0);
    snap.mobs = {QuestMob(2, 9.f)};  // mob 1 died during combat
    Decision d = PlanHunt(snap, state, HuntConfig{}, T0 + 5000);
    EXPECT_EQ(d.type, DecisionType::Start);
    EXPECT_EQ(d.target, 2u);
    EXPECT_TRUE(state.blacklistedAt.empty());
}

TEST(HuntLifecycle, GateDuringHuntAbandonsWithoutBlacklist)
{
    HuntSnapshot snap = Ready();
    snap.mobs = {QuestMob(1, 5.f)};
    HuntState state;
    PlanHunt(snap, state, HuntConfig{}, T0);
    snap.errandsIdle = false;
    Decision d = PlanHunt(snap, state, HuntConfig{}, T0 + 100);
    EXPECT_EQ(d.type, DecisionType::Abandon);
    EXPECT_EQ(state.target, 0u);
    EXPECT_TRUE(state.blacklistedAt.empty());
}

TEST(HuntPick, DefaultRadiusReachesBeyondAggroRange)
{
    HuntSnapshot snap = Ready();
    snap.mobs = {QuestMob(1, 40.f)};
    HuntState state;
    EXPECT_EQ(PlanHunt(snap, state, HuntConfig{}, T0).type, DecisionType::Start);
}
