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

// Castable or buyable recipe matching `pick` with the cheapest reagents.
template <class Pick>
RecipeOption const* Cheapest(CraftSnapshot const& snap, Pick pick)
{
    RecipeOption const* best = nullptr;
    for (RecipeOption const& r : snap.recipes)
        if ((r.castable || r.buyable) && pick(r) && (!best || r.reagentCost < best->reagentCost))
            best = &r;
    return best;
}

// A buyable pick waits for the vendor reagents instead of crafting.
CraftDecision Pick(CraftState& state, RecipeOption const* r, bool forMaster, std::string const& what)
{
    return r->castable ? Result(state, CraftAction::Craft, r, forMaster, "craft for " + what)
                       : Result(state, CraftAction::Shop, r, forMaster, "buy for " + what);
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
        bool waitsForStation = false;
        for (RecipeOption const& r : snap.recipes)
        {
            if (r.spell != state.approvedSpell)
                continue;
            if (r.castable || r.buyable)
            {
                if (r.castable)
                    state.approvedSpell = state.approvedProduct = 0;
                return Pick(state, &r, true, "master");
            }
            waitsForStation = r.atFocus;  // crafted at the next forge or anvil of a capital trip
        }
        if (!waitsForStation)
        {
            state.approvedSpell = state.approvedProduct = 0;
            return Result(state, CraftAction::None, nullptr, false, "cannot craft the approved recipe now");
        }
    }

    if (RecipeOption const* r = Cheapest(snap, [](RecipeOption const& o) { return o.usefulToMaster && !o.declined; }))
    {
        state.askedSpell = r->spell;
        state.askedProduct = r->product;
        state.askedAt = now;
        return Result(state, CraftAction::Ask, r, false, "asked master");
    }

    if (RecipeOption const* r = Cheapest(snap, [](RecipeOption const& o) { return o.usefulToGroup; }))
        return Pick(state, r, false, "group");

    // Cooldown crafts are valuable: made as soon as they are ready, then offered.
    if (RecipeOption const* r = Cheapest(snap, [](RecipeOption const& o) { return o.cooldown; }))
        return Pick(state, r, snap.hasMaster, "cooldown");

    // Nobody wants the item: its dust and essences feed enchanting skill-ups.
    if (snap.disenchantItem)
    {
        CraftDecision d = Result(state, CraftAction::Disenchant, nullptr, false, "disenchant");
        d.item = snap.disenchantItem;
        return d;
    }

    if (RecipeOption const* r = Cheapest(snap, [](RecipeOption const& o) { return o.skillUp; }))
        return Pick(state, r, false, "skill");

    return Result(state, CraftAction::None, nullptr, false, "nothing to craft");
}

CraftDecision PlanStationCraft(CraftSnapshot const& snap, CraftState& state)
{
    auto here = [&](auto wanted)
    {
        RecipeOption const* best = nullptr;
        for (RecipeOption const& r : snap.recipes)
            if (r.focus && r.castable && wanted(r) && (!best || r.reagentCost < best->reagentCost))
                best = &r;
        return best;
    };

    if (state.approvedSpell)
        if (RecipeOption const* r = here([&](RecipeOption const& o) { return o.spell == state.approvedSpell; }))
        {
            state.approvedSpell = state.approvedProduct = 0;
            return Result(state, CraftAction::Craft, r, true, "craft for master");
        }
    if (RecipeOption const* r = here([](RecipeOption const& o) { return o.smelt; }))
        return Result(state, CraftAction::Craft, r, false, "craft for smelting");
    if (RecipeOption const* r = here([](RecipeOption const& o) { return o.usefulToGroup; }))
        return Result(state, CraftAction::Craft, r, false, "craft for group");
    if (RecipeOption const* r = here([](RecipeOption const& o) { return o.cooldown; }))
        return Result(state, CraftAction::Craft, r, snap.hasMaster, "craft for cooldown");
    if (RecipeOption const* r = here([](RecipeOption const& o) { return o.skillUp; }))
        return Result(state, CraftAction::Craft, r, false, "craft for skill");
    return Result(state, CraftAction::None, nullptr, false, "nothing to craft here");
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
