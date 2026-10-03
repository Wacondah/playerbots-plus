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

namespace
{
// Snapshot + state with an errand to candidate 7 started at T0 + IdleDelay.
struct Running
{
    Snapshot snap = Ready();
    ErrandState state;
    PlannerConfig cfg;
    uint32_t startedAt = T0 + PlannerConfig{}.idleDelayMs;

    Running()
    {
        Candidate giver = Npc(7, 4.f);
        giver.canAccept = true;
        snap.candidates = {giver};
        Decision d = PlanAfterIdle(snap, state, cfg);
        EXPECT_EQ(d.type, DecisionType::Start);
    }
};
}  // namespace

TEST(Lifecycle, ContinuesActiveErrand)
{
    Running r;
    Decision d = Plan(r.snap, r.state, r.cfg, r.startedAt + 100);
    EXPECT_EQ(d.type, DecisionType::Continue);
    EXPECT_EQ(d.target, 7u);
    EXPECT_TRUE(d.Acts());
}

TEST(Lifecycle, AbandonsWhenMasterMoves)
{
    Running r;
    r.snap.masterPos = {2.f, 0.f, 0.f};
    Decision d = Plan(r.snap, r.state, r.cfg, r.startedAt + 100);
    EXPECT_EQ(d.type, DecisionType::Abandon);
    EXPECT_EQ(d.reason, "master moving");
    EXPECT_FALSE(r.state.active.IsActive());
    EXPECT_FALSE(d.Acts());
}

TEST(Lifecycle, AbandonsOnCombat)
{
    Running r;
    r.snap.botInCombat = true;
    EXPECT_EQ(Plan(r.snap, r.state, r.cfg, r.startedAt + 100).type, DecisionType::Abandon);
}

TEST(Lifecycle, TimeoutAbandonsAndBlacklists)
{
    Running r;
    Decision d = Plan(r.snap, r.state, r.cfg, r.startedAt + r.cfg.timeoutMs);
    EXPECT_EQ(d.type, DecisionType::Abandon);
    EXPECT_EQ(d.reason, "timeout");
    EXPECT_EQ(r.state.blacklistedAt.count(7), 1u);
}

TEST(Lifecycle, AbandonsWhenTargetGone)
{
    Running r;
    r.snap.candidates.clear();
    Decision d = Plan(r.snap, r.state, r.cfg, r.startedAt + 100);
    EXPECT_EQ(d.type, DecisionType::Abandon);
    EXPECT_EQ(d.reason, "target gone");
}

TEST(Lifecycle, MarkDoneRecordsVisitAndFreesBot)
{
    Running r;
    MarkDone(r.state, 99, r.startedAt + 500);
    EXPECT_FALSE(r.state.active.IsActive());
    ASSERT_EQ(r.state.visits.count(7), 1u);
    EXPECT_EQ(r.state.visits[7].fingerprint, 99u);
    EXPECT_EQ(r.state.visits[7].at, r.startedAt + 500);
}

TEST(Lifecycle, MarkFailedBlacklists)
{
    Running r;
    MarkFailed(r.state, r.startedAt + 500);
    EXPECT_FALSE(r.state.active.IsActive());
    EXPECT_EQ(r.state.blacklistedAt[7], r.startedAt + 500);
}

TEST(Lifecycle, TimeWrapIsHandled)
{
    Snapshot snap = Ready();
    Candidate giver = Npc(1, 3.f);
    giver.canAccept = true;
    snap.candidates = {giver};
    ErrandState state;
    PlannerConfig cfg;
    uint32_t const nearWrap = 0xFFFFFFFFu - 1000;
    Plan(snap, state, cfg, nearWrap);
    EXPECT_EQ(Plan(snap, state, cfg, nearWrap + cfg.idleDelayMs).type, DecisionType::Start);
}

