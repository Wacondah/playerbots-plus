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
// Discount the bot gets from the faction of a creature template (reputation).
float TemplateDiscount(Player* bot, uint32 entry);

// Profession taught by a tradeskill trainer, 0 if the creature is not one.
uint32 TrainerSkill(Creature* npc);
uint32 TrainerSkillOf(uint32 entry);

// Primary professions the player knows, as (skill, value).
std::vector<std::pair<uint32, uint32>> KnownPrimaries(Player* player);

// Something to do at this trainer: learn an assigned profession (forgetting an
// unassigned one if no slot is free), a rank or a recipe the bot can afford.
bool CanTrainAt(Player* bot, Creature* npc, std::vector<uint32> const& assigned);
void TrainAt(Player* bot, Creature* npc, std::vector<uint32> const& assigned);
// Same, from the trainer template (a city NPC not loaded yet), at a known price discount.
bool CanTrainWith(Player* bot, uint32 entry, std::vector<uint32> const& assigned, float discount);

// Class trainer of the bot's class teaching something it can learn and afford.
bool CanTrainClassWith(Player* bot, uint32 entry, float discount);
bool CanTrainClassAt(Player* bot, Creature* npc);
void TrainClassAt(Player* bot, Creature* npc);

// This vendor sells the tool of an assigned profession the bot lacks and can afford.
bool MissingToolAt(Player* bot, Creature* npc, std::vector<uint32> const& assigned);
void BuyToolsAt(Player* bot, Creature* npc, std::vector<uint32> const& assigned);
bool SellsMissingTool(Player* bot, uint32 entry, std::vector<uint32> const& assigned, float discount);
}  // namespace PlayerbotsPlus

#endif
