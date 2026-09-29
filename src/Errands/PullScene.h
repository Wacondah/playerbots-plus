/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_PULL_SCENE_H
#define PLAYERBOTS_PLUS_PULL_SCENE_H

#include "PullPlanner.h"

#include <map>
#include <string>
#include <vector>

class Player;
class Unit;

// Reads the world for the pull planner: mobs around the target, firing and hiding spots.
namespace PlayerbotsPlus
{
struct PullScene
{
    std::vector<PullMob> mobs;
    std::vector<FiringOption> firing;
    std::map<uint64_t, std::string> names;  // mob id → name, for messages
};

// Pull spell the player knows (first of PullSpellNames), empty if none; its max range in yards.
std::string KnownPullSpell(Player* player, float& range);

// Mean position of the living group members on the map, within 40 yd of the tank.
PullPoint GroupCentre(Player* tank);

PullScene BuildScene(Player* puller, Unit* target, float range, bool bodyPull);

// Hiding spots around the group centre (the centre itself included), with the path from each
// of the best firing spots.
std::vector<ReturnOption> BuildReturns(Player* puller, Unit* target, PullScene const& scene,
                                       std::vector<size_t> const& best, PullPoint centre);
}  // namespace PlayerbotsPlus

#endif
