/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_EQUIP_FITTING_ACTION_H
#define PLAYERBOTS_PLUS_EQUIP_FITTING_ACTION_H

#include "EquipAction.h"
#include "Multiplier.h"

namespace PlayerbotsPlus
{
// mod-playerbots' "equip upgrades packet action", minus the pieces the class does not wear:
// its item usage, a score comparison against a worn piece of the wrong type, says "equip"
// for a cloth piece to a warrior in leather.
class EquipFittingAction : public EquipAction
{
public:
    EquipFittingAction(PlayerbotAI* botAI) : EquipAction(botAI, "errands equip upgrades") {}

    bool Execute(Event event) override;
};

// Replaces the upstream action above for the bots with errands.
class EquipReplaceMultiplier : public Multiplier
{
public:
    EquipReplaceMultiplier(PlayerbotAI* botAI) : Multiplier(botAI, "errands equip") {}

    float GetValue(Action* action) override;
};
}  // namespace PlayerbotsPlus

#endif
