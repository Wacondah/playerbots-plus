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
}  // namespace PlayerbotsPlus
