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
