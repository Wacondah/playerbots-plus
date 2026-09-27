/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_ERRAND_PLANNER_H
#define PLAYERBOTS_PLUS_ERRAND_PLANNER_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

// Pure decision logic for the "errands" strategy. Standard library only, so it
// builds and runs in unit tests without the core.
namespace PlayerbotsPlus
{
struct Vec3
{
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
};

float Distance(Vec3 const& a, Vec3 const& b);

// Wrap-safe: getMSTime() wraps after ~49 days.
inline bool Elapsed(uint32_t now, uint32_t since, uint32_t duration)
{
    return static_cast<uint32_t>(now - since) >= duration;
}

// Declaration order is the priority order.
enum class ErrandKind : uint8_t
{
    None,
    TurnIn,
    Accept,
    Repair,
    Sell,
    Train,
    TrainClass,
    BuyReagents,
    BuyTool
};

char const* ToString(ErrandKind kind);

struct Candidate
{
    uint64_t id = 0;
    Vec3 pos;
    bool canTurnIn = false;
    bool canAccept = false;
    bool canRepair = false;
    bool canSell = false;
    bool canTrain = false;        // tradeskill trainer teaching an assigned profession
    bool canTrainClass = false;   // class trainer teaching the bot something affordable
    bool canSellTool = false;     // vendor selling a missing tool of an assigned profession
    bool canSellReagent = false;  // vendor selling an item of the craft shopping list
};

struct Snapshot
{
    bool hasMaster = false;
    bool masterSameMap = false;
    bool masterInCombat = false;
    bool masterMounted = false;
    bool masterOnTaxi = false;
    bool masterMoving = false;
    Vec3 masterPos;
    Vec3 botPos;
    bool botInCombat = false;
    bool botNeedsRest = false;
    bool botHasStayOrGuard = false;
    bool inInstance = false;
    bool needsRepair = false;
    bool hasJunk = false;
    uint64_t questFingerprint = 0;
    std::vector<Candidate> candidates;
};

struct PlannerConfig
{
    float radius = 20.f;
    float margin = 5.f;
    uint32_t idleDelayMs = 3000;
    uint32_t timeoutMs = 20000;
    uint32_t blacklistMs = 60000;
    bool inInstances = false;
};

struct ActiveErrand
{
    uint64_t target = 0;
    ErrandKind kind = ErrandKind::None;
    uint32_t startedAt = 0;

    bool IsActive() const { return target != 0; }
};

struct Visit
{
    uint64_t fingerprint = 0;
    uint32_t at = 0;
};

struct ErrandState
{
    bool masterPosKnown = false;
    Vec3 lastMasterPos;
    uint32_t masterStillSince = 0;
    ActiveErrand active;
    std::unordered_map<uint64_t, uint32_t> blacklistedAt;
    std::unordered_map<uint64_t, Visit> visits;
    std::string lastReason;
};

enum class DecisionType : uint8_t
{
    Idle,
    Start,
    Continue,
    Abandon
};

struct Decision
{
    DecisionType type = DecisionType::Idle;
    uint64_t target = 0;
    ErrandKind kind = ErrandKind::None;
    std::string reason;

    bool Acts() const { return type == DecisionType::Start || type == DecisionType::Continue; }
};

// One step of the state machine. Mutates `state` (master idle tracking,
// blacklist expiry, active errand).
Decision Plan(Snapshot const& snap, ErrandState& state, PlannerConfig const& cfg, uint32_t now);

// The action finished its visit: remember the quest state seen at that target.
void MarkDone(ErrandState& state, uint64_t fingerprint, uint32_t now);

// The action could not use the target: blacklist it.
void MarkFailed(ErrandState& state, uint32_t now);
}  // namespace PlayerbotsPlus

#endif
