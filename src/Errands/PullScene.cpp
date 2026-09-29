/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "PullScene.h"

#include "CellImpl.h"
#include "CreatureGroups.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "PathGenerator.h"
#include "Playerbots.h"
#include "SpellInfo.h"
#include "SpellMgr.h"

#include <cmath>

namespace PlayerbotsPlus
{
namespace
{
constexpr float SceneRadius = 60.f;
constexpr float AssistRadius = 10.f;  // CreatureFamilyAssistanceRadius default
constexpr float FiringRings[] = {0.70f, 0.82f, 0.95f};
constexpr int FiringAngles = 16;
constexpr float HideRings[] = {10.f, 18.f};
constexpr int HideAngles = 6;
constexpr float EyeHeight = 2.f;

RangedWeapon WeaponOf(Player* player)
{
    Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_RANGED);
    ItemTemplate const* proto = item ? item->GetTemplate() : nullptr;
    if (!proto || proto->Class != ITEM_CLASS_WEAPON)
        return RangedWeapon::None;
    switch (proto->SubClass)
    {
        case ITEM_SUBCLASS_WEAPON_BOW:
            return RangedWeapon::Bow;
        case ITEM_SUBCLASS_WEAPON_GUN:
            return RangedWeapon::Gun;
        case ITEM_SUBCLASS_WEAPON_CROSSBOW:
            return RangedWeapon::Crossbow;
        case ITEM_SUBCLASS_WEAPON_THROWN:
            return RangedWeapon::Thrown;
        case ITEM_SUBCLASS_WEAPON_WAND:
            return RangedWeapon::Wand;
        default:
            return RangedWeapon::None;
    }
}

bool Ground(Unit* around, float x, float y, PullPoint& out)
{
    float const z = around->GetMap()->GetHeight(x, y, around->GetPositionZ() + 5.f, true, 30.f);
    if (z <= INVALID_HEIGHT)
        return false;
    out = {x, y, z};
    return true;
}

// Server path; empty when the destination cannot be reached.
PullPath Path(Player* mover, PullPoint const* from, PullPoint const& to)
{
    PathGenerator path(mover);
    bool const ok = from ? path.CalculatePath(from->x, from->y, from->z, to.x, to.y, to.z, false)
                         : path.CalculatePath(to.x, to.y, to.z, false);
    PullPath points;
    if (!ok || !(path.GetPathType() & PATHFIND_NORMAL))
        return points;
    for (G3D::Vector3 const& p : path.GetPath())
        points.push_back({p.x, p.y, p.z});
    return points;
}
}  // namespace

std::string KnownPullSpell(Player* player, float& range)
{
    PlayerbotAI* ai = GET_PLAYERBOT_AI(player);
    if (!ai)
        return "";
    for (std::string const& name : PullSpellNames(player->getClass(), WeaponOf(player)))
    {
        uint32 const id = ai->GetAiObjectContext()->GetValue<uint32>("spell id", name)->Get();
        SpellInfo const* info = id ? sSpellMgr->GetSpellInfo(id) : nullptr;
        if (!info || !player->HasSpell(id))
            continue;
        range = info->GetMaxRange(false);
        if (range > 5.f)
            return name;
    }
    return "";
}

PullPoint GroupCentre(Player* tank)
{
    float x = 0.f, y = 0.f, z = 0.f;
    int n = 0;
    Group* group = tank->GetGroup();
    for (GroupReference* ref = group ? group->GetFirstMember() : nullptr; ref; ref = ref->next())
    {
        Player* m = ref->GetSource();
        if (!m || !m->IsAlive() || m->GetMapId() != tank->GetMapId() || tank->GetDistance(m) > 40.f)
            continue;
        x += m->GetPositionX();
        y += m->GetPositionY();
        z += m->GetPositionZ();
        ++n;
    }
    if (!n)
        return {tank->GetPositionX(), tank->GetPositionY(), tank->GetPositionZ()};
    return {x / n, y / n, z / n};
}

PullScene BuildScene(Player* puller, Unit* target, float range, bool bodyPull)
{
    PullScene scene;
    std::list<Unit*> units;
    Acore::AnyUnfriendlyUnitInObjectRangeCheck check(target, puller, SceneRadius);
    Acore::UnitListSearcher<Acore::AnyUnfriendlyUnitInObjectRangeCheck> searcher(target, units, check);
    Cell::VisitObjects(target, searcher, SceneRadius);

    Creature* targetCreature = target->ToCreature();
    CreatureGroup const* formation = targetCreature ? targetCreature->GetFormation() : nullptr;
    for (Unit* unit : units)
    {
        Creature* creature = unit->ToCreature();
        if (!creature || creature == target || creature->IsInCombat() || !creature->IsHostileTo(puller) ||
            creature->HasReactState(REACT_PASSIVE))
            continue;
        PullMob mob;
        mob.id = creature->GetGUID().GetRawValue();
        mob.pos = {creature->GetPositionX(), creature->GetPositionY(), creature->GetPositionZ()};
        mob.radius = creature->GetAggroRange(puller);
        mob.unavoidable = (formation && creature->GetFormation() == formation) ||
                          (creature->GetFaction() == target->GetFaction() && creature->GetDistance(target) <= AssistRadius);
        scene.mobs.push_back(mob);
        scene.names[mob.id] = creature->GetName();
    }

    std::vector<float> radii;
    if (bodyPull)
        radii.push_back(std::max(1.f, (targetCreature ? targetCreature->GetAggroRange(puller) : 10.f) - 1.f));
    else
        for (float ring : FiringRings)
            radii.push_back(range * ring);

    for (float r : radii)
        for (int a = 0; a < FiringAngles; ++a)
        {
            float const angle = 2.f * float(M_PI) * a / FiringAngles;
            FiringOption option;
            if (!Ground(target, target->GetPositionX() + r * std::cos(angle), target->GetPositionY() + r * std::sin(angle),
                        option.pos))
                continue;
            option.lineOfSight = target->IsWithinLOS(option.pos.x, option.pos.y, option.pos.z + EyeHeight);
            if (option.lineOfSight)
                option.path = Path(puller, nullptr, option.pos);
            scene.firing.push_back(option);
        }
    return scene;
}

std::vector<ReturnOption> BuildReturns(Player* puller, Unit* target, PullScene const& scene,
                                       std::vector<size_t> const& best, PullPoint centre)
{
    std::vector<std::pair<PullPoint, bool>> spots = {{centre, false}};
    for (float r : HideRings)
        for (int a = 0; a < HideAngles; ++a)
        {
            float const angle = 2.f * float(M_PI) * a / HideAngles;
            PullPoint p;
            if (Ground(puller, centre.x + r * std::cos(angle), centre.y + r * std::sin(angle), p) &&
                !target->IsWithinLOS(p.x, p.y, p.z + EyeHeight))
                spots.emplace_back(p, true);
        }

    std::vector<ReturnOption> returns;
    for (size_t i = 0; i < best.size() && i < PullReturnCandidates; ++i)
        for (auto const& [pos, hidden] : spots)
        {
            ReturnOption option;
            option.firing = best[i];
            option.pos = pos;
            option.hidden = hidden;
            option.path = Path(puller, &scene.firing[best[i]].pos, pos);
            returns.push_back(option);
        }
    return returns;
}
}  // namespace PlayerbotsPlus
