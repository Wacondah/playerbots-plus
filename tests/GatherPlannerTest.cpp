#include "GatherPlanner.h"

#include <gtest/gtest.h>

using namespace PlayerbotsPlus;

namespace
{
constexpr uint32_t T0 = 1000;

GatherNode Node(uint64_t id, float x)
{
    GatherNode n;
    n.id = id;
    n.pos = {x, 0.f, 0.f};
    n.gatherable = true;
    return n;
}

GatherSnapshot Idle(std::vector<GatherNode> nodes)
{
    GatherSnapshot snap;
    snap.idle = true;
    snap.nodes = std::move(nodes);
    return snap;
}
}  // namespace

TEST(GatherGate, BlocksAndAbandons)
{
    GatherState state;
    GatherConfig const cfg;
    GatherSnapshot snap = Idle({Node(1, 10.f)});
    snap.idle = false;
    EXPECT_EQ(PlanGather(snap, state, cfg, T0).reason, "master busy");
    snap.idle = true;
    snap.bagsFull = true;
    EXPECT_EQ(PlanGather(snap, state, cfg, T0).reason, "bags full");
    GatherConfig off;
    off.radius = 0.f;
    EXPECT_EQ(PlanGather(Idle({Node(1, 10.f)}), state, off, T0).reason, "disabled");

    EXPECT_EQ(PlanGather(Idle({Node(1, 10.f)}), state, cfg, T0).type, DecisionType::Start);
    snap.bagsFull = false;
    snap.idle = false;
    Decision const d = PlanGather(snap, state, cfg, T0 + 1);
    EXPECT_EQ(d.type, DecisionType::Abandon);
    EXPECT_EQ(d.target, 1u);
    EXPECT_EQ(state.target, 0u);
}

TEST(GatherPick, NearestEligibleWithinRadiusOfTheMaster)
{
    GatherState state;
    GatherConfig const cfg;  // 50 yd
    std::vector<GatherNode> nodes = {Node(1, 49.f), Node(2, 51.f), Node(3, 30.f), Node(4, 5.f), Node(5, 8.f),
                                     Node(6, 9.f)};
    nodes[3].gatherable = false;  // skill too low
    nodes[4].contested = true;    // another bot or the master is on it
    nodes[5].guarded = true;      // a hostile next to it
    GatherSnapshot snap = Idle(nodes);
    snap.botPos = {40.f, 0.f, 0.f};
    Decision const d = PlanGather(snap, state, cfg, T0);
    EXPECT_EQ(d.type, DecisionType::Start);
    EXPECT_EQ(d.target, 1u);  // 9 yd from the bot; 2 is out of range of the master
}

TEST(GatherPick, NothingLeft)
{
    GatherState state;
    EXPECT_EQ(PlanGather(Idle({}), state, GatherConfig{}, T0).reason, "no node");
}

TEST(GatherLifecycle, ContinueThenNextThenTimeout)
{
    GatherState state;
    GatherConfig const cfg;
    std::vector<GatherNode> both = {Node(1, 10.f), Node(2, 20.f)};
    EXPECT_EQ(PlanGather(Idle(both), state, cfg, T0).target, 1u);
    Decision d = PlanGather(Idle(both), state, cfg, T0 + 1000);
    EXPECT_EQ(d.type, DecisionType::Continue);
    EXPECT_EQ(d.reason, "gathering");

    // Gathered: the node disappears, the next one starts.
    d = PlanGather(Idle({Node(2, 20.f)}), state, cfg, T0 + 2000);
    EXPECT_EQ(d.type, DecisionType::Start);
    EXPECT_EQ(d.target, 2u);

    d = PlanGather(Idle({Node(2, 20.f)}), state, cfg, T0 + 2000 + cfg.timeoutMs);
    EXPECT_EQ(d.type, DecisionType::Abandon);
    EXPECT_EQ(d.reason, "timeout");
    EXPECT_EQ(PlanGather(Idle({Node(2, 20.f)}), state, cfg, T0 + 3000 + cfg.timeoutMs).reason, "no node");
    d = PlanGather(Idle({Node(2, 20.f)}), state, cfg, T0 + 2000 + cfg.timeoutMs + cfg.blacklistMs);
    EXPECT_EQ(d.target, 2u);  // blacklist expired
}

TEST(GatherLifecycle, MarkFailedBlacklists)
{
    GatherState state;
    GatherConfig const cfg;
    PlanGather(Idle({Node(1, 10.f)}), state, cfg, T0);
    MarkGatherFailed(state, T0 + 1);
    EXPECT_EQ(state.target, 0u);
    EXPECT_EQ(PlanGather(Idle({Node(1, 10.f)}), state, cfg, T0 + 2).reason, "no node");
}

TEST(GatherLifecycle, TimeWrap)
{
    GatherState state;
    GatherConfig const cfg;
    uint32_t const nearWrap = 0xFFFFFF00u;
    PlanGather(Idle({Node(1, 10.f)}), state, cfg, nearWrap);
    EXPECT_EQ(PlanGather(Idle({Node(1, 10.f)}), state, cfg, nearWrap + 1000).type, DecisionType::Continue);
}

TEST(CanGather, SkillToolAndKind)
{
    constexpr uint32_t Mining = 186, Herbalism = 182, Lockpicking = 633, Fishing = 356;
    EXPECT_FALSE(CanGather(Mining, 18, 1, false));  // no pick
    EXPECT_TRUE(CanGather(Mining, 18, 1, true));
    EXPECT_FALSE(CanGather(Mining, 64, 65, true));
    EXPECT_TRUE(CanGather(Mining, 65, 65, true));
    EXPECT_TRUE(CanGather(Herbalism, 1, 0, false));  // req 0 counts as 1, no tool
    EXPECT_FALSE(CanGather(Herbalism, 0, 0, false));  // not learned
    EXPECT_FALSE(CanGather(Lockpicking, 300, 1, true));
    EXPECT_FALSE(CanGather(Fishing, 300, 1, true));
}
