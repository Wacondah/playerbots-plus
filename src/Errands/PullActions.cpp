/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "PullActions.h"

#include "AttackAction.h"
#include "Event.h"
#include "GenericSpellActions.h"
#include "Playerbots.h"
#include "PullPlanner.h"
#include "PullScene.h"
#include "SpellInfo.h"
#include "SpellMgr.h"

#include <algorithm>

namespace PlayerbotsPlus
{
namespace
{
constexpr float Arrived = 2.f;
constexpr uint8 SkullIcon = 7;

uint64 GroupKey(Player* bot) { return bot->GetGroup() ? bot->GetGroup()->GetGUID().GetRawValue() : 0; }

std::string Names(std::vector<uint64_t> const& ids, std::map<uint64_t, std::string> const& names)
{
    std::string text;
    for (uint64_t id : ids)
    {
        auto const it = names.find(id);
        text += (text.empty() ? "" : ", ") + (it == names.end() ? std::string("?") : it->second);
    }
    return text;
}

// Units fighting a group member.
std::vector<Unit*> GroupAttackers(Player* bot)
{
    std::vector<Unit*> attackers;
    Group* group = bot->GetGroup();
    for (GroupReference* ref = group ? group->GetFirstMember() : nullptr; ref; ref = ref->next())
        if (Player* m = ref->GetSource())
            for (Unit* a : m->getAttackers())
                if (std::find(attackers.begin(), attackers.end(), a) == attackers.end())
                    attackers.push_back(a);
    return attackers;
}

Player* GroupMember(Player* bot, uint64 guid)
{
    Group* group = bot->GetGroup();
    for (GroupReference* ref = group ? group->GetFirstMember() : nullptr; ref; ref = ref->next())
        if (Player* m = ref->GetSource(); m && m->GetGUID().GetRawValue() == guid)
            return m;
    return nullptr;
}
}  // namespace

bool PullRequestAction::Execute(Event event)
{
    Player* master = event.getOwner() ? event.getOwner() : GetMaster();
    if (!PlayerbotAI::IsTank(bot))
    {
        if (event.getParam() != "party")
            botAI->TellMaster("pull: ask the tank");
        return false;
    }
    Unit* target = master ? master->GetSelectedUnit() : nullptr;
    if (!target)
    {
        botAI->TellMaster("pull: select a target first");
        return false;
    }
    return Request(target);
}

bool PullSkullAction::Execute(Event /*event*/)
{
    Group* group = bot->GetGroup();
    if (!group)
        return false;
    ObjectGuid const skull = group->GetTargetIcon(SkullIcon);
    PullBoard::SetLastSkull(GroupKey(bot), skull.GetRawValue());
    Unit* target = botAI->GetUnit(skull);
    return target && Request(target);
}

bool PullRequestAction::Request(Unit* target)
{
    uint64 const key = GroupKey(bot);
    if (!key || PullBoard::Get(key))
    {
        botAI->TellMaster("pull: already pulling");
        return false;
    }
    if (!target->IsAlive() || target->IsInCombat() || !target->ToCreature() || target->GetMapId() != bot->GetMapId() ||
        bot->GetDistance(target) > 100.f)
    {
        botAI->TellMaster("pull: cannot pull that");
        return false;
    }

    // Puller: facts of every living bot of the group on the map.
    std::vector<PullerFacts> facts;
    std::map<uint64_t, std::pair<std::string, float>> spells;
    Group* group = bot->GetGroup();
    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* m = ref->GetSource();
        if (!m || !m->IsAlive() || m->GetMapId() != bot->GetMapId() || !GET_PLAYERBOT_AI(m))
            continue;
        float range = 0.f;
        std::string const spell = KnownPullSpell(m, range);
        PullerFacts f;
        f.id = m->GetGUID().GetRawValue();
        f.tank = m == bot;
        f.healer = PlayerbotAI::IsHeal(m);
        f.ranged = PlayerbotAI::IsRanged(m);
        f.hasPullSpell = !spell.empty();
        f.distance = m->GetDistance(target);
        facts.push_back(f);
        spells[f.id] = {spell, range};
    }
    PullerChoice const choice = ChoosePuller(facts);
    Player* puller = choice.found ? GroupMember(bot, choice.id) : nullptr;
    if (!puller)
        return false;

