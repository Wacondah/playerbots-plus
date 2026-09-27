/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_SHARE_PLANNER_H
#define PLAYERBOTS_PLUS_SHARE_PLANNER_H

#include "ErrandPlanner.h"

#include <map>
#include <utility>

// Pure decision logic for the "errands share" strategy.
namespace PlayerbotsPlus
{
// Our own profession bits (the adapter maps core skill ids onto them).
namespace ProfessionBit
{
constexpr uint32_t Tailoring = 1u << 0;
constexpr uint32_t Leatherworking = 1u << 1;
constexpr uint32_t Blacksmithing = 1u << 2;
constexpr uint32_t Engineering = 1u << 3;
constexpr uint32_t Alchemy = 1u << 4;
constexpr uint32_t Enchanting = 1u << 5;
constexpr uint32_t Jewelcrafting = 1u << 6;
constexpr uint32_t Inscription = 1u << 7;
constexpr uint32_t FirstAid = 1u << 8;
constexpr uint32_t Cooking = 1u << 9;
constexpr uint32_t Fishing = 1u << 10;
constexpr uint32_t Primary = Tailoring | Leatherworking | Blacksmithing | Engineering | Alchemy | Enchanting |
                             Jewelcrafting | Inscription;
constexpr uint32_t Secondary = FirstAid | Cooking | Fishing;
}  // namespace ProfessionBit

// 2: a known primary profession consumes the item, 1: only a secondary one, 0: none.
uint8_t TierFor(uint32_t usedBy, uint32_t known);

enum class ShareUsage : uint8_t
{
    Other,
    Equip,
    Replace,
    Quest
};

struct ShareReceiver
{
    uint64_t guid = 0;
    ShareUsage usage = ShareUsage::Other;
    uint8_t tier = 0;
    float gain = 0.f;   // score gain if equipped (Equip/Replace only)
    uint32_t held = 0;  // copies of the item already owned
};

struct ShareItem
{
    uint64_t id = 0;  // item instance GUID
    ShareUsage usage = ShareUsage::Other;
    uint8_t tier = 0;
    std::vector<ShareReceiver> receivers;
    float masterGain = 0.f;       // upgrade for the master (0: none, consumables never)
    bool masterDeclined = false;  // the master refused this item recently
};

struct ShareSnapshot
{
    bool errandsIdle = false;
    std::vector<ShareItem> items;
    uint64_t master = 0;  // the real player, offered upgrades before any bot
};

struct ShareConfig
{
    uint32_t blacklistMs = 60000;
};

struct ShareState
{
    std::map<std::pair<uint64_t, uint64_t>, uint32_t> blacklistedAt;  // (item, receiver)
    std::string lastReason;
};

struct ShareDecision
{
    uint64_t item = 0;
    uint64_t receiver = 0;
    bool toMaster = false;  // an offer through a trade window, not a transfer
    std::string reason;

    bool Acts() const { return item && receiver; }
};

ShareDecision PlanShare(ShareSnapshot const& snap, ShareState& state, ShareConfig const& cfg, uint32_t now);

// Some bot of the group would take the item under the PlanShare rules (distance
// and blacklist ignored). Selling keeps such items; quest items count as wanted.
bool WantedByGroup(ShareItem const& item);

// The receiver could not store the item: skip that pair for a while.
void MarkShareFailed(ShareState& state, uint64_t item, uint64_t receiver, uint32_t now);
}  // namespace PlayerbotsPlus

#endif
