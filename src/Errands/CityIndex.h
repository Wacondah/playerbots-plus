/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_CITY_INDEX_H
#define PLAYERBOTS_PLUS_CITY_INDEX_H

#include "Define.h"

#include <vector>

class Creature;
class GameObject;
class Map;

// Trainers and vendors of each capital, from the creature spawns; its forges and anvils.
namespace PlayerbotsPlus
{
namespace CityIndex
{
struct Spawn
{
    uint64 spawnId = 0;
    uint32 entry = 0;
    uint32 map = 0;
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
};

bool IsCapital(uint32 zone);
// Built on the first call for that zone (world thread), from the spawns of `map` within
// SearchRadius of (x, y): only those need the costly zone lookup.
std::vector<Spawn> const& SpawnsIn(uint32 zone, uint32 map, float x, float y);
// Already built: the zone's spawns (empty otherwise).
std::vector<Spawn> const& SpawnsIn(uint32 zone);
// The spawn's creature when its grid is loaded, nullptr otherwise.
Creature* LiveCreature(Map* map, uint64 spawnId);

// A forge (focus 3) or an anvil (focus 1) of the capital.
struct Station
{
    uint64 spawnId = 0;
    uint32 map = 0;
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
    uint32 focus = 0;
    float dist = 0.f;  // the focus works within this range
};

// Same building rules as SpawnsIn, for the spell focus game objects.
std::vector<Station> const& StationsIn(uint32 zone, uint32 map, float x, float y);
std::vector<Station> const& StationsIn(uint32 zone);
GameObject* LiveGameObject(Map* map, uint64 spawnId);
}  // namespace CityIndex
}  // namespace PlayerbotsPlus

#endif
