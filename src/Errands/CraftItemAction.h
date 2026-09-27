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

// Adapter between the core and PlanCraft: lists recipes, asks the master, casts.
class CraftItemAction : public Action
{
public:
    CraftItemAction(PlayerbotAI* botAI) : Action(botAI, "craft item") {}

    bool isUseful() override;
    bool Execute(Event event) override;

private:
    CraftSnapshot BuildSnapshot(ErrandsData& data, uint32 now);
    bool HasReagents(SpellInfo const* spell);
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
