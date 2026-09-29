/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_PULL_RULES_H
#define PLAYERBOTS_PLUS_PULL_RULES_H

#include "PullPlanner.h"

#include <cstdint>
#include <optional>
#include <string>

// Pure rules of a running pull (who may act, when it ends) and the in-memory pull board.
namespace PlayerbotsPlus
{
enum class PullStep : uint8_t
{
    None,
    AwaitingForce,  // announced with extra mobs, waiting for "pull force"
    Moving,         // puller walking to the firing spot
    Returning,      // shot fired, puller walking to the hiding spot
    Waiting         // puller hidden, the tank picks the mobs up
};

enum class ActionKind : uint8_t
{
    Move,
    Attack,
    OffensiveSpell,
    Other
};

enum class PullEnd : uint8_t
{
    Continue,
    Done,
    Cancel
};

struct HoldFacts
{
    PullStep step = PullStep::None;
    bool puller = false;
    bool tank = false;
    bool attacked = false;     // a mob is attacking this bot
    bool mobNearTank = false;  // a mob fighting the group is within PullNearTank of the tank
};

struct EndFacts
{
    PullStep step = PullStep::None;
    uint32_t msSinceStart = 0;
    uint32_t msSinceShot = 0;
    bool pullerAlive = true;
    bool targetAlive = true;
    bool masterOnOtherTarget = false;
    bool anyAttacker = false;         // a mob fights the group
    bool attackersAllOnTank = false;  // every such mob targets the tank
};

struct PullRun
{
    PullStep step = PullStep::None;
    uint64_t tank = 0, puller = 0, target = 0;
    uint32_t mapId = 0;
    bool bodyPull = false;
    std::string spell;
    PullPoint firing, hide;
    uint32_t startedAt = 0, shotAt = 0;  // getMSTime()
};

constexpr uint32_t PullForceWaitMs = 30000;
constexpr uint32_t PullReachTimeoutMs = 30000;
constexpr uint32_t PullAfterShotMs = 20000;
constexpr float PullNearTank = 10.f;

// May a bot run an action of that kind during the pull.
bool HoldAllows(HoldFacts const& facts, ActionKind kind);

PullEnd CheckEnd(EndFacts const& facts);

// One pull per group (group GUID raw value); thread-safe, copies in and out.
namespace PullBoard
{
std::optional<PullRun> Get(uint64_t group);
void Put(uint64_t group, PullRun const& run);
void Erase(uint64_t group);
uint64_t LastSkull(uint64_t group);
void SetLastSkull(uint64_t group, uint64_t skull);
}  // namespace PullBoard
}  // namespace PlayerbotsPlus

#endif
