/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_ERRANDS_COMMON_H
#define PLAYERBOTS_PLUS_ERRANDS_COMMON_H

#include "Log.h"
#include "Playerbots.h"

namespace PlayerbotsPlus
{
// Below mod-playerbots' rest thresholds: let eat/drink/rest act first.
inline bool NeedsRest(Player* player)
{
    return player->GetHealthPct() < sPlayerbotAIConfig.mediumHealth ||
           (player->getPowerType() == POWER_MANA && player->GetPowerPct(POWER_MANA) < sPlayerbotAIConfig.mediumMana);
}

// Logged and told to the master only while "debug errands" is set.
inline void DebugErrands(PlayerbotAI* botAI, std::string const& text)
{
    if (!botAI->HasStrategy("debug errands", BotState::BOT_STATE_NON_COMBAT))
        return;
    LOG_INFO("playerbots", "[errands] {}: {}", botAI->GetBot()->GetName(), text);
    botAI->TellMasterNoFacing("[errands] " + text);
}
}  // namespace PlayerbotsPlus

#endif
