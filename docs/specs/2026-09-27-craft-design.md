# Errands craft, and offers to the master

Status: approved design, pending review.
Builds on the share and professions specs; ships with them as v0.5.0.

## Problem

Bots never craft usefully on their own. Upstream `cast <recipe>` crafts one item (its count
is parsed but ignored), `craft [item]` is a trade-pricing service that never casts, and the
`maintenance` strategy crafts a random recipe (its priority check is inverted) and may burn
valuable materials. The master never gets items from the share either: it only targets bots.

## Goal

1. `errands craft`: when idle, a bot crafts deliberately: first what the group can use,
   then recipes that raise its skill; nothing else.
2. Offers to the master: share offers the master a tradeable upgrade first; craft asks the
   master before crafting an upgrade for them. Items reach the master only through a trade
   window the master accepts.

Not in scope: item enchants, disenchanting, consumables for the master.

## Master upgrade

`MasterGain(master, item)`: the master can use it (`CanUseItem`), then either a bag larger
than the master's smallest regular bag (gain = extra slots), or gear whose
`StatsWeightCalculator(master)` score beats the item in the same slot (gain = difference).
Consumables never count.

## Offers (shared by share and craft)

One pending offer per bot: the bot says `I have [item] for you`, sends a trade request when
within 10 yd, puts the item in the window once the master accepts the request, and upstream
trade handling accepts on the bot side once the master accepts the trade.

- Success: the item left the bot.
- Declined: request unanswered for 30 s, or window closed with the item still on the bot.
  That item entry is not offered again for 30 min.

## Share to the master

`ShareItem` gains `masterGain` and `masterDeclined`. The master comes first: a tradeable
item that is an upgrade for the master (and not for the holder) becomes an offer instead of
a transfer. `WantedByGroup` counts the master too, so selling keeps it.

## Errands craft

**Activation:** `nc +errands craft` (party `/p nc +errands craft`), after the share and
before the hunt (relevance 1.93); same idle gate. Bags checked at most every 5 s.

**Recipes:** known spells of a profession skill line that create an item; castable now
(`CanCastSpell`, reagents in bags, tools, anvil or fire nearby).

**Order** (`CraftPlanner`, pure):
1. A pending question waits for the answer (expires after 2 min).
2. An approved recipe is crafted, then offered to the master.
3. Ask the master (whisper `I can craft [item] for you. Whisper 'craft yes' or 'craft no'.`)
   for a recipe whose product is an upgrade for the master, not declined, not already in
   the master's or the group's bags.
4. Craft what the group uses: product usage `EQUIP`, `REPLACE` or `USE` for a group bot;
   gear only if no unequipped copy is already in the group's bags. Cheapest reagents first.
5. Craft for a skill-up (`SpellGivesSkillUp`), cheapest reagents first.
6. Nothing else.

**Answers:** chat commands `craft yes` / `craft no` (distinct from upstream `craft`).
`craft no` stores the product as declined, saved with the bot (`craft declined` value).

**Diagnostics:** `errands` shows `craft: <reason>` and the pending offer.

## Testing

- Unit: share master-first and decline, `WantedByGroup` with the master; craft order
  (answer wait and expiry, approved first, ask before group, group before skill-up,
  cheapest first, declined never asked, nothing uncastable).
- In game (`docs/testing.md`): a bag crafted by the tailor is offered to the master and
  lands after accepting; cancelling the trade stops offers for that item; a green
  upgrade for the master in a bot's bags is offered; linen bandages crafted for the group;
  skill-up crafting stops when recipes turn grey; `craft no` is remembered after restart.
