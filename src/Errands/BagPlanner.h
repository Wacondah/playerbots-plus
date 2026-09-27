/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_BAG_PLANNER_H
#define PLAYERBOTS_PLUS_BAG_PLANNER_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

// Pure decision logic for the "errands bags" strategy.
namespace PlayerbotsPlus
{
struct BagStack
{
    uint64_t id = 0;  // item instance GUID
    uint32_t entry = 0;
    uint32_t count = 0;
    uint32_t maxStack = 1;
    bool movable = false;  // tradeable and not an item the holder relies on
    uint8_t tier = 0;      // holder's profession tier for the item
};

struct BagMate
{
    uint64_t guid = 0;
    uint32_t freeSlots = 0;
    std::unordered_map<uint32_t, uint32_t> room;  // entry -> free room in its partial stacks
    std::unordered_map<uint32_t, uint8_t> tier;   // entry -> its profession tier
};

struct BagSnapshot
{
    uint32_t freeSlots = 0;
    std::vector<BagStack> stacks;
    std::vector<BagMate> mates;
};

struct BagConfig
{
    uint32_t minFreeSlots = 1;
};

struct BagState
{
    bool warned = false;
    std::string lastReason;
};

struct BagMove
{
    uint64_t item = 0;
    uint64_t receiver = 0;
    bool merge = false;
    bool warn = false;  // tell the master the group is full (once)
    std::string reason;

    bool Acts() const { return item && receiver; }
};

BagMove PlanBags(BagSnapshot const& snap, BagState& state, BagConfig const& cfg);
}  // namespace PlayerbotsPlus

#endif
