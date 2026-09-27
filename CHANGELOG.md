# Changelog

## 0.6.0 — 2026-09-27

- Selling: crafted materials none of the holder's professions uses (copper tube, bolts)
  are no longer kept because of an upstream cache bug; gatherers still keep raw materials.
- Craft buys missing vendor reagents (primary professions) at a vendor within the errands
  radius, within `Craft.MaxShoppingCopper` per visit and above a level-based reserve.
- Errands city: in a capital, bots leave the leash to train (class, professions), buy tools
  and craft reagents across the city, then come back (`City.Auto`, `errands city`).
- Class training at a class trainer within the errands radius.
- Share: the receiver equips gear it is given (bags included) right away.
- README: restored the "Assigned professions" and "Install" headings lost in 0.5.0.

## 0.5.0 — 2026-09-27

- Assigned professions: `professions` command; errands trains them and buys their tools.
- `errands on` / `errands off`: enable or disable all errands strategies at once.
- `errands craft` strategy: craft for the master (asked first), the party, then skill-ups.
- Share offers upgrades to the master first, through a trade window.
- Disenchanting: enchanters with `errands craft` disenchant what nobody wants (up to
  `Craft.MaxDisenchantQuality`); share routes it to them, selling keeps it.

## 0.4.0 — 2026-09-27

- Errands also sells food and drink no recipe makes, for bots with the `food` cheat
  (`SellFood`, on by default).

## 0.3.0 — 2026-09-27

- `errands bags` strategy: alts pass stackables around so each keeps a free bag slot.

## 0.2.0 — 2026-09-27

- `errands hunt` strategy: one elected bot pulls a quest mob near the idle master.
- `errands share` strategy: alts pass tradeable gear and materials to the alt who can use them.
- Errands also sells bound junk and tradeable items nobody in the group wants, up to
  `MaxSellQuality` (blue by default).

## 0.1.0 — 2026-09-26

- `errands` strategy: altbots turn in / accept quests, repair and sell junk near their
  idle master.
