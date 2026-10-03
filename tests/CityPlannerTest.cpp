#include "CityPlanner.h"

#include <gtest/gtest.h>

using namespace PlayerbotsPlus;

namespace
{
constexpr uint32_t T0 = 100000;

CitySnapshot InCity()
{
    CitySnapshot s;
    s.masterInCapital = true;
    s.zone = 1519;
    s.needs = CityNeed::ClassTraining | CityNeed::Reagents;
    s.stops = {{1, {100, 0, 0}, CityNeed::ClassTraining}, {2, {10, 0, 0}, CityNeed::Reagents},
               {3, {5, 0, 0}, CityNeed::Profession}};
    return s;
}

// Plans once to record the master idle since T0, then after the idle delay.
CityDecision Started(CitySnapshot const& s, CityState& state, CityConfig const& cfg = {})
{
    PlanCity(s, state, cfg, T0);
    return PlanCity(s, state, cfg, T0 + cfg.idleDelayMs);
}
}  // namespace

TEST(City, StartsAtTheNearestCoveringStopAfterIdle)
{
    CityState state;
    CitySnapshot s = InCity();
    EXPECT_EQ(PlanCity(s, state, {}, T0).step, CityStep::None);
    CityDecision d = PlanCity(s, state, {}, T0 + 3000);
    EXPECT_EQ(d.step, CityStep::GoTo);
    EXPECT_EQ(d.stop, 2u);  // stop 3 is nearer but covers nothing needed
    EXPECT_TRUE(state.Active());
    EXPECT_EQ(state.stopsPlanned, 2u);
}

TEST(City, NoTripOutsideCapitalWithoutNeedsOrWhenBusy)
{
    CityState a;
    CitySnapshot s = InCity();
    s.masterInCapital = false;
    EXPECT_EQ(Started(s, a).step, CityStep::None);
    CityState b;
    s = InCity();
    s.needs = CityNeed::Profession;
    s.stops.pop_back();  // nobody teaches it here
    EXPECT_EQ(Started(s, b).reason, "city: nothing to do");
    for (auto busy : {&CitySnapshot::masterInCombat, &CitySnapshot::masterMounted, &CitySnapshot::masterOnTaxi,
                      &CitySnapshot::inInstance, &CitySnapshot::botInCombat})
    {
        CityState c;
        s = InCity();
        s.*busy = true;
        EXPECT_EQ(Started(s, c).step, CityStep::None);
    }
}

TEST(City, AutoOffNeedsTheCommand)
{
    CityState state;
    CitySnapshot s = InCity();
    s.autoEnabled = false;
    EXPECT_EQ(Started(s, state).step, CityStep::None);
    s.requested = true;
    EXPECT_EQ(PlanCity(s, state, {}, T0 + 4000).step, CityStep::GoTo);
}

TEST(City, VisitsThenNextThenEnds)
{
    CityState state;
    CitySnapshot s = InCity();
    Started(s, state);
    s.atStop = true;
    EXPECT_EQ(PlanCity(s, state, {}, T0 + 5000).step, CityStep::Visit);
    MarkCityVisited(state);
    s.atStop = false;
    s.needs = CityNeed::ClassTraining;  // reagents bought
    CityDecision d = PlanCity(s, state, {}, T0 + 6000);
    EXPECT_EQ(d.step, CityStep::GoTo);
    EXPECT_EQ(d.stop, 1u);
    s.atStop = true;
    PlanCity(s, state, {}, T0 + 7000);
    MarkCityVisited(state);
    s.atStop = false;
    s.needs = 0;
    d = PlanCity(s, state, {}, T0 + 8000);
    EXPECT_EQ(d.step, CityStep::End);
    EXPECT_EQ(d.reason, "city: done");
    EXPECT_FALSE(state.Active());
}

TEST(City, UnreachableOrMissingStopIsSkipped)
{
    CityConfig cfg;
    CityState a;
    CitySnapshot s = InCity();
    Started(s, a, cfg);
    CityDecision d = PlanCity(s, a, cfg, T0 + 3000 + cfg.reachMs);
    EXPECT_EQ(d.stop, 1u);
    CityState b;
    s = InCity();
    Started(s, b, cfg);
    s.stopMissing = true;
    EXPECT_EQ(PlanCity(s, b, cfg, T0 + 4000).stop, 1u);
}

TEST(City, TimeoutEnds)
{
    CityConfig cfg;
    CityState state;
    CitySnapshot s = InCity();
    Started(s, state, cfg);
    CityDecision d = PlanCity(s, state, cfg, T0 + 3000 + cfg.timeoutMs);
    EXPECT_EQ(d.step, CityStep::End);
    EXPECT_EQ(d.reason, "city: timeout");
}

TEST(City, AbortsWhenTheMasterLeavesButNotWhenMovingInside)
{
    CityState state;
    CitySnapshot s = InCity();
    Started(s, state);
    s.masterMoving = true;
    EXPECT_EQ(PlanCity(s, state, {}, T0 + 4000).step, CityStep::GoTo);
    for (auto leave : {&CitySnapshot::masterOnTaxi, &CitySnapshot::masterInCombat, &CitySnapshot::inInstance})
    {
        CityState st;
        CitySnapshot c = InCity();
        Started(c, st);
        c.*leave = true;
        EXPECT_EQ(PlanCity(c, st, {}, T0 + 5000).step, CityStep::Abort);
    }
    CityState st;
    CitySnapshot c = InCity();
    Started(c, st);
    c.masterInCapital = false;
    EXPECT_EQ(PlanCity(c, st, {}, T0 + 5000).step, CityStep::Abort);
    EXPECT_FALSE(st.Active());
}

