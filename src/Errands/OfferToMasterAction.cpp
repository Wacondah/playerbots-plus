/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "OfferToMasterAction.h"

#include "ErrandsCommon.h"
#include "GroupItems.h"
#include "Playerbots.h"

namespace PlayerbotsPlus
{
void StartOffer(ErrandsData& data, Item* item, uint32 now)
{
    data.offer = MasterOffer{item->GetGUID().GetRawValue(), item->GetEntry(), now, false, false};
}

bool OfferToMasterAction::isUseful()
{
    ErrandsData& data = AI_VALUE(ErrandsData&, "errands data");
    return data.OfferBusy() && !bot->IsInCombat();
}

bool OfferToMasterAction::Execute(Event /*event*/)
{
    ErrandsData& data = AI_VALUE(ErrandsData&, "errands data");
    uint32 const now = getMSTime();
    Player* master = RealMaster(botAI);
    if (!master)
    {
        data.offer = MasterOffer{};
        data.pendingOfferProduct = 0;
        return false;
    }

    // A crafted item becomes an offer once the cast has put it in the bags.
    if (!data.offer.Active())
    {
        if (Item* crafted = FindEntry(data.pendingOfferProduct))
        {
            StartOffer(data, crafted, now);
            data.pendingOfferProduct = 0;
        }
        else if (Elapsed(now, data.pendingOfferSince, CraftedOfferWaitMs))
            data.pendingOfferProduct = 0;
        return false;
    }

    Item* item = FindBagItem(bot, data.offer.item);
    if (!item)
    {
        // Gone from the bags: the trade went through.
        DebugErrands(botAI, "offer: accepted");
        data.offer = MasterOffer{};
        return true;
    }

    if (!data.offer.placed)
    {
        if (bot->GetTrader() == master && bot->GetTradeData())
        {
            WorldPacket packet(CMSG_SET_TRADE_ITEM, 3);
            packet << uint8(0) << uint8(item->GetBagSlot()) << uint8(item->GetSlot());
            bot->GetSession()->HandleSetTradeItemOpcode(packet);
            data.offer.placed = true;
            return true;
        }
        if (!data.offer.requested)
        {
            if (bot->GetTrader() || bot->GetDistance(master) > OfferDistance)
                return false;  // busy, or wait for follow to bring the bot close
            botAI->TellMaster("I have " + chat->FormatItem(item->GetTemplate(), item->GetCount()) +
                              " for you, accept the trade");
            WorldPacket packet(CMSG_INITIATE_TRADE, 8);
            packet << master->GetGUID();
            bot->GetSession()->HandleInitiateTradeOpcode(packet);
            data.offer.requested = true;
            data.offer.startedAt = now;
            return true;
        }
        if (!bot->GetTrader() && Elapsed(now, data.offer.startedAt, OfferRequestMs))
            Decline(data, now, "no answer");
        return false;
    }

    if (!bot->GetTrader())
        Decline(data, now, "trade cancelled");
    return false;
}

Item* OfferToMasterAction::FindEntry(uint32 entry)
{
    Item* found = nullptr;
    if (entry)
        ForEachBagItem(bot,
                       [&](Item* i)
                       {
                           if (!found && i->GetEntry() == entry)
                               found = i;
                       });
    return found;
}

void OfferToMasterAction::Decline(ErrandsData& data, uint32 now, char const* why)
{
    data.offerDeclinedAt[data.offer.entry] = now;
    data.offer = MasterOffer{};
    DebugErrands(botAI, std::string("offer: declined (") + why + ")");
}
}  // namespace PlayerbotsPlus
