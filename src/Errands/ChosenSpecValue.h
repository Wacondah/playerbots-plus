/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_CHOSEN_SPEC_VALUE_H
#define PLAYERBOTS_PLUS_CHOSEN_SPEC_VALUE_H

#include "Value.h"

#include <string>

namespace PlayerbotsPlus
{
// Premade spec name the master picked with "talents spec <name>" (mod-playerbots forgets
// it once the points are spent); saved with the bot ("chosen spec>holy pve").
class ChosenSpecValue : public ManualSetValue<std::string&>
{
public:
    ChosenSpecValue(PlayerbotAI* botAI) : ManualSetValue<std::string&>(botAI, data, "chosen spec") {}

    std::string const Save() override { return data; }

    bool Load(std::string const text) override
    {
        data = text;
        return true;
    }

private:
    std::string data;
};
}  // namespace PlayerbotsPlus

#endif
