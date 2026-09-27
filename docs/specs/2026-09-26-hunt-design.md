# Errands hunt: group pulls quest mobs near an idle master

Status: approved design, pending review.
Builds on `2026-09-26-errands-design.md` (v0.1.0).

## Problem

Altbots only fight when the master or the group is attacked. Kill credit is shared and
`loot` picks up quest items and quest objects, so objectives progress only when the
player does the killing. Comfort feature, not a blocker.

## Goal

A non-combat strategy, `errands hunt`: when the master stands still and the bots have no
errand left, one bot pulls a quest mob near the master; the group then fights it through
the normal mod-playerbots combat reactions.

Not in scope: elites, packs, dungeons, mobs no group member needs, chasing beyond the
radius.

## Behaviour

**Activation:** `nc +errands hunt`, on top of `errands` (same leash, same config).

**Order:** hunting is considered only when the errand planner returns `Idle` with
`nothing to do`. Errands always come first.

**Puller:** exactly one bot of the group acts. The first bot with `errands hunt` that is a
tank; otherwise the one with the lowest GUID among bots with `errands hunt`. Every bot
computes the same election from the group roster; only the elected bot hunts.

**Target** — among hostile units visible to the puller, within `Hunt.Radius` (45 yd) of the
master, wider than the errands `Radius`: mobs closer than ~20 yd aggro before the master
has been idle long enough:

- needed by at least one group bot: a missing kill objective
  (`RequiredNpcOrGo` / `CreatureOrGOCount`) or quest loot it still needs
  (`LootTemplates_Creature.HaveQuestLootForPlayer`);
- alive, non-elite, not in combat, not tapped by another player;
- level ≤ lowest group bot level + `MaxLevelAbove`;
- no other attackable hostile within `PackRadius` of it;
- not blacklisted.

The nearest to the master wins.

**Group readiness:** every group member (bots and master) out of combat, alive, above
the rest thresholds (`mediumHealth`, `mediumMana`). Otherwise no hunt.

**Pull:** the puller attacks through mod-playerbots' `AttackAction::Attack`. From then
on the group is in combat and the non-combat strategy is idle. If the puller has not
engaged the target within `Timeout`, the target is blacklisted for `Blacklist` ms (same
values as errands).

**Leash:** same blockers as errands (master still for `IdleDelay`, not mounted, same map,
no instance, no `stay`/`guard`, bot within `Radius` + 5 yd).

**Diagnostics:** `errands` whisper shows the hunt state on the puller (target, last
reason: `not puller`, `group not ready`, `no quest mob`, `pack nearby`…).
`debug errands` logs hunt decisions.

## Design

- Leash reuse: hunting runs only when this tick's errands decision is
  `Idle / nothing to do` (recorded with a timestamp in `errands data`), which implies
  every leash condition holds; no code is extracted.
- `HuntPlanner.{h,cpp}` (pure, unit tested):
  - `ElectPuller(std::vector<GroupBot> const&) -> uint64_t` (`GroupBot`: guid, isTank,
    hasHunt);
  - `PlanHunt(HuntSnapshot const&, HuntState&, HuntConfig const&, uint32_t now) ->
    HuntDecision` (`Idle`/`Start`/`Continue`/`Abandon`, target, reason). `HuntSnapshot`
    holds the leash fields, `isPuller`, `groupReady`, `errandsIdle`, `minGroupLevel` and
    candidate mobs (`id`, `pos`, `level`, `elite`, `inCombat`, `tappedByOther`,
    `needed`, `hostilesNearby`).
- `HuntQuestMobAction : AttackAction` (`"hunt quest mob"`): builds the snapshot, runs
  `PlanHunt` in `isUseful()`, attacks in `Execute()`. Neededness copies the logic of
  `GrindTargetValue::needForQuest` (private upstream), evaluated for each group bot.
- `ErrandsHuntStrategy` (`"errands hunt"`, non-combat): trigger `"errands tick"` →
  `"hunt quest mob"`, relevance just below `run errand`.
- Per-bot `HuntState` lives in the existing `"errands data"` value.
- Registry: add the strategy and action to the 10 class contexts.

## Configuration

| Key | Default | |
|---|---|---|
| `PlayerbotsPlus.Hunt.Radius` | `45` | yd from the master |
| `PlayerbotsPlus.Hunt.MaxLevelAbove` | `2` | levels above the lowest group bot |
| `PlayerbotsPlus.Hunt.PackRadius` | `8` | yd; other hostiles this close veto the pull |

## Testing

- Unit: puller election (tank first, lowest GUID, strategy required), target filters
  (each rule), nearest to master, blacklist on timeout, errands priority, group
  readiness, leash reuse.
- In game (`docs/testing.md`): lone quest mob pulled and killed; pack ignored; elite
  ignored; no hunt while healer is low on mana; master moves mid-pull (bots follow once
  combat ends); only one bot pulls; mob needed only by one bot still hunted.

## Risks

- A pull still aggroes a patrol passing by: accepted, the group fights it as usual.
- `needForQuest` logic is copied from upstream: listed in `upstream-dependencies.md`.
