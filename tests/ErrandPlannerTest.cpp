#include "ErrandPlanner.h"

#include <gtest/gtest.h>

using namespace PlayerbotsPlus;

namespace
{
constexpr uint32_t T0 = 1000;

Snapshot Ready()
{
    Snapshot snap;
    snap.hasMaster = true;
    snap.masterSameMap = true;
    snap.masterPos = {0.f, 0.f, 0.f};
    snap.botPos = {2.f, 0.f, 0.f};
    return snap;
}

Candidate Npc(uint64_t id, float x)
{
    Candidate c;
    c.id = id;
    c.pos = {x, 0.f, 0.f};
    return c;
}

// First call starts the idle clock, second call is IdleDelay later.
Decision PlanAfterIdle(Snapshot const& snap, ErrandState& state, PlannerConfig const& cfg)
{
    Plan(snap, state, cfg, T0);
    return Plan(snap, state, cfg, T0 + cfg.idleDelayMs);
}
}  // namespace

TEST(Blockers, NoMasterIsIdle)
{
    Snapshot snap = Ready();
    snap.hasMaster = false;
    ErrandState state;
    Decision d = PlanAfterIdle(snap, state, PlannerConfig{});
    EXPECT_EQ(d.type, DecisionType::Idle);
    EXPECT_EQ(d.reason, "no master");
}

TEST(Blockers, MasterMustStayStillForIdleDelay)
{
    Snapshot snap = Ready();
    snap.candidates.push_back(Npc(1, 5.f));
    snap.candidates.back().canAccept = true;
    ErrandState state;
    PlannerConfig cfg;

    EXPECT_EQ(Plan(snap, state, cfg, T0).reason, "master moving");
    EXPECT_EQ(Plan(snap, state, cfg, T0 + cfg.idleDelayMs - 1).reason, "master moving");
    EXPECT_EQ(Plan(snap, state, cfg, T0 + cfg.idleDelayMs).type, DecisionType::Start);
}

TEST(Blockers, MasterMovementResetsIdleClock)
{
    Snapshot snap = Ready();
    ErrandState state;
    PlannerConfig cfg;
    Plan(snap, state, cfg, T0);
    snap.masterPos = {3.f, 0.f, 0.f};
    EXPECT_EQ(Plan(snap, state, cfg, T0 + cfg.idleDelayMs).reason, "master moving");
}

TEST(Blockers, EachConditionBlocks)
{
    struct Case
    {
        void (*apply)(Snapshot&);
        char const* reason;
    };
    Case const cases[] = {
        {[](Snapshot& s) { s.masterSameMap = false; }, "master on another map"},
        {[](Snapshot& s) { s.inInstance = true; }, "in instance"},
        {[](Snapshot& s) { s.botHasStayOrGuard = true; }, "stay or guard order"},
        {[](Snapshot& s) { s.botInCombat = true; }, "combat"},
        {[](Snapshot& s) { s.masterInCombat = true; }, "combat"},
        {[](Snapshot& s) { s.botNeedsRest = true; }, "resting"},
        {[](Snapshot& s) { s.masterMounted = true; }, "master mounted"},
        {[](Snapshot& s) { s.masterOnTaxi = true; }, "master mounted"},
        {[](Snapshot& s) { s.botPos = {26.f, 0.f, 0.f}; }, "too far from master"},
    };
    for (Case const& c : cases)
    {
        Snapshot snap = Ready();
        c.apply(snap);
        ErrandState state;
        Decision d = PlanAfterIdle(snap, state, PlannerConfig{});
        EXPECT_EQ(d.type, DecisionType::Idle) << c.reason;
        EXPECT_EQ(d.reason, c.reason);
        EXPECT_EQ(state.lastReason, c.reason);
    }
}

TEST(Blockers, InstanceAllowedByConfig)
{
    Snapshot snap = Ready();
    snap.inInstance = true;
    ErrandState state;
    PlannerConfig cfg;
    cfg.inInstances = true;
    EXPECT_EQ(PlanAfterIdle(snap, state, cfg).reason, "nothing to do");
}

