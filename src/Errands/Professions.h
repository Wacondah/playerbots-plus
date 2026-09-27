/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_PROFESSIONS_H
#define PLAYERBOTS_PLUS_PROFESSIONS_H

#include "Define.h"

#include <utility>
#include <vector>

class Creature;
class Player;

// Core side of assigned professions: trainers, forgetting, tools.
namespace PlayerbotsPlus
{
// Vendor or trainer price after the reputation discount.
uint32 Price(Player* bot, Creature* npc, uint32 cost);

// Profession taught by a tradeskill trainer, 0 if the creature is not one.
uint32 TrainerSkill(Creature* npc);

// Primary professions the player knows, as (skill, value).
std::vector<std::pair<uint32, uint32>> KnownPrimaries(Player* player);

// Something to do at this trainer: learn an assigned profession (forgetting an
// unassigned one if no slot is free), a rank or a recipe the bot can afford.
bool CanTrainAt(Player* bot, Creature* npc, std::vector<uint32> const& assigned);
void TrainAt(Player* bot, Creature* npc, std::vector<uint32> const& assigned);

// This vendor sells the tool of an assigned profession the bot lacks and can afford.
bool MissingToolAt(Player* bot, Creature* npc, std::vector<uint32> const& assigned);
void BuyToolsAt(Player* bot, Creature* npc, std::vector<uint32> const& assigned);
}  // namespace PlayerbotsPlus

#endif
