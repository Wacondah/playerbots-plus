/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_ERRANDS_STRATEGY_H
#define PLAYERBOTS_PLUS_ERRANDS_STRATEGY_H

#include "Strategy.h"

namespace PlayerbotsPlus
{
// Above follow (default action, 1.0) so an errand is not interrupted by it,
// below rest, loot and the other non-combat triggers.
constexpr float RunErrandRelevance = 2.0f;
// Same relevance as mod-playerbots chat commands (PassThroughStrategy default).
constexpr float ChatCommandRelevance = 100.0f;
// Just below run errand: errands always come first.
constexpr float HuntRelevance = 1.9f;
// Between the two: sharing costs nothing, so it goes before hunting.
constexpr float ShareRelevance = 1.95f;

class ErrandsStrategy : public Strategy
{
public:
    ErrandsStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}

    std::string const getName() override { return "errands"; }
    uint32 GetType() const override { return STRATEGY_TYPE_NONCOMBAT; }

    void InitTriggers(std::vector<TriggerNode*>& triggers) override
    {
        triggers.push_back(new TriggerNode("errands tick", {NextAction("run errand", RunErrandRelevance)}));
        triggers.push_back(new TriggerNode("errands", {NextAction("errands status", ChatCommandRelevance)}));
    }
};

// Opt-in on top of "errands": one elected bot pulls a quest mob near the idle master.
class ErrandsHuntStrategy : public Strategy
{
public:
    ErrandsHuntStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}

    std::string const getName() override { return "errands hunt"; }
    uint32 GetType() const override { return STRATEGY_TYPE_NONCOMBAT; }

    void InitTriggers(std::vector<TriggerNode*>& triggers) override
    {
        triggers.push_back(new TriggerNode("errands tick", {NextAction("hunt quest mob", HuntRelevance)}));
    }
};

// Opt-in on top of "errands": give bag items another alt of the group can use.
class ErrandsShareStrategy : public Strategy
{
public:
    ErrandsShareStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}

    std::string const getName() override { return "errands share"; }
    uint32 GetType() const override { return STRATEGY_TYPE_NONCOMBAT; }

    void InitTriggers(std::vector<TriggerNode*>& triggers) override
    {
        triggers.push_back(new TriggerNode("errands tick", {NextAction("share item", ShareRelevance)}));
    }
};

// Marker strategy: errands actions log their decisions while it is set.
class DebugErrandsStrategy : public Strategy
{
public:
    DebugErrandsStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}

    std::string const getName() override { return "debug errands"; }
    uint32 GetType() const override { return STRATEGY_TYPE_NONCOMBAT | STRATEGY_TYPE_COMBAT; }
};
}  // namespace PlayerbotsPlus

#endif
