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
#include "Playerbots.h"
#include "ReagentIndex.h"
#include "SpellInfo.h"
#include "SpellMgr.h"

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
}  // namespace

bool CraftItemAction::isUseful()
{
    ErrandsData& data = AI_VALUE(ErrandsData&, "errands data");
    uint32 const now = getMSTime();
    if (!ErrandsIdle(data, now) || data.OfferBusy() || bot->IsNonMeleeSpellCast(false) ||
        !Elapsed(now, data.lastCraftScanAt, CraftIntervalMs))
        return false;
    data.lastCraftScanAt = now;

    data.craftDecision = PlanCraft(BuildSnapshot(data, now), data.craft, CraftConfig{}, now);
    CraftDecision const& d = data.craftDecision;
    if (d.action == CraftAction::Ask)
        if (ItemTemplate const* proto = sObjectMgr->GetItemTemplate(d.product))
            botAI->TellMaster("I can craft " + chat->FormatItem(proto) +
                              " for you. Whisper 'craft yes' or 'craft no'.");
    return d.action == CraftAction::Craft || d.action == CraftAction::Disenchant;
}

bool CraftItemAction::Execute(Event /*event*/)
{
    ErrandsData& data = AI_VALUE(ErrandsData&, "errands data");
    CraftDecision const d = data.craftDecision;
    if (d.action == CraftAction::Disenchant)
    {
        Item* target = nullptr;
        ForEachBagItem(bot,
                       [&](Item* item)
                       {
                           if (item->GetGUID().GetRawValue() == d.item)
                               target = item;
                       });
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

bool CraftItemAction::HasReagents(SpellInfo const* spell)
{
    for (uint32 i = 0; i < MAX_SPELL_REAGENTS; ++i)
        if (spell->Reagent[i] > 0 && !bot->HasItemCount(uint32(spell->Reagent[i]), spell->ReagentCount[i], false))
            return false;
    return true;
}

CraftSnapshot CraftItemAction::BuildSnapshot(ErrandsData& data, uint32 now)
{
    CraftSnapshot snap;
    snap.errandsIdle = ErrandsIdle(data, now);

    Player* master = RealMaster(botAI);
    std::vector<Player*> group = GroupBots(bot, 0.f);
    group.push_back(bot);
    std::set<uint32> const& declined = AI_VALUE(std::set<uint32>&, "craft declined");

    for (auto const& [spellId, known] : bot->GetSpellMap())
    {
        if (known->State == PLAYERSPELL_REMOVED || !known->Active || !ReagentIndex::RecipeSkill(spellId))
            continue;
        SpellInfo const* spell = sSpellMgr->GetSpellInfo(spellId);
        uint32 const product = spell ? ProductOf(spell) : 0;
        ItemTemplate const* proto = product ? sObjectMgr->GetItemTemplate(product) : nullptr;
        if (!proto || !HasReagents(spell))
            continue;

        RecipeOption r;
        r.spell = spellId;
        r.product = product;
        r.castable = botAI->CanCastSpell(spellId, bot, true);
        if (!r.castable)
            continue;
        for (uint32 i = 0; i < MAX_SPELL_REAGENTS; ++i)
            if (spell->Reagent[i] > 0)
                if (ItemTemplate const* reagent = sObjectMgr->GetItemTemplate(uint32(spell->Reagent[i])))
                    r.reagentCost += reagent->SellPrice * spell->ReagentCount[i];
        r.skillUp = ItemUsageValue::SpellGivesSkillUp(spellId, bot);

        // Gear: never a second copy while one waits in the group's bags to be shared.
        bool const gear = proto->Class == ITEM_CLASS_ARMOR || proto->Class == ITEM_CLASS_WEAPON ||
                          proto->Class == ITEM_CLASS_CONTAINER;
        bool const copyWaiting = gear && CountInBags(group, product) > 0;
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

    // First item nobody wants (bound: only the bot's own use counts) to disenchant.
    std::vector<Player*> const others = GroupBots(bot, 0.f);
    ForEachBagItem(bot,
                   [&](Item* item)
                   {
                       if (snap.disenchantItem || !CanDisenchant(bot, item->GetTemplate()))
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
