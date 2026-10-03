/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "CityIndex.h"

#include "Log.h"
#include "Creature.h"
#include "GameObject.h"
#include "Map.h"
#include "MapMgr.h"
#include "ObjectMgr.h"

#include <algorithm>
#include <iterator>
#include <unordered_map>

namespace PlayerbotsPlus
{
namespace
{
// Stormwind, Ironforge, Darnassus, The Exodar, Orgrimmar, Undercity, Thunder Bluff,
// Silvermoon, Shattrath, Dalaran.
constexpr uint32 Capitals[] = {1519, 1537, 1657, 3557, 1637, 1497, 1638, 3487, 3703, 4395};
// Wider than any capital, around the master when the index is built.
constexpr float SearchRadius = 1500.f;

std::unordered_map<uint32, std::vector<CityIndex::Spawn>>& Cache()
{
    static std::unordered_map<uint32, std::vector<CityIndex::Spawn>> cache;
    return cache;
}

std::unordered_map<uint32, std::vector<CityIndex::Station>>& StationCache()
{
    static std::unordered_map<uint32, std::vector<CityIndex::Station>> cache;
    return cache;
}

bool Useful(CreatureTemplate const* tmpl)
{
    return tmpl && (tmpl->npcflag & (UNIT_NPC_FLAG_TRAINER | UNIT_NPC_FLAG_VENDOR));
}

// Forges and anvils, the stations of smelting and blacksmithing.
constexpr uint32 AnvilFocus = 1;
constexpr uint32 ForgeFocus = 3;
}  // namespace

std::vector<CityIndex::Station> const& CityIndex::StationsIn(uint32 zone)
{
    static std::vector<Station> const none;
    auto const it = StationCache().find(zone);
    return it == StationCache().end() ? none : it->second;
}

std::vector<CityIndex::Station> const& CityIndex::StationsIn(uint32 zone, uint32 map, float x, float y)
{
    auto [it, inserted] = StationCache().try_emplace(zone);
    if (!inserted || !IsCapital(zone))
        return it->second;
    for (auto const& [spawnId, data] : sObjectMgr->GetAllGOData())
    {
        float const dx = data.posX - x, dy = data.posY - y;
        if (data.mapid != map || dx * dx + dy * dy > SearchRadius * SearchRadius)
            continue;
        GameObjectTemplate const* tmpl = sObjectMgr->GetGameObjectTemplate(data.id);
        if (!tmpl || tmpl->type != GAMEOBJECT_TYPE_SPELL_FOCUS || tmpl->spellFocus.serverOnly ||
            (tmpl->spellFocus.focusId != AnvilFocus && tmpl->spellFocus.focusId != ForgeFocus))
            continue;
        // The DB zone columns of these spawns are 0: the map tells the zone.
        if (sMapMgr->GetZoneId(data.phaseMask, data.mapid, data.posX, data.posY, data.posZ) != zone)
            continue;
        it->second.push_back({spawnId, data.mapid, data.posX, data.posY, data.posZ, tmpl->spellFocus.focusId,
                              float(tmpl->spellFocus.dist)});
    }
    LOG_INFO("server.loading", ">> playerbots-plus: city index {}: {} forges and anvils", zone, it->second.size());
    return it->second;
}

GameObject* CityIndex::LiveGameObject(Map* map, uint64 spawnId)
{
    if (!map || !spawnId)
        return nullptr;
    auto const range = map->GetGameObjectBySpawnIdStore().equal_range(ObjectGuid::LowType(spawnId));
    for (auto it = range.first; it != range.second; ++it)
        if (it->second->IsInWorld() && it->second->isSpawned())
            return it->second;
    return nullptr;
}

bool CityIndex::IsCapital(uint32 zone)
{
    return std::find(std::begin(Capitals), std::end(Capitals), zone) != std::end(Capitals);
}

std::vector<CityIndex::Spawn> const& CityIndex::SpawnsIn(uint32 zone)
{
    static std::vector<Spawn> const none;
    auto const it = Cache().find(zone);
    return it == Cache().end() ? none : it->second;
}

std::vector<CityIndex::Spawn> const& CityIndex::SpawnsIn(uint32 zone, uint32 map, float x, float y)
{
    auto [it, inserted] = Cache().try_emplace(zone);
    if (!inserted || !IsCapital(zone))
        return it->second;
    for (auto const& [spawnId, data] : sObjectMgr->GetAllCreatureData())
    {
        float const dx = data.posX - x, dy = data.posY - y;
        if (data.mapid != map || dx * dx + dy * dy > SearchRadius * SearchRadius)
            continue;
        if (!Useful(sObjectMgr->GetCreatureTemplate(data.id)))
            continue;
        if (sMapMgr->GetZoneId(data.phaseMask, data.mapid, data.posX, data.posY, data.posZ) != zone)
            continue;
        it->second.push_back({spawnId, data.id, data.mapid, data.posX, data.posY, data.posZ});
    }
    LOG_INFO("server.loading", ">> playerbots-plus: city index {}: {} NPCs", zone, it->second.size());
    return it->second;
}

Creature* CityIndex::LiveCreature(Map* map, uint64 spawnId)
{
    if (!map || !spawnId)
        return nullptr;
    auto const range = map->GetCreatureBySpawnIdStore().equal_range(ObjectGuid::LowType(spawnId));
    for (auto it = range.first; it != range.second; ++it)
        if (it->second->IsInWorld())
            return it->second;
    return nullptr;
}
}  // namespace PlayerbotsPlus
