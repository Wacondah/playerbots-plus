/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_CRAFT_DECLINED_VALUE_H
#define PLAYERBOTS_PLUS_CRAFT_DECLINED_VALUE_H

#include "ProfessionCatalog.h"
#include "Value.h"

#include <set>

namespace PlayerbotsPlus
{
// Products the master answered "craft no" to; saved with the bot ("craft declined>1,2").
class CraftDeclinedValue : public ManualSetValue<std::set<uint32>&>
{
public:
    CraftDeclinedValue(PlayerbotAI* botAI) : ManualSetValue<std::set<uint32>&>(botAI, data, "craft declined") {}

    std::string const Save() override
    {
        std::string out;
        for (uint32 entry : data)
            out += (out.empty() ? "" : ",") + std::to_string(entry);
        return out;
    }

    bool Load(std::string const text) override
    {
        data.clear();
        std::string part;
        for (char c : text + ",")
        {
            if (c != ',')
            {
                part += c;
                continue;
            }
            if (!part.empty() && part.find_first_not_of("0123456789") == std::string::npos)
                data.insert(uint32(std::stoul(part)));
            part.clear();
        }
        return true;
    }

private:
    std::set<uint32> data;
};
}  // namespace PlayerbotsPlus

#endif
