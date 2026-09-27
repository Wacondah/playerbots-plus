/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_PROFESSION_CATALOG_H
#define PLAYERBOTS_PLUS_PROFESSION_CATALOG_H

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

// Pure data and rules for assigned professions.
namespace PlayerbotsPlus
{
struct ProfessionInfo
{
    char const* name;  // lower-case English, as typed in the command
    uint32_t skill;    // core skill id
    uint32_t tool;     // item bought from vendors, 0 if none
};

// The 11 primary professions of 3.3.5.
std::vector<ProfessionInfo> const& PrimaryProfessions();
ProfessionInfo const* FindProfession(uint32_t skill);

struct ParsedAssignment
{
    std::vector<uint32_t> skills;
    std::string error;

    bool Ok() const { return error.empty(); }
};

// "mining tailoring" -> {186, 197}. One or two names, case-insensitive, no duplicates.
ParsedAssignment ParseAssignment(std::string const& text);

// Persistence format of the assignment: "186,197".
std::string SaveAssignment(std::vector<uint32_t> const& skills);
std::vector<uint32_t> LoadAssignment(std::string const& text);  // unknown ids dropped

// Primary profession to forget so that `wanted` can be learned: the unassigned one with
// the lowest skill value. 0 when nothing must go (free slot, already known, or nothing
// unassigned to drop).
uint32_t ProfessionToForget(std::vector<std::pair<uint32_t, uint32_t>> const& known, std::vector<uint32_t> const& assigned,
                            uint32_t freeSlots, uint32_t wanted);
}  // namespace PlayerbotsPlus

#endif
