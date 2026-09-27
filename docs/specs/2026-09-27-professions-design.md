# Assigned professions

Status: approved design, pending review.
Builds on `2026-09-26-errands-design.md`.

## Problem

Altbots never learn professions on their own: mod-playerbots' `trainer` action skips
tradeskill trainers when the master is a real player (`TrainerAction.cpp`), unless the
player targets the trainer and whispers `trainer learn`. Nothing buys profession tools.

## Goal

Assign one or two primary professions per alt, like `talents spec`; within the errands
leash the alt learns only those (ranks and recipes) and buys their tools.

Not in scope: cooking, fishing, first aid; crafting; gathering (upstream `gather`);
enchanting rods (crafted, not sold).

## Commands (whisper a bot)

| Command | Effect |
|---|---|
| `professions mining tailoring` | assign one or two primary professions (English names) |
| `professions` | assignment, current skill, conflicts |
| `professions clear` | remove the assignment |
| `professions reset` | asks to confirm forgetting both primary professions |
| `professions reset confirm` | within 30 s: forgets them now, anywhere; the assignment stays |

The assignment is a bot value with `Save`/`Load`, stored by mod-playerbots with the bot's
strategies (`playerbots_db_store`), so it survives restarts. Saved right after a change.

## Errands

Two new errand kinds, after `Sell`. Within a visit the bot repairs, then sells, then
trains and buys tools: repairs are paid first and selling funds the rest. (Kinds are
picked in that order too.)

- **Train**, at a tradeskill trainer within the radius teaching an assigned profession,
  when the bot can learn something there now (rank or recipe it can afford), or when it
  lacks the profession and needs a slot. At the visit:
  1. if the assigned profession is unknown and no primary slot is free, forget the
     unassigned primary with the lowest skill (`SetSkill(skill, 0, 0, 0)`, the client's
     unlearn path, which frees the slot);
  2. teach, through the core `Trainer::TeachSpell`, every spell of that trainer the bot
     can learn and afford; repeat a few passes, since a new rank unlocks recipes.
- **BuyTool**, at a vendor within the radius selling the tool of an assigned profession
  the bot has neither in its bags nor equipped, if it can afford it:

| Profession | Tool |
|---|---|
| Mining | Mining Pick (2901) |
| Skinning | Skinning Knife (7005) |
| Blacksmithing | Blacksmith Hammer (5956) |
| Engineering | Arclight Spanner (6219) |
| Jewelcrafting | Jeweler's Kit (20815) |
| Inscription | Virtuoso Inking Set (39505) |

Herbalism, tailoring, alchemy, leatherworking and enchanting need no bought tool.

Both kinds use the vendor rest rule: a trainer or vendor just visited is not picked again
for these kinds during `Blacklist` ms.

A bot without assignment never forgets anything.

## Design

- `ProfessionCatalog.{h,cpp}` (pure, unit tested): the 11 primary professions (name,
  skill id, tool); `ParseAssignment(text)` (case-insensitive names, 1–2, no duplicates,
  error text); `SaveAssignment`/`LoadAssignment`; `ProfessionToForget(known, assigned,
  freeSlots, wanted)`.
- `ErrandPlanner`: `ErrandKind::Train`, `ErrandKind::BuyTool` after `Sell`; candidate
  flags `canTrain`, `canSellTool`.
- `Professions.{h,cpp}` (core): trainer profession (a spell's `ReqSkillLine`, else the
  skill step effect of its learned spell), `CanTrainAt`, `TrainAt`, `MissingToolAt`,
  `BuyToolAt` (`Player::BuyItemFromVendorSlot`), known primaries.
- `ProfessionsValue` (`"assigned professions"`) and `ProfessionsAction`
  (`"professions"` chat command) in the errands strategy.

## Testing

- Unit: parsing (names, case, count, duplicates, unknown), save/load round trip,
  forget choice (no slot needed, lowest skill, never an assigned one, none without
  assignment), planner priority and rest rule for the new kinds.
- In game (`docs/testing.md`): assign mining+tailoring near a trainer: learns apprentice;
  higher rank and recipes when level/skill allow; buys a mining pick; a bot with skinning
  assigned mining+tailoring forgets skinning at the trainer only; `reset` asks, `reset
  confirm` forgets both; assignment survives a restart; no assignment → nothing happens.
