#include "RoleRules.h"

#include <gtest/gtest.h>

using namespace PlayerbotsPlus;

namespace
{
constexpr uint8_t Warrior = 1, Paladin = 2, Rogue = 4, Priest = 5, Shaman = 7, Mage = 8, Druid = 11;
}  // namespace

TEST(Role, Parse)
{
    EXPECT_EQ(ParseRole("tank"), Role::Tank);
    EXPECT_EQ(ParseRole("Heal"), Role::Heal);
    EXPECT_EQ(ParseRole("dps"), Role::Dps);
    EXPECT_EQ(ParseRole("healer"), Role::None);
}

TEST(Role, TankAndHealTrees)
{
    EXPECT_EQ(TabForRole(Warrior, Role::Tank, 0), 2);
    EXPECT_EQ(TabForRole(Paladin, Role::Tank, 2), 1);
    EXPECT_EQ(TabForRole(Paladin, Role::Heal, -1), 0);
    EXPECT_EQ(TabForRole(Shaman, Role::Heal, 1), 2);
    EXPECT_EQ(TabForRole(Druid, Role::Tank, 2), 1);
    EXPECT_EQ(TabForRole(Priest, Role::Heal, 0), 0);  // discipline heals too: kept
    EXPECT_EQ(TabForRole(Priest, Role::Heal, 2), 1);  // shadow: holy
}

TEST(Role, DpsKeepsTheCurrentDpsTree)
{
    EXPECT_EQ(TabForRole(Rogue, Role::Dps, 1), 1);
    EXPECT_EQ(TabForRole(Rogue, Role::Dps, -1), 0);
    EXPECT_EQ(TabForRole(Warrior, Role::Dps, 2), 0);  // from protection: arms
    EXPECT_EQ(TabForRole(Paladin, Role::Dps, 0), 2);
    EXPECT_EQ(TabForRole(Mage, Role::Dps, 2), 2);
    EXPECT_EQ(TabForRole(Shaman, Role::Dps, 2), 0);  // from restoration: elemental
}

TEST(Role, ImpossibleRoles)
{
    EXPECT_EQ(TabForRole(Rogue, Role::Tank, 0), -1);
    EXPECT_EQ(TabForRole(Mage, Role::Heal, 0), -1);
    EXPECT_EQ(TabForRole(Warrior, Role::Heal, 0), -1);
    EXPECT_FALSE(TabFitsRole(Warrior, Role::Tank, 0));
    EXPECT_TRUE(TabFitsRole(Warrior, Role::Dps, 1));
}
