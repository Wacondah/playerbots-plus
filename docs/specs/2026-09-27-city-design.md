# Errands city: shopping and training in capitals

Status: approved design, pending review.
Builds on the errands, professions and shopping specs.

## Problem

Errands only reach NPCs within 20 yd of the idle master. In a capital, trainers and
vendors are spread over the whole city: bots never train their class (altbots do not
auto-learn trainer spells upstream, `AutoLearnTrainerSpells` is for random bots), rarely
reach their profession trainer, and cannot buy vendor reagents sold across town.

## Goal

1. In a capital, the leash is released while a bot runs its errands across the city, then
   it comes back to the master.
2. Class training: a bot learns the class spells and ranks it can afford, at a class
   trainer of its class, in town (within the leash) and in capitals.

Not in scope: weapon masters, riding, flight paths, banks, the auction house.

## Capitals

Zones: Stormwind 1519, Ironforge 1537, Darnassus 1657, The Exodar 3557, Orgrimmar 1637,
Undercity 1497, Thunder Bluff 1638, Silvermoon 3487, Shattrath 3703, Dalaran 4395.
The master is in a capital when its zone is one of them.

## City index

`CityIndex` (core), built per capital on first use: creature spawns whose zone
(`sMapMgr->GetZoneId` on the spawn position) is that capital and whose template is a
class trainer (`Trainer::Type::Class`, requirement = class), a tradeskill trainer (with its
profession), or a vendor (with the items it sells without a supply limit). Each entry keeps
the spawn position and the template faction; a destination only counts for a bot friendly
to it (not hostile, `FactionTemplateEntry`).

## Needs

Computed per bot when a trip may start:

- **class**: its class trainer teaches a spell the bot can learn and afford
  (`IsTrainerValidForPlayer`, `CanTeachSpell`, reputation price);
- **profession**: an assigned profession to learn or train (existing rules);
- **reagents**: the craft shopping list (shopping spec) is not empty;
- **tool**: an assigned profession's tool is missing (existing rules).

## City trip

`CityPlanner` (pure) holds the trip:

- **Start**: master in a capital, standing still for `Errands.IdleDelay`, not in combat,
  mounted, on a taxi or in an instance; the bot has at least one need a destination
  covers; `City.Auto` is on, or the master whispered `errands city` (party: `/p errands
  city`); no trip ended less than `City.Cooldown` ago with the same needs.
- **Destinations**: the nearest unvisited destination covering a remaining need, then the
  next one; each is visited once per trip.
- **Travel**: `MoveFarTo` to the spawn position; within interaction range of the real NPC,
  the shared visit runs (training, selling, repair, reagents, tools, as in errands).
- **End**: nothing left, or `City.Timeout` reached; the bot then walks back to the master
  (follow resumes). A destination whose NPC is absent on arrival, or unreachable within
  60 s, is skipped for the trip.
- **Abort**: the master leaves the capital, takes a taxi or portal, enters combat or an
  instance; the bot follows at once.
- The master moving inside the capital does not abort the trip.

During a trip the city action outranks follow (relevance 2.05, between bags and run
errand), so the leash is released.

**Money order** at a visit: class training, profession training, tools, then reagents.
Only reagents keep the shopping reserve.

## Class training in errands

`Candidate.canTrainClass`: a class trainer of the bot's class within the leash teaches
something the bot can learn and afford; new errand kind `TrainClass`, after `Train`.
`TrainClassAt` teaches every such spell through `Trainer::TeachSpell`.

## Replies and diagnostics

`errands city` answers `city: going (<n> stops)` or `city: nothing to do`. `errands`
shows `city: <reason>` or `city: to <NPC> (2/4)`.

## Configuration

| Key | Default | Meaning |
|---|---|---|
| `PlayerbotsPlus.City.Auto` | 1 | start trips when the master idles in a capital |
| `PlayerbotsPlus.City.Timeout` | 180000 | trip length limit (ms) |
| `PlayerbotsPlus.City.Cooldown` | 600000 | no new trip with unchanged needs before (ms) |

## Testing

- Unit (`CityPlanner`): start conditions (idle, capital, auto or command, cooldown with
  unchanged needs, restarted when needs change); nearest destination covering a need;
  visited once; unreachable skipped after 60 s; timeout ends the trip; abort on leaving,
  taxi, combat, instance; master moving inside the capital does not abort. Errand
  planner: `TrainClass` order.
- In game (`docs/testing.md`): at Stormwind's gate, a level-up priest walks to the
  Cathedral, learns its ranks, comes back; a tailor with linen and no thread buys it across
  town then crafts; leaving the city mid-trip brings the bots back; `errands city` with
  nothing to do answers so; `City.Auto = 0` does nothing until `errands city`; a class
  trainer in Goldshire within the leash trains the bot.
