/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_LEVEL_UP_RULES_H
#define PLAYERBOTS_PLUS_LEVEL_UP_RULES_H

#include <cstdint>

// Pure rules for "errands levelup", "errands quests", "errands loot" and "errands revive".
namespace PlayerbotsPlus
{
enum class TalentSource : uint8_t
{
    None,     // no free point
    Stored,   // the premade spec the master picked with "talents spec <name>"
    Current,  // keep filling the tree the bot already spent points in
    Ask       // nothing to go by: ask the master
};

TalentSource PickTalentSource(uint32_t freePoints, bool hasStoredSpec, uint32_t spentPoints);

struct QuestShareCheck
{
    bool sharable = false;       // the quest may be shared at all
    bool targetIsBot = false;    // never the master: no quest window for them
    bool inRange = false;
    bool targetCanTake = false;  // requirements met, log room, neither held nor done
};

bool ShouldShareQuest(QuestShareCheck const& check);

struct LootFacts
{
    bool gearUpgrade = false;  // gear the bot would wear (item usage equip or replace)
    bool uniqueHeld = false;   // unique-equipped and already owned
    bool ownMaterial = false;  // consumed by one of its professions, or raw material it gathers
};

// Need, where mod-playerbots' own rolls would greed or pass. Otherwise it decides.
bool ShouldRollNeed(LootFacts const& facts);

struct ReleaseFacts
{
    bool dead = false;
    bool ghost = false;      // already released
    bool inDungeon = false;  // dungeons and raids: wait for the group
    uint32_t calmMs = 0;     // how long the whole group has been out of combat
    bool someoneCanResurrect = false;  // a living member (the master included) knows a resurrection
};

// Release the spirit when nobody could bring the bot back, once the fight is over.
bool ShouldRelease(ReleaseFacts const& facts, uint32_t calmDelayMs);
}  // namespace PlayerbotsPlus

#endif
