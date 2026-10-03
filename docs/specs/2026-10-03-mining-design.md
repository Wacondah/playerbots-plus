# Mining, smelting, forge trips, node detours (and saved strategies)

Status: approved by the user ("toutes, on peut passer le 4 à 50 yd"); technical choices delegated.

## Problem

Brogan (miner + blacksmith) never has ore to smelt:

- the reagent index ignores Mining (skill line 186), so ore has no user: it is sold as junk at
  every vendor errand (ore alone even starts a sell errand), never shared to a miner;
- smelting spells are not recipes for `errands craft`, and both smelting (Forge, spell focus 3)
  and anvil recipes (Anvil, focus 1) need a station nobody walks to;
- bots only go for nodes within mod-playerbots' `LootDistance` (15 yd).

Found on the way: `playerbots_db_store.value` is `VARCHAR(255)`; the non-combat strategy lists
of Ashyra (264 chars) and Fizzlewick (266) fail to save (MySQL 1406) after mod-playerbots has
deleted the old row, so they lose their strategies at the next login.

## 1. Mining in the reagent index

- `ProfessionBit::Mining = 1 << 11`, part of `Primary` (tier 2 like crafting professions).
- `{SKILL_MINING, Mining}` in the index: `Known()` reports miners; every Mining spell becomes a
  recipe (`RecipeSkill`), so smelting is a craft option.
- Selective reagent rule: only the reagents of forge smelts made entirely of uncrafted Metal &
  Stone (plain ore) get the Mining bit. Alloys (bronze, steel), the Black Forge's dark iron,
  elementium, primals and dream dust keep today's behaviour, so bars still flow from a pure
  miner to smiths, and a tin bar nobody uses is sold as before.
- Smelted bars are not added to `Crafted()`: refining, not crafting; a miner keeps them through
  its gather feeds as today.

Result: a miner keeps its ore; ore held by a non-miner goes to the group's miner; a smith
without mining does not keep ore.

## 2. Strategy column

The module widens `playerbots_db_store.value` to `TEXT` at startup (database hook
`OnAfterDatabasesLoaded`), only while it is still a `VARCHAR` of 255 or less, unless the
Playerbots database updates are disabled (warning instead). No SQL file: mod-playerbots' updater
does not scan other modules. After the first start, `/p errands on` rewrites the lost rows.

## 3. Forge and anvil during capital trips

Part of `errands city`, for bots with `errands craft`:

- the city index also lists spell-focus game objects (Forge 3, Anvil 1) of each capital, zone
  from the map (the DB zone columns are 0);
- a bot needs a station when a recipe it could craft (reagents and tools held, known, no
  cooldown) only lacks that focus, and the craft rules would pick it: raw-ore smelting always,
  else the master's approved recipe, a group upgrade, a cooldown craft or a skill-up;
- station stops come after the forge before the anvil (bars first); at the stop the bot stops,
  dismounts and crafts one recipe per tick (`PlanStationCraft`), then leaves when nothing is
  left; time spent at a station does not count toward `City.Timeout`, capped at 2 min per
  station and per focus for the whole trip (no hopping from forge to forge); 3 failed casts
  give up the stop; a recipe whose product has no bag room is neither crafted nor a station
  need, and an approved one waits (no prospecting exception: raw ore is always smelted).

## 4. Gathering detour

Part of `errands` (`gather node`, below hunt, above follow): when the master is idle (every
errands gate except the bot's own distance to the master), a bot with Mining (and a pick) or
Herbalism goes to the nearest node it can gather within `PlayerbotsPlus.Gather.Radius` (50 yd)
of the master, not claimed by another bot or by the master standing at it, without a hostile
within 10 yd of it (searched around the node, seen or not), in groups whose loot rules let bots
loot (no free-for-all); it opens it through mod-playerbots' own loot actions, after checking
mod-playerbots would accept it. 20 s timeout, or the node turning unsafe, then it is skipped
for 60 s, doubled at each new failure. Skinning is out of scope. mod-playerbots' `LootDistance` is unchanged.

## Testing

- Unit: `KeptForOwnSkill` for miners, `TierFor` with Mining; station planning
  (`FocusNeed`, `StationNeeds`, forge before anvil, multi-tick visit, timeout pause, work cap,
  id tags); `PlanStationCraft` order; `PlanGather` gates, pick, lifecycle; `CanGather`;
  `ErrandState.masterIdle` ignoring the bot's distance.
- In game: M1 Brogan keeps copper ore after a vendor; M2 ore picked by another alt goes to
  Brogan; M3 in Stormwind Brogan walks to the forge, smelts, then the anvil; M4 Brogan detours
  to a copper vein 40 yd away while you stand still; M5 after restart, Fizzlewick's and
  Ashyra's strategies survive a relog (`nc ?`).
