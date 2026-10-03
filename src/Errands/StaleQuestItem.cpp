/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "StaleQuestItem.h"

namespace PlayerbotsPlus
{
bool IsStaleQuestItem(StaleFacts const& f)
{
    return f.isQuestItem && !f.sellPrice && f.quests > 0 && f.questsRewarded >= f.quests && !f.neededByGroup &&
           !f.neededByMaster;
}
}  // namespace PlayerbotsPlus
