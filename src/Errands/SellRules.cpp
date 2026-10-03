/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "SellRules.h"

namespace PlayerbotsPlus
{
bool SellableFood(FoodItem const& food, bool foodCheat, bool enabled, uint32_t maxQuality)
{
    return enabled && foodCheat && food.isFood && !food.crafted && !food.quest && food.hasSellPrice &&
           food.quality <= maxQuality;
}

uint32_t GatherFeeds(bool mining, bool herbalism, bool skinning)
{
    uint32_t feeds = 0;
    if (mining)
        feeds |= ProfessionBit::Blacksmithing | ProfessionBit::Jewelcrafting | ProfessionBit::Engineering;
    if (herbalism)
        feeds |= ProfessionBit::Alchemy | ProfessionBit::Inscription;
    if (skinning)
        feeds |= ProfessionBit::Leatherworking;
    return feeds;
}

bool KeptForOwnSkill(uint32_t usedBy, uint32_t known, uint32_t gatherFeeds, bool crafted)
{
    return (usedBy & known) || (!crafted && (usedBy & gatherFeeds));
}
}  // namespace PlayerbotsPlus
