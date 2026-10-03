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

// Nearest stop not visited this trip that covers a remaining need; forges first, so the
// bars are smelted before the anvil.
CityStop const* Next(CitySnapshot const& snap, CityState const& state)
{
    auto nearest = [&](uint32_t needs)
    {
        CityStop const* best = nullptr;
        for (CityStop const& s : snap.stops)
            if ((s.covers & needs) && !state.done.count(s.id) &&
                (!best || Distance(s.pos, snap.botPos) < Distance(best->pos, snap.botPos)))
                best = &s;
        return best;
    };
    if (snap.needs & CityNeed::Forge)
        if (CityStop const* forge = nearest(CityNeed::Forge))
            return forge;
    return nearest(snap.needs);
}

// Closes the work time at the current station.
void StopWorking(CityState& state, uint32_t now)
{
    if (state.visitSince && now)
        state.workedMs += now - state.visitSince;
    state.visitSince = 0;
}

CityDecision Finish(CityState& state, uint32_t needs, uint32_t now, CityStep step, std::string reason)
{
    StopWorking(state, now);
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
        uint32_t const working = state.visitSince ? now - state.visitSince : 0;
        if (Elapsed(now, state.startedAt, cfg.timeoutMs + state.workedMs + working))
            return Finish(state, snap.needs, now, CityStep::End, "city: timeout");

        if (state.current)
        {
            CityStop const* stop = Find(snap, state.current);
            bool const station = IsStationStop(state.current);
            if (stop && snap.atStop)
            {
                if (!station)
                    return Decide(state, CityStep::Visit, "city: visit", stop);
                if (!state.visitSince)
                    state.visitSince = now;
                if (!Elapsed(now, state.visitSince, cfg.workMs))
                    return Decide(state, CityStep::Visit, "city: work", stop);
                StopWorking(state, now);  // work cap: leave the rest for the next trip
                state.done.insert(state.current);
                state.current = 0;
            }
            else if (station)
                StopWorking(state, now);
        }
        if (state.current)
        {
            CityStop const* stop = Find(snap, state.current);
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
    state.visitSince = 0;
    state.workedMs = 0;
    state.stopsPlanned = CountCityStops(snap);
    state.current = first->id;
    state.currentSince = now;
    return Decide(state, CityStep::GoTo, "city: going", first);
}

void MarkCityVisited(CityState& state, uint32_t now)
{
    StopWorking(state, now);
    if (state.current)
        state.done.insert(state.current);
    state.current = 0;
}

uint32_t FocusNeed(uint32_t focus)
{
    switch (focus)
    {
        case 1:
            return CityNeed::Anvil;
        case 3:
            return CityNeed::Forge;
        default:
            return 0;
    }
}

uint32_t StationNeeds(CraftSnapshot const& snap, CraftState const& state)
{
    uint32_t needs = 0;
    for (RecipeOption const& r : snap.recipes)
        if (r.focus && (r.atFocus || r.castable) &&
            (r.smelt || r.spell == state.approvedSpell || r.usefulToGroup || r.cooldown || r.skillUp))
            needs |= FocusNeed(r.focus);
    return needs;
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
