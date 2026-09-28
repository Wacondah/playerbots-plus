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
| upstream usages `ITEM_USAGE_VENDOR` / `ITEM_USAGE_AH` / `ITEM_USAGE_SKILL` | `RunErrandAction::ExtraJunk` | what may be sold |
| `SellAction::Sell(Item*)` | `RunErrandAction::VisitTarget` | selling one item |
| `BotCheatMask::food` (`PlayerbotAI::HasCheat`) | `RunErrandAction::ExtraJunk` | food is sold only when bots never eat items |
| value `Save`/`Load` + `PlayerbotRepository::Save` (`playerbots_db_store`) | `ProfessionsValue`, `ProfessionsAction` | assignment persistence |
| trade opcodes `CMSG_INITIATE_TRADE`, `CMSG_SET_TRADE_ITEM`; bot-side accept in `TradeStatusAction` | `OfferToMasterAction` | offers to the master |
| `PlayerbotAI::CastSpell`, `CanCastSpell`, `ItemUsageValue::SpellGivesSkillUp` | `CraftItemAction` | crafting |
| core `ObjectMgr::GetNpcVendorItemList`, `Player::BuyItemFromVendorSlot`, `Player::HasItemTotemCategory` | `ReagentIndex`, `Shopping`, `CraftItemAction` | vendor reagents |
| `NewRpgBaseAction::MoveFarTo`, `WorldPosition` | `CityErrandAction` | long walks across a capital |
| core `Trainer::Trainer` (class trainers, `IsTrainerValidForPlayer`, `CanTeachSpell`, `TeachSpell`) | `Professions` | class training |
| core `ObjectMgr::GetAllCreatureData`, `MapMgr::GetZoneId`, `Map::GetCreatureBySpawnIdStore`, `Player::GetReputationPriceDiscount(FactionTemplateEntry const*)` | `CityIndex`, `CityErrandAction`, `Professions` | capital NPCs |
| action `"equip upgrade"` (`EquipUpgradeAction`) | `ShareItemAction::Execute` | receivers equip shared gear (no item push packet on a direct move) |
| `PlayerbotFactory::InitTalentsBySpecNo`, `InitTalentsTree`, `InitPetTalents`; `sPlayerbotAIConfig.premadeSpecName`; `AiFactory::GetPlayerSpecTabs`; trigger `"levelup"` | `LevelUpAction`, `PlayerbotsPlusScript` | talents on level-up |
| `Group::GetRolls`, `Roll::playerVote`, `Group::CountRollVote`; action `"release"`; triggers `"very often"`, `"often"` | `AltCareActions` | need rolls, release when nobody can resurrect |
| `PlayerbotAI::ResetStrategies`, `GetStrategies`, `IsTank`/`IsHeal`; strategy `"threat"`; `AiFactory::GetPlayerSpecTab(s)` | `RoleAction` | role command |
| `BOT\t` addon-message commands and `#a` reply routing (`PlayerbotAI::HandleCommand`) | `QuestLogAction`, addon | questlog protocol |
