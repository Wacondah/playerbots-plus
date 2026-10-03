/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "GatherNodeAction.h"

#include "CellImpl.h"
#include "ErrandsCommon.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Group.h"
#include "GroupItems.h"
#include "LootAction.h"
#include "LootObjectStack.h"
#include "NearestGameObjects.h"
#include "Playerbots.h"
#include "PlayerbotsPlusConfig.h"

#include <algorithm>

namespace PlayerbotsPlus
{
namespace
{
// Skill and required value of a node's lock; skill 0 when it is not a gathering lock.
std::pair<uint32, uint32> NodeLock(GameObject* go)
{
    LockEntry const* lock = sLockStore.LookupEntry(go->GetGOInfo()->GetLockId());
    for (uint8 i = 0; lock && i < MAX_LOCK_CASE; ++i)
        if (lock->Type[i] == LOCK_KEY_SKILL)
            return {uint32(SkillByLockType(LockType(lock->Index[i]))), std::max<uint32>(1, lock->Skill[i])};
    return {0, 0};
}

bool HasGathering(Player* bot) { return bot->HasSkill(SKILL_MINING) || bot->HasSkill(SKILL_HERBALISM); }

// A hostile next to the node, whatever the bot can see from where it stands (a mine tunnel
// hides the kobolds by the vein). Triggers and non-attackable dummies do not count.
bool Guarded(Player* bot, GameObject* go)
{
    std::list<Unit*> near;
    Acore::AnyUnfriendlyUnitInObjectRangeCheck check(go, bot, GatherGuardRadius);
    Acore::UnitListSearcher<Acore::AnyUnfriendlyUnitInObjectRangeCheck> searcher(go, near, check);
    Cell::VisitObjects(go, searcher, GatherGuardRadius);
    return std::any_of(near.begin(), near.end(),
                       [&](Unit* u)
                       {
                           Creature* c = u->ToCreature();
                           return bot->IsHostileTo(u) && !u->HasUnitFlag(UNIT_FLAG_NOT_SELECTABLE) &&
                                  !u->HasUnitFlag(UNIT_FLAG_NON_ATTACKABLE) && !u->HasUnitFlag(UNIT_FLAG_IMMUNE_TO_PC) &&
                                  !(c && (c->IsTrigger() || c->IsTotem()));
                       });
}
}  // namespace

bool GatherNodeAction::isUseful()
{
    ErrandsData& data = AI_VALUE(ErrandsData&, "errands data");
    Player* master = RealMaster(botAI);
    if (!HasGathering(bot) || !master)
    {
        data.gather.target = 0;  // no stale claim for the other bots
        return false;
    }
    uint32 const now = getMSTime();

    GatherSnapshot snap;
    snap.idle = DetourIdle(data, now) && !bot->IsInCombat();
    snap.bagsFull = FreeSlots(bot) == 0;
    // mod-playerbots' own loot policy (LootAction::isUseful): no looting in free-for-all groups.
    Group* group = bot->GetGroup();
    snap.lootAllowed = botAI->HasStrategy("loot", BOT_STATE_NON_COMBAT) &&
                       (sPlayerbotAIConfig.freeMethodLoot || !group || group->GetLootMethod() != FREE_FOR_ALL);
    snap.masterPos = {master->GetPositionX(), master->GetPositionY(), master->GetPositionZ()};
    snap.botPos = {bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ()};
    if (snap.idle && !snap.bagsFull && snap.lootAllowed)
    {
        if (!data.gatherScannedAt || Elapsed(now, data.gatherScannedAt, GatherScanIntervalMs))
        {
            data.gatherNodes = ScanNodes(master);
            data.gatherScannedAt = now;
        }
        snap.nodes = data.gatherNodes;
    }
    else
        data.gatherScannedAt = 0;

    data.gatherDecision = PlanGather(snap, data.gather, GatherSettings(), now);
    if (data.gatherDecision.type == DecisionType::Start || data.gatherDecision.type == DecisionType::Abandon)
        DebugErrands(botAI, "gather: " + data.gatherDecision.reason);
    return data.gatherDecision.Acts();
}

bool GatherNodeAction::Execute(Event /*event*/)
{
    ErrandsData& data = AI_VALUE(ErrandsData&, "errands data");
    ObjectGuid const guid(data.gatherDecision.target);
    GameObject* go = botAI->GetGameObject(guid);
    if (!go)
        return false;
    // Keep the tick while casting or looting: follow would cancel the cast.
    if (bot->IsNonMeleeSpellCast(false) || bot->GetLootGUID() == guid)
        return true;
    if (bot->GetDistance(go) > INTERACTION_DISTANCE - 2.0f)
    {
        if (!MoveNear(go, sPlayerbotAIConfig.contactDistance))
            MoveTo(go->GetMapId(), go->GetPositionX(), go->GetPositionY(), go->GetPositionZ());
        return true;
    }

    LootObject loot(bot, guid);
    if (loot.IsEmpty() || !loot.IsLootPossible(bot))
    {
        MarkGatherFailed(data.gather, getMSTime(), GatherSettings());
        return false;
    }
    AI_VALUE(LootObjectStack*, "available loot")->Add(guid);
    context->GetValue<LootObject>("loot target")->Set(loot);
    OpenLootAction open(botAI);
    open.Execute(Event());
    return true;
}

std::vector<GatherNode> GatherNodeAction::ScanNodes(Player* master)
{
    std::vector<GatherNode> nodes;
    float const radius = GatherSettings().radius;
    std::list<GameObject*> gos;
    AnyGameObjectInObjectRangeCheck check(master, radius);
    Acore::GameObjectListSearcher<AnyGameObjectInObjectRangeCheck> searcher(master, gos, check);
    Cell::VisitObjects(master, searcher, radius);

    bool const pick = bot->HasItemTotemCategory(TC_MINING_PICK);
    for (GameObject* go : gos)
    {
        if (go->GetGoType() != GAMEOBJECT_TYPE_CHEST || go->GetGoState() != GO_STATE_READY ||
            go->getLootState() != GO_READY || go->HasFlag(GAMEOBJECT_FLAGS, GO_FLAG_NOT_SELECTABLE | GO_FLAG_IN_USE))
            continue;
        if (go->HasFlag(GAMEOBJECT_FLAGS, GO_FLAG_INTERACT_COND) && !go->ActivateToQuest(bot))
            continue;
        auto const [skill, required] = NodeLock(go);
        if (skill != SKILL_MINING && skill != SKILL_HERBALISM)
            continue;

        GatherNode n;
        n.id = go->GetGUID().GetRawValue();
        n.pos = {go->GetPositionX(), go->GetPositionY(), go->GetPositionZ()};
        n.gatherable = CanGather(skill, bot->HasSkill(skill) ? bot->GetSkillValue(skill) : 0, required, pick);
        if (!n.gatherable)
            continue;
        // mod-playerbots' own view of the node (its pick list, quest-only loot): what it refuses
        // on arrival is not worth the walk.
        LootObject loot(bot, go->GetGUID());
        if (loot.IsEmpty() || loot.skillId != skill || (loot.reqItem && !bot->HasItemCount(loot.reqItem, 1)))
            continue;
        n.contested = ClaimedByOther(master, go, skill);
        n.guarded = Guarded(bot, go);
        nodes.push_back(n);
    }
    return nodes;
}

// Another bot of the group goes for it, or the master (who has the skill) stands at it.
bool GatherNodeAction::ClaimedByOther(Player* master, GameObject* go, uint32 skill)
{
    if (master->HasSkill(skill) && master->GetDistance(go) <= INTERACTION_DISTANCE)
        return true;
    uint64 const id = go->GetGUID().GetRawValue();
    uint32 const now = getMSTime();
    uint32 const timeout = GatherSettings().timeoutMs;
    for (Player* mate : GroupBots(bot, 0.f))
    {
        PlayerbotAI* ai = GET_PLAYERBOT_AI(mate);
        if (!ai || !ai->HasStrategy("errands", BOT_STATE_NON_COMBAT))
            continue;
        // A claim older than the mate's own timeout is one it has dropped (or stopped looking at).
        GatherState const& gather = ai->GetAiObjectContext()->GetValue<ErrandsData&>("errands data")->Get().gather;
        if (gather.target == id && !Elapsed(now, gather.startedAt, timeout))
            return true;
    }
    return false;
}
}  // namespace PlayerbotsPlus
