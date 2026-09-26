/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "HuntQuestMobAction.h"

#include "ErrandsCommon.h"
#include "Group.h"
#include "LootMgr.h"
#include "ObjectMgr.h"
#include "Playerbots.h"
#include "PlayerbotsPlusConfig.h"

#include <algorithm>
#include <limits>

namespace PlayerbotsPlus
{
bool HuntQuestMobAction::isUseful()
{
    ErrandsData& data = AI_VALUE(ErrandsData&, "errands data");
    uint32 const now = getMSTime();
    HuntSnapshot const snap = BuildSnapshot(data, now);
    data.huntDecision = PlanHunt(snap, data.hunt, HuntSettings(), now);

    if (data.huntDecision.type == DecisionType::Start || data.huntDecision.type == DecisionType::Abandon)
        DebugErrands(botAI, "hunt: " + data.huntDecision.reason);

    return data.huntDecision.Acts();
}

bool HuntQuestMobAction::Execute(Event /*event*/)
{
    ErrandsData& data = AI_VALUE(ErrandsData&, "errands data");
    Unit* target = botAI->GetUnit(ObjectGuid(data.huntDecision.target));
    return target && Attack(target);
}

HuntSnapshot HuntQuestMobAction::BuildSnapshot(ErrandsData& data, uint32 now)
{
    HuntSnapshot snap;
    snap.errandsIdle = ErrandsIdle(data, now);

    Player* master = botAI->GetMaster();
    Group* group = bot->GetGroup();
    if (!master || !group)
        return snap;
    snap.masterPos = {master->GetPositionX(), master->GetPositionY(), master->GetPositionZ()};

    std::vector<GroupBot> roster;
    std::vector<Player*> bots;
    bool ready = true;
    uint32 minLevel = std::numeric_limits<uint32>::max();
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member)
            continue;
        if (!member->IsAlive() || member->IsInCombat() || NeedsRest(member))
            ready = false;

        PlayerbotAI* memberAI = GET_PLAYERBOT_AI(member);
        if (!memberAI)
            continue;  // the master or another real player
        bots.push_back(member);
        minLevel = std::min<uint32>(minLevel, member->GetLevel());
        roster.push_back({member->GetGUID().GetRawValue(), PlayerbotAI::IsTank(member),
                          memberAI->HasStrategy("errands hunt", BotState::BOT_STATE_NON_COMBAT)});
    }

    snap.isPuller = ElectPuller(roster) == bot->GetGUID().GetRawValue();
    snap.groupReady = ready;
    snap.minGroupLevel = bots.empty() ? 0 : minLevel;
    if (!snap.errandsIdle || !snap.isPuller || !snap.groupReady)
        return snap;  // no need to scan

    float const radius = Config().huntRadius;
    GuidVector const targets = AI_VALUE(GuidVector, "possible targets");
    for (ObjectGuid const& guid : targets)
    {
        Unit* unit = botAI->GetUnit(guid);
        Creature* creature = unit ? unit->ToCreature() : nullptr;
        if (!creature || !unit->IsAlive() || master->GetDistance(unit) > radius)
            continue;
        if (!bot->IsValidAttackTarget(unit) || !bot->IsWithinLOSInMap(unit))
            continue;

        Mob m;
        m.id = guid.GetRawValue();
        m.pos = {unit->GetPositionX(), unit->GetPositionY(), unit->GetPositionZ()};
        m.level = unit->GetLevel();
        m.elite = creature->GetCreatureTemplate()->rank > CREATURE_ELITE_NORMAL;
        m.inCombat = unit->IsInCombat();
        m.tappedByOther = creature->hasLootRecipient() && !creature->isTappedBy(bot);
        m.needed = std::any_of(bots.begin(), bots.end(), [unit](Player* p) { return NeededBy(p, unit); });
        m.hostilesNearby = HostilesNear(unit, targets);
        snap.mobs.push_back(m);
    }
    return snap;
}

// Same rules as mod-playerbots' GrindTargetValue::needForQuest (private upstream),
// evaluated for any group bot instead of only this one.
bool HuntQuestMobAction::NeededBy(Player* player, Unit* unit)
{
    for (auto const& [questId, status] : player->getQuestStatusMap())
    {
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest || status.Status != QUEST_STATUS_INCOMPLETE ||
            quest->GetQuestLevel() > int32(player->GetLevel()) + 5)
            continue;
        for (uint8 j = 0; j < QUEST_OBJECTIVES_COUNT; ++j)
        {
            int32 const entry = quest->RequiredNpcOrGo[j];
            uint32 const required = quest->RequiredNpcOrGoCount[j];
            if (entry > 0 && required && status.CreatureOrGOCount[j] < required && unit->GetEntry() == uint32(entry))
                return true;
        }
    }

    CreatureTemplate const* data = sObjectMgr->GetCreatureTemplate(unit->GetEntry());
    return data && data->lootid && LootTemplates_Creature.HaveQuestLootForPlayer(data->lootid, player);
}

uint32 HuntQuestMobAction::HostilesNear(Unit* unit, GuidVector const& targets)
{
    float const packRadius = Config().huntPackRadius;
    uint32 count = 0;
    for (ObjectGuid const& guid : targets)
    {
        if (guid == unit->GetGUID())
            continue;
        Unit* other = botAI->GetUnit(guid);
        if (other && other->IsAlive() && bot->IsHostileTo(other) && unit->GetDistance(other) <= packRadius)
            ++count;
    }
    return count;
}
}  // namespace PlayerbotsPlus