TEST(City, CooldownWithSameNeedsButNotWhenTheyChangeOrOnCommand)
{
    CityConfig cfg;
    CityState state;
    CitySnapshot s = InCity();
    Started(s, state, cfg);
    PlanCity(s, state, cfg, T0 + 3000 + cfg.timeoutMs);  // ends, same needs
    uint32_t const after = T0 + 3000 + cfg.timeoutMs + 5000;
    PlanCity(s, state, cfg, after);
    EXPECT_EQ(PlanCity(s, state, cfg, after + cfg.idleDelayMs).reason, "city: cooldown");
    s.needs = CityNeed::Reagents;
    EXPECT_EQ(PlanCity(s, state, cfg, after + cfg.idleDelayMs + 1).step, CityStep::GoTo);
    CityState other;
    s = InCity();
    Started(s, other, cfg);
    PlanCity(s, other, cfg, T0 + 3000 + cfg.timeoutMs);
    s.requested = true;
    EXPECT_EQ(PlanCity(s, other, cfg, after).step, CityStep::GoTo);
}

TEST(City, CountStops)
{
    EXPECT_EQ(CountCityStops(InCity()), 2u);
}

TEST(CityStation, FocusNeeds)
{
    EXPECT_EQ(FocusNeed(1), CityNeed::Anvil);
    EXPECT_EQ(FocusNeed(3), CityNeed::Forge);
    EXPECT_EQ(FocusNeed(4), 0u);    // cooking fire
    EXPECT_EQ(FocusNeed(543), 0u);  // Black Forge
    EXPECT_EQ(FocusNeed(0), 0u);
}

TEST(CityStation, StationNeedsFromTheCraftRules)
{
    RecipeOption smelt;
    smelt.spell = 1;
    smelt.focus = 3;
    smelt.atFocus = true;
    smelt.smelt = true;
    RecipeOption grey;  // nobody wants it, no skill-up
    grey.spell = 2;
    grey.focus = 1;
    grey.atFocus = true;
    CraftSnapshot snap;
    snap.recipes = {smelt, grey};
    CraftState state;
    EXPECT_EQ(StationNeeds(snap, state), CityNeed::Forge);
    state.approvedSpell = 2;
    EXPECT_EQ(StationNeeds(snap, state), CityNeed::Forge | CityNeed::Anvil);
    RecipeOption plain;  // no focus needed: not a station matter
    plain.spell = 3;
    plain.castable = true;
    plain.skillUp = true;
    snap.recipes = {plain};
    EXPECT_EQ(StationNeeds(snap, state), 0u);
}

TEST(CityStation, IdsNeverCollideWithCreatureSpawns)
{
    uint64_t const station = StationStopId(5);
    EXPECT_NE(station, 5u);
    EXPECT_TRUE(IsStationStop(station));
    EXPECT_FALSE(IsStationStop(5));
    EXPECT_EQ(StationSpawnId(station), 5u);
}

TEST(CityStation, ForgeBeforeAnvil)
{
    CityState state;
    CitySnapshot s = InCity();
    s.needs = CityNeed::Forge | CityNeed::Anvil;
    s.stops = {{StationStopId(1), {5, 0, 0}, CityNeed::Anvil}, {StationStopId(2), {50, 0, 0}, CityNeed::Forge}};
    EXPECT_EQ(Started(s, state).stop, StationStopId(2));
    MarkCityVisited(state);
    EXPECT_EQ(PlanCity(s, state, {}, T0 + 4000).stop, StationStopId(1));
}

TEST(CityStation, WorkIsMultiTickPausesTheTimeoutAndIsCapped)
{
    CityState state;
    CityConfig cfg;
    cfg.timeoutMs = 60000;
    CitySnapshot s = InCity();
    s.needs = CityNeed::Forge | CityNeed::ClassTraining;
    s.stops = {{StationStopId(1), {5, 0, 0}, CityNeed::Forge}, {7, {50, 0, 0}, CityNeed::ClassTraining}};
    Started(s, state, cfg);
    s.atStop = true;
    uint32_t const at = T0 + cfg.idleDelayMs + 1000;
    EXPECT_EQ(PlanCity(s, state, cfg, at).step, CityStep::Visit);
    // Still working past the trip timeout: the time at the forge does not count.
    uint32_t const late = T0 + cfg.idleDelayMs + cfg.timeoutMs + 1000;
    ASSERT_LT(late - at, cfg.workMs);
    EXPECT_EQ(PlanCity(s, state, cfg, late).step, CityStep::Visit);
    // Capped: the stop is left for the next one.
    CityDecision d = PlanCity(s, state, cfg, at + cfg.workMs);
    EXPECT_EQ(d.step, CityStep::GoTo);
    EXPECT_EQ(d.stop, 7u);
    s.atStop = false;
    EXPECT_EQ(PlanCity(s, state, cfg, at + cfg.workMs + 1000).step, CityStep::GoTo);  // no timeout yet
}

TEST(CityStation, CreatureStopsAreNotCapped)
{
    CityState state;
    CityConfig cfg;
    CitySnapshot s = InCity();
    Started(s, state, cfg);
    s.atStop = true;
    EXPECT_EQ(PlanCity(s, state, cfg, T0 + cfg.idleDelayMs + cfg.workMs + 1).step, CityStep::Visit);
}
