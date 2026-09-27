#include "SharePlanner.h"

#include <gtest/gtest.h>

using namespace PlayerbotsPlus;

namespace
{
constexpr uint32_t T0 = 1000;

ShareReceiver Receiver(uint64_t guid, ShareUsage usage, uint8_t tier = 0, float gain = 0.f, uint32_t held = 0)
{
    return ShareReceiver{guid, usage, tier, gain, held};
}

ShareSnapshot One(ShareItem item)
{
    ShareSnapshot snap;
    snap.errandsIdle = true;
    snap.items = {std::move(item)};
    return snap;
}
}  // namespace

TEST(Tier, PrimaryOverSecondaryOverNone)
{
    using namespace ProfessionBit;
    EXPECT_EQ(TierFor(Tailoring | FirstAid, Tailoring), 2);
    EXPECT_EQ(TierFor(Tailoring | FirstAid, FirstAid), 1);
    EXPECT_EQ(TierFor(Tailoring | FirstAid, Tailoring | FirstAid), 2);
    EXPECT_EQ(TierFor(Tailoring, Alchemy | Cooking), 0);
}

TEST(Share, GateBlocks)
{
    ShareSnapshot snap = One({1, ShareUsage::Other, 0, {Receiver(10, ShareUsage::Replace, 0, 5.f)}});
    snap.errandsIdle = false;
    ShareState state;
    ShareDecision d = PlanShare(snap, state, ShareConfig{}, T0);
    EXPECT_FALSE(d.Acts());
    EXPECT_EQ(d.reason, "errands first");
}

TEST(Share, UpgradeForAnotherMoves)
{
    ShareState state;
    ShareDecision d =
        PlanShare(One({1, ShareUsage::Other, 0, {Receiver(10, ShareUsage::Replace, 0, 5.f)}}), state, ShareConfig{}, T0);
    EXPECT_TRUE(d.Acts());
    EXPECT_EQ(d.item, 1u);
    EXPECT_EQ(d.receiver, 10u);
}

TEST(Share, GiverWantingItKeepsIt)
{
    for (ShareUsage mine : {ShareUsage::Equip, ShareUsage::Replace})
    {
        ShareState state;
        EXPECT_FALSE(PlanShare(One({1, mine, 0, {Receiver(10, ShareUsage::Equip, 0, 5.f)}}), state, ShareConfig{}, T0)
                         .Acts());
    }
}

TEST(Share, QuestItemsNeverMove)
{
    ShareState state;
    EXPECT_FALSE(
        PlanShare(One({1, ShareUsage::Quest, 0, {Receiver(10, ShareUsage::Equip, 2, 5.f)}}), state, ShareConfig{}, T0)
            .Acts());
}

TEST(Share, LargestGainWins)
{
    ShareState state;
    ShareDecision d = PlanShare(One({1, ShareUsage::Other, 0,
                                     {Receiver(10, ShareUsage::Replace, 0, 2.f), Receiver(20, ShareUsage::Equip, 0, 9.f),
                                      Receiver(30, ShareUsage::Replace, 0, 4.f)}}),
                                state, ShareConfig{}, T0);
    EXPECT_EQ(d.receiver, 20u);
}

TEST(Share, MaterialGoesToHigherTier)
{
    ShareState state;
    ShareDecision d = PlanShare(
        One({1, ShareUsage::Other, 1, {Receiver(10, ShareUsage::Other, 1, 0.f, 50), Receiver(20, ShareUsage::Other, 2)}}),
        state, ShareConfig{}, T0);
    EXPECT_EQ(d.receiver, 20u);
}

TEST(Share, EqualTierNeverMoves)
{
    ShareState state;
    EXPECT_FALSE(
        PlanShare(One({1, ShareUsage::Other, 2, {Receiver(10, ShareUsage::Other, 2, 0.f, 99)}}), state, ShareConfig{}, T0)
            .Acts());
}

TEST(Share, SameTierConsolidatesThenLowestGuid)
{
    ShareState s1;
    EXPECT_EQ(PlanShare(One({1, ShareUsage::Other, 0,
                             {Receiver(10, ShareUsage::Other, 2, 0.f, 3), Receiver(20, ShareUsage::Other, 2, 0.f, 12)}}),
                        s1, ShareConfig{}, T0)
                  .receiver,
              20u);
    ShareState s2;
    EXPECT_EQ(PlanShare(One({1, ShareUsage::Other, 0,
                             {Receiver(30, ShareUsage::Other, 2, 0.f, 5), Receiver(20, ShareUsage::Other, 2, 0.f, 5)}}),
                        s2, ShareConfig{}, T0)
                  .receiver,
              20u);
}

