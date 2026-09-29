# Group pull (pull planner)

Status: approved design, pending review.

## Problem

mod-playerbots' `pull` sends the tank to range of the master's target, shoots, and walks back
to where it stood. It never checks which other mobs the trip wakes up, the rest of the group
keeps following and attacking, and casters that stay at range are chased into their pack.
In dungeons (Deadmines) the tank ends up charging into packs.

## Goal

On request, the group pulls one mob away from the others, waking up as few extra mobs as
possible: a chosen puller shoots from a safe spot, runs back to a hiding spot near the group,
the tank picks the mob up, the group waits frozen until then.

Not in scope: automatic chain pulling (the tank never pulls on its own initiative), crowd
control (sheep, sap), patrol timing, pulling in battlegrounds.

## Commands

| Trigger | Effect |
|---|---|
| master whispers `pull` to the tank, with a target selected | plan, then pull (replaces mod-playerbots' `pull` while errands is on) |
| master sets the skull raid icon on a mob, out of combat | same, without a whisper |
| `pull force` | accept the announced pull that wakes extra mobs |
| `pull cancel` | cancel the pull and release the group |

Only the tank of the group (the bot `IsTank`) handles them; others ignore them. A pull request
while one is running is refused ("already pulling").

## Flow

1. **Request**: the tank snapshots the scene (below) and runs the planner.
2. **Plan** (pure):
   - puller: the tank if it has a ranged weapon or a ranged pull spell; else the ranged group
     member closest to the target; else the tank on foot (body pull);
   - extra mobs:
     - unavoidable: the target's formation members, and mobs of the target's faction within
       the server's assistance radius (`CreatureFamilyAssistanceRadius`, 10 yd) of it —
       announced, never block;
     - avoidable: every other hostile living mob; each has its aggro radius toward the
       puller (`Creature::GetAggroRange`);
   - a safe route: firing spot and the way there and back to the hiding spot keep every
     path point farther than aggro radius + 3 yd from each avoidable mob.
3. **Decision**:
   - safe route found: go;
   - none: the tank says who would come ("Pulling Defias Miner wakes 2 more: Defias Evoker,
     Defias Miner. Whisper 'pull force'.") and waits 30 s for `pull force`, then drops it;
   - unavoidable adds are told once ("Defias Miner comes with 2 friends"), no confirmation.
4. **Run**:
   - the group enters hold;
   - the puller walks to the firing spot, shoots once (or steps into the target's aggro
     radius for a body pull), walks to the hiding spot;
   - the tank, when not the puller, stays and takes the mobs as they reach the group
     (its usual `tank assist`).
5. **End**: when the tank holds the threat of every mob in combat with the group, or 20 s
   after the shot; also on puller death, target gone, `pull cancel`, or the master entering
   combat on another target. The hold is released and normal combat resumes.

## Scene and planner

Adapter (in game, once per request, in the tank's context):

- mobs: hostile living creatures within 60 yd of the target: position, aggro radius toward
  the puller, unavoidable flag;
- firing candidates: about 48 points on rings around the target at 70–95 % of the puller's
  range, every 15°, snapped to the ground; for a body pull, one ring 1 yd inside the target's
  aggro radius. Each keeps: line of sight to the target, the server path from the puller
  (`PathGenerator`, dropped when not complete);
- hiding candidates: about 12 points 8–20 yd from the group's centre, kept when out of line
  of sight of the target and reachable; each with the path from its firing spot computed
  on demand for the best firing spots only (at most 5);
- the group's position is always a fallback hiding spot.

Planner (pure, `PullPlanner`, unit-tested):

- input: puller facts, mobs, firing candidates with their paths, hiding candidates with
  paths from the firing spots;
- a route is safe when every segment of go and return stays farther than radius + 3 yd from
  every avoidable mob (point–segment distance in 2D, z ignored);
- choice: the safe route with the shortest total length; a hidden spot beats the group
  position when both are safe;
- no safe route: the route waking the fewest avoidable mobs, with their list;
- puller choice as in Flow.

## Group hold

- Shared state: an in-memory board keyed by group GUID: step (planning, awaiting force,
  moving, returning, done), puller, target, firing and hiding spots, deadline. Nothing is
  saved in the database; a server restart forgets pulls.
- A multiplier added to the module's `errands` strategies zeroes, for bots that are neither
  the puller nor the tank, while the board says a pull runs:
  - movement actions (follow, reach, move), attack actions, offensive spells;
  - but not heals, buffs, nor anything when a mob is attacking the bot itself.
- The tank, when not the puller, does not move toward the target during the pull.

## Testing

- Unit (`PullPlanner`): segment clearance (inside, outside, margin); shortest safe route
  wins; hidden spot preferred over the group; no safe route returns the fewest wakes and
  their list; unavoidable mobs never block; puller choice (ranged tank, ranged member,
  body pull).
- In game (Deadmines):
  - P1: single mob pulled with the tank's ranged weapon, nobody else moves until it arrives;
  - P2: tank without ranged weapon, Fizzlewick pulls, Brogan picks the mob up;
  - P3: a caster is pulled, the puller hides behind a corner, the caster comes into melee;
  - P4: target inside a pack: announcement, nothing happens until `pull force`;
  - P5: `pull cancel` during the approach releases the group;
  - P6: the puller dies: pull cancelled, group released;
  - P7: skull icon triggers the pull.
