// Copyright (c) 2013-2026 The Dimecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cachemap.h>
#include <cachemultimap.h>
#include <streams.h>
#include <test/test_dimecoin.h>

#include <boost/test/unit_test.hpp>

// CacheMap and CacheMultiMap serialise their own nMaxSize field. Persisted
// governance caches therefore carry their bound on disk, and CGovernanceManager
// re-anchors that bound after deserialisation. These tests pin both halves of
// that contract: the raw round-trip does take the on-disk nMaxSize, and an
// explicit SetMaxSize() puts the caller-defined bound back.

BOOST_FIXTURE_TEST_SUITE(cachemap_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(cachemap_ondisk_nmaxsize_is_authoritative)
{
    // Persist a cache with a large cap and a few entries.
    CacheMap<int, int> src(1000);
    for (int i = 0; i < 5; ++i) src.Insert(i, i * 10);
    BOOST_CHECK_EQUAL(src.GetMaxSize(), 1000U);
    BOOST_CHECK_EQUAL(src.GetSize(), 5U);

    CDataStream ss(SER_DISK, 0);
    ss << src;

    // A caller-chosen constructor bound is not what governs the deserialised
    // map: the on-disk nMaxSize wins. This is exactly the surface that
    // CGovernanceManager has to guard against, because governance.dat could
    // land with any nMaxSize value (including 0, which disables pruning).
    CacheMap<int, int> dst(1);
    ss >> dst;
    BOOST_CHECK_EQUAL(dst.GetMaxSize(), 1000U);
    BOOST_CHECK_EQUAL(dst.GetSize(), 5U);

    // The fix: after loading, callers must re-assert their intended cap.
    dst.SetMaxSize(1);
    BOOST_CHECK_EQUAL(dst.GetMaxSize(), 1U);
}

BOOST_AUTO_TEST_CASE(cachemap_load_clear_and_reset_maxsize)
{
    // Fill a cache and persist it with cap = 3.
    CacheMap<int, int> m(3);
    for (int i = 0; i < 10; ++i) m.Insert(i, i);
    // With cap = 3, only the three most-recent inserts survive.
    BOOST_CHECK_EQUAL(m.GetSize(), 3U);
    BOOST_CHECK(m.HasKey(9));
    BOOST_CHECK(!m.HasKey(0));

    // Reload targeting a smaller intended cap. The on-disk nMaxSize still
    // wins, so the reloaded map keeps three entries even though the caller
    // wanted at most two.
    CDataStream ss(SER_DISK, 0);
    ss << m;
    CacheMap<int, int> reloaded(2);
    ss >> reloaded;
    BOOST_CHECK_EQUAL(reloaded.GetMaxSize(), 3U);
    BOOST_CHECK_EQUAL(reloaded.GetSize(), 3U);

    // The governance-manager fix pattern: if loaded size exceeds the caller
    // cap, clear first, then reset the cap. After that PruneLast() will fire
    // on the next Insert once nCurrentSize catches up to nMaxSize, because the
    // Insert-time check is nCurrentSize == nMaxSize (not >=).
    const uint32_t nCap = 2;
    if (reloaded.GetSize() > nCap) reloaded.Clear();
    reloaded.SetMaxSize(nCap);
    BOOST_CHECK_EQUAL(reloaded.GetMaxSize(), nCap);
    BOOST_CHECK_EQUAL(reloaded.GetSize(), 0U);

    reloaded.Insert(100, 100);
    reloaded.Insert(101, 101);
    reloaded.Insert(102, 102); // should prune the oldest (100)
    BOOST_CHECK_EQUAL(reloaded.GetSize(), nCap);
    BOOST_CHECK(!reloaded.HasKey(100));
    BOOST_CHECK(reloaded.HasKey(101));
    BOOST_CHECK(reloaded.HasKey(102));
}

BOOST_AUTO_TEST_CASE(cachemultimap_ondisk_nmaxsize_is_authoritative)
{
    CacheMultiMap<int, int> src(1000);
    for (int i = 0; i < 5; ++i) src.Insert(i, i * 10);
    BOOST_CHECK_EQUAL(src.GetMaxSize(), 1000U);
    BOOST_CHECK_EQUAL(src.GetSize(), 5U);

    CDataStream ss(SER_DISK, 0);
    ss << src;

    CacheMultiMap<int, int> dst(1);
    ss >> dst;
    // Same lesson: on-disk nMaxSize replaced the constructor's small cap.
    BOOST_CHECK_EQUAL(dst.GetMaxSize(), 1000U);
    BOOST_CHECK_EQUAL(dst.GetSize(), 5U);

    dst.SetMaxSize(1);
    BOOST_CHECK_EQUAL(dst.GetMaxSize(), 1U);
}

BOOST_AUTO_TEST_SUITE_END()
