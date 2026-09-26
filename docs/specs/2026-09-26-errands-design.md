# Errands: leashed semi-autonomy for altbots

Date: 2026-09-26
Status: design approved, pending spec review
Module: `mod-playerbots-plus` (this repository)
Target: AzerothCore Playerbot fork (`mod-playerbots/azerothcore-wotlk`) + `mod-playerbots` `master`
(designed against `azerothcore-wotlk@06234df3d`, `mod-playerbots@b6696bdb`)

## 1. Problem

Altbots (bots controlled by a real player) handle quests poorly:

- `accept all quests` only fires once, on the **master's** interaction packet
  (`CMSG_GOSSIP_HELLO`, `CMSG_QUESTGIVER_HELLO`, `CMSG_GAMEOBJ_USE`). A bot that is out of
  interaction range at that moment, or whose own quest menu differs from the master's
  (desynced quest chain), silently gets nothing. There is no retry.
- `talk to quest giver` and `accept all quests` are queued on the same trigger with the
  same relevance, so their ordering is not guaranteed.
- Result: some bots accept and others don't, bots fall behind on quest chains, and the
  player has to reopen dialogs or type `accept *` repeatedly.

The "new rpg" strategy already knows how to find quest givers, accept, turn in and
organise the quest log, but it picks its own destinations and wanders off. It is not
usable for bots that should stay with their player.

## 2. Goal

A non-combat strategy, `errands`, giving altbots **semi-autonomy within a radius around
their master**: when the master stops, each bot handles its own errands with nearby NPCs
(turn in, accept quests, repair, sell junk), then returns to following.

### In scope (v1)

- Turning in completed quests and accepting available quests from NPCs / game objects
  within the leash radius.
- Repairing when durability is low and a repairer is within the radius.
- Selling grey items (optionally useless white items) to a vendor within the radius.
- Diagnostics: a whisper command reporting a bot's errand state and last refusal reason.

### Out of scope (v1), designed for later

- Going to kill / collect quest objectives within the radius (future `errands quests`
  strategy, reusing the same leash and planner).
- Class trainers, bank, mail, auction house.
- Random bots. The strategy is meant for bots with a master; it is inert without one.

### Non-goals

- No patching of `mod-playerbots` files. No fork.
- The module never enables itself; the server owner opts in.

## 3. Integration approach

Standalone AzerothCore module, cloned next to `mod-playerbots` under `modules/`, compiled
with the core. It registers its own strategy, triggers, actions and values into
mod-playerbots' shared object registries.

### Registration facts (verified against the target commits)

- Each bot's AI context is its **class** context (`PriestAiObjectContext`, ...,
  `DKAiObjectContext`, 10 classes). Each class owns four static
  `SharedNamedObjectContextList<T>` (strategy, action, trigger, value), declared
  **public**. The base `AiObjectContext` lists are private and not needed.
- Per-bot `NamedObjectContextList` holds the shared `creators` map **by reference**, so
  creators added after startup are visible to every bot, including already-logged-in ones.
- The shared lists are built in `PlayerbotAIConfig::Initialize()`
  (`AiObjectContext::BuildAllSharedContexts()`), called from mod-playerbots'
  `WorldScript`.

### Registration design

- `Registry` adds one `NamedObjectContext<T>` per type (strategy/action/trigger/value)
  to all 10 class lists through `SharedNamedObjectContextList::Add`.
- It runs from the module's own `WorldScript` hook, after mod-playerbots has built its
  lists. Registration is idempotent (the `creators` map is keyed by name) and is re-run
  if the playerbots config is reloaded and the lists are rebuilt.
- If mod-playerbots is absent or its lists are empty at registration time, the module
  logs one error and stays disabled.
- The module owns the lifetime of nothing it registers beyond what `Add` takes
  (the shared list deletes its contexts on shutdown).

### Reuse

