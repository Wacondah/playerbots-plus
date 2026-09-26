# Errands selling: bound junk and items nobody in the group wants

Status: approved design, pending review.
Builds on `2026-09-26-errands-design.md` and `2026-09-26-share-design.md`; ships in v0.2.0.

## Problem

Errands sells greys only (and, optionally, white weapons and armor). Everything else the
bot cannot use piles up in its bags:

- bound items it no longer uses, typically replaced quest rewards (upstream usage
  `ITEM_USAGE_VENDOR`): nobody can ever take them;
- tradeable items it does not use (upstream `ITEM_USAGE_AH`): some may help another alt,
  the rest are dead weight.

## Goal

At a vendor within the errands radius, also sell:

1. **Bound junk**: usage `VENDOR` for the bot.
2. **Unwanted tradeables**: usage `AH` for the bot, and no bot of the group wants it
   by the `errands share` rules (not an upgrade for anyone, not a material any group
   profession ranks higher than the holder's).

Both capped by `MaxSellQuality`. Greys are always sold, as today.

Not in scope: selling to free bag space regardless of usage, auction house, mailing items
to the master.

## Behaviour

- An item counts as junk (drives both the `Sell` errand and the visit) when, in the bot's
  bags, not equipped, with a sell price:
  - grey; or
  - `SellWhite` rule (unchanged); or
  - quality ≤ `MaxSellQuality` and usage `VENDOR`; or
  - quality ≤ `MaxSellQuality`, usage `AH`, `Item::CanBeTraded()`, and not wanted by the
    group.
- "Wanted by the group" looks at every bot of the group on the map, at any distance
  (unlike sharing, which needs 10 yd): an item a far alt would take is kept for a later
  share.
- Upstream usages `QUEST`, `SKILL`, `USE`, `KEEP`, `AMMO`, `EQUIP`, `REPLACE`,
  `DISENCHANT`, `GUILD_TASK` are never sold.
- At the vendor, the bot keeps selling greys (and whites) through mod-playerbots' `sell`
  action, then sells each other junk item one by one through `SellAction::Sell(Item*)`.
  Upstream `sell vendor` is not used: it sells every `VENDOR`/`AH` item regardless of
  quality and of the group.
- Unchanged: one visit per vendor, then the vendor rests for `Blacklist` ms.

## Design

- `SharePlanner`: new pure `bool WantedByGroup(ShareItem const& item)`: some receiver has
  `Equip`/`Replace` while the holder has neither, or some receiver's tier is higher than
  the item's. No blacklist, no distance. Same rules as `PlanShare`, so selling and sharing
  never disagree.
- `ShareItemAction`: the per-item description (usage, tier, gain, held) moves to a free
  function `DescribeForGroup(PlayerbotAI*, Item*, std::vector<Player*> const&)` in
  `GroupItems.{h,cpp}`, used by sharing (receivers within 10 yd) and by selling (all
  group bots).
- `RunErrandAction::IsJunk` gains the two new rules; `VisitTarget` sells the new junk
  item by item.

## Configuration

| Key | Default | |
|---|---|---|
| `PlayerbotsPlus.Errands.MaxSellQuality` | `3` | highest quality sold: 0 grey, 1 white, 2 green, 3 blue, 4 epic |

## Testing

- Unit: `WantedByGroup` for each rule (upgrade for another, holder also wants it, higher
  tier, equal tier, quest).
- In game (`docs/testing.md`): a replaced soulbound quest reward is sold; a BoE green
  usable by another alt is not sold (and gets shared); a BoE green nobody uses is sold;
  with `MaxSellQuality = 2` a useless blue is kept; linen is kept when a tailor is in the
  group and sold otherwise; an equipped item is never sold.

## Risks

- A player who wanted to keep an unwanted blue for the auction house loses it: lower
  `MaxSellQuality`.
- Upstream `AH`/`VENDOR` classification changes: listed in `upstream-dependencies.md`.
