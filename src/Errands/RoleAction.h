/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_ROLE_ACTION_H
#define PLAYERBOTS_PLUS_ROLE_ACTION_H

#include "Action.h"
#include "PlayerbotAI.h"

#include <string>
#include <vector>

namespace PlayerbotsPlus
{
// "role [tank|heal|dps] [spec]", whispered to a bot: talents of the fitting tree (kept for
// level-ups), combat strategies of that role, "threat" for dps; non-combat and dead
// strategies are kept (mod-playerbots' "talents spec" resets them all).
class RoleAction : public Action
{
public:
    RoleAction(PlayerbotAI* botAI) : Action(botAI, "role") {}

    bool Execute(Event event) override;

private:
    std::string Status();
    void Restore(BotState state, std::vector<std::string> const& saved);
};
}  // namespace PlayerbotsPlus

#endif
