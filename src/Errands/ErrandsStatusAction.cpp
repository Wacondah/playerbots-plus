/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "ErrandsStatusAction.h"

#include "ChatCommands.h"
#include "CityErrandAction.h"
#include "ErrandsValues.h"
#include "GameObject.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Playerbots.h"
#include "PlayerbotsPlusConfig.h"

#include <sstream>

namespace PlayerbotsPlus
{
bool ErrandsStatusAction::Execute(Event event)
{
    // "errands on/off" is handled by the module's chat hook, not a status request.
    if (ParseErrandsSwitch("errands " + event.getParam()) != ErrandsSwitch::None)
        return true;
    ErrandsData& data = AI_VALUE(ErrandsData&, "errands data");
    if (event.getParam() == "city")
        return RequestCity(data);
    ActiveErrand const& active = data.state.active;

    std::ostringstream out;
    out << "errands: ";
    if (active.IsActive())
    {
        WorldObject* target = ObjectAccessor::GetWorldObject(*bot, ObjectGuid(active.target));
        out << ToString(active.kind) << " at " << (target ? target->GetName() : "?") << ", "
            << (getMSTime() - active.startedAt) / 1000 << "s/" << Config().planner.timeoutMs / 1000 << "s";
    }
    else
        out << "idle";

    out << " | last: " << (data.state.lastReason.empty() ? "-" : data.state.lastReason)
        << " | blacklisted: " << data.state.blacklistedAt.size();
    if (botAI->HasStrategy("errands hunt", BotState::BOT_STATE_NON_COMBAT))
        out << " | hunt: " << (data.hunt.lastReason.empty() ? "-" : data.hunt.lastReason);
    if (bot->HasSkill(SKILL_MINING) || bot->HasSkill(SKILL_HERBALISM))
        out << " | gather: " << (data.gather.lastReason.empty() ? "-" : data.gather.lastReason);
    if (botAI->HasStrategy("errands share", BotState::BOT_STATE_NON_COMBAT))
        out << " | share: " << (data.share.lastReason.empty() ? "-" : data.share.lastReason);
    if (botAI->HasStrategy("errands bags", BotState::BOT_STATE_NON_COMBAT))
        out << " | bags: " << (data.bags.lastReason.empty() ? "-" : data.bags.lastReason);
    if (botAI->HasStrategy("errands craft", BotState::BOT_STATE_NON_COMBAT))
    {
        out << " | craft: " << (data.craft.lastReason.empty() ? "-" : data.craft.lastReason);
        if (data.shopping.Any())
        {
            out << " | shopping:";
            for (Purchase const& p : data.shopping.purchases)
                if (ItemTemplate const* proto = sObjectMgr->GetItemTemplate(p.item))
                    out << " " << p.count << "x " << chat->FormatItem(proto);
            if (ItemTemplate const* product = sObjectMgr->GetItemTemplate(data.shoppingProduct))
                out << " for " << data.shopping.crafts << "x " << chat->FormatItem(product);
        }
        else if (data.craftDecision.action == CraftAction::Shop)
            out << " | shopping: " << data.shopping.reason;
    }
    if (data.city.Active())
    {
        std::string name = "?";
        if (IsStationStop(data.city.current))
        {
            if (GameObject* go = CityIndex::LiveGameObject(bot->GetMap(), StationSpawnId(data.city.current)))
                name = go->GetName();
        }
        else if (Creature* stop = CityIndex::LiveCreature(bot->GetMap(), data.city.current))
            name = stop->GetName();
        out << " | city: to " << name << " (" << data.city.done.size() + 1 << "/"
            << data.city.stopsPlanned << ")";
    }
    else if (!data.city.lastReason.empty() && data.city.lastReason != "city: -")
        out << " | " << data.city.lastReason;
    if (data.offer.Active())
        out << " | offering item " << data.offer.entry << (data.offer.placed ? " (in trade)" : "");

    botAI->TellMaster(out.str());
    return true;
}

bool ErrandsStatusAction::RequestCity(ErrandsData& data)
{
    int32 const stops = CityErrandAction(botAI).CountStops();
    if (stops < 0)
        botAI->TellMaster("city: not in a capital");
    else if (!stops)
        botAI->TellMaster("city: nothing to do");
    else
    {
        data.cityRequested = true;
        botAI->TellMaster("city: going (" + std::to_string(stops) + " stops)");
    }
    return true;
}
}  // namespace PlayerbotsPlus
