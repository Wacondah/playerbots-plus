# Party quest progress (client addon + questlog command)

Status: approved design, pending review.

## Problem

The 3.3.5 client cannot read another player's quest log, and bots run no addon. Managing
alts means whispering `quests` to each one and reading free text.

## Goal

A client addon, `PlayerbotsPlusQuests`, shows every quest of the party with each member's
progress (`Brogan 15/15 ✓`, `Kyra 6/15`), fed by a machine-readable `questlog` command the
module adds to bots.

Not in scope: acting on quests from the window (share, abandon), random bots outside the
party, quests of real players other than the viewer.

## Protocol

- Request: the addon sends each party member the addon whisper `BOT\t#a questlog`
  (`SendAddonMessage("BOT", "#a questlog", "WHISPER", name)`). mod-playerbots turns messages
  starting with `BOT\t` into commands; players ignore them.
- Reply: the bot whispers its master addon messages with prefix `PPQ` (`CHAT_MSG_WHISPER`,
  `LANG_ADDON`, text `PPQ\t<payload>`), one per quest, then `END`:

  `Q\t<questId>\t<status>\t<title>\t<objective>|<objective>…`

  - status: `0` in progress, `1` complete (to turn in), `2` failed;
  - objective: `<done>/<required>:<name>`, creatures/objects kills first, then items;
  - title and names in the master's locale (quest, creature, item locales), else English;
  - a line never exceeds 250 bytes: names are cut first, then objectives dropped.
- Only the bot's master may ask; anyone else gets nothing.

## Addon

- Window: movable, resizable, position and size saved (`SavedVariables`); `/pq` toggles it,
  a minimap button too (draggable around the minimap).
- Content: the viewer's quests first (from the client's own quest log), then quests only
  alts have, greyed, in a collapsible section. Per quest: one line per member per objective,
  `✓ to turn in` when complete; red when a member is at less than half, orange otherwise.
- Refresh: on opening, on `QUEST_LOG_UPDATE` and `PARTY_MEMBERS_CHANGED` (throttled to once
  per 2 s), and every 10 s while shown. Nothing while hidden. A member that does not answer
  within 5 s is shown as `(no answer)`.
- Code in `client/PlayerbotsPlusQuests/` in the module repo; installed by linking it into
  `Interface/AddOns`.

## Testing

- Unit: payload formatting (statuses, objectives order, 250-byte cut, separators escaped).
- In game: the window lists the party's quests with correct counts; a quest only an alt has
  shows greyed; counts update within 10 s of a kill or loot; a real player in the party
  shows `(no answer)` without errors.
