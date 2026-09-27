/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "ShareQuestsAction.h"

#include "ErrandsCommon.h"
#include "ErrandsValues.h"
#include "GroupItems.h"
#include "LevelUpRules.h"
#include "ObjectMgr.h"
#include "Playerbots.h"

namespace PlayerbotsPlus
{
bool ShareQuestsAction::isUseful()
{
    ErrandsData& data = AI_VALUE(ErrandsData&, "errands data");
    uint32 const now = getMSTime();
    if (!ErrandsIdle(data, now) || !Elapsed(now, data.lastQuestShareAt, QuestShareIntervalMs))
        return false;
    data.lastQuestShareAt = now;
    return !GroupBots(bot, QuestShareDistance).empty();
}

bool ShareQuestsAction::Execute(Event /*event*/)
{
    std::vector<Player*> const mates = GroupBots(bot, QuestShareDistance);  // bots only
    bool shared = false;
    for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        Quest const* quest = sObjectMgr->GetQuestTemplate(bot->GetQuestSlotQuestId(slot));
        if (!quest)
            continue;
        for (Player* mate : mates)
        {
            QuestShareCheck check;
            check.sharable = quest->HasFlag(QUEST_FLAGS_SHARABLE);
            check.targetIsBot = GET_PLAYERBOT_AI(mate) != nullptr;
            check.inRange = bot->IsWithinDistInMap(mate, QuestShareDistance);
            check.targetCanTake = mate->GetQuestStatus(quest->GetQuestId()) == QUEST_STATUS_NONE &&
                                  mate->CanTakeQuest(quest, false) && mate->CanAddQuest(quest, false);
            if (!ShouldShareQuest(check))
                continue;
            mate->AddQuestAndCheckCompletion(quest, bot);
            botAI->TellMasterNoFacing("Shared " + chat->FormatQuest(quest) + " with " + mate->GetName());
            shared = true;
        }
    }
    DebugErrands(botAI, shared ? "quests: shared" : "quests: nothing to share");
    return shared;
}
}  // namespace PlayerbotsPlus
