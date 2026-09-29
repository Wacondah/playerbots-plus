#include "PullPlanner.h"

#include <gtest/gtest.h>

using namespace PlayerbotsPlus;

namespace
{
PullMob Mob(uint64_t id, float x, float y, float radius, bool unavoidable = false)
{
    PullMob m;
    m.id = id;
    m.pos = {x, y, 0.f};
    m.radius = radius;
    m.unavoidable = unavoidable;
    return m;
}

FiringOption Firing(float x, float y, PullPath path, bool los = true)
{
    FiringOption f;
    f.pos = {x, y, 0.f};
    f.lineOfSight = los;
    f.path = std::move(path);
    return f;
}

ReturnOption Return(size_t firing, float x, float y, bool hidden, PullPath path)
{
    ReturnOption r;
    r.firing = firing;
    r.pos = {x, y, 0.f};
    r.hidden = hidden;
    r.path = std::move(path);
    return r;
}
}  // namespace

TEST(PullPlanner, PathLength)
{
    EXPECT_FLOAT_EQ(PathLength({{0, 0, 0}, {3, 4, 0}, {3, 10, 0}}), 11.f);
    EXPECT_FLOAT_EQ(PathLength({{1, 1, 0}}), 0.f);
    EXPECT_FLOAT_EQ(PathLength({}), 0.f);
}

TEST(PullPlanner, SegmentClearance)
{
    std::vector<PullMob> const mobs = {Mob(1, 10, 5, 10)};  // 10 + 3 margin = 13
    PullPath const straight = {{0, 0, 0}, {20, 0, 0}};      // passes 5 yd from the mob
    EXPECT_EQ(WokenBy(straight, mobs, PullMargin), std::vector<uint64_t>{1});
    PullPath const far = {{0, -20, 0}, {20, -20, 0}};  // 25 yd away
    EXPECT_TRUE(WokenBy(far, mobs, PullMargin).empty());
    PullPath const edge = {{0, -7.5f, 0}, {20, -7.5f, 0}};  // 12.5 yd: inside radius + margin
    EXPECT_EQ(WokenBy(edge, mobs, PullMargin).size(), 1u);
}

TEST(PullPlanner, SinglePointPathAndUnavoidableIgnored)
{
    std::vector<PullMob> const mobs = {Mob(1, 0, 0, 10, true), Mob(2, 5, 0, 10)};
    EXPECT_EQ(WokenBy({{0, 0, 0}}, mobs, PullMargin), std::vector<uint64_t>{2});
}

TEST(PullPlanner, RankFiringDropsBlindAndUnreachable)
{
    std::vector<PullMob> const mobs = {Mob(1, 50, 0, 10)};
    std::vector<FiringOption> const firing = {
        Firing(0, 30, {{0, 0, 0}, {0, 30, 0}}, false),  // no line of sight
        Firing(0, 20, {}),                              // unreachable
        Firing(0, 25, {{0, 0, 0}, {0, 25, 0}}),         // safe, 25
        Firing(40, 0, {{0, 0, 0}, {40, 0, 0}}),         // wakes mob 1, 40
        Firing(0, 10, {{0, 0, 0}, {0, 10, 0}}),         // safe, 10
    };
    EXPECT_EQ(RankFiring(firing, mobs, PullMargin), (std::vector<size_t>{4, 2, 3}));
}

TEST(PullPlanner, ShortestSafeRouteWins)
{
    std::vector<PullMob> const mobs = {Mob(1, 100, 100, 10)};
    std::vector<FiringOption> const firing = {
        Firing(0, 30, {{0, 0, 0}, {0, 30, 0}}),
        Firing(0, 20, {{0, 0, 0}, {0, 20, 0}}),
    };
    std::vector<ReturnOption> const returns = {
        Return(0, 0, 0, true, {{0, 30, 0}, {0, 0, 0}}),  // 60 total
        Return(1, 0, 0, true, {{0, 20, 0}, {0, 0, 0}}),  // 40 total
    };
    PullRoute const route = ChooseRoute(firing, returns, mobs, PullMargin);
    ASSERT_TRUE(route.found);
    EXPECT_EQ(route.firing, 1u);
    EXPECT_FLOAT_EQ(route.firingPos.y, 20.f);
    EXPECT_TRUE(route.woken.empty());
}

TEST(PullPlanner, HiddenSpotBeatsGroupPosition)
{
    std::vector<FiringOption> const firing = {Firing(0, 20, {{0, 0, 0}, {0, 20, 0}})};
    std::vector<ReturnOption> const returns = {
        Return(0, 0, 0, false, {{0, 20, 0}, {0, 0, 0}}),              // the group, shorter
        Return(0, 5, -5, true, {{0, 20, 0}, {0, 0, 0}, {5, -5, 0}}),  // behind a corner
    };
    PullRoute const route = ChooseRoute(firing, returns, {}, PullMargin);
    ASSERT_TRUE(route.found);
    EXPECT_TRUE(route.hidden);
    EXPECT_FLOAT_EQ(route.hidePos.x, 5.f);
}

