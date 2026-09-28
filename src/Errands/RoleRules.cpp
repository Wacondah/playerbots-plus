/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "RoleRules.h"

#include <algorithm>
#include <cctype>

namespace PlayerbotsPlus
{
namespace
{
struct ClassRoles
{
    uint8_t tank;  // bitmask of trees
    uint8_t heal;
    uint8_t dps;
    int8_t defaults[3];  // tank, heal, dps default tree (-1: cannot)
};

// Indexed by class id (1 warrior .. 11 druid; 10 unused).
constexpr ClassRoles Classes[12] = {
    {0, 0, 0, {-1, -1, -1}},        // none
    {0b100, 0, 0b011, {2, -1, 0}},  // warrior: protection; arms, fury
    {0b010, 0b001, 0b100, {1, 0, 2}},  // paladin: protection; holy; retribution
    {0, 0, 0b111, {-1, -1, 0}},     // hunter
    {0, 0, 0b111, {-1, -1, 0}},     // rogue
    {0, 0b011, 0b100, {-1, 1, 2}},  // priest: discipline, holy; shadow
    {0b001, 0, 0b110, {0, -1, 1}},  // death knight: blood; frost, unholy
    {0, 0b100, 0b011, {-1, 2, 0}},  // shaman: restoration; elemental, enhancement
    {0, 0, 0b111, {-1, -1, 2}},     // mage (frost, as mod-playerbots at low level)
    {0, 0, 0b111, {-1, -1, 1}},     // warlock (demonology, same)
    {0, 0, 0, {-1, -1, -1}},        // unused
    {0b010, 0b100, 0b011, {1, 2, 0}},  // druid: feral; restoration; balance, feral
};

uint8_t MaskFor(ClassRoles const& c, Role role)
{
    switch (role)
    {
        case Role::Tank: return c.tank;
        case Role::Heal: return c.heal;
        case Role::Dps: return c.dps;
        default: return 0;
    }
}
}  // namespace

Role ParseRole(std::string const& word)
{
    std::string w = word;
    std::transform(w.begin(), w.end(), w.begin(), [](unsigned char c) { return std::tolower(c); });
    if (w == "tank")
        return Role::Tank;
    if (w == "heal")
        return Role::Heal;
    if (w == "dps")
        return Role::Dps;
    return Role::None;
}

char const* RoleName(Role role)
{
    switch (role)
    {
        case Role::Tank: return "tank";
        case Role::Heal: return "heal";
        case Role::Dps: return "dps";
        default: return "none";
    }
}

bool TabFitsRole(uint8_t cls, Role role, int32_t tab)
{
    return cls < 12 && tab >= 0 && tab < 3 && (MaskFor(Classes[cls], role) & (1u << tab));
}

int32_t TabForRole(uint8_t cls, Role role, int32_t currentTab)
{
    if (cls >= 12 || role == Role::None || !MaskFor(Classes[cls], role))
        return -1;
    if (TabFitsRole(cls, role, currentTab))
        return currentTab;
    return Classes[cls].defaults[uint8_t(role) - 1];
}
}  // namespace PlayerbotsPlus
