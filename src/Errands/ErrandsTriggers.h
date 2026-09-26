/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_ERRANDS_TRIGGERS_H
#define PLAYERBOTS_PLUS_ERRANDS_TRIGGERS_H

#include "Playerbots.h"
#include "PlayerbotsPlusConfig.h"
#include "Trigger.h"

namespace PlayerbotsPlus
{
// Cheap gate; the planner, run from the action's isUseful(), decides the rest.
class ErrandsTickTrigger : public Trigger
{
public:
    ErrandsTickTrigger(PlayerbotAI* botAI) : Trigger(botAI, "errands tick") {}

    bool IsActive() override { return Config().enabled && botAI->GetMaster(); }
};
}  // namespace PlayerbotsPlus

#endif
