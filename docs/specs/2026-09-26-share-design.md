# Errands share: altbots pass items to the alt who can use them

Status: approved design, pending review.
Builds on `2026-09-26-errands-design.md`; ships with `errands hunt` as v0.2.0.

## Problem

An altbot keeps loot that is useless to it but would be an upgrade for another alt, or
a crafting material another alt's profession uses. Today the player has to trade by
hand, or the item ends up sold.

## Goal

A non-combat strategy, `errands share`: when the master stands still and no errand is
left, each altbot hands over, from its bags, items that another alt of the group can use
and it cannot.

Not in scope: bound items (no trade a player could not do), equipped items, the master's
or any real player's inventory, the bank, mail.

## Behaviour

**Activation:** `nc +errands share` (party: `/p nc +errands share`), on top of `errands`.

**When:** same gate as hunt: this tick's errands decision is `Idle / nothing to do`
(implies the leash holds). Relevance between `run errand` and `hunt quest mob`: sharing
costs nothing, so it goes before hunting.

**What may move:** items in the giver's bags that the core reports tradeable
(`Item::CanBeTraded()`: excludes soulbound, conjured, quest-bound items), excluding
quest items (`ITEM_USAGE_QUEST`).

**Receivers:** other bots of the group (have a `PlayerbotAI`), alive, within trade
distance (10 yd) of the giver. Never the master or another real player.

**Rules** (usages are each bot's own `"item usage"` value):

- Equipment: give if usage is `EQUIP` or `REPLACE` for a receiver and neither for the
  giver. Receiver: largest gain, gain = `StatsWeightCalculator` score of the item minus
  the score of the receiver's current item in that slot (0 if empty).
- Materials: give if usage is `SKILL` for a receiver and not for the giver. Receiver:
  the one already holding the most of that item (consolidates stacks); ties to the lowest
  GUID.
- No ping-pong by construction: the receiver finds the item useful, so the
  "useless to me" rule stops it from giving it back.

**Pace:** one item per bot per tick (the errands tick). The receiver's periodic
`equip upgrades` equips gear; nothing extra is needed.

**Transfer:** same mechanism as mod-playerbots' `GiveItemAction` (move out of the giver's
inventory, store in the receiver's). If the receiver cannot store it, skip that pair for
`Blacklist` ms.

**Announce:** the giver tells the master `Gave [item] to <name>`; `debug errands` logs
each decision.

## Design

- `SharePlanner.{h,cpp}` (pure, unit tested): `PlanShare(ShareSnapshot const&, ShareState&,
  ShareConfig const&, uint32_t now) -> ShareDecision` (`item`, `receiver`, reason).
  `ShareSnapshot`: `errandsIdle`, giver's items, each with `giverUsage` and per receiver
  `{guid, usage, gain, held}`. Usage is a small enum mirroring the few upstream values
  used (`Equip`, `Replace`, `Skill`, `Quest`, `Other`).
- `ShareItemAction` (`"share item"`): builds the snapshot, runs `PlanShare` in
  `isUseful()`, transfers in `Execute()`.
- `ErrandsShareStrategy` (`"errands share"`, non-combat): trigger `"errands tick"` →
  `"share item"`, relevance 1.95.
- `ShareState` (per-pair blacklist) lives in `"errands data"`.
- Registry: strategy and action in the 10 class contexts.

## Testing

- Unit: each rule (equip vs replace vs none, skill, quest excluded, giver also wants it),
  largest gain wins, stack consolidation and GUID tie-break, pair blacklist and expiry,
  gate.
- In game (`docs/testing.md`): a BoE green usable by another class moves once and gets
  equipped; a soulbound item never moves; herbs go to the herbalist-alchemist; linen goes
  to the tailor rather than to every first-aid bot (consolidation); receiver with full
  bags is skipped.

## Risks

- `"item usage"` of another bot is read from that bot's context: fine while all bots of a
  group share the map thread (same group, same map).
- First aid and cooking make many bots `SKILL` for cloth or meat: consolidation keeps
  items from spreading, but the "most held" alt may not be the one the player prefers.
