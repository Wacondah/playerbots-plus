# Upstream dependencies

Internal mod-playerbots symbols this module relies on. When the weekly build breaks,
start here.

| Symbol | Used in | Why |
|---|---|---|
| `{DK,Druid,Hunter,Mage,Paladin,Priest,Rogue,Shaman,Warlock,Warrior}AiObjectContext::shared{Strategy,Action,Trigger,Value}Contexts` (public statics) | `PlayerbotsPlusRegistry.cpp` | registration |
| `SharedNamedObjectContextList<T>::Add`, `::creators`; `NamedObjectContext<T>` | `PlayerbotsPlusRegistry.cpp` | registration |
| `NewRpgBaseAction` (`CanInteractWithQuestGiver`, `IsWithinInteractionDist`, `IsQuestWorthDoing`, `IsQuestCapableDoing`, `InteractWithNpcOrGameObjectForQuest`, `OrganizeQuestLog`, `MoveWorldObjectTo`) | `RunErrandAction` | quest logic and movement |
| `ManualSetValue<T&>` | `ErrandsValues.h` | per-bot state |
| `ChatCommandTrigger` | registry | `errands` whisper |
| actions `"repair"`, `"sell"` (params `gray`, `white`) | `RunErrandAction::VisitTarget` | vendor work |
| values `"nearest npcs"`, `"nearest game objects no los"` | `RunErrandAction::Scan` | candidates |
| `sPlayerbotAIConfig.mediumHealth`, `.mediumMana` | `RunErrandAction::BuildSnapshot` | rest threshold |
| strategies `"stay"`, `"guard"` | `RunErrandAction::BuildSnapshot` | player orders win |
| `PlayerbotAIConfig::Initialize()` building the lists before `OnStartup` | `PlayerbotsPlusScript.cpp` | registration timing |
| `AttackAction::Attack` (protected) | `HuntQuestMobAction` | engaging switches to the combat engine |
| `PlayerbotAI::IsTank`, `GET_PLAYERBOT_AI` | `HuntQuestMobAction` | puller election |
| value `"possible targets"` | `HuntQuestMobAction` | mob candidates |
| logic of `GrindTargetValue::needForQuest` (copied) | `HuntQuestMobAction::NeededBy` | quest need |
| value `"item usage"` of other bots (`ITEM_USAGE_EQUIP/REPLACE/QUEST`) | `ShareItemAction` | who can use an item |
| `StatsWeightCalculator::CalculateItem` | `ShareItemAction` | upgrade gain |
| item move as in `GiveItemAction` (`MoveItemFromInventory`/`MoveItemToInventory`) | `ShareItemAction::Execute` | transfer |
