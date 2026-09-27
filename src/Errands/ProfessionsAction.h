/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_PROFESSIONS_ACTION_H
#define PLAYERBOTS_PLUS_PROFESSIONS_ACTION_H

#include "Action.h"

namespace PlayerbotsPlus
{
// "professions [names | clear | reset [confirm]]", whispered to a bot.
class ProfessionsAction : public Action
{
public:
    ProfessionsAction(PlayerbotAI* botAI) : Action(botAI, "professions") {}

    bool Execute(Event event) override;

private:
    std::string Status();
};
}  // namespace PlayerbotsPlus

#endif
