/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "CraftItemAction.h"

#include "CraftDeclinedValue.h"
#include "ErrandsCommon.h"
#include "GroupItems.h"
#include "ItemUsageValue.h"
#include "ObjectMgr.h"
#include "PlayerbotRepository.h"
#include "PlayerbotsPlusConfig.h"
#include "Playerbots.h"
#include "ProfessionCatalog.h"
#include "ReagentIndex.h"
#include "SpellInfo.h"
#include "SpellMgr.h"

#include <algorithm>

namespace PlayerbotsPlus
{
namespace
{
uint32 ProductOf(SpellInfo const* spell)
{
    for (SpellEffectInfo const& effect : spell->Effects)
        if (effect.Effect == SPELL_EFFECT_CREATE_ITEM && effect.ItemType)
            return effect.ItemType;
    return 0;
}

// Equipping or using it: gear upgrades, bigger bags, wanted consumables.
bool GroupUses(ItemUsage usage)
{
    return usage == ITEM_USAGE_EQUIP || usage == ITEM_USAGE_REPLACE || usage == ITEM_USAGE_USE;
}

ItemUsage UsageOf(Player* player, uint32 entry)
{
    PlayerbotAI* ai = GET_PLAYERBOT_AI(player);
    return ai ? ai->GetAiObjectContext()->GetValue<ItemUsage>("item usage", int32(entry))->Get() : ITEM_USAGE_NONE;
}

// Ore a miner smelts as soon as it is at a forge: raw metal (the Mining bit), not the
// alloys' bars; not ore a jewelcrafter of the group could prospect for gems.
bool RawOre(uint32 entry, bool groupProspects)
{
    ItemTemplate const* proto = sObjectMgr->GetItemTemplate(entry);
    return proto && proto->Class == ITEM_CLASS_TRADE_GOODS && proto->SubClass == ITEM_SUBCLASS_METAL_STONE &&
           (ReagentIndex::UsedBy(entry) & ProfessionBit::Mining) && !ReagentIndex::IsCrafted(entry) &&
           !(groupProspects && proto->HasFlag(ITEM_FLAG_IS_PROSPECTABLE));
}
}  // namespace

bool CraftItemAction::isUseful()
{
    ErrandsData& data = AI_VALUE(ErrandsData&, "errands data");
    uint32 const now = getMSTime();
    if (!ErrandsIdle(data, now) || data.OfferBusy() || bot->IsNonMeleeSpellCast(false) ||
        !Elapsed(now, data.lastCraftScanAt, CraftIntervalMs))
        return false;
    data.lastCraftScanAt = now;

    CraftSnapshot const snap = BuildSnapshot(data, now);
    data.craftDecision = PlanCraft(snap, data.craft, CraftConfig{}, now);
    CraftDecision const& d = data.craftDecision;
    if (d.action == CraftAction::Ask)
        if (ItemTemplate const* proto = sObjectMgr->GetItemTemplate(d.product))
            botAI->TellMaster("I can craft " + chat->FormatItem(proto) +
                              " for you. Whisper 'craft yes' or 'craft no'.");
    UpdateShopping(data, snap);
    // Shop: the "buy reagents" errand does the buying.
    return d.action == CraftAction::Craft || d.action == CraftAction::Disenchant;
}

void CraftItemAction::UpdateShopping(ErrandsData& data, CraftSnapshot const& snap)
{
    CraftDecision const& d = data.craftDecision;
    if (d.action != CraftAction::Shop)
    {
        data.shopping = ShoppingList{};
        data.shoppingSpell = data.shoppingProduct = 0;
        return;
    }

    for (RecipeOption const& r : snap.recipes)
        if (r.spell == d.spell)
        {
            ShoppingBudget budget;
            budget.money = bot->GetMoney();
            budget.reserve = ShoppingReserve(bot->GetLevel(), Config().shoppingReservePer10Levels);
            budget.cap = Config().shoppingMaxCopper;
            budget.maxCrafts = Config().shoppingMaxCrafts;
            budget.freeSlots = FreeSlots(bot);
            data.shopping = PlanShopping(r, budget);
        }
    data.shoppingSpell = d.spell;
    data.shoppingProduct = d.product;

    if (d.forMaster && data.shopping.Any() && data.shoppingToldSpell != d.spell)
    {
        data.shoppingToldSpell = d.spell;
        if (ItemTemplate const* proto = sObjectMgr->GetItemTemplate(data.shopping.purchases.front().item))
            botAI->TellMaster("I need a vendor for " + chat->FormatItem(proto));
    }
}

bool CraftItemAction::Execute(Event /*event*/)
{
    ErrandsData& data = AI_VALUE(ErrandsData&, "errands data");
    CraftDecision const d = data.craftDecision;
    if (d.action == CraftAction::Disenchant)
    {
        Item* target = FindBagItem(bot, d.item);
        if (!target)
            return false;
        std::string const what = chat->FormatItem(target->GetTemplate());
        if (!botAI->CastSpell(DisenchantSpell, bot, target))
            return false;
        DebugErrands(botAI, "craft: disenchant " + what);
        return true;
    }
    if (!botAI->CastSpell(d.spell, bot))
        return false;
    if (d.forMaster)
    {
        data.pendingOfferProduct = d.product;
        data.pendingOfferSince = getMSTime();
    }
    DebugErrands(botAI, "craft: " + d.reason);
    return true;
}

bool CraftItemAction::Reagents(SpellInfo const* spell, std::vector<ReagentNeed>& needs)
{
    bool all = true;
    for (uint32 i = 0; i < MAX_SPELL_REAGENTS; ++i)
    {
        if (spell->Reagent[i] <= 0)
            continue;
        ReagentNeed n;
        n.item = uint32(spell->Reagent[i]);
        n.perCraft = spell->ReagentCount[i];
        n.held = bot->GetItemCount(n.item, false);
        n.vendor = ReagentIndex::VendorSells(n.item);
        if (ItemTemplate const* proto = sObjectMgr->GetItemTemplate(n.item))
        {
            n.lotPrice = proto->BuyPrice;
            n.lotSize = std::max<uint32>(proto->BuyCount, 1);
            n.maxStack = std::max<uint32>(proto->GetMaxStackSize(), 1);
        }
        all = all && n.held >= n.perCraft;
        needs.push_back(n);
    }
    return all;
}

bool CraftItemAction::HasTools(SpellInfo const* spell)
{
    for (uint32 totem : spell->Totem)
        if (totem && !bot->HasItemCount(totem, 1, false))
            return false;
    for (uint32 category : spell->TotemCategory)
        if (category && !bot->HasItemTotemCategory(category))
            return false;
    return true;
}

CraftSnapshot CraftItemAction::BuildSnapshot(ErrandsData& data, uint32 now, bool stationOnly)
{
    CraftSnapshot snap;
    snap.errandsIdle = ErrandsIdle(data, now);

    Player* master = RealMaster(botAI);
    std::vector<Player*> group = GroupBots(bot, 0.f);
    snap.hasMaster = master != nullptr;
    bool const groupProspects = std::any_of(group.begin(), group.end(),
                                            [](Player* p) { return p->HasSkill(SKILL_JEWELCRAFTING); });
    group.push_back(bot);
    std::set<uint32> const& declined = AI_VALUE(std::set<uint32>&, "craft declined");

    for (auto const& [spellId, known] : bot->GetSpellMap())
    {
        if (known->State == PLAYERSPELL_REMOVED || !known->Active || !ReagentIndex::RecipeSkill(spellId))
            continue;
        SpellInfo const* spell = sSpellMgr->GetSpellInfo(spellId);
        uint32 const product = spell ? ProductOf(spell) : 0;
        ItemTemplate const* proto = product ? sObjectMgr->GetItemTemplate(product) : nullptr;
        if (!proto || (stationOnly && !spell->RequiresSpellFocus))
            continue;

        RecipeOption r;
        r.spell = spellId;
        r.product = product;
        bool const allHeld = Reagents(spell, r.reagents);
        r.castable = allHeld && botAI->CanCastSpell(spellId, bot, true);
        // Only vendor reagents missing; primary professions only (no cooking salt runs).
        r.buyable = !allHeld && FindProfession(ReagentIndex::RecipeSkill(spellId)) && HasTools(spell) &&
                    std::all_of(r.reagents.begin(), r.reagents.end(),
                                [](ReagentNeed const& n) { return n.held >= n.perCraft || n.vendor; });
        // Everything held but the forge or the anvil: a capital trip goes there.
        r.focus = spell->RequiresSpellFocus;
        r.atFocus = allHeld && r.focus && !r.castable && HasTools(spell) && !bot->HasSpellCooldown(spellId);
        if (!r.castable && !r.buyable && !r.atFocus)
            continue;
        r.smelt = ReagentIndex::RecipeSkill(spellId) == SKILL_MINING &&
                  std::all_of(r.reagents.begin(), r.reagents.end(),
                              [&](ReagentNeed const& n) { return RawOre(n.item, groupProspects); });
        for (uint32 i = 0; i < MAX_SPELL_REAGENTS; ++i)
            if (spell->Reagent[i] > 0)
                if (ItemTemplate const* reagent = sObjectMgr->GetItemTemplate(uint32(spell->Reagent[i])))
                    r.reagentCost += reagent->SellPrice * spell->ReagentCount[i];
        r.skillUp = ItemUsageValue::SpellGivesSkillUp(spellId, bot);
        r.cooldown = std::max(spell->RecoveryTime, spell->CategoryRecoveryTime) >= LongCooldownMs &&
                     !bot->HasSpellCooldown(spellId);

        // Gear: never a second copy while one waits in the group's bags to be shared.
        bool const gear = IsGear(proto);
        bool const copyWaiting = gear && CountInBags(group, product) > 0;
        r.gear = gear;
        for (Player* member : group)
        {
            ItemUsage const usage = UsageOf(member, product);
            if (GroupUses(usage) && !(copyWaiting && usage != ITEM_USAGE_USE))
                r.usefulToGroup = true;
        }
        r.usefulToMaster = master && !copyWaiting && !master->HasItemCount(product, 1, false) &&
                           MasterGain(master, proto, 0) > 0.f;
        r.declined = declined.count(product) > 0;
        snap.recipes.push_back(r);
    }

    if (stationOnly)
        return snap;

    // First item nobody wants (bound: only the bot's own use counts) to disenchant.
    std::vector<Player*> const others(group.begin(), group.end() - 1);  // without the bot
    std::set<uint64> const future = KeptFutureGearIds(bot);
    ForEachBagItem(bot,
                   [&](Item* item)
                   {
                       if (snap.disenchantItem || !CanDisenchant(bot, item->GetTemplate()) ||
                           future.count(item->GetGUID().GetRawValue()))
                           return;
                       ShareItem const described = DescribeForGroup(botAI, item, others);
                       bool const wanted = item->CanBeTraded()
                                               ? WantedByGroup(described)
                                               : (described.usage != ShareUsage::Other);
                       if (!wanted)
                           snap.disenchantItem = item->GetGUID().GetRawValue();
                   });
    return snap;
}

bool CraftAnswerAction::Execute(Event /*event*/)
{
    ErrandsData& data = AI_VALUE(ErrandsData&, "errands data");
    uint32 const product = data.craft.askedProduct;
    if (!AnswerCraft(data.craft, yes))
    {
        botAI->TellMaster("craft: nothing was asked");
        return false;
    }
    if (yes)
    {
        botAI->TellMaster("craft: on it");
        data.lastCraftScanAt = 0;  // craft at the next tick
    }
    else
    {
        AI_VALUE(std::set<uint32>&, "craft declined").insert(product);
        PlayerbotRepository::instance().Save(botAI);
        botAI->TellMaster("craft: I will not offer it again");
    }
    return true;
}
}  // namespace PlayerbotsPlus
