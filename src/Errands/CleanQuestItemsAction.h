/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_CLEAN_QUEST_ITEMS_ACTION_H
#define PLAYERBOTS_PLUS_CLEAN_QUEST_ITEMS_ACTION_H

#include "Action.h"

namespace PlayerbotsPlus
{
// Quest items are looked at this often per bot.
constexpr uint32 CleanQuestItemsIntervalMs = 30000;

// Destroys quest items nobody needs any more: quests done, no price at a vendor.
class CleanQuestItemsAction : public Action
{
public:
    CleanQuestItemsAction(PlayerbotAI* botAI) : Action(botAI, "clean quest items") {}

    bool isUseful() override;
    bool Execute(Event event) override;

private:
    Item* FindStale();
};
}  // namespace PlayerbotsPlus

#endif
