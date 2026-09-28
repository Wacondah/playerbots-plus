# mod-playerbots-plus

Extra behaviours for [mod-playerbots](https://github.com/mod-playerbots/mod-playerbots),
as a separate AzerothCore module: nothing in mod-playerbots is patched.

## Errands

When you stand still, your altbots handle their own errands with NPCs within 20 yards of
you, then follow you again:

- turn in completed quests, accept available ones (organising a full quest log);
- repair when an item drops below 30 % durability;
- sell grey items, bound items the bot no longer uses, and tradeable items no bot of
  the group wants (crafting materials included, unless one of its own professions
  consumes them or it gathers them), up to blue quality (`MaxSellQuality`); food and
  drink no recipe makes, when bots have the `food` cheat (they never eat items;
  `SellFood`); optionally white weapons and armor;
- train at a class trainer of their class (spells and ranks they can afford).

They drop the errand as soon as you move, enter combat or mount, and they obey `stay` and
`guard`. Nothing happens inside instances unless enabled.

## Errands hunt

Optional, on top of errands: `nc +errands hunt` (or `/p nc +errands hunt` for the whole
party). When you stand still and no errand is left, one bot (a tank first) pulls a quest
mob within 45 yd (`Hunt.Radius`: wider than the errands radius, so the mob is spotted
before it aggroes) that some bot of the group still needs; the group fights it as usual.
Never elites, never a mob with another hostile within 8 yd, never while someone is low
on health or mana, never above the lowest bot level + 2.

## Errands share

Optional, on top of errands: `nc +errands share` (party: `/p nc +errands share`). When
you stand still and no errand is left, each alt gives from its bags, to another alt of
the party within 10 yd:

- tradeable gear that is an upgrade for that alt and not for itself (largest gain wins);
- materials a profession of that alt consumes, when the giver's own professions rank
  lower (a primary crafting profession beats first aid, cooking and fishing): linen goes
  to the tailor.

Bound items, quest items and equipped items never move.

You come first: a tradeable item that would be an upgrade for you (gear, or a bag larger
than your smallest one) is offered to you through a trade window ("I have [item] for
you, accept the trade"). Cancel the trade and that item is not offered again for 30 min.

## Errands bags

Optional: `nc +errands bags` (party: `/p nc +errands bags`). Out of combat, even while
you keep walking, an alt with no free bag slot gives stackable items to an alt within
10 yd: into a partial stack of the same item first (costs the receiver no slot), else into
a free slot of an alt that still keeps one free (`Bags.MinFreeSlots`). Quest items, food,
water, potions and ammo the bot uses never move, and a material never goes to an alt
whose professions rank it lower. When the whole party is full, the bot tells you once.

## Errands craft

Optional, on top of errands: `nc +errands craft` (party: `/p nc +errands craft`). When
you stand still and no errand is left, a bot crafts on purpose with the materials in its
bags: first it asks you about upgrades for you ("I can craft [item] for you. Whisper
'craft yes' or 'craft no'."), then crafts what the party can use (gear upgrades, bigger
bags, bandages, potions), then recipes that raise its skill, cheapest materials first.
An enchanter with `errands craft` also disenchants weapons and armor nobody in the party
(you included) wants, up to blue (`Craft.MaxDisenchantQuality`), before crafting for skill.
With `errands share`, other alts hand such items to it; selling keeps them for it.
For primary professions, a missing reagent that vendors sell (thread, dyes, flux, vials)
does not stop a recipe: the bot buys it at a vendor within the errands radius, for as many
crafts as its other materials allow (one for gear, at most `Craft.MaxShoppingCrafts`),
spending at most `Craft.MaxShoppingCopper` per visit and never its reserve
(`Craft.ReserveCopperPer10Levels` per 10 levels). After `craft yes` it tells you
"I need a vendor for [item]" when it must shop first.
Nothing else, never at random. `craft no` is remembered.

## Assigned professions

Part of errands. Whisper `professions mining tailoring` to an alt (one or two primary
professions, English names): within the errands radius it learns only those at their
trainers (ranks and recipes it can afford, after repairing and selling) and buys their
tools (mining pick, skinning knife, blacksmith hammer, arclight spanner, jeweler's kit,
virtuoso inking set). If it lacks an assigned profession and has no free slot, it forgets
its least-trained unassigned one, at that trainer only. The assignment is saved with the
bot. Cooking, fishing and first aid are left alone.

## Level-up and quest sharing

Whisper `role tank`, `role heal` or `role dps` (optionally a spec: `role dps combat`) to set
an alt's role: mod-playerbots picks the role from the tree with the most talent points,
so at low level a single point decides. `role` respecs into the fitting tree, sets that
role's combat strategies (tanks get `tank` and `tank assist`), adds `threat` to dps (they
stop attacking near the tank's threat) and keeps the errands and other non-combat
strategies, which mod-playerbots' own `talents spec` wipes. `role` alone shows it.

Part of `errands on`. On level-up an alt spends its talent points: in the premade spec you
picked with `talents spec <name>` (the module remembers it, mod-playerbots forgets it once
the points are spent), else in the tree it already uses; with neither, it asks you to
choose. Spells stay with its class trainer. `errands loot`: in group rolls an alt picks
Need on gear it would wear and on materials its own professions use. `errands revive`: a
dead alt releases once the fight is over if nobody alive in the group can resurrect it.
Long-cooldown crafts (transmutes, mooncloth) are made when ready and offered to you. `errands quests`: when the group is idle,
alts give each other the sharable quests they can take, never to you (no quest window).

## Errands city

Part of errands. When you stand still for a few seconds in a capital (Stormwind,
Ironforge, Darnassus, the Exodar, Orgrimmar, Undercity, Thunder Bluff, Silvermoon,
Shattrath, Dalaran), bots leave the leash to run their errands across the city, then come
back: class training (every spell and rank they can afford), training of their assigned
professions, missing profession tools, and vendor reagents for crafting. They keep going
while you walk around the city and come back at once if you leave it, take a flight or
portal, enter combat or an instance. A trip lasts at most 3 minutes (`City.Timeout`) and
is not repeated with unchanged needs for 10 minutes (`City.Cooldown`). `City.Auto = 0`
keeps them home until you whisper `errands city`.

Outside capitals, a class trainer of the bot's class within the errands radius is a
normal errand too.

## Install

Requires the Playerbot core fork (`mod-playerbots/azerothcore-wotlk`) and mod-playerbots.

```bash
cd azerothcore/modules
git clone https://github.com/Wacondah/playerbots-plus.git mod-playerbots-plus
```

The directory name must be `mod-playerbots-plus`. Rebuild the core, copy
`etc/modules/playerbots-plus.conf.dist` to `playerbots-plus.conf`, then enable the
strategy for all altbots in `playerbots.conf`:

```
AiPlayerbot.NonCombatStrategies = "+errands"
```

or per bot with a whisper: `nc +errands`.

`AiPlayerbot.NonCombatStrategies` only applies to bots that have no saved strategies.
mod-playerbots saves a bot's strategy list as soon as you change it with `nc`/`co`, and
that saved list replaces the defaults on every login. For bots you have already
configured, add it once from party chat — `/p nc +errands` — and it is saved.

## Commands (whisper a bot)

| Command | Effect |
|---|---|
| `errands on` / `errands off` | enable / disable all errands strategies at once (party chat: `/p errands on` for every bot); works on bots with nothing enabled yet |
| `nc +errands` / `nc -errands` | enable / disable errands |
| `errands` | current errand and last reason (e.g. `master moving`, `in instance`) |
| `role tank\|heal\|dps [spec]` / `role` | set / show the alt's role (talents, combat strategies, `threat` for dps) |
| `errands city` | start a capital trip now (party chat: `/p errands city`); answers `city: going (n stops)` or `city: nothing to do` |
| `nc +errands hunt` | enable hunting (party chat: `/p nc +errands hunt`) |
| `nc +errands share` | enable sharing (party chat: `/p nc +errands share`) |
| `nc +errands bags` | keep a free bag slot on every alt (party chat: `/p nc +errands bags`) |
| `professions <p1> [p2]` / `professions` / `professions clear` | assign, show, remove assigned professions |
| `professions reset` then `professions reset confirm` | forget both primary professions now (assignment kept) |
| `nc +errands craft` | enable crafting (party chat: `/p nc +errands craft`) |
| `craft yes` / `craft no` | answer a crafting offer |
| `nc +debug errands` | log each decision to chat and the `playerbots` log |

## Configuration

See `conf/playerbots-plus.conf.dist`: radius, idle delay, timeout, blacklist, repair
threshold, white items, instances.

## Compatibility

Tested with `azerothcore-wotlk@06234df3d` and `mod-playerbots@b6696bdb`. The module uses
mod-playerbots internals (listed in `docs/upstream-dependencies.md`); a weekly CI build
against mod-playerbots `master` catches breakage.

## Development

Unit tests (planner only, no core needed): `tests/run.sh` (needs cmake and GoogleTest).
In-game checklist: `docs/testing.md`.

This module was written with the help of an AI assistant (Claude) and reviewed and
tested by its maintainer.

## License

GPL-2.0-or-later.
