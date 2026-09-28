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
| S7 | Tailor crafts a Linen Bag, an alt has an empty bag slot | the alt receives it and equips it at once |

## Selling

| # | Scenario | Expected |
|---|---|---|
| V1 | Replaced soulbound quest reward in bags, vendor in camp | sold |
| V2 | BoE green usable by another alt | not sold (shared instead) |
| V3 | BoE green nobody in the group uses | sold |
| V4 | `MaxSellQuality = 2`, useless blue | kept |
| V5 | Linen, tailor in the group / no tailor nor first aid | kept / sold |
| V6 | Equipped items | never sold |
| V7 | Stale bread, small pumpkin, spring water (bots with the `food` cheat) | sold |
| V8 | Cooked food, or `SellFood = 0`, or `food` cheat off | kept |
| V9 | Copper tube or bolts held by a non-engineer, no engineer in the group | sold |
| V10 | Copper ore held by a miner, no smith in the group; mining pick | kept |

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

## Assigned professions

| # | Scenario | Expected |
|---|---|---|
| P1 | `professions mining tailoring` near both trainers, bot without professions | learns apprentice mining and tailoring |
| P2 | Level/skill allow a higher rank or new recipes | learned at the next trainer visit |
| P3 | General goods vendor in camp, no mining pick | buys one |
| P4 | Bot knowing skinning + herbalism, assigned mining + tailoring | forgets the least-trained one at the mining trainer, learns mining; then the other at the tailoring trainer |
| P5 | `professions reset` | asks for confirmation; `professions reset confirm` within 30 s forgets both |
| P6 | Server restart | `professions` still shows the assignment |
| P7 | No assignment | nothing learned, bought or forgotten |
| P8 | `professions mining tailoring alchemy` / `professions cooking` | error reply, assignment unchanged |

## Errands craft and offers to the master

| # | Scenario | Expected |
|---|---|---|
| C1 | Tailor with linen, `nc +errands craft`, you have a small bag | asks "I can craft [Linen Bag] for you"; `craft yes` → crafts, trade window opens, bag lands after you accept |
| C2 | Cancel that trade | not offered again for 30 min |
| C3 | `craft no` | never asked again for that item, also after a restart |
| C4 | Bandages useful to the party (first aid) | crafted without asking |
| C5 | Recipes still orange/yellow, nothing useful | crafts for skill, cheapest materials first; stops when recipes turn grey |
| C6 | A BoE green upgrade for you in a bot's bags (`errands share`) | offered to you before any bot |
| C7 | Enchanter with `errands craft`, a BoE green nobody wants in another alt's bags | handed to the enchanter (share), disenchanted; not sold |
| C8 | Soulbound replaced quest reward on the enchanter | disenchanted instead of sold |
| C9 | Tailor with linen, no coarse thread, vendor in camp | buys thread, then crafts |
| C10 | Bot under the reserve | buys nothing, status `shopping: no budget` |
| C11 | Cook without salt | never buys |
| C12 | `craft yes` on a buyable bag | `I need a vendor for [thread]`, buys, crafts, offers |

## Switch

| # | Scenario | Expected |
|---|---|---|
| W1 | `/p errands on` with bots that have no errands strategy | each bot answers `errands: on (hunt, share, bags, craft, levelup, quests, loot, revive)`; `nc ?` lists the eight, `dead ?` shows `errands revive` |
| W2 | `/p errands off` | each bot answers `errands: off`; the eight and `errands revive` are gone, `debug errands` untouched |
| W3 | Whisper `errands on` to one bot | only that bot changes |

## Errands city

| # | Scenario | Expected |
|---|---|---|
| Y1 | Stop at Stormwind's gate with a priest that has ranks to learn | walks to the Cathedral, learns, comes back |
| Y2 | Tailor with linen and no thread, vendor across town | buys thread, comes back, crafts |
| Y3 | Leave the city mid-trip | bots abort and follow |
| Y4 | `errands city` with nothing to do | `city: nothing to do` |
| Y5 | `City.Auto = 0` | nothing until `errands city` |
| Y6 | Class trainer within 20 yd in Goldshire | trains as a normal errand |
| Y7 | Walk around inside the city during a trip | bots continue |

## Level-up and quest sharing

| # | Scenario | Expected |
|---|---|---|
| L1 | `talents spec holy pve` at level 9, then level 10 | the point goes into the holy pve template |
| L2 | Level-up of an alt with points in one tree and no chosen spec | the new point follows that tree |
| L3 | Level 10 with no talents and no chosen spec | whispers `Choose my spec: …`, spends nothing |
| L4 | Restart the server, level up | the chosen spec is still used |
| Q1 | An alt holds a sharable quest another alt can take, group idle | `Shared [quest] with X`; X has it, no quest window for you |
| Q2 | A quest the other alt already did, or cannot take (level, class) | not shared |
| R1 | Alt dies in the open, no priest/paladin/shaman alive in the group | 10 s after the fight it releases and runs back |
| R2 | Same with a living priest (or you as a priest) | it waits to be resurrected |
| R3 | Death in a dungeon | waits, as before |
| N1 | Group roll on a green the alt would equip, also mid-fight | the alt rolls Need |
| N2 | Group roll on linen cloth, a tailor in the group | the tailor rolls Need; others follow mod-playerbots |
| K1 | Alchemist knowing a transmute, reagents in bags | crafts it when ready and opens a trade with you |
| O1 | `role tank` to a warrior with arms points | talents reset into prot pve, `co ?` shows `tank`, `tank assist`; errands strategies still there |
| O2 | `role dps` to a rogue / mage | `co ?` shows `threat`; stops attacking before pulling aggro off the tank |
| O3 | `role heal` to a rogue | `role: my class cannot heal`, nothing changed |
| O4 | `role` alone | `role: tank (prot pve)` |
| S8 | Brogan has 11 Goretusk Livers for 8, Kyra 5 for 8 (same quest) | Brogan gives Kyra 3 (`… (quest)`), keeps 8; `bags` never moves Brogan's livers |
| S10 | Quest complete but not turned in (15/15), another alt at 6/15 | keeps its 15, nothing passes around |
| S9 | An alt holds livers no quest of its own needs, another alt needs them | all go to the one that needs them; none sold |

## Party quest window

| # | Scenario | Expected |
|---|---|---|
| P1 | `/pq` in a party of alts | your quests, each with every member's progress; alt-only quests greyed below |
| P2 | Kill or loot for a quest while the window is open | counts update within 10 s |
| P3 | Collapse "Quêtes des alts", reload UI | stays collapsed; window position and size kept |
| P4 | A real player in the party | shown as `(pas de réponse)`, no error |
