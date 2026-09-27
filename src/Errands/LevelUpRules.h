/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_LEVEL_UP_RULES_H
#define PLAYERBOTS_PLUS_LEVEL_UP_RULES_H

#include <cstdint>

// Pure rules for "errands levelup" and "errands quests".
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
}  // namespace PlayerbotsPlus

#endif
