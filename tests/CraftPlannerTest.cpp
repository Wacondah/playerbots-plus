#include "CraftPlanner.h"

#include <gtest/gtest.h>

using namespace PlayerbotsPlus;

namespace
{
constexpr uint32_t T0 = 1000;

RecipeOption Recipe(uint32_t spell, uint32_t cost)
{
    RecipeOption r;
    r.spell = spell;
    r.product = spell + 1000;
    r.castable = true;
    r.reagentCost = cost;
    return r;
}

CraftSnapshot Idle(std::vector<RecipeOption> recipes)
{
    CraftSnapshot snap;
    snap.errandsIdle = true;
    snap.recipes = std::move(recipes);
    return snap;
}
}  // namespace

TEST(Craft, GateBlocks)
{
    RecipeOption r = Recipe(1, 5);
    r.skillUp = true;
    CraftSnapshot snap = Idle({r});
    snap.errandsIdle = false;
    CraftState state;
    EXPECT_EQ(PlanCraft(snap, state, CraftConfig{}, T0).reason, "errands first");
}

TEST(Craft, AskBeforeGroupAndSkill)
{
    RecipeOption group = Recipe(1, 1);
    group.usefulToGroup = true;
    RecipeOption master = Recipe(2, 9);
    master.usefulToMaster = true;
    CraftState state;
    CraftDecision d = PlanCraft(Idle({group, master}), state, CraftConfig{}, T0);
    EXPECT_EQ(d.action, CraftAction::Ask);
    EXPECT_EQ(d.spell, 2u);
    EXPECT_EQ(state.askedProduct, 1002u);
}

TEST(Craft, WaitsForTheAnswerThenExpires)
{
    RecipeOption master = Recipe(2, 9);
    master.usefulToMaster = true;
    RecipeOption skill = Recipe(3, 1);
    skill.skillUp = true;
    CraftSnapshot snap = Idle({master, skill});
    CraftState state;
    CraftConfig cfg;
    PlanCraft(snap, state, cfg, T0);
    CraftDecision waiting = PlanCraft(snap, state, cfg, T0 + 1000);
    EXPECT_EQ(waiting.action, CraftAction::None);
    EXPECT_EQ(waiting.reason, "waiting for answer");

    CraftDecision after = PlanCraft(snap, state, cfg, T0 + cfg.askTimeoutMs);
    EXPECT_EQ(after.action, CraftAction::Ask);  // asked again once expired
}

TEST(Craft, ApprovedIsCraftedForTheMasterOnce)
{
    RecipeOption master = Recipe(2, 9);
    master.usefulToMaster = true;
    CraftSnapshot snap = Idle({master});
    CraftState state;
    PlanCraft(snap, state, CraftConfig{}, T0);
    ASSERT_TRUE(AnswerCraft(state, true));
    CraftDecision d = PlanCraft(snap, state, CraftConfig{}, T0 + 10);
    EXPECT_EQ(d.action, CraftAction::Craft);
    EXPECT_TRUE(d.forMaster);
    EXPECT_EQ(d.product, 1002u);
    EXPECT_EQ(state.approvedSpell, 0u);
}

TEST(Craft, ApprovedButNotCastableIsDropped)
{
    RecipeOption master = Recipe(2, 9);
    master.usefulToMaster = true;
    CraftState state;
    PlanCraft(Idle({master}), state, CraftConfig{}, T0);
    AnswerCraft(state, true);
    master.castable = false;
    CraftDecision d = PlanCraft(Idle({master}), state, CraftConfig{}, T0 + 10);
    EXPECT_EQ(d.action, CraftAction::None);
    EXPECT_EQ(state.approvedSpell, 0u);
}

TEST(Craft, DeclinedIsNeverAsked)
{
    RecipeOption master = Recipe(2, 9);
    master.usefulToMaster = true;
    master.declined = true;
    CraftState state;
    EXPECT_EQ(PlanCraft(Idle({master}), state, CraftConfig{}, T0).action, CraftAction::None);
}

TEST(Craft, GroupBeforeSkillCheapestFirst)
{
    RecipeOption skill = Recipe(1, 1);
    skill.skillUp = true;
    RecipeOption groupDear = Recipe(2, 50);
    groupDear.usefulToGroup = true;
    RecipeOption groupCheap = Recipe(3, 20);
    groupCheap.usefulToGroup = true;
    CraftState state;
    CraftDecision d = PlanCraft(Idle({skill, groupDear, groupCheap}), state, CraftConfig{}, T0);
    EXPECT_EQ(d.action, CraftAction::Craft);
    EXPECT_EQ(d.spell, 3u);
    EXPECT_FALSE(d.forMaster);

    RecipeOption skillDear = Recipe(4, 7);
    skillDear.skillUp = true;
    CraftState s2;
    EXPECT_EQ(PlanCraft(Idle({skillDear, skill}), s2, CraftConfig{}, T0).spell, 1u);
}

TEST(Craft, NothingUsefulOrCastable)
{
    RecipeOption grey = Recipe(1, 1);
    RecipeOption blocked = Recipe(2, 1);
    blocked.usefulToGroup = true;
    blocked.castable = false;
    CraftState state;
    CraftDecision d = PlanCraft(Idle({grey, blocked}), state, CraftConfig{}, T0);
    EXPECT_EQ(d.action, CraftAction::None);
    EXPECT_EQ(d.reason, "nothing to craft");
}

TEST(Craft, AnswerWithoutQuestionOrNo)
{
    CraftState state;
    EXPECT_FALSE(AnswerCraft(state, true));

    RecipeOption master = Recipe(2, 9);
    master.usefulToMaster = true;
    PlanCraft(Idle({master}), state, CraftConfig{}, T0);
    EXPECT_TRUE(AnswerCraft(state, false));
    EXPECT_EQ(state.askedSpell, 0u);
    EXPECT_EQ(state.approvedSpell, 0u);
}

TEST(Craft, DisenchantAfterGroupBeforeSkill)
{
    RecipeOption skill = Recipe(1, 1);
    skill.skillUp = true;
    CraftSnapshot snap = Idle({skill});
    snap.disenchantItem = 77;
    CraftState s1;
    CraftDecision d = PlanCraft(snap, s1, CraftConfig{}, T0);
    EXPECT_EQ(d.action, CraftAction::Disenchant);
    EXPECT_EQ(d.item, 77u);

    RecipeOption group = Recipe(2, 5);
    group.usefulToGroup = true;
    snap.recipes.push_back(group);
    CraftState s2;
    EXPECT_EQ(PlanCraft(snap, s2, CraftConfig{}, T0).spell, 2u);
}
