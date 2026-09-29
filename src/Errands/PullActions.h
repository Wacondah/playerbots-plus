/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_PULL_ACTIONS_H
#define PLAYERBOTS_PLUS_PULL_ACTIONS_H

#include "Action.h"
#include "MovementActions.h"
#include "Multiplier.h"
#include "PullRules.h"

namespace PlayerbotsPlus
{
// "pull" / skull: the tank plans and starts (or asks for "pull force").
class PullRequestAction : public Action
{
public:
    PullRequestAction(PlayerbotAI* botAI, std::string const name = "errands pull request") : Action(botAI, name) {}
    bool Execute(Event event) override;

protected:
    bool Request(Unit* target);
};

class PullSkullAction : public PullRequestAction
{
public:
    PullSkullAction(PlayerbotAI* botAI) : PullRequestAction(botAI, "errands pull skull") {}
    bool Execute(Event event) override;
};

class PullForceAction : public Action
{
public:
    PullForceAction(PlayerbotAI* botAI) : Action(botAI, "errands pull force") {}
    bool Execute(Event event) override;
};

class PullCancelAction : public Action
{
public:
    PullCancelAction(PlayerbotAI* botAI) : Action(botAI, "errands pull cancel") {}
    bool Execute(Event event) override;
};

// The tank watches the end of the pull (in isUseful); the puller walks, shoots, walks back.
class PullStepAction : public MovementAction
{
public:
    PullStepAction(PlayerbotAI* botAI) : MovementAction(botAI, "errands pull step") {}
    bool isUseful() override;
    bool Execute(Event event) override;

private:
    void Supervise(uint64 group, PullRun const& run);
};

// Freezes the group (not the puller on its way, not the tank once the mob is close).
class PullHoldMultiplier : public Multiplier
{
public:
    PullHoldMultiplier(PlayerbotAI* botAI) : Multiplier(botAI, "errands pull hold") {}
    float GetValue(Action* action) override;
};
}  // namespace PlayerbotsPlus

#endif
