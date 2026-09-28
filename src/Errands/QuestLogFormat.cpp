/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "QuestLogFormat.h"

#include <algorithm>

namespace PlayerbotsPlus
{
namespace
{
constexpr size_t MinNameLength = 8;

std::string Build(QuestLine const& line, std::string const& title, size_t nameLength, size_t objectives)
{
    std::string out = "Q\t" + std::to_string(line.id) + "\t" + std::to_string(line.status) + "\t" + title + "\t";
    for (size_t i = 0; i < objectives; ++i)
    {
        QuestObjective const& o = line.objectives[i];
        if (i)
            out += "|";
        out += std::to_string(o.done) + "/" + std::to_string(o.required) + ":" +
               CleanField(o.name).substr(0, nameLength);
    }
    return out;
}
}  // namespace

std::string CleanField(std::string const& text)
{
    std::string out = text;
    std::replace(out.begin(), out.end(), '\t', ' ');
    std::replace(out.begin(), out.end(), '|', ' ');
    return out;
}

std::string FormatQuestLine(QuestLine const& line, size_t maxBytes)
{
    std::string const title = CleanField(line.title);
    size_t longest = 0;
    for (QuestObjective const& o : line.objectives)
        longest = std::max(longest, o.name.size());

    for (size_t length = longest; length + 1 > MinNameLength; --length)
    {
        std::string const out = Build(line, title, length, line.objectives.size());
        if (out.size() <= maxBytes)
            return out;
        if (length == 0)
            break;
    }
    for (size_t count = line.objectives.size(); count-- > 0;)
    {
        std::string const out = Build(line, title, MinNameLength, count);
        if (out.size() <= maxBytes)
            return out;
    }
    return Build(line, title, 0, 0).substr(0, maxBytes);
}
}  // namespace PlayerbotsPlus