Actions derive from `NewRpgBaseAction` to reuse `CanInteractWithQuestGiver`,
`HasQuestToAcceptOrReward`, `IsQuestWorthDoing`, `IsQuestCapableDoing`, `AcceptQuest`,
`TurnInQuest`, `OrganizeQuestLog`, `MoveWorldObjectTo`, `IsWithinInteractionDist`.
Selling reuses the logic of `SellAction` (grey items); repairing uses the core vendor
repair API. Anything reused from mod-playerbots internals is listed in
`docs/upstream-dependencies.md` so breakage from upstream refactors is quick to locate.

## 4. Architecture

```
src/
  PlayerbotsPlusLoader.cpp     module entry (Addmod_playerbots_plusScripts)
  Registry.{h,cpp}             injection into the 10 class contexts, idempotent
  Config.{h,cpp}               reads playerbots-plus.conf
  Errands/
    ErrandPlanner.{h,cpp}      pure decision logic, no core dependency (unit tested)
    ErrandsStrategy.{h,cpp}    "errands" non-combat strategy
    ErrandsValues.{h,cpp}      per-bot errand state (current target, deadline, blacklist)
    ErrandsTriggers.{h,cpp}    "errands available", "errand in progress", "leash broken"
    ErrandsActions.{h,cpp}     turn in / accept / repair / sell / abandon errand
    ErrandsCommand.{h,cpp}     "errands" whisper command, "debug errands"
tests/
  ErrandPlannerTest.cpp        GoogleTest, standalone
conf/playerbots-plus.conf.dist
docs/
  specs/                       this document
  testing.md                   in-game test checklist
  upstream-dependencies.md
.forgejo/workflows/            CI
```

### ErrandPlanner (the core of the design)

A pure component. Input is a plain snapshot built by the adapter layer:

- master: position, moving/mounted/flying/in-combat flags, time since last movement;
- bot: position, in-combat, health/mana ratios, active `stay`/`guard`, map is instance;
- candidate targets within radius of the master: id, position, kind flags
  (`hasQuestToReward`, `hasQuestToAccept`, `canRepair`, `isVendor`);
- bot needs: lowest item durability %, count of sellable items;
- per-bot state: current errand (target, kind, deadline), blacklist (target → expiry),
  set of targets already handled with a quest-state fingerprint;
- config values and current time.

Output: one decision: `None`, `Start(target, kind)`, `Continue`, `Abandon(reason)`,
`Complete`, plus a human-readable reason for diagnostics.

Because it has no dependency on AzerothCore types, it compiles and runs in unit tests in
seconds, and it is the piece a future `errands quests` strategy extends.

## 5. Behaviour

### Interplay with follow

Altbots keep their `follow` strategy (default action, relevance 1.0). `errands` only adds
triggers with higher relevance. When no errand is active, nothing fires and follow drives
the bot as today. No strategy is disabled or toggled.

### States

```
FOLLOW --(master idle >= IdleDelay, out of combat, not in instance,
          errand available within Radius)--> ERRAND(target, kind)
ERRAND --(done | target invalid | Timeout)--> FOLLOW
ERRAND --(master moves | combat | bot-master distance > Radius + Margin)--> FOLLOW (abandon now)
```

`Margin` is fixed at 5 yd to avoid oscillation at the boundary.

### Start conditions (`errands available`)

All of:

- the bot has a master and the master is in the world, on the same map;
- master stationary for `IdleDelay`, not mounted, not on a taxi, not in combat;
- bot not in combat, above the rest thresholds (otherwise existing rest/eat/drink wins);
- no `stay` or `guard` strategy active on the bot: explicit player orders always win;
- not in a dungeon, raid or battleground, unless `InInstances = 1`.

### Picking an errand

Candidates are NPCs and game objects within `Radius` **of the master**, excluding
blacklisted ones and ones already handled with an unchanged quest-state fingerprint.
Priority, first match wins:

1. **Turn in** a completed quest.
2. **Accept** available quests (same filter as new rpg: `CanTakeQuest`, `CanAddQuest`,
   `IsQuestWorthDoing`, `IsQuestCapableDoing`). If the log is full, `OrganizeQuestLog`
   runs first.
