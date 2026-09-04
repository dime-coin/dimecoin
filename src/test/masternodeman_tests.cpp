// Copyright (c) 2013-2025 The Dimecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <masternode/masternodeman.h>
#include <test/test_dimecoin.h>
#include <uint256.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(masternodeman_tests, BasicTestingSetup)

// mapSeenMasternodeBroadcast is populated from unauthenticated peers before the
// announcement is validated, and it is deliberately not expired on a timer
// (see the note in CMasternodeMan::CheckAndRemove). The size cap added in 2.5.5
// is therefore the only thing bounding it. These tests pin that behaviour.

BOOST_AUTO_TEST_CASE(seen_mnb_limit_evicts_oldest)
{
    CMasternodeMan mnman;

    const size_t nOverflow = 100;
    const size_t nTotal = CMasternodeMan::MAX_SEEN_MNB_ENTRIES + nOverflow;

    // Insert with increasing timestamps so entry i is older than entry i+1.
    // The key is a hash, so map iteration order is unrelated to insertion age;
    // that is precisely why the eviction path has to sort by the stored time.
    for (size_t i = 0; i < nTotal; ++i) {
        mnman.mapSeenMasternodeBroadcast[ArithToUint256(arith_uint256(i))] =
            std::make_pair(static_cast<int64_t>(i), CMasternodeBroadcast());
    }
    BOOST_CHECK_EQUAL(mnman.mapSeenMasternodeBroadcast.size(), nTotal);
    mnman.EnforceSeenBroadcastLimit();

    BOOST_CHECK_EQUAL(mnman.mapSeenMasternodeBroadcast.size(),
                      CMasternodeMan::MAX_SEEN_MNB_ENTRIES);

    // The oldest nOverflow entries must be the ones that went.
    for (size_t i = 0; i < nOverflow; ++i) {
        BOOST_CHECK(mnman.mapSeenMasternodeBroadcast.count(ArithToUint256(arith_uint256(i))) == 0);
    }
    // Everything newer must have survived.
    for (size_t i = nOverflow; i < nTotal; ++i) {
        BOOST_CHECK(mnman.mapSeenMasternodeBroadcast.count(ArithToUint256(arith_uint256(i))) == 1);
    }
}

BOOST_AUTO_TEST_CASE(seen_mnb_limit_is_noop_under_cap)
{
    CMasternodeMan mnman;

    // A node carrying a normal number of announcements must be left untouched;
    // the cap is ~100x the live masternode count, so it should never fire in
    // ordinary operation.
    const size_t nEntries = 200;
    for (size_t i = 0; i < nEntries; ++i) {
        mnman.mapSeenMasternodeBroadcast[ArithToUint256(arith_uint256(i))] =
            std::make_pair(static_cast<int64_t>(i), CMasternodeBroadcast());
    }
    mnman.EnforceSeenBroadcastLimit();

    BOOST_CHECK_EQUAL(mnman.mapSeenMasternodeBroadcast.size(), nEntries);
}

BOOST_AUTO_TEST_CASE(seen_mnb_limit_exactly_at_cap)
{
    CMasternodeMan mnman;

    // Boundary: at exactly the cap nothing is evicted, one over evicts one.
    for (size_t i = 0; i < CMasternodeMan::MAX_SEEN_MNB_ENTRIES; ++i) {
        mnman.mapSeenMasternodeBroadcast[ArithToUint256(arith_uint256(i))] =
            std::make_pair(static_cast<int64_t>(i), CMasternodeBroadcast());
    }
    mnman.EnforceSeenBroadcastLimit();
    BOOST_CHECK_EQUAL(mnman.mapSeenMasternodeBroadcast.size(),
                      CMasternodeMan::MAX_SEEN_MNB_ENTRIES);

    mnman.mapSeenMasternodeBroadcast[ArithToUint256(arith_uint256(CMasternodeMan::MAX_SEEN_MNB_ENTRIES))] =
        std::make_pair(static_cast<int64_t>(CMasternodeMan::MAX_SEEN_MNB_ENTRIES), CMasternodeBroadcast());
    mnman.EnforceSeenBroadcastLimit();
    BOOST_CHECK_EQUAL(mnman.mapSeenMasternodeBroadcast.size(),
                      CMasternodeMan::MAX_SEEN_MNB_ENTRIES);
    BOOST_CHECK(mnman.mapSeenMasternodeBroadcast.count(ArithToUint256(arith_uint256(0))) == 0);
}

BOOST_AUTO_TEST_SUITE_END()
