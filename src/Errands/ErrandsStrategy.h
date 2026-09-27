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
// The move is instant, so it may go before an errand.
constexpr float BagsRelevance = 2.1f;
// Offers to the master progress before new share decisions.
constexpr float OfferRelevance = 1.97f;
// After share, so materials reach the right crafter first; before hunting.
constexpr float CraftRelevance = 1.93f;
// A capital trip outranks errands and follow: the leash is released meanwhile.
constexpr float CityRelevance = 2.05f;
// Between share and offers: sharing quests is instant.
constexpr float QuestShareRelevance = 1.96f;
// A level-up packet is handled at once, like a chat command.
constexpr float LevelUpRelevance = ChatCommandRelevance;

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
        triggers.push_back(new TriggerNode("professions", {NextAction("professions", ChatCommandRelevance)}));
        triggers.push_back(new TriggerNode("errands tick", {NextAction("offer to master", OfferRelevance)}));
        triggers.push_back(new TriggerNode("errands tick", {NextAction("city errand", CityRelevance)}));
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

// Keeps a free bag slot on every alt by passing stackables around, out of combat.
class ErrandsBagsStrategy : public Strategy
{
public:
    ErrandsBagsStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}

    std::string const getName() override { return "errands bags"; }
    uint32 GetType() const override { return STRATEGY_TYPE_NONCOMBAT; }

    void InitTriggers(std::vector<TriggerNode*>& triggers) override
    {
        triggers.push_back(new TriggerNode("bags tick", {NextAction("rebalance bags", BagsRelevance)}));
    }
};

// Opt-in on top of "errands": craft for the master (asking), the group, then skill-ups.
class ErrandsCraftStrategy : public Strategy
{
public:
    ErrandsCraftStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}

    std::string const getName() override { return "errands craft"; }
    uint32 GetType() const override { return STRATEGY_TYPE_NONCOMBAT; }

    void InitTriggers(std::vector<TriggerNode*>& triggers) override
    {
        triggers.push_back(new TriggerNode("errands tick", {NextAction("craft item", CraftRelevance)}));
        triggers.push_back(new TriggerNode("craft yes", {NextAction("craft yes", ChatCommandRelevance)}));
        triggers.push_back(new TriggerNode("craft no", {NextAction("craft no", ChatCommandRelevance)}));
    }
};

// Spends the talent points gained on level-up (the spec picked with "talents spec").
class ErrandsLevelUpStrategy : public Strategy
{
public:
    ErrandsLevelUpStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}

    std::string const getName() override { return "errands levelup"; }
    uint32 GetType() const override { return STRATEGY_TYPE_NONCOMBAT; }

    void InitTriggers(std::vector<TriggerNode*>& triggers) override
    {
        triggers.push_back(new TriggerNode("levelup", {NextAction("errands levelup", LevelUpRelevance)}));
    }
};

// Gives the other alts of the group the quests they can take (never the master).
class ErrandsQuestsStrategy : public Strategy
{
public:
    ErrandsQuestsStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}

    std::string const getName() override { return "errands quests"; }
    uint32 GetType() const override { return STRATEGY_TYPE_NONCOMBAT; }

    void InitTriggers(std::vector<TriggerNode*>& triggers) override
    {
        triggers.push_back(new TriggerNode("errands tick", {NextAction("share quests", QuestShareRelevance)}));
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