3. **Repair** if any equipped item is below `RepairThreshold` % durability.
4. **Sell** grey items (and useless whites if `SellWhite = 1`).

Among targets offering the chosen kind, the nearest to the bot wins. On arrival the bot
does everything that target offers (turn in, accept, repair, sell) in one visit, then the
planner re-evaluates immediately so the bot can chain targets without returning first.

### Anti-stuck

- Each errand has `Timeout`. On timeout, or if pathing to the target fails, the target is
  blacklisted for that bot for `Blacklist` ms.
- A target is not revisited while the bot's quest-state fingerprint for it is unchanged.
- Quests already accepted are never abandoned when an errand is cut short.

### Diagnostics

- Whisper `errands` to a bot: current state, target, time left, last refusal reason
  (e.g. "master moving", "quest log full", "target blacklisted", "in instance").
- `nc +debug errands`: logs each planner decision to the `playerbots` log channel.

## 6. Configuration

`conf/playerbots-plus.conf.dist`, copied to `playerbots-plus.conf` like any AC module:

| Key | Default | Meaning |
|---|---|---|
| `PlayerbotsPlus.Enable` | `1` | master switch for the module |
| `PlayerbotsPlus.Errands.Radius` | `20` | yards, measured from the master |
| `PlayerbotsPlus.Errands.IdleDelay` | `3000` | ms the master must stay still |
| `PlayerbotsPlus.Errands.Timeout` | `20000` | ms per errand |
| `PlayerbotsPlus.Errands.Blacklist` | `60000` | ms a failed target is ignored |
| `PlayerbotsPlus.Errands.RepairThreshold` | `30` | durability % |
| `PlayerbotsPlus.Errands.SellWhite` | `0` | also sell useless white items |
| `PlayerbotsPlus.Errands.InInstances` | `0` | allow errands inside instances |

Activation is left to the server owner, in `playerbots.conf`:
`AiPlayerbot.NonCombatStrategies = "+errands"` (all altbots), or `nc +errands` per bot.

Distances are in game yards (1 yd = 0.914 m), the unit used by the core.

## 7. Testing

1. **Unit (TDD)**: `ErrandPlanner` with GoogleTest, built standalone without the core.
   Covers: start conditions, priority order, radius measured from master, margin,
   timeout and blacklist expiry, fingerprint skip, abandon on master movement/combat,
   `stay`/`guard` precedence, instance rule.
2. **In-game checklist** (`docs/testing.md`), run on a real server:
   quest chain with a bot one step behind; bot 15 yd away when the master talks;
   full quest log; unreachable NPC (blacklist then recovery); master leaves mid-errand;
   `stay` order; vendor + repair in the same camp; no master (inert); dungeon (inert).
3. **Build**: full compile of the core + mod-playerbots + this module.

## 8. CI (Codeberg, Forgejo Actions)

- `unit`: on every push, builds and runs the planner tests.
- `build`: weekly and on tags, compiles the Playerbot core fork + `mod-playerbots`
  `master` + this module, to detect upstream header changes early. If Codeberg's hosted
  runners cannot fit a full core build, this job moves to a self-hosted runner and the
  README says so.

## 9. Distribution

- License: AGPL-3.0 (AzerothCore's license, compatible with mod-playerbots' GPL-2.0+).
- README in English: install (clone into `modules/`, rebuild, copy conf), activation,
  commands, tested upstream commits, known limitations.
- `CHANGELOG.md`, tagged releases.

## 10. Risks

| Risk | Mitigation |
|---|---|
| Upstream refactors internal headers / registries | weekly CI build, `upstream-dependencies.md`; propose a tiny public registration hook upstream later |
| Registration order vs mod-playerbots init | register from our own hook after theirs; verify lists are non-empty, else disable with a log |
| Config reload rebuilds shared lists | re-register on reload (idempotent) |
| Bots blocking each other at one NPC | acceptable in v1; the interaction takes seconds |
| Behaviour clashing with other movement strategies | `stay`/`guard` precedence; abandon on any master movement |
