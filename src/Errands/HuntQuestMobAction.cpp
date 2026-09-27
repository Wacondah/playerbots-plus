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
#include <unordered_map>

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
    {
        data.huntScannedAt = 0;  // after a fight or an errand, scan afresh
        return snap;
    }

    if (!data.huntScannedAt || Elapsed(now, data.huntScannedAt, HuntScanIntervalMs))
    {
        data.huntMobs = ScanMobs(master, bots);
        data.huntScannedAt = now;
    }
    snap.mobs = data.huntMobs;
    return snap;
}

// Only mobs PlanHunt could pick: the cheap disqualifiers run first, line of sight last.
std::vector<Mob> HuntQuestMobAction::ScanMobs(Player* master, std::vector<Player*> const& bots)
{
    std::vector<Mob> mobs;
    float const radius = Config().huntRadius;
    GuidVector const targets = AI_VALUE(GuidVector, "possible targets");
    std::unordered_map<uint32, bool> neededByEntry;  // a camp holds many mobs of one kind
    for (ObjectGuid const& guid : targets)
    {
        Unit* unit = botAI->GetUnit(guid);
        Creature* creature = unit ? unit->ToCreature() : nullptr;
        if (!creature || !unit->IsAlive() || master->GetDistance(unit) > radius || unit->IsInCombat() ||
            creature->GetCreatureTemplate()->rank > CREATURE_ELITE_NORMAL ||
            (creature->hasLootRecipient() && !creature->isTappedBy(bot)) || !bot->IsValidAttackTarget(unit))
            continue;

        auto [it, fresh] = neededByEntry.try_emplace(unit->GetEntry(), false);
        if (fresh)
            it->second = std::any_of(bots.begin(), bots.end(), [unit](Player* p) { return NeededBy(p, unit); });
        if (!it->second || !bot->IsWithinLOSInMap(unit))
            continue;

        Mob m;
        m.id = guid.GetRawValue();
        m.pos = {unit->GetPositionX(), unit->GetPositionY(), unit->GetPositionZ()};
        m.level = unit->GetLevel();
        m.needed = true;
        m.hostilesNearby = HostilesNear(unit, targets);
        mobs.push_back(m);
    }
    return mobs;
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
