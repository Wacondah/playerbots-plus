/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "RunErrandAction.h"

#include "Bag.h"
#include "ErrandsCommon.h"
#include "Item.h"
#include "ObjectAccessor.h"
#include "Playerbots.h"
#include "PlayerbotsPlusConfig.h"

namespace PlayerbotsPlus
{
ErrandsData& RunErrandAction::Data()
{
    return AI_VALUE(ErrandsData&, "errands data");
}

bool RunErrandAction::isUseful()
{
    ErrandsData& data = Data();
    uint32 const now = getMSTime();
    Snapshot const snap = BuildSnapshot(data, now);
    data.decision = Plan(snap, data.state, Config().planner, now);
    data.decidedAt = now;

    if (data.decision.type == DecisionType::Start || data.decision.type == DecisionType::Abandon)
        DebugErrands(botAI, data.decision.reason);

    return data.decision.Acts();
}

bool RunErrandAction::Execute(Event /*event*/)
{
    ErrandsData& data = Data();
    Decision const decision = data.decision;
    if (!decision.Acts())
        return false;

    ObjectGuid const guid(decision.target);
    WorldObject* object = ObjectAccessor::GetWorldObject(*bot, guid);
    if (!object)
    {
        MarkFailed(data.state, getMSTime());
        data.scanned = false;
        DebugErrands(botAI, "target vanished");
        return false;
    }

    if (IsWithinInteractionDist(object))
    {
        VisitTarget(object);
        MarkDone(data.state, QuestFingerprint(), getMSTime());
        data.scanned = false;  // rescan now so the next errand sees fresh quest flags
        DebugErrands(botAI, std::string("done at ") + object->GetName());
        return true;
    }

    // false means "still walking the previous path", not "unreachable":
    // unreachable targets end on the planner timeout. Keep the tick either way
    // so follow does not pull the bot back mid-errand.
    MoveWorldObjectTo(guid);
    return true;
}

Snapshot RunErrandAction::BuildSnapshot(ErrandsData& data, uint32 now)
{
    Snapshot snap;
    snap.botPos = {bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ()};
    snap.botInCombat = bot->IsInCombat();
    snap.botHasStayOrGuard = botAI->HasStrategy("stay", BotState::BOT_STATE_NON_COMBAT) ||
                             botAI->HasStrategy("guard", BotState::BOT_STATE_NON_COMBAT);
    snap.inInstance = bot->GetMap() && bot->GetMap()->Instanceable();
    snap.botNeedsRest = NeedsRest(bot);
    snap.needsRepair = NeedsRepair();
    snap.hasJunk = HasJunk();
    snap.questFingerprint = QuestFingerprint();

    Player* master = botAI->GetMaster();
    snap.hasMaster = master && master->IsInWorld();
    if (!snap.hasMaster)
        return snap;

    snap.masterSameMap = master->GetMapId() == bot->GetMapId();
    snap.masterInCombat = master->IsInCombat();
    snap.masterMounted = master->IsMounted();
    snap.masterOnTaxi = master->IsInFlight();
    snap.masterMoving = master->isMoving();
    snap.masterPos = {master->GetPositionX(), master->GetPositionY(), master->GetPositionZ()};

    if (snap.masterSameMap && !snap.masterMoving && (!data.scanned || now - data.lastScanAt >= ScanIntervalMs))
        Scan(data, master, now);

    snap.candidates = data.candidates;
    return snap;
}

void RunErrandAction::Scan(ErrandsData& data, Player* master, uint32 now)
{
    data.candidates.clear();
    float const radius = Config().planner.radius;

    auto consider = [&](WorldObject* object)
    {
        if (!object || !object->IsInWorld() || master->GetDistance(object) > radius)
            return;
        Candidate const c = Describe(object);
        if (c.canTurnIn || c.canAccept || c.canRepair || c.canSell)
            data.candidates.push_back(c);
    };

    for (ObjectGuid const& guid : AI_VALUE(GuidVector, "nearest npcs"))
        consider(ObjectAccessor::GetWorldObject(*bot, guid));
    for (ObjectGuid const& guid : AI_VALUE(GuidVector, "nearest game objects no los"))
        consider(ObjectAccessor::GetWorldObject(*bot, guid));

    data.lastScanAt = now;
    data.scanned = true;
}

Candidate RunErrandAction::Describe(WorldObject* object)
{
    Candidate c;
    c.id = object->GetGUID().GetRawValue();
    c.pos = {object->GetPositionX(), object->GetPositionY(), object->GetPositionZ()};

    if (CanInteractWithQuestGiver(object))
    {
        bot->PrepareQuestMenu(object->GetGUID());
        QuestMenu const& menu = bot->PlayerTalkClass->GetQuestMenu();
        for (uint8 i = 0; i < menu.GetMenuItemCount(); ++i)
        {
            uint32 const questId = menu.GetItem(i).QuestId;
            Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
            if (!quest)
                continue;

            QuestStatus const status = bot->GetQuestStatus(questId);
            if (status == QUEST_STATUS_COMPLETE && bot->CanRewardQuest(quest, 0, false))
                c.canTurnIn = true;
            // A full log does not rule the quest out: the visit organises the log first.
            else if (status == QUEST_STATUS_NONE && bot->CanTakeQuest(quest, false) &&
                     (bot->CanAddQuest(quest, false) || !bot->SatisfyQuestLog(false)) &&
                     IsQuestWorthDoing(quest) && IsQuestCapableDoing(quest))
                c.canAccept = true;
        }
    }

    if (Creature* creature = object->ToCreature())
    {
        c.canRepair = creature->HasNpcFlag(UNIT_NPC_FLAG_REPAIR);
        c.canSell = creature->HasNpcFlag(UNIT_NPC_FLAG_VENDOR);
    }
    return c;
}

// Does everything the target offers in one visit.
void RunErrandAction::VisitTarget(WorldObject* object)
{
    if (!bot->SatisfyQuestLog(false))
        OrganizeQuestLog();

    if (bot->CanInteractWithQuestGiver(object))
        InteractWithNpcOrGameObjectForQuest(object->GetGUID());

    Creature* creature = object->ToCreature();
    if (!creature)
        return;

    if (creature->HasNpcFlag(UNIT_NPC_FLAG_REPAIR) && NeedsRepair())
        botAI->DoSpecificAction("repair", Event("run errand"), true);

    if (creature->HasNpcFlag(UNIT_NPC_FLAG_VENDOR))
    {
        botAI->DoSpecificAction("sell", Event("run errand", "gray"), true);
        if (Config().sellWhite)
            botAI->DoSpecificAction("sell", Event("run errand", "white"), true);
    }
}

bool RunErrandAction::NeedsRepair()
{
    uint32 const threshold = Config().repairThreshold;
    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
    {
        Item* item = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (!item)
            continue;
        uint32 const max = item->GetUInt32Value(ITEM_FIELD_MAXDURABILITY);
        if (max && item->GetUInt32Value(ITEM_FIELD_DURABILITY) * 100 < max * threshold)
            return true;
    }
    return false;
}

// Mirrors what "sell gray" / "sell white" actually sell (see SellQualityItemsVisitor),
// so a bot never walks to a vendor for items the sell action would skip.
bool RunErrandAction::IsJunk(Item* item)
{
    if (!item)
        return false;
    ItemTemplate const* proto = item->GetTemplate();
    if (!proto->SellPrice)
        return false;
    if (proto->Quality == ITEM_QUALITY_POOR)
        return true;
    if (!Config().sellWhite || proto->Quality != ITEM_QUALITY_NORMAL)
        return false;
    if (proto->Class == ITEM_CLASS_ARMOR)
        return true;
    return proto->Class == ITEM_CLASS_WEAPON && proto->SubClass != ITEM_SUBCLASS_WEAPON_MISC &&
           proto->SubClass != ITEM_SUBCLASS_WEAPON_FISHING_POLE && !proto->TotemCategory;
}

bool RunErrandAction::HasJunk()
{
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
        if (IsJunk(bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot)))
            return true;

    for (uint8 bagSlot = INVENTORY_SLOT_BAG_START; bagSlot < INVENTORY_SLOT_BAG_END; ++bagSlot)
        if (Bag* bag = bot->GetBagByPos(bagSlot))
            for (uint32 i = 0; i < bag->GetBagSize(); ++i)
                if (IsJunk(bag->GetItemByPos(i)))
                    return true;

    return false;
}

// Changes whenever the bot could see different quests at a giver:
// level up, quest rewarded, quest log or objective state changed.
uint64_t RunErrandAction::QuestFingerprint()
{
    uint64_t hash = 1469598103934665603ULL;  // FNV-1a
    auto mix = [&hash](uint64_t value)
    {
        hash ^= value;
        hash *= 1099511628211ULL;
    };

    mix(bot->GetLevel());
    mix(bot->getRewardedQuests().size());
    for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 const questId = bot->GetQuestSlotQuestId(slot);
        mix(questId);
        if (questId)
            mix(bot->GetQuestStatus(questId));
    }
    return hash;
}
}  // namespace PlayerbotsPlus
