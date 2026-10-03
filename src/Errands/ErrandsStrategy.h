/*
 * This file is part of mod-playerbots-plus. Released under GNU GPL v2 or later.
 */

#ifndef PLAYERBOTS_PLUS_ERRANDS_STRATEGY_H
#define PLAYERBOTS_PLUS_ERRANDS_STRATEGY_H

#include "EquipFittingAction.h"
#include "PullActions.h"
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
// Just above mod-playerbots' "loot roll" (100): its roll then skips what we voted.
constexpr float LootNeedRelevance = 101.0f;
// Below hunt (the tank pulls first), above follow: a node near the idle master.
constexpr float GatherRelevance = 1.85f;
// Dead state: above the default dead actions, which wait for a resurrection.
constexpr float ReleaseRelevance = 50.0f;
// Above mod-playerbots' own pull (105–107) and every combat action: the puller's walk wins.
constexpr float PullStepRelevance = 108.0f;

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
        triggers.push_back(new TriggerNode("role", {NextAction("role", ChatCommandRelevance)}));
        triggers.push_back(new TriggerNode("questlog", {NextAction("questlog", ChatCommandRelevance)}));
        triggers.push_back(new TriggerNode("errands tick", {NextAction("offer to master", OfferRelevance)}));
        triggers.push_back(new TriggerNode("errands tick", {NextAction("city errand", CityRelevance)}));
        triggers.push_back(new TriggerNode("errands tick", {NextAction("gather node", GatherRelevance)}));
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
        triggers.push_back(new TriggerNode("bags tick", {NextAction("clean quest items", BagsRelevance - 0.05f)}));
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
        triggers.push_back(new TriggerNode("share quests", {NextAction("share quests command", ChatCommandRelevance)}));
        triggers.push_back(new TriggerNode("errands tick", {NextAction("share quests to master", QuestShareRelevance)}));
    }
};

// Rolls need on gear the bot would wear and on materials of its own professions, and wears
// upgrades the class can wear (mod-playerbots' own auto-equip is replaced).
class ErrandsLootStrategy : public Strategy
{
public:
    ErrandsLootStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}

    std::string const getName() override { return "errands loot"; }

    void InitTriggers(std::vector<TriggerNode*>& triggers) override
    {
        triggers.push_back(new TriggerNode("very often", {NextAction("errands loot roll", LootNeedRelevance)}));
        // The triggers of upstream's "equip upgrades packet action", with our action instead.
        for (char const* trigger : {"trade status", "item push result", "loot roll won"})
            triggers.push_back(new TriggerNode(trigger, {NextAction("errands equip upgrades", ChatCommandRelevance)}));
        triggers.push_back(new TriggerNode("random", {NextAction("errands equip upgrades", 6.0f)}));
    }

    void InitMultipliers(std::vector<Multiplier*>& multipliers) override
    {
        multipliers.push_back(new EquipReplaceMultiplier(botAI));
    }
};

// Dead state: release the spirit when no living member could resurrect the bot.
class ErrandsReviveStrategy : public Strategy
{
public:
    ErrandsReviveStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}

    std::string const getName() override { return "errands revive"; }

    void InitTriggers(std::vector<TriggerNode*>& triggers) override
    {
        triggers.push_back(new TriggerNode("often", {NextAction("errands release", ReleaseRelevance)}));
    }
};

// Group pull: the tank plans, the puller walks and shoots, the others hold.
class ErrandsPullStrategy : public Strategy
{
public:
    ErrandsPullStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}

    std::string const getName() override { return "errands pull"; }
    uint32 GetType() const override { return STRATEGY_TYPE_NONCOMBAT | STRATEGY_TYPE_COMBAT; }

    void InitTriggers(std::vector<TriggerNode*>& triggers) override
    {
        triggers.push_back(new TriggerNode("errands pull tick", {NextAction("errands pull step", PullStepRelevance)}));
        triggers.push_back(
            new TriggerNode("errands pull skull", {NextAction("errands pull skull", ChatCommandRelevance)}));
    }

    void InitMultipliers(std::vector<Multiplier*>& multipliers) override
    {
        multipliers.push_back(new PullHoldMultiplier(botAI));
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
