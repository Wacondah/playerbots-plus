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

## Errands hunt

Setup: `/p nc +errands hunt`, party with quests needing kills.

| # | Scenario | Expected |
|---|---|---|
| H1 | Stop near a lone quest mob | one bot pulls it, the group kills it, bots return |
| H2 | Only packed quest mobs around | nothing; puller's `errands` shows `hunt: pack nearby` |
| H3 | Elite quest mob | ignored |
| H4 | Healer below rest mana | no pull until rested (`group not ready`) |
| H5 | Move during the fight | fight ends normally, bots follow |
| H6 | Mob needed by one bot only | still hunted |
| H7 | `errands` to a non-puller | `hunt: not puller` |

## Errands share

Setup: `/p nc +errands share`, party with different classes and professions.

| # | Scenario | Expected |
|---|---|---|
| S1 | A BoE green usable by another alt in a bot's bags | moves once, then gets equipped |
| S2 | A soulbound item useful to another alt | never moves |
| S3 | Linen on a first-aid bot, a tailor in the party | linen goes to the tailor |
| S4 | Herbs on a non-alchemist, an alchemist in the party | herbs go to the alchemist |
| S5 | Receiver with full bags | skipped; giver's `errands` shows `share: receiver bags full` |
| S6 | Two tailors | cloth gathers on the one holding the most |

## Selling

| # | Scenario | Expected |
|---|---|---|
| V1 | Replaced soulbound quest reward in bags, vendor in camp | sold |
| V2 | BoE green usable by another alt | not sold (shared instead) |
| V3 | BoE green nobody in the group uses | sold |
| V4 | `MaxSellQuality = 2`, useless blue | kept |
| V5 | Linen, tailor in the group / no tailor nor first aid | kept / sold |
| V6 | Equipped items | never sold |

## Errands bags

Setup: `/p nc +errands bags`, `nc +debug errands` on the bot you fill up.

| # | Scenario | Expected |
|---|---|---|
| B1 | Full bot with linen, another bot with a partial linen stack | linen merges into that stack |
| B2 | Full bot, another bot with several free slots | a stack moves to the bot with most room |
| B3 | Full mage with its water | water stays |
| B4 | Full bot with quest items | quest items never move |
| B5 | Everyone full | one `bags full, nothing to rebalance`, again only after space comes back |
| B6 | Master keeps walking | rebalancing still happens (out of combat) |