TEST(Pick, TrainAndBuyToolComeAfterSell)
{
    Snapshot snap = Ready();
    snap.hasJunk = true;
    Candidate vendor = Npc(1, 3.f);
    vendor.canSell = true;
    Candidate trainer = Npc(2, 2.f);
    trainer.canTrain = true;
    Candidate toolVendor = Npc(3, 1.f);
    toolVendor.canSellTool = true;

    snap.candidates = {toolVendor, trainer, vendor};
    ErrandState s1;
    EXPECT_EQ(PlanAfterIdle(snap, s1, PlannerConfig{}).kind, ErrandKind::Sell);

    snap.candidates = {toolVendor, trainer};
    ErrandState s2;
    EXPECT_EQ(PlanAfterIdle(snap, s2, PlannerConfig{}).kind, ErrandKind::Train);

    snap.candidates = {toolVendor};
    ErrandState s3;
    Decision d = PlanAfterIdle(snap, s3, PlannerConfig{});
    EXPECT_EQ(d.kind, ErrandKind::BuyTool);
    EXPECT_EQ(d.reason, "start buy tool");
}

TEST(Pick, TrainerRestsAfterVisit)
{
    Snapshot snap = Ready();
    Candidate trainer = Npc(1, 3.f);
    trainer.canTrain = true;
    snap.candidates = {trainer};
    PlannerConfig cfg;
    ErrandState state;
    state.visits[1] = Visit{0, T0};
    EXPECT_EQ(PlanAfterIdle(snap, state, cfg).reason, "nothing to do");
    EXPECT_EQ(Plan(snap, state, cfg, T0 + cfg.blacklistMs).kind, ErrandKind::Train);
}

TEST(Pick, BuyReagentsAfterTrainBeforeTool)
{
    Snapshot snap = Ready();
    Candidate trainer = Npc(1, 3.f);
    trainer.canTrain = true;
    Candidate reagentVendor = Npc(2, 2.f);
    reagentVendor.canSellReagent = true;
    Candidate toolVendor = Npc(3, 1.f);
    toolVendor.canSellTool = true;

    snap.candidates = {toolVendor, reagentVendor, trainer};
    ErrandState s1;
    EXPECT_EQ(PlanAfterIdle(snap, s1, PlannerConfig{}).kind, ErrandKind::Train);

    snap.candidates = {toolVendor, reagentVendor};
    ErrandState s2;
    Decision d = PlanAfterIdle(snap, s2, PlannerConfig{});
    EXPECT_EQ(d.kind, ErrandKind::BuyReagents);
    EXPECT_EQ(d.reason, "start buy reagents");
}

TEST(Pick, TrainClassAfterTrainBeforeReagents)
{
    Snapshot snap = Ready();
    Candidate trainer = Npc(1, 3.f);
    trainer.canTrain = true;
    Candidate classTrainer = Npc(2, 2.f);
    classTrainer.canTrainClass = true;
    Candidate reagentVendor = Npc(3, 1.f);
    reagentVendor.canSellReagent = true;

    snap.candidates = {reagentVendor, classTrainer, trainer};
    ErrandState s1;
    EXPECT_EQ(PlanAfterIdle(snap, s1, PlannerConfig{}).kind, ErrandKind::Train);

    snap.candidates = {reagentVendor, classTrainer};
    ErrandState s2;
    Decision d = PlanAfterIdle(snap, s2, PlannerConfig{});
    EXPECT_EQ(d.kind, ErrandKind::TrainClass);
    EXPECT_EQ(d.reason, "start train class");
}

TEST(MasterIdle, IgnoresTheBotsDistance)
{
    Snapshot snap = Ready();
    snap.botPos = {40.f, 0.f, 0.f};
    ErrandState state;
    Decision d = PlanAfterIdle(snap, state, PlannerConfig{});
    EXPECT_EQ(d.reason, "too far from master");
    EXPECT_TRUE(state.masterIdle);
}

TEST(MasterIdle, FalseWhileTheMasterMovesOrFights)
{
    Snapshot snap = Ready();
    ErrandState state;
    PlannerConfig const cfg;
    Plan(snap, state, cfg, T0);
    EXPECT_FALSE(state.masterIdle);  // idle delay not over yet
    Plan(snap, state, cfg, T0 + cfg.idleDelayMs);
    EXPECT_TRUE(state.masterIdle);
    snap.masterInCombat = true;
    Plan(snap, state, cfg, T0 + cfg.idleDelayMs + 1);
    EXPECT_FALSE(state.masterIdle);
}
