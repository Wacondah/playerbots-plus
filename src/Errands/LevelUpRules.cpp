/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "LevelUpRules.h"

namespace PlayerbotsPlus
{
TalentSource PickTalentSource(uint32_t freePoints, bool hasStoredSpec, uint32_t spentPoints)
{
    if (!freePoints)
        return TalentSource::None;
    if (hasStoredSpec)
        return TalentSource::Stored;
    return spentPoints ? TalentSource::Current : TalentSource::Ask;
}

bool ShouldShareQuest(QuestShareCheck const& check)
{
    return check.sharable && check.targetIsBot && check.inRange && check.targetCanTake;
}
}  // namespace PlayerbotsPlus
