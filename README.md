# mod-playerbots-plus

Extra behaviours for [mod-playerbots](https://github.com/mod-playerbots/mod-playerbots),
as a separate AzerothCore module: nothing in mod-playerbots is patched.

## Errands

When you stand still, your altbots handle their own errands with NPCs within 20 yards of
you, then follow you again:

- turn in completed quests, accept available ones (organising a full quest log);
- repair when an item drops below 30 % durability;
- sell grey items, bound items the bot no longer uses, and tradeable items no bot of
  the group wants, up to blue quality (`MaxSellQuality`); food and drink no recipe makes,
  when bots have the `food` cheat (they never eat items; `SellFood`); optionally white
  weapons and armor.

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

## Errands bags

Optional: `nc +errands bags` (party: `/p nc +errands bags`). Out of combat, even while
you keep walking, an alt with no free bag slot gives stackable items to an alt within
10 yd: into a partial stack of the same item first (costs the receiver no slot), else into
a free slot of an alt that still keeps one free (`Bags.MinFreeSlots`). Quest items, food,
water, potions and ammo the bot uses never move, and a material never goes to an alt
whose professions rank it lower. When the whole party is full, the bot tells you once.

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
| `nc +errands` / `nc -errands` | enable / disable errands |
| `errands` | current errand and last reason (e.g. `master moving`, `in instance`) |
| `nc +errands hunt` | enable hunting (party chat: `/p nc +errands hunt`) |
| `nc +errands share` | enable sharing (party chat: `/p nc +errands share`) |
| `nc +errands bags` | keep a free bag slot on every alt (party chat: `/p nc +errands bags`) |
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
