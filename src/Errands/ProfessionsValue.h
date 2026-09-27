/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_PROFESSIONS_VALUE_H
#define PLAYERBOTS_PLUS_PROFESSIONS_VALUE_H

#include "ProfessionCatalog.h"
#include "Value.h"

namespace PlayerbotsPlus
{
struct ProfessionsData
{
    std::vector<uint32> skills;  // assigned primary professions
    bool resetPending = false;   // "professions reset" asked, waiting for "confirm"
    uint32 resetAskedAt = 0;
};

// Saved by mod-playerbots with the bot's strategies ("assigned professions>186,197").
class ProfessionsValue : public ManualSetValue<ProfessionsData&>
{
public:
    ProfessionsValue(PlayerbotAI* botAI) : ManualSetValue<ProfessionsData&>(botAI, data, "assigned professions") {}

    std::string const Save() override { return SaveAssignment({data.skills.begin(), data.skills.end()}); }

    bool Load(std::string const text) override
    {
        std::vector<uint32_t> const skills = LoadAssignment(text);
        data.skills.assign(skills.begin(), skills.end());
        return true;
    }

private:
    ProfessionsData data;
};
}  // namespace PlayerbotsPlus

#endif
