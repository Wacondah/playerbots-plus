/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "ProfessionCatalog.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace PlayerbotsPlus
{
namespace
{
std::string Lower(std::string text)
{
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return std::tolower(c); });
    return text;
}

ProfessionInfo const* FindByName(std::string const& name)
{
    for (ProfessionInfo const& p : PrimaryProfessions())
        if (name == p.name)
            return &p;
    return nullptr;
}

bool Contains(std::vector<uint32_t> const& list, uint32_t value)
{
    return std::find(list.begin(), list.end(), value) != list.end();
}
}  // namespace

std::vector<ProfessionInfo> const& PrimaryProfessions()
{
    static std::vector<ProfessionInfo> const professions = {
        {"alchemy", 171, 0},          {"blacksmithing", 164, 5956}, {"enchanting", 333, 0},
        {"engineering", 202, 6219},   {"herbalism", 182, 0},        {"inscription", 773, 39505},
        {"jewelcrafting", 755, 20815}, {"leatherworking", 165, 0},  {"mining", 186, 2901},
        {"skinning", 393, 7005},      {"tailoring", 197, 0},
    };
    return professions;
}

ProfessionInfo const* FindProfession(uint32_t skill)
{
    for (ProfessionInfo const& p : PrimaryProfessions())
        if (p.skill == skill)
            return &p;
    return nullptr;
}

ParsedAssignment ParseAssignment(std::string const& text)
{
    ParsedAssignment result;
    std::istringstream words(Lower(text));
    std::string word;
    while (words >> word)
    {
        ProfessionInfo const* p = FindByName(word);
        if (!p)
            return {{}, "unknown profession: " + word};
        if (Contains(result.skills, p->skill))
            return {{}, "duplicate profession: " + word};
        result.skills.push_back(p->skill);
    }
    if (result.skills.size() > 2)
        return {{}, "at most two primary professions"};
    if (result.skills.empty())
        return {{}, "no profession given"};
    return result;
}

std::string SaveAssignment(std::vector<uint32_t> const& skills)
{
    std::string out;
    for (uint32_t skill : skills)
        out += (out.empty() ? "" : ",") + std::to_string(skill);
    return out;
}

std::vector<uint32_t> LoadAssignment(std::string const& text)
{
    std::vector<uint32_t> skills;
    std::istringstream parts(text);
    std::string part;
    while (std::getline(parts, part, ','))
    {
        if (part.empty() || !std::all_of(part.begin(), part.end(), [](unsigned char c) { return std::isdigit(c); }))
            continue;
        uint32_t const skill = static_cast<uint32_t>(std::stoul(part));
        if (FindProfession(skill) && !Contains(skills, skill))
            skills.push_back(skill);
    }
    return skills;
}

uint32_t ProfessionToForget(std::vector<std::pair<uint32_t, uint32_t>> const& known, std::vector<uint32_t> const& assigned,
                            uint32_t freeSlots, uint32_t wanted)
{
    if (freeSlots > 0 || assigned.empty())
        return 0;

    uint32_t forget = 0;
    uint32_t lowest = 0;
    for (auto const& [skill, value] : known)
    {
        if (skill == wanted)
            return 0;  // already known
        if (Contains(assigned, skill))
            continue;
        if (!forget || value < lowest || (value == lowest && skill < forget))
        {
            forget = skill;
            lowest = value;
        }
    }
    return forget;
}
}  // namespace PlayerbotsPlus