TEST(PullPlanner, NoSafeRouteGivesFewestWoken)
{
    std::vector<PullMob> const mobs = {Mob(1, -5, 10, 5), Mob(2, -2, 12, 5), Mob(3, 20, 10, 5), Mob(9, 0, 30, 5, true)};
    std::vector<FiringOption> const firing = {
        Firing(0, 20, {{0, 0, 0}, {0, 20, 0}}),    // wakes 1 and 2
        Firing(20, 20, {{0, 0, 0}, {20, 20, 0}}),  // wakes 3 only
    };
    std::vector<ReturnOption> const returns = {
        Return(0, 0, 0, false, {{0, 20, 0}, {0, 0, 0}}),
        Return(1, 0, 0, false, {{20, 20, 0}, {0, 0, 0}}),
    };
    PullRoute const route = ChooseRoute(firing, returns, mobs, PullMargin);
    ASSERT_TRUE(route.found);
    EXPECT_EQ(route.firing, 1u);
    EXPECT_EQ(route.woken, std::vector<uint64_t>{3});
    EXPECT_EQ(route.unavoidable, std::vector<uint64_t>{9});
}

TEST(PullPlanner, NothingUsable)
{
    std::vector<FiringOption> const firing = {Firing(0, 20, {{0, 0, 0}, {0, 20, 0}})};
    std::vector<ReturnOption> const returns = {Return(0, 0, 0, false, {})};  // unreachable return
    EXPECT_FALSE(ChooseRoute(firing, returns, {}, PullMargin).found);
}

namespace
{
PullerFacts Member(uint64_t id, bool tank, bool healer, bool ranged, bool spell, float distance)
{
    PullerFacts f;
    f.id = id;
    f.tank = tank;
    f.healer = healer;
    f.ranged = ranged;
    f.hasPullSpell = spell;
    f.distance = distance;
    return f;
}

constexpr uint8_t Warrior = 1, Paladin = 2, Hunter = 3, Rogue = 4, Mage = 8, Druid = 11;
}  // namespace

TEST(PullPuller, RangedTankFirst)
{
    PullerChoice const c = ChoosePuller({Member(1, false, false, true, true, 10), Member(2, true, false, false, true, 30)});
    EXPECT_TRUE(c.found);
    EXPECT_EQ(c.id, 2u);
    EXPECT_FALSE(c.bodyPull);
}

TEST(PullPuller, ClosestRangedDpsWhenTheTankCannotShoot)
{
    PullerChoice const c = ChoosePuller({Member(1, true, false, false, false, 5), Member(2, false, true, true, true, 10),
                                         Member(3, false, false, true, true, 25), Member(4, false, false, true, true, 20)});
    EXPECT_EQ(c.id, 4u);  // 2 is the healer
    EXPECT_FALSE(c.bodyPull);
}

TEST(PullPuller, BodyPullAsLastResort)
{
    PullerChoice const c = ChoosePuller({Member(1, true, false, false, false, 5), Member(2, false, false, false, false, 3)});
    EXPECT_EQ(c.id, 1u);
    EXPECT_TRUE(c.bodyPull);
    EXPECT_FALSE(ChoosePuller({Member(2, false, false, true, true, 3)}).found);  // no tank: nobody pulls
}

TEST(PullSpells, WeaponThenClassSpells)
{
    EXPECT_EQ(PullSpellNames(Warrior, RangedWeapon::Bow), (std::vector<std::string>{"shoot bow", "heroic throw"}));
    EXPECT_EQ(PullSpellNames(Warrior, RangedWeapon::None), (std::vector<std::string>{"heroic throw"}));
    EXPECT_EQ(PullSpellNames(Rogue, RangedWeapon::Thrown), (std::vector<std::string>{"throw"}));
    EXPECT_EQ(PullSpellNames(Mage, RangedWeapon::Wand), (std::vector<std::string>{"frostbolt", "fireball", "shoot"}));
    EXPECT_EQ(PullSpellNames(Hunter, RangedWeapon::Bow), (std::vector<std::string>{"arcane shot", "auto shot"}));
    EXPECT_EQ(PullSpellNames(Paladin, RangedWeapon::None),
              (std::vector<std::string>{"avenger's shield", "hand of reckoning", "exorcism"}));
    EXPECT_EQ(PullSpellNames(Druid, RangedWeapon::None),
              (std::vector<std::string>{"faerie fire (feral)", "faerie fire", "wrath"}));
}
