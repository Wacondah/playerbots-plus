/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_HUNT_QUEST_MOB_ACTION_H
#define PLAYERBOTS_PLUS_HUNT_QUEST_MOB_ACTION_H

#include "AttackAction.h"
#include "ErrandsValues.h"

namespace PlayerbotsPlus
{
// Mobs are rescanned at most this often (line of sight and quest loot checks are costly).
constexpr uint32 HuntScanIntervalMs = 1000;

// Adapter between the core and PlanHunt. Attack() switches the bot to its
// combat engine; the group's normal combat reactions take it from there.
class HuntQuestMobAction : public AttackAction
{
public:
    HuntQuestMobAction(PlayerbotAI* botAI) : AttackAction(botAI, "hunt quest mob") {}

    bool isUseful() override;
    bool Execute(Event event) override;

private:
    HuntSnapshot BuildSnapshot(ErrandsData& data, uint32 now);
    std::vector<Mob> ScanMobs(Player* master, std::vector<Player*> const& bots);
    static bool NeededBy(Player* player, Unit* unit);
    uint32 HostilesNear(Unit* unit, GuidVector const& targets);
};
}  // namespace PlayerbotsPlus

#endif
