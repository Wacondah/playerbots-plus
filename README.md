# mod-playerbots-plus

Extra behaviours for [mod-playerbots](https://github.com/mod-playerbots/mod-playerbots),
as a separate AzerothCore module: nothing in mod-playerbots is patched.

## Errands

When you stand still, your altbots handle their own errands with NPCs within 20 yards of
you, then follow you again:

- turn in completed quests, accept available ones (organising a full quest log);
- repair when an item drops below 30 % durability;
- sell grey items (optionally white weapons and armor).

They drop the errand as soon as you move, enter combat or mount, and they obey `stay` and
`guard`. Nothing happens inside instances unless enabled.

## Errands hunt

Optional, on top of errands: `nc +errands hunt` (or `/p nc +errands hunt` for the whole
party). When you stand still and no errand is left, one bot (a tank first) pulls a quest
mob within the radius that some bot of the group still needs; the group fights it as
usual. Never elites, never a mob with another hostile within 8 yd, never while someone
is low on health or mana, never above the lowest bot level + 2.

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
