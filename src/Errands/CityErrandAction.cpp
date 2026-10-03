/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "CityErrandAction.h"

#include "CraftItemAction.h"
#include "DBCStores.h"
#include "ErrandsCommon.h"
#include "GameObject.h"
#include "ObjectMgr.h"
#include "Playerbots.h"
#include "PlayerbotsPlusConfig.h"
#include "Professions.h"
#include "Shopping.h"

#include <algorithm>

namespace PlayerbotsPlus
{
namespace
{
// Near enough to the spawn to tell the NPC is not there.
constexpr float MissingDistance = 20.f;
// Stand this close to a forge or an anvil, within its range less this margin.
constexpr float StationApproach = 2.f;
constexpr float StationMargin = 2.f;
// Failed casts at one station before giving up on it.
constexpr uint32 StationMaxFailures = 3;
}  // namespace

bool CityErrandAction::isUseful()
{
    ErrandsData& data = Data();
    uint32 const now = getMSTime();
    CitySnapshot const snap = BuildCitySnapshot(data, now);
    CityConfig const cfg{Config().planner.idleDelayMs, Config().cityTimeoutMs, Config().cityCooldownMs, CityReachMs};
    bool const wasActive = data.city.Active();
    data.cityDecision = PlanCity(snap, data.city, cfg, now);

    CityStep const step = data.cityDecision.step;
    if (step == CityStep::End || step == CityStep::Abort || (!wasActive && data.city.Active()))
        DebugErrands(botAI, data.cityDecision.reason);
    return step == CityStep::GoTo || step == CityStep::Visit;
}

bool CityErrandAction::Execute(Event /*event*/)
{
    ErrandsData& data = Data();
    CityDecision const d = data.cityDecision;
    if (IsStationStop(d.stop))
        return WorkAtStation(data, d);
    Creature* creature = CityIndex::LiveCreature(bot->GetMap(), d.stop);

    if (d.step == CityStep::Visit && creature)
    {
        VisitTarget(creature);
        MarkCityVisited(data.city);
        data.cityStopsAt = 0;       // needs changed
        data.lastCraftScanAt = 0;   // bought reagents can be crafted
        DebugErrands(botAI, std::string("city: visited ") + creature->GetName());
        return true;
    }
    if (d.step != CityStep::GoTo)
        return false;

    if (creature && bot->IsWithinDistInMap(creature, MissingDistance))
        MoveWorldObjectTo(creature->GetGUID());
    else
        MoveFarTo(WorldPosition(bot->GetMapId(), d.pos.x, d.pos.y, d.pos.z));
    return true;  // keep the tick: follow must not pull the bot back
}

// At a forge or an anvil: one craft per tick, by the craft rules, until nothing is left.
bool CityErrandAction::WorkAtStation(ErrandsData& data, CityDecision const& d)
{
    GameObject* station = CityIndex::LiveGameObject(bot->GetMap(), StationSpawnId(d.stop));
    if (d.step == CityStep::GoTo)
    {
        if (station && bot->IsWithinDistInMap(station, MissingDistance))
            MoveWorldObjectTo(station->GetGUID(), StationApproach);
        else
            MoveFarTo(WorldPosition(bot->GetMapId(), d.pos.x, d.pos.y, d.pos.z));
        return true;  // keep the tick: follow must not pull the bot back
    }
    if (d.step != CityStep::Visit)
        return false;

    if (bot->IsNonMeleeSpellCast(false))
        return true;  // let the cast end
    if (bot->isMoving())
    {
        bot->StopMoving();
        return true;
    }
    if (bot->IsMounted())
    {
        bot->Dismount();
        return true;
    }

    uint32 const now = getMSTime();
    CraftItemAction craft(botAI);
    CraftDecision const decision = PlanStationCraft(craft.BuildSnapshot(data, now, true), data.craft);
    if (decision.action == CraftAction::Craft && data.city.stationFailures < StationMaxFailures)
    {
        data.craftDecision = decision;
        if (!craft.Execute(Event()))
            ++data.city.stationFailures;
        else if (decision.forMaster)
            data.craft.approvedSpell = data.craft.approvedProduct = 0;  // started: offered afterwards
        return true;
    }

    MarkCityVisited(data.city, now);
    data.cityStopsAt = 0;      // needs changed
    data.lastCraftScanAt = 0;  // the craft errand looks again (offers to the master)
    DebugErrands(botAI, std::string("city: worked at ") + (station ? station->GetName() : "station") + " (" +
                            decision.reason + ")");
    return true;
}

int32 CityErrandAction::CountStops()
{
    Player* master = botAI->GetMaster();
    if (!master || !master->IsInWorld() || !CityIndex::IsCapital(master->GetZoneId()))
        return -1;
    ErrandsData& data = Data();
    data.cityStopsAt = 0;
    RefreshStops(data, master, getMSTime());
    CitySnapshot snap;
    snap.stops = data.cityStops;
    snap.needs = data.cityNeeds;
    return int32(CountCityStops(snap));
}

CitySnapshot CityErrandAction::BuildCitySnapshot(ErrandsData& data, uint32 now)
{
    CitySnapshot snap;
    snap.autoEnabled = Config().cityAuto;
    snap.requested = data.cityRequested;
    data.cityRequested = false;
    snap.botInCombat = bot->IsInCombat();
    snap.inInstance = bot->GetMap() && bot->GetMap()->Instanceable();
    snap.botPos = {bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ()};

    Player* master = botAI->GetMaster();
    if (!master || !master->IsInWorld() || master->GetMapId() != bot->GetMapId())
        return snap;  // not in the capital: an active trip aborts
    snap.zone = master->GetZoneId();
    snap.masterInCapital = CityIndex::IsCapital(snap.zone);
    snap.masterMoving = master->isMoving();
    snap.masterInCombat = master->IsInCombat();
    snap.masterMounted = master->IsMounted();
    snap.masterOnTaxi = master->IsInFlight();
    if (!snap.masterInCapital)
        return snap;

    RefreshStops(data, master, now);
    snap.stops = data.cityStops;
    snap.needs = data.cityNeeds;

    if (IsStationStop(data.city.current))
    {
        uint64 const spawnId = StationSpawnId(data.city.current);
        GameObject* station = CityIndex::LiveGameObject(bot->GetMap(), spawnId);
        // Same test as the core's spell focus check, with a margin.
        float const range = station ? std::max(1.f, station->GetGOInfo()->spellFocus.dist - StationMargin) : 0.f;
        snap.atStop = station && bot->IsWithinDistInMap(station, range);
        for (CityIndex::Station const& s : CityIndex::StationsIn(snap.zone))
            if (s.spawnId == spawnId)
                snap.stopMissing = !station && bot->GetDistance(s.x, s.y, s.z) < MissingDistance;
    }
    else if (data.city.current)
    {
        Creature* creature = CityIndex::LiveCreature(bot->GetMap(), data.city.current);
        bool const alive = creature && creature->IsAlive();
        snap.atStop = alive && IsWithinInteractionDist(creature);
        if (CityIndex::Spawn const* spawn = SpawnOf(snap.zone, data.city.current))
            snap.stopMissing = !alive && bot->GetDistance(spawn->x, spawn->y, spawn->z) < MissingDistance;
    }
    return snap;
}

void CityErrandAction::RefreshStops(ErrandsData& data, Player* master, uint32 now)
{
    uint32 const zone = master->GetZoneId();
    if (data.cityZone == zone && data.cityStopsAt && !Elapsed(now, data.cityStopsAt, CityStopsIntervalMs))
        return;
    data.cityZone = zone;
    data.cityStopsAt = now;
    data.cityStops.clear();
    data.cityNeeds = 0;

    FactionTemplateEntry const* botFaction = bot->GetFactionTemplateEntry();
    std::vector<uint32> const& assigned = Assigned();
    std::vector<CityIndex::Spawn> const& spawns =
        CityIndex::SpawnsIn(zone, master->GetMapId(), master->GetPositionX(), master->GetPositionY());
    for (CityIndex::Spawn const& spawn : spawns)
    {
        if (spawn.map != bot->GetMapId())
            continue;
        CreatureTemplate const* tmpl = sObjectMgr->GetCreatureTemplate(spawn.entry);
        FactionTemplateEntry const* faction = tmpl ? sFactionTemplateStore.LookupEntry(tmpl->faction) : nullptr;
        if (!faction || !botFaction || faction->IsHostileTo(*botFaction))
            continue;

        float const discount = TemplateDiscount(bot, spawn.entry);
        CityStop stop;
        stop.id = spawn.spawnId;
        stop.pos = {spawn.x, spawn.y, spawn.z};
        if (CanTrainClassWith(bot, spawn.entry, discount))
            stop.covers |= CityNeed::ClassTraining;
        if (!assigned.empty() && CanTrainWith(bot, spawn.entry, assigned, discount))
            stop.covers |= CityNeed::Profession;
        if (!assigned.empty() && SellsMissingTool(bot, spawn.entry, assigned, discount))
            stop.covers |= CityNeed::Tool;
        if (SellsShoppingEntry(spawn.entry, data.shopping))
            stop.covers |= CityNeed::Reagents;
        if (!stop.covers)
            continue;
        data.cityNeeds |= stop.covers;
        data.cityStops.push_back(stop);
    }

    // Forges and anvils, for ore to smelt and recipes the craft rules would make there.
    if (!botAI->HasStrategy("errands craft", BotState::BOT_STATE_NON_COMBAT))
        return;
    CraftItemAction craft(botAI);
    uint32 const needs = StationNeeds(craft.BuildSnapshot(data, now, true), data.craft);
    if (!needs)
        return;
    for (CityIndex::Station const& s :
         CityIndex::StationsIn(zone, master->GetMapId(), master->GetPositionX(), master->GetPositionY()))
    {
        uint32 const covers = FocusNeed(s.focus) & needs;
        if (s.map != bot->GetMapId() || !covers)
            continue;
        data.cityStops.push_back({StationStopId(s.spawnId), {s.x, s.y, s.z}, covers});
        data.cityNeeds |= covers;
    }
}

CityIndex::Spawn const* CityErrandAction::SpawnOf(uint32 zone, uint64 spawnId)
{
    for (CityIndex::Spawn const& spawn : CityIndex::SpawnsIn(zone))
        if (spawn.spawnId == spawnId)
            return &spawn;
    return nullptr;
}
}  // namespace PlayerbotsPlus
