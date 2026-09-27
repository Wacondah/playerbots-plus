/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "ProfessionsAction.h"

#include "ErrandPlanner.h"
#include "Event.h"
#include "PlayerbotRepository.h"
#include "Playerbots.h"
#include "Professions.h"
#include "ProfessionsValue.h"

#include <algorithm>
#include <sstream>

namespace PlayerbotsPlus
{
namespace
{
// "professions reset" must be confirmed within this delay.
constexpr uint32 ResetConfirmMs = 30000;

std::string NameOf(uint32 skill)
{
    ProfessionInfo const* p = FindProfession(skill);
    return p ? p->name : std::to_string(skill);
}
}  // namespace

bool ProfessionsAction::Execute(Event event)
{
    ProfessionsData& data = AI_VALUE(ProfessionsData&, "assigned professions");
    std::string const param = event.getParam();
    uint32 const now = getMSTime();

    if (param.empty())
    {
        botAI->TellMaster(Status());
        return true;
    }

    if (param == "clear")
    {
        data.skills.clear();
        PlayerbotRepository::instance().Save(botAI);
        botAI->TellMaster("professions: assignment cleared");
        return true;
    }

    std::vector<std::pair<uint32, uint32>> const known = KnownPrimaries(bot);
    if (param == "reset")
    {
        if (known.empty())
        {
            botAI->TellMaster("professions: no primary profession to forget");
            return true;
        }
        std::ostringstream out;
        out << "professions: will forget";
        for (auto const& [skill, value] : known)
            out << " " << NameOf(skill) << " (" << value << ")";
        out << ". Whisper 'professions reset confirm' within 30 s";
        data.resetPending = true;
        data.resetAskedAt = now;
        botAI->TellMaster(out.str());
        return true;
    }

    if (param == "reset confirm")
    {
        if (!data.resetPending || Elapsed(now, data.resetAskedAt, ResetConfirmMs))
        {
            data.resetPending = false;
            botAI->TellMaster("professions: nothing to confirm, whisper 'professions reset' first");
            return true;
        }
        data.resetPending = false;
        for (auto const& [skill, value] : known)
            bot->SetSkill(skill, 0, 0, 0);
        botAI->TellMaster("professions: primary professions forgotten; the assignment is kept");
        return true;
    }

    ParsedAssignment const parsed = ParseAssignment(param);
    if (!parsed.Ok())
    {
        botAI->TellMaster("professions: " + parsed.error);
        return false;
    }
    data.skills.assign(parsed.skills.begin(), parsed.skills.end());
    PlayerbotRepository::instance().Save(botAI);
    botAI->TellMaster(Status());
    return true;
}

std::string ProfessionsAction::Status()
{
    ProfessionsData& data = AI_VALUE(ProfessionsData&, "assigned professions");
    std::vector<std::pair<uint32, uint32>> const known = KnownPrimaries(bot);
    auto knows = [&](uint32 skill)
    { return std::any_of(known.begin(), known.end(), [skill](auto const& k) { return k.first == skill; }); };

    std::ostringstream out;
    out << "professions:";
    if (data.skills.empty())
        out << " none assigned";
    for (uint32 skill : data.skills)
    {
        out << " " << NameOf(skill) << " ";
        if (knows(skill))
            out << bot->GetSkillValue(skill) << "/" << bot->GetMaxSkillValue(skill);
        else
            out << "(not learned)";
    }
    if (!data.skills.empty())
        for (auto const& [skill, value] : known)
            if (std::find(data.skills.begin(), data.skills.end(), skill) == data.skills.end())
                out << " | knows " << NameOf(skill) << " (" << value << "), not assigned";
    return out.str();
}
}  // namespace PlayerbotsPlus
