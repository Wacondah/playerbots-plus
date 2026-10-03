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
    if (shared)
        DebugErrands(botAI, "quests: shared");  // silent otherwise: it runs every 30 s
    return shared;
}

bool ShareQuestsCommandAction::Execute(Event /*event*/)
{
    ErrandsData& data = AI_VALUE(ErrandsData&, "errands data");
    data.questPush = true;
    data.questPushOffered = 0;
    data.questPushAt = 0;
    return true;
}

bool ShareQuestsToMasterAction::isUseful()
{
    ErrandsData& data = AI_VALUE(ErrandsData&, "errands data");
    Player* master = RealMaster(botAI);
    if (!data.questPush || !Elapsed(getMSTime(), data.questPushAt, QuestPushIntervalMs))
        return false;
    if (!master || !master->IsInMap(bot))
    {
        data.questPush = false;
        botAI->TellMaster("share quests: you are not with me");
        return false;
    }
    return !master->GetDivider();  // he is still looking at the last one
}

bool ShareQuestsToMasterAction::Execute(Event /*event*/)
{
    ErrandsData& data = AI_VALUE(ErrandsData&, "errands data");
    data.questPushAt = getMSTime();
    Player* master = RealMaster(botAI);
    for (uint8 slot = 0; master && slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        Quest const* quest = sObjectMgr->GetQuestTemplate(bot->GetQuestSlotQuestId(slot));
        // The checks of the game's party push, for the master alone (the bots are not asked).
        if (!quest || !bot->CanShareQuest(quest->GetQuestId()) || !master->SatisfyQuestStatus(quest, false) ||
            master->GetQuestStatus(quest->GetQuestId()) == QUEST_STATUS_COMPLETE || !master->CanTakeQuest(quest, false) ||
            !master->SatisfyQuestLog(false))
            continue;
        if (data.questPushDeclined.count(quest->GetQuestId()))
            continue;  // offered already: not twice for one command
        data.questPushDeclined.insert(quest->GetQuestId());
        bot->SendPushToPartyResponse(master, QUEST_PARTY_MSG_SHARING_QUEST);
        if (quest->IsAutoAccept() && master->CanAddQuest(quest, true) && master->CanTakeQuest(quest, true))
            master->AddQuestAndCheckCompletion(quest, bot);
        if (quest->IsAutoComplete() || !quest->GetQuestMethod())
            master->PlayerTalkClass->SendQuestGiverRequestItems(quest, bot->GetGUID(),
                                                                master->CanCompleteRepeatableQuest(quest), true);
        else
        {
            master->SetDivider(bot->GetGUID());
            master->PlayerTalkClass->SendQuestGiverQuestDetails(quest, master->GetGUID(), true);
        }
        ++data.questPushOffered;
        return true;
    }
    data.questPush = false;
    data.questPushDeclined.clear();
    botAI->TellMaster(data.questPushOffered ? "share quests: that was all"
                                            : "share quests: nothing I have that you can take");
    return true;
}
}  // namespace PlayerbotsPlus
