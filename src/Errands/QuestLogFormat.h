/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_QUEST_LOG_FORMAT_H
#define PLAYERBOTS_PLUS_QUEST_LOG_FORMAT_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Pure formatting of the "questlog" reply read by the PlayerbotsPlusQuests addon:
// Q<tab>id<tab>status<tab>title<tab>done/required:name|done/required:name...
namespace PlayerbotsPlus
{
struct QuestObjective
{
    uint32_t done = 0;
    uint32_t required = 0;
    std::string name;
};

struct QuestLine
{
    uint32_t id = 0;
    uint8_t status = 0;  // 0 in progress, 1 complete (to turn in), 2 failed
    std::string title;
    std::vector<QuestObjective> objectives;
};

// Tabs and '|' (the separators) become spaces.
std::string CleanField(std::string const& text);

// Never longer than maxBytes: names are shortened first (down to 8 characters), then the
// last objectives are dropped, then the title is cut.
std::string FormatQuestLine(QuestLine const& line, size_t maxBytes = 250);
}  // namespace PlayerbotsPlus

#endif