TEST(Share, FirstShareableItemWins)
{
    ShareSnapshot snap;
    snap.errandsIdle = true;
    snap.items = {{1, ShareUsage::Other, 0, {}}, {2, ShareUsage::Other, 0, {Receiver(10, ShareUsage::Other, 1)}}};
    ShareState state;
    EXPECT_EQ(PlanShare(snap, state, ShareConfig{}, T0).item, 2u);
}

TEST(Share, FailedPairIsSkippedUntilExpiry)
{
    ShareSnapshot snap = One({1, ShareUsage::Other, 0,
                              {Receiver(10, ShareUsage::Replace, 0, 9.f), Receiver(20, ShareUsage::Replace, 0, 1.f)}});
    ShareState state;
    ShareConfig cfg;
    MarkShareFailed(state, 1, 10, T0);
    EXPECT_EQ(PlanShare(snap, state, cfg, T0 + 1).receiver, 20u);
    EXPECT_EQ(PlanShare(snap, state, cfg, T0 + cfg.blacklistMs).receiver, 10u);
    EXPECT_TRUE(state.blacklistedAt.empty());
}

TEST(Share, NothingToShare)
{
    ShareState state;
    ShareDecision d =
        PlanShare(One({1, ShareUsage::Other, 0, {Receiver(10, ShareUsage::Other, 0)}}), state, ShareConfig{}, T0);
    EXPECT_FALSE(d.Acts());
    EXPECT_EQ(d.reason, "nothing to share");
    EXPECT_EQ(state.lastReason, "nothing to share");
}

TEST(WantedByGroup, UpgradeForAnotherBot)
{
    EXPECT_TRUE(WantedByGroup({1, ShareUsage::Other, 0, {Receiver(10, ShareUsage::Replace, 0, 1.f)}}));
}

TEST(WantedByGroup, HolderAlsoWantsItIsNotAGroupNeed)
{
    EXPECT_FALSE(WantedByGroup({1, ShareUsage::Equip, 0, {Receiver(10, ShareUsage::Replace, 0, 1.f)}}));
}

TEST(WantedByGroup, HigherTierProfession)
{
    EXPECT_TRUE(WantedByGroup({1, ShareUsage::Other, 0, {Receiver(10, ShareUsage::Other, 1)}}));
}

TEST(WantedByGroup, EqualTierOrNobody)
{
    EXPECT_FALSE(WantedByGroup({1, ShareUsage::Other, 1, {Receiver(10, ShareUsage::Other, 1)}}));
    EXPECT_FALSE(WantedByGroup({1, ShareUsage::Other, 0, {}}));
}

TEST(WantedByGroup, QuestItemsCountAsWanted)
{
    EXPECT_TRUE(WantedByGroup({1, ShareUsage::Quest, 0, {}}));
}

namespace
{
ShareSnapshot WithMaster(ShareItem item, float masterGain, bool declined = false)
{
    item.masterGain = masterGain;
    item.masterDeclined = declined;
    ShareSnapshot snap = One(std::move(item));
    snap.master = 99;
    return snap;
}
}  // namespace

TEST(ShareMaster, MasterFirst)
{
    ShareState state;
    ShareDecision d = PlanShare(
        WithMaster({1, ShareUsage::Other, 0, {Receiver(10, ShareUsage::Replace, 0, 50.f)}}, 1.f), state,
        ShareConfig{}, T0);
    EXPECT_EQ(d.receiver, 99u);
    EXPECT_TRUE(d.toMaster);
}

TEST(ShareMaster, DeclinedFallsBackToBots)
{
    ShareState state;
    ShareDecision d = PlanShare(
        WithMaster({1, ShareUsage::Other, 0, {Receiver(10, ShareUsage::Replace, 0, 5.f)}}, 1.f, true), state,
        ShareConfig{}, T0);
    EXPECT_EQ(d.receiver, 10u);
    EXPECT_FALSE(d.toMaster);
}

TEST(ShareMaster, HolderKeepsItsUpgrade)
{
    ShareState state;
    EXPECT_FALSE(PlanShare(WithMaster({1, ShareUsage::Replace, 0, {}}, 3.f), state, ShareConfig{}, T0).Acts());
}

TEST(WantedByGroup, MasterCounts)
{
    ShareItem item{1, ShareUsage::Other, 0, {}};
    item.masterGain = 2.f;
    EXPECT_TRUE(WantedByGroup(item));
    item.masterDeclined = true;
    EXPECT_FALSE(WantedByGroup(item));
}
