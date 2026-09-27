/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_ERRANDS_STATUS_ACTION_H
#define PLAYERBOTS_PLUS_ERRANDS_STATUS_ACTION_H

#include "Action.h"

namespace PlayerbotsPlus
{
struct ErrandsData;
}

namespace PlayerbotsPlus
{
// Reply to the "errands" whisper: current errand and last planner reason;
// "errands city" starts a capital trip.
class ErrandsStatusAction : public Action
{
public:
    ErrandsStatusAction(PlayerbotAI* botAI) : Action(botAI, "errands status") {}

    bool Execute(Event event) override;

private:
    bool RequestCity(ErrandsData& data);
};
}  // namespace PlayerbotsPlus

#endif
