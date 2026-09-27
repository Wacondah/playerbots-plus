/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#include "ChatCommands.h"

#include <algorithm>
#include <cctype>

namespace PlayerbotsPlus
{
namespace
{
char const* const Strategies[] = {"errands", "errands hunt", "errands share", "errands bags", "errands craft"};
}  // namespace

ErrandsSwitch ParseErrandsSwitch(std::string const& message)
{
    std::string text = message;
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return std::tolower(c); });
    size_t const first = text.find_first_not_of(' ');
    size_t const last = text.find_last_not_of(' ');
    if (first == std::string::npos)
        return ErrandsSwitch::None;
    text = text.substr(first, last - first + 1);

    if (text == "errands on")
        return ErrandsSwitch::On;
    if (text == "errands off")
        return ErrandsSwitch::Off;
    return ErrandsSwitch::None;
}

std::string ErrandsStrategies(bool on)
{
    std::string list;
    for (char const* name : Strategies)
        list += std::string(list.empty() ? "" : ",") + (on ? "+" : "-") + name;
    return list;
}
}  // namespace PlayerbotsPlus
