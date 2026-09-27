# Errands bags: keep a free bag slot on every alt

Status: approved design, pending review.
Builds on `2026-09-26-share-design.md`.

## Problem

While farming away from vendors, one alt's bags fill up while another has room or a
half-empty stack of the same item. A full bot stops looting.

## Goal

A non-combat strategy, `errands bags`: an alt with fewer than `MinFreeSlots` free slots
hands stackable items to another alt of the group so that it gets a free slot back,
preferably merging into a partial stack that costs the receiver nothing.

Not in scope: destroying or selling items, non-stackable items, the master's bags,
profession bags.

## Behaviour

**Activation:** `nc +errands bags` (party: `/p nc +errands bags`). Independent of the
errands leash: runs whenever the bot is out of combat, master moving or not.

**Check:** each bot looks at its own bags at most every 5 s. Free slots count the
backpack and regular bags (`ITEM_SUBCLASS_CONTAINER`), not profession bags.

**Movable stacks:** stackable (`MaxStackSize > 1`), `Item::CanBeTraded()`, and not an
upstream usage the holder relies on: `QUEST`, `USE`, `KEEP`, `AMMO`.

**Receivers:** other bots of the group within 10 yd, out of combat, and whose profession
tier for the item (as in share) is not lower than the giver's: share never sends it back.

**Choice** (one move per pass):

1. Merge: a receiver whose partial stacks of that item have room for the whole stack.
   Smallest giver stack first; ties to the receiver with most room, then lowest GUID.
2. Free slot: a receiver keeping at least `MinFreeSlots` free slots after taking the
   stack. Smallest giver stack first; receiver with most free slots, then lowest GUID.

**Group full:** no move possible → the bot tells the master once
`bags full, nothing to rebalance`; it warns again only after getting back above the
threshold.

**Transfer:** the share transfer (`CanStoreItem` + move), which merges into partial stacks
by itself. Moves are logged with `debug errands` only, to keep chat quiet while farming.

## Design

- `BagPlanner.{h,cpp}` (pure, unit tested): `PlanBags(BagSnapshot const&, BagState&,
  BagConfig const&) -> BagMove` (`item`, `receiver`, `merge`, `warn`, reason).
  `BagSnapshot`: own free slots, own stacks (`id`, `entry`, `count`, `maxStack`,
  `movable`, `tier`), mates (`guid`, `freeSlots`, per entry `room` and `tier`).
- `GroupItems`: `FreeSlots(Player*)` and `GiveItemTo(Player* giver, Item*, Player*
  receiver)` (the share transfer, now shared).
- `RebalanceBagsAction` (`"rebalance bags"`), trigger `"bags tick"` (out of combat, in a
  group), strategy `ErrandsBagsStrategy` (`"errands bags"`), relevance 2.1: the move is
  instant, so it may run before an errand.
- `BagState` in `"errands data"`; `errands` whisper shows `bags: <reason>`.

## Configuration

| Key | Default | |
|---|---|---|
| `PlayerbotsPlus.Bags.MinFreeSlots` | `1` | free slots each alt tries to keep |

## Testing

- Unit: enough space → nothing; merge preferred over slot; smallest stack first; merge
  needs room for the whole stack; slot move keeps the receiver above the threshold;
  lower-tier receiver refused; non-movable skipped; warn once then silent, re-armed after
  recovery.
- In game (`docs/testing.md`): full bot with linen, another bot with a partial linen stack
  → merge; full bot, other bot with room → a stack moves; mage keeps its water; quest
  items never move; everyone full → one warning.
