# Errands craft: buying vendor reagents

Status: approved design, pending review.
Builds on the craft spec. A later spec (capital errands) reuses the buying errand.

## Problem

`errands craft` only considers recipes whose reagents are all in the bags. Many recipes
also need an item only vendors sell (thread, dyes, flux, vials, salt), so a tailor with a
stack of linen never makes a bag, and skill-ups stall.

## Goal

A crafter buys the vendor reagents missing for the recipes `errands craft` would make,
at a vendor within the errands leash, within a money budget. Primary professions only:
cooking, first aid and fishing never trigger a purchase (their crafting is unchanged).

Not in scope: leaving the leash to shop (capital errands spec), limited-supply vendor
items, reputation or auction-house purchases.

## Vendor reagents

At startup `ReagentIndex` records the items at least one vendor sells without a supply
limit (`npc_vendor.maxcount = 0`, any vendor template). Only those count as buyable.

## Buyable recipes

`CraftItemAction` lists, besides castable recipes, recipes that are **buyable**:

- the recipe belongs to a primary profession (`RecipeSkill` in the profession catalog);
- every missing reagent is a vendor reagent; every other reagent is in the bags;
- the required tools (totem categories) are in the bags. A spell focus (anvil, forge) is
  not checked: it is checked when crafting.

`RecipeOption` gains `buyable` and the recipe's reagent list (entry, count per craft, held,
vendor-only flag, unit price). `CraftPlanner` treats a buyable recipe like a castable one
when choosing (approved, ask the master, group, skill-up, cheapest first), but a buyable
decision does not cast: it stores the **shopping list** in `ErrandsData` and waits.

Crafts per purchase: as many as the held non-vendor reagents allow, capped by
`Craft.MaxShoppingCrafts` (20); one craft for gear the group or the master uses.

## Shopping list and budget

`PlanShopping` (pure) turns the chosen recipe into purchases:

- quantity = crafts × count per craft − held, rounded up to the vendor's stack
  (`BuyCount`);
- budget = `min(Craft.MaxShoppingCopper, money − reserve)`, with reserve =
  `floor(level / 10) × Craft.ReserveCopperPer10Levels`; crafts are lowered until the
  purchases fit the budget, none if even one craft does not fit;
- crafts are also lowered until the purchases fit the free bag space.

The list is recomputed at most every 5 s, and dropped once the recipe becomes castable.

## Buying errand

New `ErrandKind::BuyReagents`, next to `BuyTool`: a vendor within the leash qualifies
(`Candidate.canSellReagent`) when its item list sells an item on the shopping list. It
runs at the same visit as selling, after repairs and sales, before tools. Purchases use
`BuyItemFromVendorSlot` at the vendor's price; a failed purchase (money, bags) blacklists
the vendor like any other errand. Crafting itself stays with `errands craft`: once bought,
the recipe is castable.

## Master

A buyable upgrade for the master is asked like a castable one. After `craft yes`, the bot
whispers `I need a vendor for [item]` once, buys at the next vendor, crafts and offers it.

## Configuration

| Key | Default | Meaning |
|---|---|---|
| `PlayerbotsPlus.Craft.ReserveCopperPer10Levels` | 10000 | money kept per 10 levels |
| `PlayerbotsPlus.Craft.MaxShoppingCopper` | 5000 | spending cap per vendor visit |
| `PlayerbotsPlus.Craft.MaxShoppingCrafts` | 20 | crafts bought for at most |

## Diagnostics

`errands` shows `shopping: <n>x [reagent] for [product]` or the reason nothing is bought
(`no budget`, `no vendor reagent missing`).

## Testing

- Unit: buyable recipes compete with castable ones in the usual order; secondary
  professions never buyable; quantity from held materials and stack size; budget with
  reserve and cap lowers crafts, none when one craft does not fit; bag space; the
  one-craft limit for gear.
- In game (`docs/testing.md`): a tailor with linen and no thread buys coarse thread at a
  camp vendor then crafts; a poor bot (under the reserve) buys nothing; a cook never buys
  salt; `craft yes` on a buyable bag leads to purchase, craft and offer.
