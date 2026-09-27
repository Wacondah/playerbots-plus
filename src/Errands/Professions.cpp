/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "Professions.h"

#include "Creature.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ProfessionCatalog.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Trainer.h"

#include <algorithm>

namespace PlayerbotsPlus
{
namespace
{
constexpr int MaxTrainingPasses = 4;  // a new rank unlocks recipes: learn again

Trainer::Trainer* TradeskillTrainer(Creature* npc)
{
    if (!npc || !npc->IsTrainer())
        return nullptr;
    Trainer::Trainer* trainer = sObjectMgr->GetTrainer(npc->GetEntry());
    return trainer && trainer->GetTrainerType() == Trainer::Type::Tradeskill ? trainer : nullptr;
}

// Skill raised by a spell (profession ranks), directly or through the spell it teaches.
uint32 SkillStepOf(uint32 spellId, int depth = 0)
{
    SpellInfo const* spell = sSpellMgr->GetSpellInfo(spellId);
    if (!spell || depth > 1)
        return 0;
    for (SpellEffectInfo const& effect : spell->Effects)
    {
        if (effect.Effect == SPELL_EFFECT_SKILL_STEP && effect.MiscValue > 0)
            return uint32(effect.MiscValue);
        if (effect.Effect == SPELL_EFFECT_LEARN_SPELL && effect.TriggerSpell)
            if (uint32 skill = SkillStepOf(effect.TriggerSpell, depth + 1))
                return skill;
    }
    return 0;
}

bool Assigned(std::vector<uint32> const& assigned, uint32 skill)
{
    return std::find(assigned.begin(), assigned.end(), skill) != assigned.end();
}

bool CanLearnSomething(Player* bot, Creature* npc, Trainer::Trainer* trainer)
{
    for (Trainer::Spell const& spell : trainer->GetSpells())
        if (trainer->CanTeachSpell(bot, &spell) && bot->HasEnoughMoney(int32(Price(bot, npc, spell.MoneyCost))))
            return true;
    return false;
}

// Unassigned primary to drop so that `skill` can be learned, 0 if none is needed.
uint32 ToForget(Player* bot, std::vector<uint32> const& assigned, uint32 skill)
{
    return ProfessionToForget(KnownPrimaries(bot), {assigned.begin(), assigned.end()},
                              bot->GetFreePrimaryProfessionPoints(), skill);
}
}  // namespace

uint32 Price(Player* bot, Creature* npc, uint32 cost)
{
    return uint32(cost * bot->GetReputationPriceDiscount(npc));
}

uint32 TrainerSkill(Creature* npc)
{
    Trainer::Trainer* trainer = TradeskillTrainer(npc);
    if (!trainer)
        return 0;
    for (Trainer::Spell const& spell : trainer->GetSpells())
        if (spell.ReqSkillLine)
            return spell.ReqSkillLine;
    for (Trainer::Spell const& spell : trainer->GetSpells())
        if (uint32 skill = SkillStepOf(spell.SpellId))
            return skill;
    return 0;
}

std::vector<std::pair<uint32, uint32>> KnownPrimaries(Player* player)
{
    std::vector<std::pair<uint32, uint32>> known;
    for (ProfessionInfo const& p : PrimaryProfessions())
        if (player->HasSkill(p.skill))
            known.emplace_back(p.skill, player->GetSkillValue(p.skill));
    return known;
}

bool CanTrainAt(Player* bot, Creature* npc, std::vector<uint32> const& assigned)
{
    Trainer::Trainer* trainer = TradeskillTrainer(npc);
    uint32 const skill = TrainerSkill(npc);
    if (!trainer || !skill || !Assigned(assigned, skill) || !trainer->IsTrainerValidForPlayer(bot))
        return false;
    if (!bot->HasSkill(skill) && !bot->GetFreePrimaryProfessionPoints())
        return ToForget(bot, assigned, skill) != 0;
    return CanLearnSomething(bot, npc, trainer);
}

void TrainAt(Player* bot, Creature* npc, std::vector<uint32> const& assigned)
{
    if (!CanTrainAt(bot, npc, assigned))
        return;
    Trainer::Trainer* trainer = TradeskillTrainer(npc);
    uint32 const skill = TrainerSkill(npc);

    if (!bot->HasSkill(skill) && !bot->GetFreePrimaryProfessionPoints())
        if (uint32 forget = ToForget(bot, assigned, skill))
            bot->SetSkill(forget, 0, 0, 0);  // the client's unlearn path: frees the slot

    for (int pass = 0; pass < MaxTrainingPasses; ++pass)
    {
        bool taught = false;
        for (Trainer::Spell const& spell : trainer->GetSpells())
        {
            if (!trainer->CanTeachSpell(bot, &spell) || !bot->HasEnoughMoney(int32(Price(bot, npc, spell.MoneyCost))))
                continue;
            trainer->TeachSpell(npc, bot, spell.SpellId);
            taught = true;
        }
        if (!taught)
            break;
    }
}

bool MissingToolAt(Player* bot, Creature* npc, std::vector<uint32> const& assigned)
{
    VendorItemData const* items = npc && npc->IsVendor() ? npc->GetVendorItems() : nullptr;
    if (!items)
        return false;
    for (uint32 skill : assigned)
    {
        ProfessionInfo const* p = FindProfession(skill);
        if (!p || !p->tool || bot->HasItemCount(p->tool, 1, false))
            continue;
        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(p->tool);
        for (uint32 slot = 0; proto && slot < items->GetItemCount(); ++slot)
            if (VendorItem const* item = items->GetItem(slot))
                if (item->item == p->tool && bot->HasEnoughMoney(int32(Price(bot, npc, proto->BuyPrice))))
                    return true;
    }
    return false;
}

void BuyToolsAt(Player* bot, Creature* npc, std::vector<uint32> const& assigned)
{
    VendorItemData const* items = npc && npc->IsVendor() ? npc->GetVendorItems() : nullptr;
    if (!items)
        return;
    for (uint32 skill : assigned)
    {
        ProfessionInfo const* p = FindProfession(skill);
        if (!p || !p->tool || bot->HasItemCount(p->tool, 1, false))
            continue;
        for (uint32 slot = 0; slot < items->GetItemCount(); ++slot)
            if (VendorItem const* item = items->GetItem(slot))
                if (item->item == p->tool)
                {
                    bot->BuyItemFromVendorSlot(npc->GetGUID(), slot, p->tool, 1, NULL_BAG, NULL_SLOT);
                    break;
                }
    }
}
}  // namespace PlayerbotsPlus