    auto const& [spell, range] = spells[choice.id];
    PullScene const scene = BuildScene(puller, target, range, choice.bodyPull);
    std::vector<size_t> const ranked = RankFiring(scene.firing, scene.mobs, PullMargin);
    std::vector<ReturnOption> const returns = BuildReturns(puller, target, scene, ranked, GroupCentre(bot));
    PullRoute const route = ChooseRoute(scene.firing, returns, scene.mobs, PullMargin);
    if (!route.found)
    {
        botAI->TellMaster("pull: no spot to pull " + target->GetName() + " from");
        return false;
    }

    PullRun run;
    run.tank = bot->GetGUID().GetRawValue();
    run.puller = choice.id;
    run.target = target->GetGUID().GetRawValue();
    run.mapId = bot->GetMapId();
    run.bodyPull = choice.bodyPull;
    run.spell = spell;
    run.firing = route.firingPos;
    run.hide = route.hidePos;
    run.startedAt = getMSTime();

    if (!route.unavoidable.empty())
        botAI->TellMaster("pull: " + target->GetName() + " comes with " + std::to_string(route.unavoidable.size()) +
                          " friends");
    if (!route.woken.empty())
    {
        run.step = PullStep::AwaitingForce;
        PullBoard::Put(key, run);
        botAI->TellMaster("pull: pulling " + target->GetName() + " wakes " + std::to_string(route.woken.size()) +
                          " more: " + Names(route.woken, scene.names) + ". Whisper 'pull force'.");
        return true;
    }
    run.step = PullStep::Moving;
    PullBoard::Put(key, run);
    botAI->TellMaster("pull: " + puller->GetName() + " pulls " + target->GetName() +
                      (choice.bodyPull ? " (on foot)" : ""));
    return true;
}

bool PullForceAction::Execute(Event /*event*/)
{
    uint64 const key = GroupKey(bot);
    std::optional<PullRun> run = key ? PullBoard::Get(key) : std::nullopt;
    if (!run || run->step != PullStep::AwaitingForce || run->tank != bot->GetGUID().GetRawValue())
        return false;
    run->step = PullStep::Moving;
    run->startedAt = getMSTime();
    PullBoard::Put(key, *run);
    botAI->TellMaster("pull: going");
    return true;
}

bool PullCancelAction::Execute(Event /*event*/)
{
    uint64 const key = GroupKey(bot);
    std::optional<PullRun> run = key ? PullBoard::Get(key) : std::nullopt;
    if (!run || run->tank != bot->GetGUID().GetRawValue())
        return false;
    PullBoard::Erase(key);
    botAI->TellMaster("pull: cancelled");
    return true;
}

bool PullStepAction::isUseful()
{
    uint64 const key = GroupKey(bot);
    std::optional<PullRun> run = key ? PullBoard::Get(key) : std::nullopt;
    if (!run)
        return false;
    uint64 const me = bot->GetGUID().GetRawValue();
    if (run->tank == me)
    {
        Supervise(key, *run);
        run = PullBoard::Get(key);
        if (!run)
            return false;
    }
    return run->puller == me && bot->IsAlive() && (run->step == PullStep::Moving || run->step == PullStep::Returning);
}

