/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_QUEST_LOG_ACTION_H
#define PLAYERBOTS_PLUS_QUEST_LOG_ACTION_H

#include "Action.h"

namespace PlayerbotsPlus
{
// Addon prefix of the "questlog" replies (PlayerbotsPlusQuests).
constexpr char const* QuestLogPrefix = "PPQ";

// "questlog" (sent by the PlayerbotsPlusQuests addon as "BOT\t#a questlog"): the bot's
// quests and objective counts, as addon whispers to its master only.
class QuestLogAction : public Action
{
public:
    QuestLogAction(PlayerbotAI* botAI) : Action(botAI, "questlog") {}

    bool Execute(Event event) override;
};
}  // namespace PlayerbotsPlus

#endif
