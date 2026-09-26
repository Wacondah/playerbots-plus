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
