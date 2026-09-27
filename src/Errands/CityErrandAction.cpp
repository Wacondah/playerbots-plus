/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "CityErrandAction.h"

#include "DBCStores.h"
#include "ErrandsCommon.h"
#include "ObjectMgr.h"
#include "Playerbots.h"
#include "PlayerbotsPlusConfig.h"
#include "Professions.h"
#include "Shopping.h"

namespace PlayerbotsPlus
{
namespace
{
// Near enough to the spawn to tell the NPC is not there.
constexpr float MissingDistance = 20.f;
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

int32 CityErrandAction::CountStops()
{
    Player* master = botAI->GetMaster();
    if (!master || !master->IsInWorld() || !CityIndex::IsCapital(master->GetZoneId()))
        return -1;
    ErrandsData& data = Data();
    data.cityStopsAt = 0;
    RefreshStops(data, master, getMSTime());
    int32 count = 0;
    for (CityStop const& s : data.cityStops)
        if (s.covers & data.cityNeeds)
            ++count;
    return count;
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

    if (data.city.current)
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
}

CityIndex::Spawn const* CityErrandAction::SpawnOf(uint32 zone, uint64 spawnId)
{
    for (CityIndex::Spawn const& spawn : CityIndex::SpawnsIn(zone))
        if (spawn.spawnId == spawnId)
            return &spawn;
    return nullptr;
}
}  // namespace PlayerbotsPlus
