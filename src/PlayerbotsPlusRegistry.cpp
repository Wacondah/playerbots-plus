/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "PlayerbotsPlusRegistry.h"

#include "AltCareActions.h"
#include "ChatCommandTrigger.h"
#include "CraftDeclinedValue.h"
#include "ChosenSpecValue.h"
#include "CityErrandAction.h"
#include "CraftItemAction.h"
#include "DKAiObjectContext.h"
#include "DruidAiObjectContext.h"
#include "ErrandsStatusAction.h"
#include "ErrandsStrategy.h"
#include "ErrandsTriggers.h"
#include "ErrandsValues.h"
#include "HunterAiObjectContext.h"
#include "HuntQuestMobAction.h"
#include "LevelUpAction.h"
#include "MageAiObjectContext.h"
#include "OfferToMasterAction.h"
#include "PaladinAiObjectContext.h"
#include "PriestAiObjectContext.h"
#include "ProfessionsAction.h"
#include "ProfessionsValue.h"
#include "RebalanceBagsAction.h"
#include "RoleAction.h"
#include "RogueAiObjectContext.h"
#include "RunErrandAction.h"
#include "ShamanAiObjectContext.h"
#include "ShareItemAction.h"
#include "ShareQuestsAction.h"
#include "WarlockAiObjectContext.h"
#include "WarriorAiObjectContext.h"

