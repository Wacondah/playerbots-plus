/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_ERRANDS_TRIGGERS_H
#define PLAYERBOTS_PLUS_ERRANDS_TRIGGERS_H

#include "Group.h"
#include "Playerbots.h"
#include "PlayerbotsPlusConfig.h"
#include "PullRules.h"
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

// Bags are rebalanced out of combat, master moving or not.
class BagsTickTrigger : public Trigger
{
public:
    BagsTickTrigger(PlayerbotAI* botAI) : Trigger(botAI, "bags tick") {}

    bool IsActive() override { return Config().enabled && !bot->IsInCombat() && bot->GetGroup(); }
};

// Every tick while grouped: the step action's isUseful() does the checks.
class PullTickTrigger : public Trigger
{
public:
    PullTickTrigger(PlayerbotAI* botAI) : Trigger(botAI, "errands pull tick") {}

    bool IsActive() override { return Config().enabled && bot->GetGroup(); }
};

// The master put the skull on a new mob, out of combat: the tank pulls it.
class PullSkullTrigger : public Trigger
{
public:
    PullSkullTrigger(PlayerbotAI* botAI) : Trigger(botAI, "errands pull skull", 1) {}

    bool IsActive() override
    {
        Group* group = bot->GetGroup();
        if (!Config().enabled || !group || bot->IsInCombat() || !PlayerbotAI::IsTank(bot))
            return false;
        uint64 const skull = group->GetTargetIcon(7).GetRawValue();
        uint64 const key = group->GetGUID().GetRawValue();
        return skull && skull != PullBoard::LastSkull(key) && !PullBoard::Get(key);
    }
};
}  // namespace PlayerbotsPlus

#endif
