/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "ReagentIndex.h"

#include "DBCStores.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "SharePlanner.h"
#include "SpellInfo.h"
#include "SpellMgr.h"

#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace PlayerbotsPlus
{
namespace
{
std::pair<uint32, uint32> const Professions[] = {
    {SKILL_TAILORING, ProfessionBit::Tailoring},         {SKILL_LEATHERWORKING, ProfessionBit::Leatherworking},
    {SKILL_BLACKSMITHING, ProfessionBit::Blacksmithing}, {SKILL_ENGINEERING, ProfessionBit::Engineering},
    {SKILL_ALCHEMY, ProfessionBit::Alchemy},             {SKILL_ENCHANTING, ProfessionBit::Enchanting},
    {SKILL_JEWELCRAFTING, ProfessionBit::Jewelcrafting}, {SKILL_INSCRIPTION, ProfessionBit::Inscription},
    {SKILL_FIRST_AID, ProfessionBit::FirstAid},          {SKILL_COOKING, ProfessionBit::Cooking},
    {SKILL_FISHING, ProfessionBit::Fishing},
};

std::unordered_map<uint32, uint32>& Index()
{
    static std::unordered_map<uint32, uint32> index;
    return index;
}

std::unordered_map<uint32, uint32>& Recipes()
{
    static std::unordered_map<uint32, uint32> recipes;  // spell -> skill
    return recipes;
}

std::unordered_set<uint32>& VendorItems()
{
    static std::unordered_set<uint32> items;  // sold without a supply limit
    return items;
}

std::unordered_set<uint32>& Crafted()
{
    static std::unordered_set<uint32> crafted;
    return crafted;
}

uint32 BitFor(uint32 skill)
{
    for (auto const& [id, bit] : Professions)
        if (id == skill)
            return bit;
    return 0;
}
}  // namespace

void ReagentIndex::Build()
{
    auto& index = Index();
    auto& crafted = Crafted();
    index.clear();
    crafted.clear();
    Recipes().clear();
    auto& vendor = VendorItems();
    vendor.clear();
    for (auto const& [entry, creature] : *sObjectMgr->GetCreatureTemplates())
        if (VendorItemData const* items = sObjectMgr->GetNpcVendorItemList(entry))
            for (uint32 slot = 0; slot < items->GetItemCount(); ++slot)
                if (VendorItem const* item = items->GetItem(slot))
                    if (!item->maxcount)
                        vendor.insert(item->item);
    for (uint32 i = 0; i < sSkillLineAbilityStore.GetNumRows(); ++i)
    {
        SkillLineAbilityEntry const* entry = sSkillLineAbilityStore.LookupEntry(i);
        uint32 const bit = entry ? BitFor(entry->SkillLine) : 0;
        if (!bit)
            continue;
        Recipes()[entry->Spell] = entry->SkillLine;
        SpellInfo const* spell = sSpellMgr->GetSpellInfo(entry->Spell);
        if (!spell)
            continue;
        for (int32 reagent : spell->Reagent)
            if (reagent > 0)
                index[uint32(reagent)] |= bit;
        for (SpellEffectInfo const& effect : spell->Effects)
            if (effect.Effect == SPELL_EFFECT_CREATE_ITEM && effect.ItemType)
                crafted.insert(effect.ItemType);
    }
}

uint32 ReagentIndex::RecipeSkill(uint32 spellId)
{
    auto const it = Recipes().find(spellId);
    return it == Recipes().end() ? 0 : it->second;
}

bool ReagentIndex::VendorSells(uint32 itemId)
{
    return VendorItems().count(itemId) > 0;
}

bool ReagentIndex::IsCrafted(uint32 itemId)
{
    return Crafted().count(itemId) > 0;
}

uint32 ReagentIndex::UsedBy(uint32 itemId)
{
    auto const it = Index().find(itemId);
    return it == Index().end() ? 0 : it->second;
}

uint32 ReagentIndex::Known(Player* player)
{
    uint32 mask = 0;
    for (auto const& [skill, bit] : Professions)
        if (player->HasSkill(skill))
            mask |= bit;
    return mask;
}
}  // namespace PlayerbotsPlus
