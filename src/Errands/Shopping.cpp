/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "Shopping.h"

#include "Creature.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Professions.h"

#include <algorithm>

namespace PlayerbotsPlus
{
bool SellsShoppingEntry(uint32 entry, ShoppingList const& list)
{
    VendorItemData const* items = sObjectMgr->GetNpcVendorItemList(entry);
    if (!items || !list.Any())
        return false;
    for (uint32 slot = 0; slot < items->GetItemCount(); ++slot)
        if (VendorItem const* item = items->GetItem(slot))
            for (Purchase const& p : list.purchases)
                if (item->item == p.item && !item->maxcount)
                    return true;
    return false;
}

bool SellsShopping(Creature* npc, ShoppingList const& list)
{
    return npc && npc->IsVendor() && SellsShoppingEntry(npc->GetEntry(), list);
}

bool BuyShoppingAt(Player* bot, Creature* npc, ShoppingList const& list)
{
    VendorItemData const* items = npc && npc->IsVendor() ? npc->GetVendorItems() : nullptr;
    bool bought = false;
    for (uint32 slot = 0; items && slot < items->GetItemCount(); ++slot)
    {
        VendorItem const* item = items->GetItem(slot);
        if (!item || item->maxcount)
            continue;
        for (Purchase const& p : list.purchases)
        {
            ItemTemplate const* proto = item->item == p.item ? sObjectMgr->GetItemTemplate(p.item) : nullptr;
            if (!proto)
                continue;
            uint32 const lotSize = std::max<uint32>(proto->BuyCount, 1);
            uint32 const perCall = std::clamp<uint32>(proto->GetMaxStackSize() / lotSize, 1, 255);
            for (uint32 left = p.lots; left > 0;)
            {
                uint32 const lots = std::min(left, perCall);
                if (!bot->HasEnoughMoney(int32(Price(bot, npc, proto->BuyPrice * lots))) ||
                    !bot->BuyItemFromVendorSlot(npc->GetGUID(), slot, p.item, uint8(lots), NULL_BAG, NULL_SLOT))
                    return bought;
                bought = true;
                left -= lots;
            }
        }
    }
    return bought;
}
}  // namespace PlayerbotsPlus
