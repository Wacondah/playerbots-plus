/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "QuestLogAction.h"

#include "Chat.h"
#include "Event.h"
#include "ObjectMgr.h"
#include "Playerbots.h"
#include "QuestLogFormat.h"

#include <algorithm>

namespace PlayerbotsPlus
{
namespace
{
std::string Localized(std::vector<std::string> const* names, LocaleConstant locale, std::string const& fallback)
{
    if (names)
    {
        std::string_view const text = ObjectMgr::GetLocaleString(*names, locale);
        if (!text.empty())
            return std::string(text);
    }
    return fallback;
}

// Creature (positive entry) or game object (negative entry) of a kill/use objective.
std::string ObjectiveName(int32 entry, LocaleConstant locale)
{
    if (entry > 0)
    {
        CreatureTemplate const* creature = sObjectMgr->GetCreatureTemplate(uint32(entry));
        CreatureLocale const* loc = sObjectMgr->GetCreatureLocale(uint32(entry));
        return creature ? Localized(loc ? &loc->Name : nullptr, locale, creature->Name) : "?";
    }
    GameObjectTemplate const* object = sObjectMgr->GetGameObjectTemplate(uint32(-entry));
    GameObjectLocale const* loc = sObjectMgr->GetGameObjectLocale(uint32(-entry));
    return object ? Localized(loc ? &loc->Name : nullptr, locale, object->name) : "?";
}
}  // namespace

bool QuestLogAction::Execute(Event event)
{
    Player* master = botAI->GetMaster();
    if (!master || !master->GetSession() || (event.getOwner() && event.getOwner() != master))
        return false;
    LocaleConstant const locale = master->GetSession()->GetSessionDbLocaleIndex();

    auto send = [&](std::string const& payload)
    {
        WorldPacket data;
        ChatHandler::BuildChatPacket(data, CHAT_MSG_WHISPER, LANG_ADDON, bot->GetGUID(), master->GetGUID(),
                                     std::string(QuestLogPrefix) + "\t" + payload, CHAT_TAG_NONE, bot->GetName());
        master->GetSession()->SendPacket(&data);
    };

    for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 const questId = bot->GetQuestSlotQuestId(slot);
        Quest const* quest = questId ? sObjectMgr->GetQuestTemplate(questId) : nullptr;
        if (!quest)
            continue;

        QuestLine line;
        line.id = questId;
        QuestStatus const status = bot->GetQuestStatus(questId);
        line.status = status == QUEST_STATUS_COMPLETE ? 1 : status == QUEST_STATUS_FAILED ? 2 : 0;
        QuestLocale const* questLoc = sObjectMgr->GetQuestLocale(questId);
        line.title = Localized(questLoc ? &questLoc->Title : nullptr, locale, quest->GetTitle());

        for (uint8 i = 0; i < QUEST_OBJECTIVES_COUNT; ++i)
            if (quest->RequiredNpcOrGo[i] && quest->RequiredNpcOrGoCount[i])
                line.objectives.push_back({bot->GetQuestSlotCounter(slot, i), quest->RequiredNpcOrGoCount[i],
                                           ObjectiveName(quest->RequiredNpcOrGo[i], locale)});
        for (uint8 i = 0; i < QUEST_ITEM_OBJECTIVES_COUNT; ++i)
        {
            uint32 const item = quest->RequiredItemId[i];
            ItemTemplate const* proto = item ? sObjectMgr->GetItemTemplate(item) : nullptr;
            if (!proto || !quest->RequiredItemCount[i])
                continue;
            ItemLocale const* itemLoc = sObjectMgr->GetItemLocale(item);
            uint32 const required = quest->RequiredItemCount[i];
            line.objectives.push_back({std::min(bot->GetItemCount(item, true), required), required,
                                       Localized(itemLoc ? &itemLoc->Name : nullptr, locale, proto->Name1)});
        }
        send(FormatQuestLine(line));
    }
    send("END");
    return true;
}
}  // namespace PlayerbotsPlus