namespace PlayerbotsPlus
{
namespace
{
class PlusStrategyContext : public NamedObjectContext<Strategy>
{
public:
    PlusStrategyContext()
    {
        creators["errands"] = [](PlayerbotAI* ai) -> Strategy* { return new ErrandsStrategy(ai); };
        creators["debug errands"] = [](PlayerbotAI* ai) -> Strategy* { return new DebugErrandsStrategy(ai); };
        creators["errands hunt"] = [](PlayerbotAI* ai) -> Strategy* { return new ErrandsHuntStrategy(ai); };
        creators["errands share"] = [](PlayerbotAI* ai) -> Strategy* { return new ErrandsShareStrategy(ai); };
        creators["errands bags"] = [](PlayerbotAI* ai) -> Strategy* { return new ErrandsBagsStrategy(ai); };
        creators["errands craft"] = [](PlayerbotAI* ai) -> Strategy* { return new ErrandsCraftStrategy(ai); };
        creators["errands levelup"] = [](PlayerbotAI* ai) -> Strategy* { return new ErrandsLevelUpStrategy(ai); };
        creators["errands quests"] = [](PlayerbotAI* ai) -> Strategy* { return new ErrandsQuestsStrategy(ai); };
        creators["errands loot"] = [](PlayerbotAI* ai) -> Strategy* { return new ErrandsLootStrategy(ai); };
        creators["errands revive"] = [](PlayerbotAI* ai) -> Strategy* { return new ErrandsReviveStrategy(ai); };
    }
};

class PlusTriggerContext : public NamedObjectContext<Trigger>
{
public:
    PlusTriggerContext()
    {
        creators["errands tick"] = [](PlayerbotAI* ai) -> Trigger* { return new ErrandsTickTrigger(ai); };
        creators["bags tick"] = [](PlayerbotAI* ai) -> Trigger* { return new BagsTickTrigger(ai); };
        creators["errands"] = [](PlayerbotAI* ai) -> Trigger* { return new ChatCommandTrigger(ai, "errands"); };
        creators["professions"] = [](PlayerbotAI* ai) -> Trigger* { return new ChatCommandTrigger(ai, "professions"); };
        creators["role"] = [](PlayerbotAI* ai) -> Trigger* { return new ChatCommandTrigger(ai, "role"); };
        creators["craft yes"] = [](PlayerbotAI* ai) -> Trigger* { return new ChatCommandTrigger(ai, "craft yes"); };
        creators["craft no"] = [](PlayerbotAI* ai) -> Trigger* { return new ChatCommandTrigger(ai, "craft no"); };
    }
};

class PlusActionContext : public NamedObjectContext<Action>
{
public:
    PlusActionContext()
    {
        creators["run errand"] = [](PlayerbotAI* ai) -> Action* { return new RunErrandAction(ai); };
        creators["city errand"] = [](PlayerbotAI* ai) -> Action* { return new CityErrandAction(ai); };
        creators["errands levelup"] = [](PlayerbotAI* ai) -> Action* { return new LevelUpAction(ai); };
        creators["share quests"] = [](PlayerbotAI* ai) -> Action* { return new ShareQuestsAction(ai); };
        creators["errands loot roll"] = [](PlayerbotAI* ai) -> Action* { return new LootNeedAction(ai); };
        creators["errands release"] = [](PlayerbotAI* ai) -> Action* { return new ReleaseWhenAloneAction(ai); };
        creators["errands status"] = [](PlayerbotAI* ai) -> Action* { return new ErrandsStatusAction(ai); };
        creators["hunt quest mob"] = [](PlayerbotAI* ai) -> Action* { return new HuntQuestMobAction(ai); };
        creators["share item"] = [](PlayerbotAI* ai) -> Action* { return new ShareItemAction(ai); };
        creators["rebalance bags"] = [](PlayerbotAI* ai) -> Action* { return new RebalanceBagsAction(ai); };
        creators["professions"] = [](PlayerbotAI* ai) -> Action* { return new ProfessionsAction(ai); };
        creators["role"] = [](PlayerbotAI* ai) -> Action* { return new RoleAction(ai); };
        creators["offer to master"] = [](PlayerbotAI* ai) -> Action* { return new OfferToMasterAction(ai); };
        creators["craft item"] = [](PlayerbotAI* ai) -> Action* { return new CraftItemAction(ai); };
        creators["craft yes"] = [](PlayerbotAI* ai) -> Action* { return new CraftAnswerAction(ai, true); };
        creators["craft no"] = [](PlayerbotAI* ai) -> Action* { return new CraftAnswerAction(ai, false); };
    }
};

class PlusValueContext : public NamedObjectContext<UntypedValue>
{
public:
    PlusValueContext()
    {
        creators["errands data"] = [](PlayerbotAI* ai) -> UntypedValue* { return new ErrandsDataValue(ai); };
        creators["assigned professions"] = [](PlayerbotAI* ai) -> UntypedValue* { return new ProfessionsValue(ai); };
        creators["craft declined"] = [](PlayerbotAI* ai) -> UntypedValue* { return new CraftDeclinedValue(ai); };
        creators["chosen spec"] = [](PlayerbotAI* ai) -> UntypedValue* { return new ChosenSpecValue(ai); };
    }
};

// Each shared list deletes the contexts it holds, so every list gets its own instance.
template <class T, class Context>
void AddOnce(SharedNamedObjectContextList<T>& list, char const* probe)
{
    if (!list.creators.count(probe))
        list.Add(new Context());
}

template <class ClassContext>
void RegisterInto()
{
    AddOnce<Strategy, PlusStrategyContext>(ClassContext::sharedStrategyContexts, "errands");
    AddOnce<Trigger, PlusTriggerContext>(ClassContext::sharedTriggerContexts, "errands tick");
    AddOnce<Action, PlusActionContext>(ClassContext::sharedActionContexts, "run errand");
    AddOnce<UntypedValue, PlusValueContext>(ClassContext::sharedValueContexts, "errands data");
}
}  // namespace

bool EnsureRegistered()
{
    if (PriestAiObjectContext::sharedStrategyContexts.creators.empty())
        return false;

    RegisterInto<DKAiObjectContext>();
    RegisterInto<DruidAiObjectContext>();
    RegisterInto<HunterAiObjectContext>();
    RegisterInto<MageAiObjectContext>();
    RegisterInto<PaladinAiObjectContext>();
    RegisterInto<PriestAiObjectContext>();
    RegisterInto<RogueAiObjectContext>();
    RegisterInto<ShamanAiObjectContext>();
    RegisterInto<WarlockAiObjectContext>();
    RegisterInto<WarriorAiObjectContext>();
    return true;
}
}  // namespace PlayerbotsPlus
