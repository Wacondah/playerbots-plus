/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_SHARE_QUESTS_ACTION_H
#define PLAYERBOTS_PLUS_SHARE_QUESTS_ACTION_H

#include "Action.h"

namespace PlayerbotsPlus
{
// Quests are offered to the other alts at most this often.
constexpr uint32 QuestShareIntervalMs = 30000;
// Same reach as the game's quest sharing.
constexpr float QuestShareDistance = 30.f;

// "errands quests": give the other alts of the group the quests they can take. Direct,
// bot to bot: the game's party push would also pop a quest window for the master.
class ShareQuestsAction : public Action
{
public:
    ShareQuestsAction(PlayerbotAI* botAI) : Action(botAI, "share quests") {}

    bool isUseful() override;
    bool Execute(Event event) override;
};
}  // namespace PlayerbotsPlus

#endif
