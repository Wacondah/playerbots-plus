/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "ErrandsStatusAction.h"

#include "ErrandsValues.h"
#include "ObjectAccessor.h"
#include "Playerbots.h"
#include "PlayerbotsPlusConfig.h"

#include <sstream>

namespace PlayerbotsPlus
{
bool ErrandsStatusAction::Execute(Event /*event*/)
{
    ErrandsData& data = AI_VALUE(ErrandsData&, "errands data");
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

    botAI->TellMaster(out.str());
    return true;
}
}  // namespace PlayerbotsPlus
