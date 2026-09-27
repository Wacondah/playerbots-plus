#include "ProfessionCatalog.h"

#include <gtest/gtest.h>

using namespace PlayerbotsPlus;

namespace
{
constexpr uint32_t Mining = 186;
constexpr uint32_t Tailoring = 197;
constexpr uint32_t Herbalism = 182;
constexpr uint32_t Skinning = 393;
constexpr uint32_t Alchemy = 171;
}  // namespace

TEST(Parse, NamesCaseInsensitive)
{
    ParsedAssignment p = ParseAssignment("Mining TAILORING");
    ASSERT_TRUE(p.Ok()) << p.error;
    EXPECT_EQ(p.skills, (std::vector<uint32_t>{Mining, Tailoring}));
}

TEST(Parse, OneProfessionIsFine)
{
    ParsedAssignment p = ParseAssignment("herbalism");
    ASSERT_TRUE(p.Ok());
    EXPECT_EQ(p.skills, (std::vector<uint32_t>{Herbalism}));
}

TEST(Parse, RejectsThreeUnknownAndDuplicates)
{
    EXPECT_EQ(ParseAssignment("mining tailoring alchemy").error, "at most two primary professions");
    EXPECT_EQ(ParseAssignment("mining cooking").error, "unknown profession: cooking");
    EXPECT_EQ(ParseAssignment("mining Mining").error, "duplicate profession: mining");
}

TEST(Persist, RoundTrip)
{
    EXPECT_EQ(SaveAssignment({Mining, Tailoring}), "186,197");
    EXPECT_EQ(LoadAssignment("186,197"), (std::vector<uint32_t>{Mining, Tailoring}));
}

TEST(Persist, LoadDropsUnknown)
{
    EXPECT_EQ(LoadAssignment("186,999,197"), (std::vector<uint32_t>{Mining, Tailoring}));
    EXPECT_TRUE(LoadAssignment("").empty());
}

TEST(Forget, NotNeededWithFreeSlotOrAlreadyKnown)
{
    std::vector<std::pair<uint32_t, uint32_t>> const known = {{Skinning, 40}};
    EXPECT_EQ(ProfessionToForget(known, {Mining}, 1, Mining), 0u);
    EXPECT_EQ(ProfessionToForget({{Mining, 10}, {Skinning, 40}}, {Mining}, 0, Mining), 0u);
}

TEST(Forget, LowestUnassigned)
{
    EXPECT_EQ(ProfessionToForget({{Skinning, 40}, {Alchemy, 12}}, {Mining, Tailoring}, 0, Mining), Alchemy);
}

TEST(Forget, NeverAssignedOrWithoutAssignment)
{
    EXPECT_EQ(ProfessionToForget({{Mining, 5}, {Skinning, 40}}, {Mining, Tailoring}, 0, Tailoring), Skinning);
    EXPECT_EQ(ProfessionToForget({{Mining, 5}, {Tailoring, 40}}, {Mining, Tailoring}, 0, Tailoring), 0u);
    EXPECT_EQ(ProfessionToForget({{Skinning, 40}, {Alchemy, 12}}, {}, 0, Mining), 0u);
}

TEST(Catalog, ToolsMatchTheSpec)
{
    EXPECT_EQ(PrimaryProfessions().size(), 11u);
    EXPECT_EQ(FindProfession(186)->tool, 2901u);
    EXPECT_EQ(FindProfession(393)->tool, 7005u);
    EXPECT_EQ(FindProfession(164)->tool, 5956u);
    EXPECT_EQ(FindProfession(202)->tool, 6219u);
    EXPECT_EQ(FindProfession(755)->tool, 20815u);
    EXPECT_EQ(FindProfession(773)->tool, 39505u);
    for (uint32_t none : {171u, 182u, 197u, 165u, 333u})
        EXPECT_EQ(FindProfession(none)->tool, 0u) << none;
    EXPECT_EQ(FindProfession(185), nullptr);  // cooking is not a primary profession
}
