# In-game test checklist

Setup: `AiPlayerbot.NonCombatStrategies = "+errands"` in `playerbots.conf`, server
restarted, a party of altbots. Bots with saved strategies ignore that setting: send
`/p nc +errands` once. Check with `nc ?` that `errands` is listed. Whisper
`nc +debug errands` to one bot to see decisions.

| # | Scenario | Expected |
|---|---|---|
| 1 | Stop near a quest giver with quests for everyone | every bot walks over and accepts within ~5 s, then follows |
| 2 | One bot a step behind in a chain | that bot turns in first, then accepts the next step |
| 3 | Bot 15 yd away when you talk to the giver | it still gets the quest once you stand still |
| 4 | Bot with a full quest log | log is organised, quest accepted |
| 5 | NPC unreachable (upper floor, behind a wall) | errand times out after 20 s, `errands` shows it blacklisted, bot follows |
| 6 | Walk away mid-errand | bots drop the errand at once and follow |
| 7 | `stay` order, then stop near a giver | the bot stays put |
| 8 | Vendor + repairer in camp, bots with greys and damaged gear | bots repair and sell greys, once |
| 9 | Bot without master (random bot) with `+errands` | nothing happens |
| 10 | Inside a dungeon | nothing happens (`errands` → `in instance`) |
| 11 | Mounted, stop without dismounting | nothing happens (`master mounted`) |
