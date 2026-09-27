/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "RunErrandAction.h"

#include "Bag.h"
#include "ErrandsCommon.h"
#include "GroupItems.h"
#include "Item.h"
#include "ItemUsageValue.h"
#include "ObjectAccessor.h"
#include "Playerbots.h"
#include "PlayerbotsPlusConfig.h"
#include "Professions.h"
#include "ProfessionsValue.h"
#include "ReagentIndex.h"
#include "SellAction.h"
#include "SellRules.h"
#include "Shopping.h"

namespace PlayerbotsPlus
{
ErrandsData& RunErrandAction::Data()
{
    return AI_VALUE(ErrandsData&, "errands data");
}

std::vector<uint32> const& RunErrandAction::Assigned()
{
    return AI_VALUE(ProfessionsData&, "assigned professions").skills;
}

bool RunErrandAction::isUseful()
{
    ErrandsData& data = Data();
    if (data.city.Active())
        return false;  // the city trip drives the bot
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
        data.junkChecked = false;
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
    snap.hasJunk = HasJunk(data, now);
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
        if (c.canTurnIn || c.canAccept || c.canRepair || c.canSell || c.canTrain || c.canSellTool ||
            c.canSellReagent || c.canTrainClass)
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
        c.canSellReagent = c.canSell && SellsShopping(creature, Data().shopping);
        c.canTrainClass = creature->IsTrainer() && CanTrainClassAt(bot, creature);
        std::vector<uint32> const& assigned = Assigned();
        if (!assigned.empty())
        {
            c.canTrain = creature->IsTrainer() && CanTrainAt(bot, creature, assigned);
            c.canSellTool = c.canSell && MissingToolAt(bot, creature, assigned);
        }
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

        // Not upstream "sell vendor": it ignores quality and the group's needs.
        SellAction sell(botAI);
        for (Item* item : ExtraJunk())
            sell.Sell(item);
    }

    // Then spending, most important first: repairs were paid, selling funded the rest.
    if (creature->IsTrainer())
        TrainClassAt(bot, creature);
    std::vector<uint32> const& assigned = Assigned();
    if (!assigned.empty())
    {
        if (creature->IsTrainer())
            TrainAt(bot, creature, assigned);
        if (creature->IsVendor())
            BuyToolsAt(bot, creature, assigned);
    }

    // Craft reagents last: they keep the shopping reserve.
    uint32 const reserve = ShoppingReserve(bot->GetLevel(), Config().shoppingReservePer10Levels);
    if (creature->IsVendor() && BuyShoppingAt(bot, creature, Data().shopping, reserve))
    {
        DebugErrands(botAI, "errands: bought reagents");
        Data().shopping = ShoppingList{};
        Data().lastCraftScanAt = 0;  // craft at the next tick
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
bool RunErrandAction::IsBasicJunk(Item* item)
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

bool RunErrandAction::HasJunk(ErrandsData& data, uint32 now)
{
    if (data.junkChecked && !Elapsed(now, data.junkCheckedAt, JunkIntervalMs))
        return data.hasJunk;

    bool basic = false;
    ForEachBagItem(bot, [&](Item* item) { basic = basic || IsBasicJunk(item); });
    data.hasJunk = basic || !ExtraJunk().empty();
    data.junkChecked = true;
    data.junkCheckedAt = now;
    return data.hasJunk;
}

namespace
{
// Crafting materials only: tools (mining pick, blacksmith hammer...) carry a totem
// category and stay, whatever upstream thinks of them.
bool IsMaterial(ItemTemplate const* proto)
{
    switch (proto->Class)
    {
        case ITEM_CLASS_TRADE_GOODS:
        case ITEM_CLASS_REAGENT:
        case ITEM_CLASS_GEM:
            return !proto->TotemCategory;
        default:
            return false;
    }
}
}  // namespace

// Beyond greys/whites: bound junk, uncrafted food the bot never eats, and tradeables
// no bot of the group would take, including materials upstream wrongly marks as used
// by the holder's professions.
std::vector<Item*> RunErrandAction::ExtraJunk()
{
    std::vector<Item*> junk;
    uint32 const maxQuality = Config().maxSellQuality;
    bool const foodCheat = botAI->HasCheat(BotCheatMask::food);
    uint32 const gatherFeeds =
        GatherFeeds(bot->HasSkill(SKILL_MINING), bot->HasSkill(SKILL_HERBALISM), bot->HasSkill(SKILL_SKINNING));
    std::vector<Player*> group;
    bool groupLoaded = false;
    // Kept when someone uses it, or when an enchanter of the group will disenchant it.
    auto wantedByGroup = [&](Item* item)
    {
        if (!groupLoaded)
        {
            group = GroupBots(bot, 0.f);
            groupLoaded = true;
        }
        ShareItem const described = DescribeForGroup(botAI, item, group);
        return WantedByGroup(described) || described.groupCanDisenchant;
    };

    ForEachBagItem(bot,
                   [&](Item* item)
                   {
                       ItemTemplate const* proto = item->GetTemplate();
                       if (!proto->SellPrice || proto->Quality == ITEM_QUALITY_POOR || proto->Quality > maxQuality)
                           return;
                       ItemUsage const usage = AI_VALUE2(ItemUsage, "item usage", int32(proto->ItemId));

                       FoodItem food;
                       food.isFood = proto->Class == ITEM_CLASS_CONSUMABLE && proto->SubClass == ITEM_SUBCLASS_FOOD;
                       food.crafted = ReagentIndex::IsCrafted(proto->ItemId);
                       food.quest = usage == ITEM_USAGE_QUEST;
                       food.hasSellPrice = proto->SellPrice > 0;
                       food.quality = proto->Quality;
                       if (SellableFood(food, foodCheat, Config().sellFood, maxQuality))
                       {
                           // Some food is also a cooking reagent: a cook of the group keeps priority.
                           if (!item->CanBeTraded() || !wantedByGroup(item))
                               junk.push_back(item);
                           return;
                       }

                       if (usage == ITEM_USAGE_VENDOR)
                       {
                           // Bound: only its holder could disenchant it.
                           if (!CanDisenchant(bot, proto))
                               junk.push_back(item);
                           return;
                       }
                       if (usage == ITEM_USAGE_SKILL && item->CanBeTraded() && IsMaterial(proto) &&
                           !KeptForOwnSkill(ReagentIndex::UsedBy(proto->ItemId), ReagentIndex::Known(bot),
                                            gatherFeeds, ReagentIndex::IsCrafted(proto->ItemId)) &&
                           !wantedByGroup(item))
                       {
                           junk.push_back(item);
                           return;
                       }
                       if (usage == ITEM_USAGE_AH && item->CanBeTraded() && !wantedByGroup(item))
                           junk.push_back(item);
                   });
    return junk;
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
