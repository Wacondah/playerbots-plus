/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "PullRules.h"

#include <map>
#include <mutex>

namespace PlayerbotsPlus
{
bool HoldAllows(HoldFacts const& f, ActionKind kind)
{
    if (f.step == PullStep::None || f.step == PullStep::AwaitingForce || kind == ActionKind::Other || f.attacked)
        return true;
    if (f.puller && (f.step == PullStep::Moving || f.step == PullStep::Returning))
        return true;
    return f.tank && f.mobNearTank;
}

PullEnd CheckEnd(EndFacts const& f)
{
    switch (f.step)
    {
        case PullStep::None:
            return PullEnd::Continue;
        case PullStep::AwaitingForce:
            return f.msSinceStart >= PullForceWaitMs ? PullEnd::Cancel : PullEnd::Continue;
        case PullStep::Moving:
            if (!f.pullerAlive || !f.targetAlive || f.masterOnOtherTarget || f.msSinceStart >= PullReachTimeoutMs)
                return PullEnd::Cancel;
            return PullEnd::Continue;
        case PullStep::Returning:
        case PullStep::Waiting:
            if (!f.pullerAlive)
                return PullEnd::Cancel;
            if (f.msSinceShot >= PullAfterShotMs || (f.anyAttacker && f.attackersAllOnTank) ||
                (!f.targetAlive && !f.anyAttacker))
                return PullEnd::Done;
            return PullEnd::Continue;
    }
    return PullEnd::Continue;
}

namespace PullBoard
{
namespace
{
std::mutex lock;
std::map<uint64_t, PullRun> runs;
std::map<uint64_t, uint64_t> skulls;
}  // namespace

std::optional<PullRun> Get(uint64_t group)
{
    std::lock_guard<std::mutex> guard(lock);
    auto const it = runs.find(group);
    return it == runs.end() ? std::nullopt : std::optional<PullRun>(it->second);
}

void Put(uint64_t group, PullRun const& run)
{
    std::lock_guard<std::mutex> guard(lock);
    runs[group] = run;
}

void Erase(uint64_t group)
{
    std::lock_guard<std::mutex> guard(lock);
    runs.erase(group);
}

uint64_t LastSkull(uint64_t group)
{
    std::lock_guard<std::mutex> guard(lock);
    auto const it = skulls.find(group);
    return it == skulls.end() ? 0 : it->second;
}

void SetLastSkull(uint64_t group, uint64_t skull)
{
    std::lock_guard<std::mutex> guard(lock);
    skulls[group] = skull;
}
}  // namespace PullBoard
}  // namespace PlayerbotsPlus
