/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_ROLE_RULES_H
#define PLAYERBOTS_PLUS_ROLE_RULES_H

#include <cstdint>
#include <string>

// Pure rules for the "role" command: which talent tree fits a role, per class.
namespace PlayerbotsPlus
{
enum class Role : uint8_t
{
    None,
    Tank,
    Heal,
    Dps
};

Role ParseRole(std::string const& word);
char const* RoleName(Role role);

// The talent tree (0..2) of that class can play that role.
bool TabFitsRole(uint8_t cls, Role role, int32_t tab);

// Tree for the role: the current one (tree with most points, -1 if none) when it fits,
// else the class default for that role; -1 when the class cannot play it.
int32_t TabForRole(uint8_t cls, Role role, int32_t currentTab);
}  // namespace PlayerbotsPlus

#endif
