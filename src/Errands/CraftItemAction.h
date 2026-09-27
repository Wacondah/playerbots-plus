/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_CRAFT_ITEM_ACTION_H
#define PLAYERBOTS_PLUS_CRAFT_ITEM_ACTION_H

#include "Action.h"
#include "ErrandsValues.h"

class SpellInfo;

namespace PlayerbotsPlus
{
// Recipes are re-evaluated at most this often per bot.
constexpr uint32 CraftIntervalMs = 5000;
// The Disenchant spell.
constexpr uint32 DisenchantSpell = 13262;

// Adapter between the core and PlanCraft: lists recipes, asks the master, casts.
class CraftItemAction : public Action
{
public:
    CraftItemAction(PlayerbotAI* botAI) : Action(botAI, "craft item") {}

    bool isUseful() override;
    bool Execute(Event event) override;

private:
    CraftSnapshot BuildSnapshot(ErrandsData& data, uint32 now);
    // Fills `needs` from the spell's reagents; true when all are in the bags.
    bool Reagents(SpellInfo const* spell, std::vector<ReagentNeed>& needs);
    bool HasTools(SpellInfo const* spell);
    void UpdateShopping(ErrandsData& data, CraftSnapshot const& snap);
};

// "craft yes" / "craft no": the master's answer to the pending question.
class CraftAnswerAction : public Action
{
public:
    CraftAnswerAction(PlayerbotAI* botAI, bool yes) : Action(botAI, yes ? "craft yes" : "craft no"), yes(yes) {}

    bool Execute(Event event) override;

private:
    bool yes;
};
}  // namespace PlayerbotsPlus

#endif
