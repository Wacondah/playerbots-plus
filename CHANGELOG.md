# Changelog

## 0.7.1 — 2026-10-03

- Loot, sharing, crafting and auto-equip ignore pieces the class does not wear: upstream's
  item usage lost its class check when something of the wrong type was worn (a warrior in
  leather pants rolled need on cloth). mod-playerbots' equip-upgrades action is replaced by
  a class-aware one for bots with `errands loot`.
- `share quests` (whisper, or `/p share quests`): an alt offers you, one by one through the
  game's quest window, each quest of its log you can take (upstream's `share` needs a link).

## 0.7.0 — 2026-10-03

- Group pull (`errands pull`): `pull` / skull icon makes the tank plan a pull that wakes as few
  mobs as possible (puller: the tank if it shoots, else the closest ranged dps, else the tank
  on foot; firing spot out of other mobs' aggro, hiding spot near the group, group holds);
  `pull force` / `pull cancel`.
- Mining: a miner keeps its ore (Mining is in the reagent index; ore had no user and was sold
  as junk), ore of other alts goes to the miner, smelting is a craft recipe.
- Capital trips go to a forge, then an anvil, to smelt and craft per the craft rules (time at
  a station does not count in `City.Timeout`, 2 min per focus per trip).
- Gathering detour: with the master idle, a miner (pick) or herbalist gathers nodes within
  `Gather.Radius` (50 yd) of him, safely (no hostile near the node, no free-for-all loot).
- Housekeeping: quest items no quest needs any more (all done, no vendor price, nobody in
  the group or the master needs them) are destroyed; `errands bags`.
- Gear for the master and future gear follow the class armor and weapon rule (no leather
  for a mail wearer) and the bots' upgrade margin.
- The module widens `playerbots_db_store.value` to TEXT at startup (long strategy lists
  failed to save and were erased).
- `errands levelup`: alts spend their talent points on level-up, following the spec picked
  with `talents spec <name>` (remembered), else their current tree; they ask otherwise.
- `errands quests`: alts share quests with each other (never with the master).
- Future gear: green+ gear an alt cannot wear yet only because of its level is kept when it
  beats what it wears (the best per slot; others go to share, disenchanting, selling), gets
  need rolls, and is equipped on level-up (`Gear.MaxLevelAhead`, 0: no limit).
- Client addon `PlayerbotsPlusQuests` (`client/`, `/pq`): every quest of the party with each
  member's progress, fed by the new `questlog` bot command (addon whispers, prefix `PPQ`).
- Quest items (even ones also used by a profession, like Goretusk Liver): an alt keeps what
  its quests need, `bags` never moves them, selling never sells them, and `share` hands a
  surplus to the alt missing the most for the same quest.
- `role tank|heal|dps [spec]`: talents of the fitting tree (remembered for level-ups),
  that role's combat strategies, `threat` for dps; non-combat strategies are kept
  (mod-playerbots' `talents spec` resets them all).
- `errands loot`: alts roll need on gear they would wear and on materials of their own
  professions (mod-playerbots' `LootNeedRollLevel = 1` turned every need into greed).
- `errands revive` (dead state): a dead alt releases once the group is out of combat and
  nobody alive can resurrect it; it still waits in dungeons and raids.
- Craft: long-cooldown recipes (transmutes, mooncloth...) are crafted when ready and
  offered to the master.
- Hunt scans mobs once a second and checks line of sight last (CPU).
- Buying reagents keeps the reserve even after training at the same visit.

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
