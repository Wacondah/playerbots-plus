/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_CRAFT_PLANNER_H
#define PLAYERBOTS_PLUS_CRAFT_PLANNER_H

#include <cstdint>
#include <string>
#include <vector>

// Pure decision logic for the "errands craft" strategy.
namespace PlayerbotsPlus
{
// One reagent of a recipe, as the bot holds it and as a vendor sells it.
struct ReagentNeed
{
    uint32_t item = 0;
    uint32_t perCraft = 0;
    uint32_t held = 0;
    bool vendor = false;    // sold by a vendor without a supply limit
    uint32_t lotPrice = 0;  // vendor price of one lot (ItemTemplate::BuyPrice)
    uint32_t lotSize = 1;   // items per lot (ItemTemplate::BuyCount)
    uint32_t maxStack = 1;
};

struct RecipeOption
{
    uint32_t spell = 0;
    uint32_t product = 0;         // item entry created
    bool castable = false;        // reagents, tools and focus available now
    bool usefulToGroup = false;   // a group bot equips or uses the product
    bool usefulToMaster = false;  // an upgrade for the master, not owned yet
    bool declined = false;        // the master said "craft no" to this product
    bool skillUp = false;         // raises the profession skill
    uint32_t reagentCost = 0;     // vendor value of the reagents
    bool buyable = false;         // only vendor reagents are missing (primary professions)
    bool gear = false;            // armor, weapon or bag: one craft at a time
    std::vector<ReagentNeed> reagents;
};

struct CraftSnapshot
{
    bool errandsIdle = false;
    std::vector<RecipeOption> recipes;
    uint64_t disenchantItem = 0;  // an item in the bags to disenchant (0: none)
};

struct CraftConfig
{
    uint32_t askTimeoutMs = 120000;
};

struct CraftState
{
    uint32_t askedSpell = 0;
    uint32_t askedProduct = 0;
    uint32_t askedAt = 0;
    uint32_t approvedSpell = 0;
    uint32_t approvedProduct = 0;
    std::string lastReason;
};

enum class CraftAction : uint8_t
{
    None,
    Craft,
    Ask,
    Disenchant,
    Shop  // buy the missing vendor reagents first (the errand does the buying)
};

struct CraftDecision
{
    CraftAction action = CraftAction::None;
    uint32_t spell = 0;
    uint32_t product = 0;
    uint64_t item = 0;       // Disenchant: the item
    bool forMaster = false;  // crafted on the master's "craft yes": offer it afterwards
    std::string reason;
};

// Order: pending answer, approved recipe, ask the master, group needs, disenchant,
// skill-ups. A buyable pick shops instead of crafting.
CraftDecision PlanCraft(CraftSnapshot const& snap, CraftState& state, CraftConfig const& cfg, uint32_t now);

// The master answered the pending question. False when nothing was asked.
bool AnswerCraft(CraftState& state, bool yes);
}  // namespace PlayerbotsPlus

#endif
