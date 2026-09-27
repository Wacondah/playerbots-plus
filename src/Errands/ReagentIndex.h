/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_REAGENT_INDEX_H
#define PLAYERBOTS_PLUS_REAGENT_INDEX_H

#include "Define.h"

class Player;

namespace PlayerbotsPlus
{
// Which professions consume an item as a reagent (ProfessionBit mask), built once
// from SkillLineAbility and spell reagents. Replaces upstream
// RandomItemMgr::IsUsedBySkill, whose cache is keyed by item only.
namespace ReagentIndex
{
void Build();
uint32 UsedBy(uint32 itemId);
uint32 Known(Player* player);  // ProfessionBit mask of the player's professions
bool IsCrafted(uint32 itemId);  // produced by a profession recipe (e.g. cooked food)
uint32 RecipeSkill(uint32 spellId);  // profession of a recipe spell, 0 if none
bool VendorSells(uint32 itemId);      // some vendor sells it without a supply limit
}  // namespace ReagentIndex
}  // namespace PlayerbotsPlus

#endif
