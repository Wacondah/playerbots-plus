/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "AltCareActions.h"

#include "ErrandsValues.h"
#include "Group.h"
#include "GroupItems.h"
#include "ItemUsageValue.h"
#include "LevelUpRules.h"
#include "ObjectMgr.h"
#include "Playerbots.h"
#include "ReagentIndex.h"
#include "SellRules.h"

namespace PlayerbotsPlus
{
namespace
{
// Out-of-combat resurrections, every rank: Resurrection, Redemption, Ancestral Spirit.
constexpr uint32 ResurrectionSpells[] = {2006,  2010,  10880, 10881, 20770, 25435, 48171, 48950,
                                         7328,  10322, 10324, 20772, 20773, 48949, 2008,  20609,
                                         20610, 20776, 20777, 25590, 49277};
}  // namespace

bool LootNeedAction::Execute(Event /*event*/)
{
    Group* group = bot->GetGroup();
    if (!group || group->GetLootMethod() == MASTER_LOOT || group->GetLootMethod() == FREE_FOR_ALL)
        return false;

    uint32 const gatherFeeds =
        GatherFeeds(bot->HasSkill(SKILL_MINING), bot->HasSkill(SKILL_HERBALISM), bot->HasSkill(SKILL_SKINNING));
    std::vector<FutureItem> const heldFuture = KeptFutureGear(bot);
    bool voted = false;
    for (Roll* roll : group->GetRolls())
    {
        auto const vote = roll->playerVote.find(bot->GetGUID());
        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(roll->itemid);
        if (vote == roll->playerVote.end() || vote->second != NOT_EMITED_YET || !proto)
            continue;

        std::string param = std::to_string(roll->itemid);
        if (roll->itemRandomPropId)
            param += "," + std::to_string(roll->itemRandomPropId);
        ItemUsage const usage = FittingUsage(bot, proto, AI_VALUE2(ItemUsage, "item usage", param));

        LootFacts facts;
        facts.gearUpgrade = (proto->Class == ITEM_CLASS_ARMOR || proto->Class == ITEM_CLASS_WEAPON) &&
                            (usage == ITEM_USAGE_EQUIP || usage == ITEM_USAGE_REPLACE);
        FutureItem future;  // too high a level for now, but better than what it wears and keeps
        if (!facts.gearUpgrade && DescribeFutureGear(bot, proto, int32(roll->itemRandomPropId), 0, future))
            facts.gearUpgrade = WouldKeep(future, heldFuture);
        facts.uniqueHeld = proto->HasFlag(ITEM_FLAG_UNIQUE_EQUIPPABLE) && bot->GetItemCount(proto->ItemId, true);
        facts.ownMaterial = KeptForOwnSkill(ReagentIndex::UsedBy(proto->ItemId), ReagentIndex::Known(bot),
                                            gatherFeeds, ReagentIndex::IsCrafted(proto->ItemId));
        if (!ShouldRollNeed(facts))
            continue;  // mod-playerbots' own roll decides
        group->CountRollVote(bot->GetGUID(), roll->itemGUID, NEED);
        voted = true;
    }
    return voted;
}

bool ReleaseWhenAloneAction::isUseful()
{
    ErrandsData& data = AI_VALUE(ErrandsData&, "errands data");
    uint32 const now = getMSTime();
    Group* group = bot->GetGroup();
    bool fighting = !group;  // alone: mod-playerbots releases on its own
    for (GroupReference* ref = group ? group->GetFirstMember() : nullptr; ref; ref = ref->next())
        if (Player* member = ref->GetSource())
            fighting = fighting || (member->IsAlive() && member->IsInCombat());
    if (fighting || !data.calmSince)
        data.calmSince = now;

    ReleaseFacts facts;
    facts.dead = bot->isDead();
    facts.ghost = bot->HasPlayerFlag(PLAYER_FLAGS_GHOST);
    facts.inDungeon = bot->GetMap() && (bot->GetMap()->IsDungeon() || bot->GetMap()->IsRaid());
    facts.calmMs = now - data.calmSince;
    if (!facts.dead || facts.ghost || facts.inDungeon || facts.calmMs < ReleaseCalmMs)
        return false;
    facts.someoneCanResurrect = SomeoneCanResurrect();
    return ShouldRelease(facts, ReleaseCalmMs);
}

bool ReleaseWhenAloneAction::Execute(Event /*event*/)
{
    AI_VALUE(ErrandsData&, "errands data").calmSince = 0;
    return botAI->DoSpecificAction("release", Event("errands release"), true);
}

bool ReleaseWhenAloneAction::SomeoneCanResurrect()
{
    Group* group = bot->GetGroup();
    for (GroupReference* ref = group ? group->GetFirstMember() : nullptr; ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member || member == bot || !member->IsAlive() || member->GetMapId() != bot->GetMapId())
            continue;
        for (uint32 spell : ResurrectionSpells)
            if (member->HasSpell(spell))
                return true;
    }
    return false;
}
}  // namespace PlayerbotsPlus
