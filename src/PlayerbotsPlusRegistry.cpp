/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "PlayerbotsPlusRegistry.h"

#include "ChatCommandTrigger.h"
#include "DKAiObjectContext.h"
#include "DruidAiObjectContext.h"
#include "ErrandsStatusAction.h"
#include "ErrandsStrategy.h"
#include "ErrandsTriggers.h"
#include "ErrandsValues.h"
#include "HunterAiObjectContext.h"
#include "MageAiObjectContext.h"
#include "PaladinAiObjectContext.h"
#include "PriestAiObjectContext.h"
#include "RogueAiObjectContext.h"
#include "RunErrandAction.h"
#include "ShamanAiObjectContext.h"
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
    }
};

class PlusTriggerContext : public NamedObjectContext<Trigger>
{
public:
    PlusTriggerContext()
    {
        creators["errands tick"] = [](PlayerbotAI* ai) -> Trigger* { return new ErrandsTickTrigger(ai); };
        creators["errands"] = [](PlayerbotAI* ai) -> Trigger* { return new ChatCommandTrigger(ai, "errands"); };
    }
};

class PlusActionContext : public NamedObjectContext<Action>
{
public:
    PlusActionContext()
    {
        creators["run errand"] = [](PlayerbotAI* ai) -> Action* { return new RunErrandAction(ai); };
        creators["errands status"] = [](PlayerbotAI* ai) -> Action* { return new ErrandsStatusAction(ai); };
    }
};

class PlusValueContext : public NamedObjectContext<UntypedValue>
{
public:
    PlusValueContext()
    {
        creators["errands data"] = [](PlayerbotAI* ai) -> UntypedValue* { return new ErrandsDataValue(ai); };
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
