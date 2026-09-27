/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_SELL_RULES_H
#define PLAYERBOTS_PLUS_SELL_RULES_H

#include "SharePlanner.h"

#include <cstdint>

// Pure selling rules that go beyond upstream item usages.
namespace PlayerbotsPlus
{
struct FoodItem
{
    bool isFood = false;   // consumable, food & drink subclass
    bool crafted = false;  // produced by a profession recipe (cooked food is kept)
    bool quest = false;
    bool hasSellPrice = false;
    uint32_t quality = 0;
};

// Food no recipe makes is dead weight for a bot with the "food" cheat, which eats and
// drinks without consuming items. Never sold without that cheat: the bot would starve.
bool SellableFood(FoodItem const& food, bool foodCheat, bool enabled, uint32_t maxQuality);

// Crafts a gatherer supplies (ProfessionBit mask), as upstream item usage assumes:
// a miner keeps ore for smiths, jewelcrafters and engineers, and so on.
uint32_t GatherFeeds(bool mining, bool herbalism, bool skinning);

// Upstream marks a material "skill" for every profession once any bot asked
// (RandomItemMgr::IsUsedBySkill caches per item). The holder really keeps it when one
// of its professions consumes it, or when it is a raw material its gathering feeds.
bool KeptForOwnSkill(uint32_t usedBy, uint32_t known, uint32_t gatherFeeds, bool crafted);
}  // namespace PlayerbotsPlus

#endif
