/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "CraftPlanner.h"

#include "ErrandPlanner.h"

#include <utility>

namespace PlayerbotsPlus
{
namespace
{
CraftDecision Result(CraftState& state, CraftAction action, RecipeOption const* r, bool forMaster, std::string reason)
{
    state.lastReason = reason;
    CraftDecision d;
    d.action = action;
    d.spell = r ? r->spell : 0;
    d.product = r ? r->product : 0;
    d.forMaster = forMaster;
    d.reason = std::move(reason);
    return d;
}

// Castable recipe matching `pick` with the cheapest reagents.
template <class Pick>
RecipeOption const* Cheapest(CraftSnapshot const& snap, Pick pick)
{
    RecipeOption const* best = nullptr;
    for (RecipeOption const& r : snap.recipes)
        if (r.castable && pick(r) && (!best || r.reagentCost < best->reagentCost))
            best = &r;
    return best;
}
}  // namespace

CraftDecision PlanCraft(CraftSnapshot const& snap, CraftState& state, CraftConfig const& cfg, uint32_t now)
{
    if (!snap.errandsIdle)
        return Result(state, CraftAction::None, nullptr, false, "errands first");

    if (state.askedSpell)
    {
        if (!Elapsed(now, state.askedAt, cfg.askTimeoutMs))
            return Result(state, CraftAction::None, nullptr, false, "waiting for answer");
        state.askedSpell = state.askedProduct = 0;  // no answer: the question may come again
    }

    if (state.approvedSpell)
    {
        uint32_t const approved = state.approvedSpell;
        state.approvedSpell = state.approvedProduct = 0;
        for (RecipeOption const& r : snap.recipes)
            if (r.spell == approved && r.castable)
                return Result(state, CraftAction::Craft, &r, true, "craft for master");
        return Result(state, CraftAction::None, nullptr, false, "cannot craft the approved recipe now");
    }

    if (RecipeOption const* r = Cheapest(snap, [](RecipeOption const& o) { return o.usefulToMaster && !o.declined; }))
    {
        state.askedSpell = r->spell;
        state.askedProduct = r->product;
        state.askedAt = now;
        return Result(state, CraftAction::Ask, r, false, "asked master");
    }

    if (RecipeOption const* r = Cheapest(snap, [](RecipeOption const& o) { return o.usefulToGroup; }))
        return Result(state, CraftAction::Craft, r, false, "craft for group");

    // Nobody wants the item: its dust and essences feed enchanting skill-ups.
    if (snap.disenchantItem)
    {
        CraftDecision d = Result(state, CraftAction::Disenchant, nullptr, false, "disenchant");
        d.item = snap.disenchantItem;
        return d;
    }

    if (RecipeOption const* r = Cheapest(snap, [](RecipeOption const& o) { return o.skillUp; }))
        return Result(state, CraftAction::Craft, r, false, "craft for skill");

    return Result(state, CraftAction::None, nullptr, false, "nothing to craft");
}

bool AnswerCraft(CraftState& state, bool yes)
{
    if (!state.askedSpell)
        return false;
    if (yes)
    {
        state.approvedSpell = state.askedSpell;
        state.approvedProduct = state.askedProduct;
    }
    state.askedSpell = state.askedProduct = 0;
    return true;
}
}  // namespace PlayerbotsPlus
