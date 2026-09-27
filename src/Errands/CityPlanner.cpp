/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "CityPlanner.h"

#include <utility>

namespace PlayerbotsPlus
{
namespace
{
CityDecision Decide(CityState& state, CityStep step, std::string reason, CityStop const* stop = nullptr)
{
    state.lastReason = reason;
    CityDecision d;
    d.step = step;
    d.reason = std::move(reason);
    if (stop)
    {
        d.stop = stop->id;
        d.pos = stop->pos;
    }
    return d;
}

CityStop const* Find(CitySnapshot const& snap, uint64_t id)
{
    for (CityStop const& s : snap.stops)
        if (s.id == id)
            return &s;
    return nullptr;
}

// Nearest stop not visited this trip that covers a remaining need.
CityStop const* Next(CitySnapshot const& snap, CityState const& state)
{
    CityStop const* best = nullptr;
    for (CityStop const& s : snap.stops)
        if ((s.covers & snap.needs) && !state.done.count(s.id) &&
            (!best || Distance(s.pos, snap.botPos) < Distance(best->pos, snap.botPos)))
            best = &s;
    return best;
}

CityDecision Finish(CityState& state, uint32_t needs, uint32_t now, CityStep step, std::string reason)
{
    state.active = false;
    state.current = 0;
    state.ended = true;
    state.endedAt = now;
    state.endedNeeds = needs;
    return Decide(state, step, std::move(reason));
}
}  // namespace

CityDecision PlanCity(CitySnapshot const& snap, CityState& state, CityConfig const& cfg, uint32_t now)
{
    if (snap.masterMoving || !state.masterMovedAt)
        state.masterMovedAt = now;

    if (state.active)
    {
        if (!snap.masterInCapital || snap.zone != state.zone || snap.masterOnTaxi || snap.masterInCombat ||
            snap.inInstance)
            return Finish(state, snap.needs, now, CityStep::Abort, "city: master left");
        if (Elapsed(now, state.startedAt, cfg.timeoutMs))
            return Finish(state, snap.needs, now, CityStep::End, "city: timeout");

        if (state.current)
        {
            CityStop const* stop = Find(snap, state.current);
            if (stop && snap.atStop)
                return Decide(state, CityStep::Visit, "city: visit", stop);
            if (!stop || snap.stopMissing || Elapsed(now, state.currentSince, cfg.reachMs))
            {
                state.done.insert(state.current);  // skipped for this trip
                state.current = 0;
            }
            else
                return Decide(state, CityStep::GoTo, "city: going", stop);
        }

        if (CityStop const* next = Next(snap, state))
        {
            state.current = next->id;
            state.currentSince = now;
            return Decide(state, CityStep::GoTo, "city: going", next);
        }
        return Finish(state, snap.needs, now, CityStep::End, "city: done");
    }

    bool const busy = snap.masterInCombat || snap.masterMounted || snap.masterOnTaxi || snap.inInstance ||
                      snap.botInCombat;
    if (!snap.masterInCapital || busy || (!snap.autoEnabled && !snap.requested))
        return Decide(state, CityStep::None, "city: -");
    if (!snap.requested && !Elapsed(now, state.masterMovedAt, cfg.idleDelayMs))
        return Decide(state, CityStep::None, "city: waiting");
    CityStop const* first = Next(snap, state);
    if (!first)
        return Decide(state, CityStep::None, "city: nothing to do");
    if (!snap.requested && state.ended && state.endedNeeds == snap.needs && !Elapsed(now, state.endedAt, cfg.cooldownMs))
        return Decide(state, CityStep::None, "city: cooldown");

    state.active = true;
    state.zone = snap.zone;
    state.startedAt = now;
    state.done.clear();
    state.stopsPlanned = CountCityStops(snap);
    state.current = first->id;
    state.currentSince = now;
    return Decide(state, CityStep::GoTo, "city: going", first);
}

void MarkCityVisited(CityState& state)
{
    if (state.current)
        state.done.insert(state.current);
    state.current = 0;
}

uint32_t CountCityStops(CitySnapshot const& snap)
{
    uint32_t count = 0;
    for (CityStop const& s : snap.stops)
        if (s.covers & snap.needs)
            ++count;
    return count;
}
}  // namespace PlayerbotsPlus
