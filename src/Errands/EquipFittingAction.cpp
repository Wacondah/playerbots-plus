/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "EquipFittingAction.h"

#include "GroupItems.h"
#include "ItemVisitors.h"
#include "ObjectMgr.h"
#include "Playerbots.h"
#include "RandomPlayerbotMgr.h"

namespace PlayerbotsPlus
{
bool EquipFittingAction::Execute(Event event)
{
    // Same conditions as upstream's packet action.
    if (!sPlayerbotAIConfig.autoEquipUpgradeLoot && !sRandomPlayerbotMgr.IsRandomBot(bot))
        return false;
    std::string const source = event.GetSource();
    if (source == "trade status")
    {
        WorldPacket p(event.getPacket());
        p.rpos(0);
        uint32 status;
        p >> status;
        if (status != TRADE_STATUS_TRADE_ACCEPT)
            return false;
    }
    else if (source == "item push result")
    {
        WorldPacket p(event.getPacket());
        p.rpos(0);
        ObjectGuid playerGuid;
        uint32 received, created, sendChatMessage, itemSlot, itemId;
        uint8 bagSlot;
        p >> playerGuid >> received >> created >> sendChatMessage >> bagSlot >> itemSlot >> itemId;
        ItemTemplate const* item = sObjectMgr->GetItemTemplate(itemId);
        if (!item || item->InventoryType == INVTYPE_NON_EQUIP)
            return false;
    }

    CollectItemsVisitor visitor;
    IterateItems(&visitor, ITERATE_ITEMS_IN_BAGS);
    ItemIds items;
    for (Item* item : visitor.items)
    {
        ItemTemplate const* proto = item ? item->GetTemplate() : nullptr;
        if (!proto || proto->InventoryType == INVTYPE_NON_EQUIP)
            continue;
        std::string param = std::to_string(proto->ItemId);
        if (item->GetItemRandomPropertyId())
            param += "," + std::to_string(item->GetItemRandomPropertyId());
        ItemUsage const usage = FittingUsage(bot, proto, AI_VALUE2(ItemUsage, "item upgrade", param));
        if (usage == ITEM_USAGE_EQUIP || usage == ITEM_USAGE_REPLACE || usage == ITEM_USAGE_BAD_EQUIP)
            items.insert(proto->ItemId);
    }
    EquipItems(items);
    return true;
}

float EquipReplaceMultiplier::GetValue(Action* action)
{
    return action && action->getName() == "equip upgrades packet action" ? 0.0f : 1.0f;
}
}  // namespace PlayerbotsPlus
