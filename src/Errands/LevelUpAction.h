/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_LEVEL_UP_ACTION_H
#define PLAYERBOTS_PLUS_LEVEL_UP_ACTION_H

#include "Action.h"

#include <string>

class Player;

namespace PlayerbotsPlus
{
// Premade spec number of that name for the player's class, -1 if unknown.
int32 PremadeSpecNo(Player* player, std::string const& name);

// On level-up, spend the free talent points: the chosen spec, else the current tree,
// else ask the master. Spells stay with the class trainer (paid).
class LevelUpAction : public Action
{
public:
    LevelUpAction(PlayerbotAI* botAI) : Action(botAI, "errands levelup") {}

    bool Execute(Event event) override;
};
}  // namespace PlayerbotsPlus

#endif
