// Copyright (c) 2026 The Dimecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/wallet.h>

#include <amount.h>
#include <chain.h>
#include <fs.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <sync.h>
#include <test/test_dimecoin.h>
#include <uint256.h>
#include <validation.h>
#include <wallet/db.h>
#include <wallet/walletdb.h>

#include <cstring>
#include <memory>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(wallet_load_tests, TestingSetup)

namespace {
CMutableTransaction MakeTx(const COutPoint& prevout, uint32_t nLockTime)
{
    CMutableTransaction mtx;
    mtx.vin.resize(1);
    mtx.vin[0].prevout = prevout;
    mtx.vout.resize(1);
    mtx.vout[0].nValue = COIN;
    mtx.vout[0].scriptPubKey = CScript() << OP_TRUE;
    mtx.nLockTime = nLockTime;
    return mtx;
}

CWalletTx MakeWalletTx(const CMutableTransaction& mtx, int64_t nOrderPos, const uint256& hashBlock, int nIndex)
{
    CWalletTx wtx(nullptr /* pwallet */, MakeTransactionRef(mtx));
    wtx.nOrderPos = nOrderPos;
    wtx.hashBlock = hashBlock;
    wtx.nIndex = nIndex;
    return wtx;
}

//! Second handle onto the in-memory database made by WalletDatabase::CreateMock():
//! both resolve to the same (environment, "wallet.dat") pair, so this reads back
//! everything the first wallet wrote. Calling CreateMock() again would instead
//! reset the mock environment and throw that data away.
std::unique_ptr<WalletDatabase> ReopenMockDatabase()
{
    return MakeUnique<BerkeleyDatabase>(fs::path(), false /* mock */);
}

DBErrors LoadWallet(CWallet& wallet)
{
    bool first_run = false;
    return wallet.LoadWallet(first_run);
}

// Find an nLockTime for a transaction spending parent_hash's first output whose
// hash sorts, byte-for-byte, before or after parent_hash as requested. Berkeley DB
// returns records in key order, and that key is derived from the txid, so this
// controls which of the pair WalletBatch::LoadWallet() reads first.
CMutableTransaction FindOrderedChild(const uint256& parent_hash, bool child_sorts_first)
{
    for (uint32_t nLockTime = 0; nLockTime < 128; ++nLockTime) {
        CMutableTransaction child = MakeTx(COutPoint(parent_hash, 0), nLockTime);
        const uint256 child_hash = child.GetHash();
        const bool child_first = std::memcmp(child_hash.begin(), parent_hash.begin(), 32) < 0;
        if (child_first == child_sorts_first) return child;
    }
    BOOST_REQUIRE_MESSAGE(false, "could not find a child transaction with the requested record order");
    return CMutableTransaction();
}

// Regression test: LoadToWallet() used to call MarkConflicted() straight away, and
// MarkConflicted() writes to the wallet database through a WalletBatch of its own.
// During startup that happens while WalletBatch::LoadWallet() still holds an open
// Berkeley DB cursor on the very same database, so the write blocks on the cursor's
// lock and the wallet never finishes loading. The conflicts are now deferred until
// WalletBatch::LoadWallet() has explicitly closed its cursor before returning.
//
// Conflict resolution also used to depend on the Berkeley DB record order: it was
// only noticed while loading the child record if the parent it conflicts through
// was already in mapWallet. This is exercised below with the child record sorting
// both after and before its parent.
//
// Without the fix this test does not fail, it hangs: LoadWallet() below never
// returns. That is deliberate, because there is no in-process way to abandon a
// thread stuck on a Berkeley DB lock (a std::future from std::async would just
// move the hang into its own destructor). Bound the run from the outside when
// checking the regression, e.g.
//   timeout 60 ./src/test/test_dimecoin --run_test=wallet_load_tests
// and treat exit status 124 as the deadlock reproducing.
void CheckLoadConflictsAreDeferredAndPersisted(bool child_sorts_first)
{
    uint256 conflict_block;
    {
        LOCK(cs_main);
        conflict_block = chainActive.Tip()->GetBlockHash();
        BOOST_REQUIRE(chainActive.Contains(LookupBlockIndex(conflict_block)));
    }

    // A transaction already recorded as conflicted (nIndex == -1 with a block hash
    // set) plus an in-wallet transaction spending it: this is the pair that makes
    // conflict resolution want to mark a conflict.
    const CMutableTransaction parent = MakeTx(COutPoint(InsecureRand256(), 0), 0);
    const uint256 parent_hash = parent.GetHash();

    const CMutableTransaction child = FindOrderedChild(parent_hash, child_sorts_first);
    const uint256 child_hash = child.GetHash();

    // First load: this is the one that used to deadlock, or miss the conflict
    // entirely when the child's record sorted before its parent's.
    {
        CWallet wallet("mock", WalletDatabase::CreateMock());
        {
            WalletBatch batch(wallet.GetDBHandle(), "cr+");
            BOOST_REQUIRE(batch.WriteTx(MakeWalletTx(parent, 0, conflict_block, -1)));
            BOOST_REQUIRE(batch.WriteTx(MakeWalletTx(child, 1, uint256(), 0)));
        }

        BOOST_CHECK(LoadWallet(wallet) == DBErrors::LOAD_OK);

        LOCK2(cs_main, wallet.cs_wallet);
        BOOST_CHECK_EQUAL(wallet.mapWallet.size(), 2U);
        const CWalletTx& loaded_child = wallet.mapWallet.at(child_hash);
        BOOST_CHECK_EQUAL(loaded_child.nIndex, -1);
        BOOST_CHECK(loaded_child.hashBlock == conflict_block);
        BOOST_CHECK(loaded_child.GetDepthInMainChain() < 0);
    }

    // Second load from the same database: the deferred conflict has to have been
    // written to the database, not just applied to the in-memory wallet.
    {
        CWallet wallet("mock", ReopenMockDatabase());
        BOOST_CHECK(LoadWallet(wallet) == DBErrors::LOAD_OK);

        LOCK2(cs_main, wallet.cs_wallet);
        BOOST_REQUIRE_EQUAL(wallet.mapWallet.count(child_hash), 1U);
        const CWalletTx& reloaded_child = wallet.mapWallet.at(child_hash);
        BOOST_CHECK_EQUAL(reloaded_child.nIndex, -1);
        BOOST_CHECK(reloaded_child.hashBlock == conflict_block);
        BOOST_CHECK(reloaded_child.GetDepthInMainChain() < 0);
    }
}
} // namespace

BOOST_AUTO_TEST_CASE(load_conflicts_are_deferred_and_persisted_parent_first)
{
    CheckLoadConflictsAreDeferredAndPersisted(false /* child_sorts_first */);
}

BOOST_AUTO_TEST_CASE(load_conflicts_are_deferred_and_persisted_child_first)
{
    CheckLoadConflictsAreDeferredAndPersisted(true /* child_sorts_first */);
}

BOOST_AUTO_TEST_SUITE_END()
