# Errands: leashed semi-autonomy for altbots

Status: approved design, pending review.
Targets `mod-playerbots/azerothcore-wotlk@06234df3d` and `mod-playerbots@b6696bdb`.

## Problem

`accept all quests` fires once, on the master's NPC interaction packet. Bots out of
interaction range, or whose quest menu differs from the master's, get nothing, and nothing
retries. New rpg handles quests well but wanders off.

## Goal

A non-combat strategy, `errands`: when the master stops, each altbot handles its own
errands with NPCs near the master, then resumes following.

v1: turn in and accept quests, repair, sell junk, diagnostics.
Later: quest objectives within the radius (`errands quests`), on the same leash and planner.
Not in scope: random bots, trainers, bank, mail, AH. No changes to mod-playerbots.

## Integration

Standalone AC module in `modules/`, next to mod-playerbots.

- Each bot uses its class context (10 classes). Their four `SharedNamedObjectContextList`
  statics are public; bots hold the `creators` map by reference, so entries added after
  startup reach every bot.
- `Registry` adds our strategy, actions, triggers and values to all 10 class contexts,
  from our own `WorldScript`, after mod-playerbots builds them
  (`PlayerbotAIConfig::Initialize`). Idempotent; re-run if the lists are rebuilt.
  Lists empty → log an error, stay disabled.
- Actions derive from `NewRpgBaseAction` (quest giver checks, accept, turn in, quest log
  organisation, movement). Selling follows `SellAction`. Every upstream symbol used is
  listed in `docs/upstream-dependencies.md`.

## Layout

```
src/
  PlayerbotsPlusLoader.cpp
  Registry.{h,cpp}
  Config.{h,cpp}
  Errands/
    ErrandPlanner.{h,cpp}     pure logic, no core dependency
    ErrandsStrategy.{h,cpp}
    ErrandsValues.{h,cpp}     per-bot state: errand, deadline, blacklist
    ErrandsTriggers.{h,cpp}
    ErrandsActions.{h,cpp}
    ErrandsCommand.{h,cpp}
tests/ErrandPlannerTest.cpp
conf/playerbots-plus.conf.dist
docs/testing.md, docs/upstream-dependencies.md
.forgejo/workflows/
```

`ErrandPlanner` takes a plain snapshot (master and bot state, candidate targets within the
radius, bot needs, per-bot errand state, config, time) and returns `None`, `Start(target,
kind)`, `Continue`, `Abandon(reason)` or `Complete`, with a reason string.

## Behaviour

`follow` stays (default action, relevance 1.0); `errands` only adds higher-relevance
triggers, so follow resumes on its own when no errand is active.

```
FOLLOW → ERRAND   master idle ≥ IdleDelay and errand available within Radius
ERRAND → FOLLOW   done, target invalid, or Timeout
ERRAND → FOLLOW   master moves, combat, or bot–master distance > Radius + 5 yd (abandon)
```

Start requires: master present on the same map, stationary for `IdleDelay`, not mounted,
not on a taxi, not in combat; bot out of combat and not resting; no `stay` or `guard`;
not in an instance or battleground unless `InInstances = 1`.

Candidates: NPCs and game objects within `Radius` of the **master**, minus blacklisted
ones and ones whose quest-state fingerprint is unchanged since the last visit.
Priority: turn in → accept (new rpg filters; `OrganizeQuestLog` if the log is full) →
repair below `RepairThreshold` → sell greys (and useless whites if `SellWhite`). Ties go
to the nearest. At a target the bot does everything it offers, then re-plans at once.

Timeout or pathing failure blacklists the target for `Blacklist` ms. Accepted quests are
never dropped when an errand is abandoned.

Diagnostics: whisper `errands` for state, target and last refusal reason;
`nc +debug errands` logs planner decisions.

## Configuration

| Key | Default | |
|---|---|---|
| `PlayerbotsPlus.Enable` | `1` | |
| `PlayerbotsPlus.Errands.Radius` | `20` | yd, from the master |
| `PlayerbotsPlus.Errands.IdleDelay` | `3000` | ms |
| `PlayerbotsPlus.Errands.Timeout` | `20000` | ms per errand |
| `PlayerbotsPlus.Errands.Blacklist` | `60000` | ms |
| `PlayerbotsPlus.Errands.RepairThreshold` | `30` | durability % |
| `PlayerbotsPlus.Errands.SellWhite` | `0` | |
| `PlayerbotsPlus.Errands.InInstances` | `0` | |

Opt-in: `AiPlayerbot.NonCombatStrategies = "+errands"`, or `nc +errands` per bot.

## Testing

- Unit (TDD, GoogleTest, standalone): `ErrandPlanner` — start conditions, priorities,
  radius from master, margin, timeout, blacklist expiry, fingerprint skip, abandon rules,
  `stay`/`guard`, instances.
- In game (`docs/testing.md`): bot a step behind in a chain, bot 15 yd away, full log,
  unreachable NPC, master leaves mid-errand, `stay`, vendor + repair in one camp,
  no master, dungeon.

## CI and distribution

Forgejo Actions: `unit` on every push; `build` (core fork + mod-playerbots `master` +
module) weekly and on tags, moved to a self-hosted runner if hosted ones can't fit it.

AGPL-3.0. English README (install, activation, commands, tested commits, limits),
`CHANGELOG.md`, tagged releases.

## Risks

- Upstream refactors break internal headers: weekly build; later propose a public
  registration hook upstream.
- Init order: register after mod-playerbots, check the lists, disable otherwise.