void PullStepAction::Supervise(uint64 key, PullRun const& run)
{
    uint32 const now = getMSTime();
    Player* puller = GroupMember(bot, run.puller);
    Unit* target = botAI->GetUnit(ObjectGuid(run.target));
    Player* master = GetMaster();
    std::vector<Unit*> const attackers = GroupAttackers(bot);

    EndFacts f;
    f.step = run.step;
    f.msSinceStart = getMSTimeDiff(run.startedAt, now);
    f.msSinceShot = run.shotAt ? getMSTimeDiff(run.shotAt, now) : 0;
    f.pullerAlive = puller && puller->IsAlive() && puller->GetMapId() == run.mapId;
    f.targetAlive = target && target->IsAlive();
    f.masterOnOtherTarget = master && master->GetVictim() && master->GetVictim() != target;
    f.anyAttacker = !attackers.empty();
    f.attackersAllOnTank =
        std::all_of(attackers.begin(), attackers.end(), [&](Unit* a) { return a->GetVictim() == bot; });

    switch (CheckEnd(f))
    {
        case PullEnd::Continue:
            return;
        case PullEnd::Done:
            PullBoard::Erase(key);
            return;
        case PullEnd::Cancel:
            PullBoard::Erase(key);
            botAI->TellMaster(run.step == PullStep::AwaitingForce ? "pull: dropped" : "pull: cancelled");
            return;
    }
}

bool PullStepAction::Execute(Event /*event*/)
{
    uint64 const key = GroupKey(bot);
    std::optional<PullRun> run = key ? PullBoard::Get(key) : std::nullopt;
    if (!run)
        return false;
    if (run->step == PullStep::Moving)
    {
        if (bot->GetDistance(run->firing.x, run->firing.y, run->firing.z) > Arrived)
            return MoveTo(run->mapId, run->firing.x, run->firing.y, run->firing.z, false, false, false, false,
                          MovementPriority::MOVEMENT_FORCED);
        if (!run->bodyPull)
        {
            Unit* target = botAI->GetUnit(ObjectGuid(run->target));
            if (!target || !botAI->CastSpell(run->spell, target))
                return false;
        }
        run->step = PullStep::Returning;
        run->shotAt = getMSTime();
        PullBoard::Put(key, *run);
        return true;
    }
    if (bot->GetDistance(run->hide.x, run->hide.y, run->hide.z) > Arrived)
        return MoveTo(run->mapId, run->hide.x, run->hide.y, run->hide.z, false, false, false, false,
                      MovementPriority::MOVEMENT_FORCED);
    run->step = PullStep::Waiting;
    PullBoard::Put(key, *run);
    return true;
}

float PullHoldMultiplier::GetValue(Action* action)
{
    uint64 const key = action ? GroupKey(bot) : 0;
    std::optional<PullRun> run = key ? PullBoard::Get(key) : std::nullopt;
    if (!run)
        return 1.0f;

    ActionKind kind = ActionKind::Other;
    if (action->getName().rfind("errands pull", 0) == 0)
        kind = ActionKind::Other;
    else if (dynamic_cast<AttackAction*>(action))
        kind = ActionKind::Attack;
    else if (dynamic_cast<MovementAction*>(action))
        kind = ActionKind::Move;
    else if (CastSpellAction* cast = dynamic_cast<CastSpellAction*>(action))
    {
        uint32 const id = AI_VALUE2(uint32, "spell id", cast->getSpell());
        SpellInfo const* info = id ? sSpellMgr->GetSpellInfo(id) : nullptr;
        if (info && !info->IsPositive())
            kind = ActionKind::OffensiveSpell;
    }

    uint64 const me = bot->GetGUID().GetRawValue();
    HoldFacts f;
    f.step = run->step;
    f.puller = run->puller == me;
    f.tank = run->tank == me;
    f.attacked = !bot->getAttackers().empty();
    if (f.tank)
        for (Unit* a : GroupAttackers(bot))
            f.mobNearTank = f.mobNearTank || bot->GetDistance(a) <= PullNearTank;
    return HoldAllows(f, kind) ? 1.0f : 0.0f;
}
}  // namespace PlayerbotsPlus
