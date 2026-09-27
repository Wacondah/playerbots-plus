/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_CHAT_COMMANDS_H
#define PLAYERBOTS_PLUS_CHAT_COMMANDS_H

#include <string>

// Pure parsing of the master's chat commands handled outside bot strategies.
namespace PlayerbotsPlus
{
enum class ErrandsSwitch
{
    None,
    On,
    Off
};

// "errands on" / "errands off" (case and surrounding spaces ignored).
ErrandsSwitch ParseErrandsSwitch(std::string const& message);

// "talents spec <name>" (the mod-playerbots command): the premade spec name as typed,
// empty for anything else ("talents spec list" included).
std::string ParseTalentsSpec(std::string const& message);

// All the strategies "errands on" enables, as a ChangeStrategy list: "+a,+b" or "-a,-b".
std::string ErrandsStrategies(bool on);

// Same, for the dead-state engine.
std::string ErrandsDeadStrategies(bool on);
}  // namespace PlayerbotsPlus

#endif
