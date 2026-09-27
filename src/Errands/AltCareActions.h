/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_ALT_CARE_ACTIONS_H
#define PLAYERBOTS_PLUS_ALT_CARE_ACTIONS_H

#include "Action.h"

namespace PlayerbotsPlus
{
// The group must have been out of combat this long before a dead alt releases.
constexpr uint32 ReleaseCalmMs = 10000;

// "errands loot": rolls need on gear the bot would wear and on materials of its own
// professions, before mod-playerbots' "loot roll" (which then skips the voted rolls).
class LootNeedAction : public Action
{
public:
    LootNeedAction(PlayerbotAI* botAI) : Action(botAI, "errands loot roll") {}

    bool Execute(Event event) override;
};

// "errands revive" (dead state): release when no living member could resurrect the bot.
class ReleaseWhenAloneAction : public Action
{
public:
    ReleaseWhenAloneAction(PlayerbotAI* botAI) : Action(botAI, "errands release") {}

    bool isUseful() override;
    bool Execute(Event event) override;

private:
    bool SomeoneCanResurrect();
};
}  // namespace PlayerbotsPlus

#endif