TEST(Pick, TurnInBeforeAcceptBeforeRepairBeforeSell)
{
    Snapshot snap = Ready();
    snap.needsRepair = true;
    snap.hasJunk = true;
    Candidate sell = Npc(1, 1.f);
    sell.canSell = true;
    Candidate repair = Npc(2, 2.f);
    repair.canRepair = true;
    Candidate accept = Npc(3, 3.f);
    accept.canAccept = true;
    Candidate turnIn = Npc(4, 4.f);
    turnIn.canTurnIn = true;

    snap.candidates = {sell, repair, accept, turnIn};
    ErrandState s1;
    Decision d = PlanAfterIdle(snap, s1, PlannerConfig{});
    EXPECT_EQ(d.type, DecisionType::Start);
    EXPECT_EQ(d.target, 4u);
    EXPECT_EQ(d.kind, ErrandKind::TurnIn);
    EXPECT_EQ(s1.active.target, 4u);

    snap.candidates = {sell, repair, accept};
    ErrandState s2;
    EXPECT_EQ(PlanAfterIdle(snap, s2, PlannerConfig{}).kind, ErrandKind::Accept);

    snap.candidates = {sell, repair};
    ErrandState s3;
    EXPECT_EQ(PlanAfterIdle(snap, s3, PlannerConfig{}).kind, ErrandKind::Repair);

    snap.candidates = {sell};
    ErrandState s4;
    EXPECT_EQ(PlanAfterIdle(snap, s4, PlannerConfig{}).kind, ErrandKind::Sell);
}

TEST(Pick, RepairAndSellOnlyWhenNeeded)
{
    Snapshot snap = Ready();
    Candidate vendor = Npc(1, 3.f);
    vendor.canRepair = true;
    vendor.canSell = true;
    snap.candidates = {vendor};
    ErrandState state;
    EXPECT_EQ(PlanAfterIdle(snap, state, PlannerConfig{}).reason, "nothing to do");
}

TEST(Pick, RadiusIsMeasuredFromMaster)
{
    Snapshot snap = Ready();
    snap.masterPos = {0.f, 0.f, 0.f};
    snap.botPos = {20.f, 0.f, 0.f};
    Candidate nearBotOnly = Npc(1, 25.f);  // 5 yd from bot, 25 from master
    nearBotOnly.canAccept = true;
    snap.candidates = {nearBotOnly};
    ErrandState state;
    EXPECT_EQ(PlanAfterIdle(snap, state, PlannerConfig{}).reason, "nothing to do");
}

TEST(Pick, NearestToBotWinsWithinSameKind)
{
    Snapshot snap = Ready();
    snap.botPos = {10.f, 0.f, 0.f};
    Candidate far = Npc(1, -10.f);
    far.canAccept = true;
    Candidate near = Npc(2, 12.f);
    near.canAccept = true;
    snap.candidates = {far, near};
    ErrandState state;
    EXPECT_EQ(PlanAfterIdle(snap, state, PlannerConfig{}).target, 2u);
}

TEST(Pick, SkipsQuestGiverVisitedWithSameFingerprint)
{
    Snapshot snap = Ready();
    snap.questFingerprint = 42;
    Candidate giver = Npc(1, 3.f);
    giver.canAccept = true;
    snap.candidates = {giver};
    ErrandState state;
    state.visits[1] = Visit{42, T0};
    EXPECT_EQ(PlanAfterIdle(snap, state, PlannerConfig{}).reason, "nothing to do");

    snap.questFingerprint = 43;
    EXPECT_EQ(Plan(snap, state, PlannerConfig{}, T0 + 5000).kind, ErrandKind::Accept);
}

TEST(Pick, VendorRestsForBlacklistDurationAfterVisit)
{
    Snapshot snap = Ready();
    snap.hasJunk = true;
    Candidate vendor = Npc(1, 3.f);
    vendor.canSell = true;
    snap.candidates = {vendor};
    PlannerConfig cfg;
    ErrandState state;
    state.visits[1] = Visit{0, T0};
    EXPECT_EQ(PlanAfterIdle(snap, state, cfg).reason, "nothing to do");
    EXPECT_EQ(Plan(snap, state, cfg, T0 + cfg.blacklistMs).kind, ErrandKind::Sell);
}

TEST(Pick, BlacklistedTargetIgnoredUntilExpiry)
{
    Snapshot snap = Ready();
    Candidate giver = Npc(1, 3.f);
    giver.canAccept = true;
    snap.candidates = {giver};
    PlannerConfig cfg;
    ErrandState state;
    state.blacklistedAt[1] = T0;
    EXPECT_EQ(PlanAfterIdle(snap, state, cfg).reason, "nothing to do");
    EXPECT_EQ(Plan(snap, state, cfg, T0 + cfg.blacklistMs).type, DecisionType::Start);
    EXPECT_TRUE(state.blacklistedAt.empty());
}
