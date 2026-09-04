// Copyright (c) 2026 The Dimecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <consensus/consensus.h>
#include <script/standard.h>
#include <validation.h>
#include <wallet/test/wallet_test_fixture.h>
#include <wallet/wallet.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(coinstake_credit_tests, TestChain100Setup)

BOOST_AUTO_TEST_CASE(credit_respects_maturity)
{
    CWallet wallet("dummy", WalletDatabase::CreateDummy());

    CKey spendableKey;
    spendableKey.MakeNewKey(true);
    {
        LOCK(wallet.cs_wallet);
        BOOST_REQUIRE(wallet.AddKeyPubKey(spendableKey, spendableKey.GetPubKey()));
    }
    const CScript spendableScript = GetScriptForRawPubKey(spendableKey.GetPubKey());

    CKey watchOnlyKey;
    watchOnlyKey.MakeNewKey(true);
    const CScript watchOnlyScript = GetScriptForRawPubKey(watchOnlyKey.GetPubKey());
    {
        LOCK(wallet.cs_wallet);
        BOOST_REQUIRE(wallet.AddWatchOnly(watchOnlyScript, 0));
    }

    CMutableTransaction stake;
    stake.vin.emplace_back(COutPoint(GetRandHash(), 0));
    stake.vout.emplace_back(0, CScript());
    stake.vout.emplace_back(40 * COIN, spendableScript);
    stake.vout.emplace_back(10 * COIN, watchOnlyScript);

    CWalletTx wtx(&wallet, MakeTransactionRef(stake));
    LOCK2(cs_main, wallet.cs_wallet);
    BOOST_REQUIRE(wtx.IsCoinStake());

    wtx.hashBlock = chainActive.Tip()->GetBlockHash();
    wtx.nIndex = 1;

    BOOST_CHECK_GT(wtx.GetBlocksToMaturity(), 0);
    BOOST_CHECK_EQUAL(wtx.GetCredit(ISMINE_ALL), 0);
    BOOST_CHECK_EQUAL(wallet.GetCredit(*wtx.tx, ISMINE_ALL), 50 * COIN);
    BOOST_CHECK_EQUAL(wtx.GetAvailableCredit(), 0);
    BOOST_CHECK_EQUAL(wtx.GetAvailableCredit(true, ISMINE_WATCH_ONLY), 0);
    BOOST_CHECK_EQUAL(wtx.GetImmatureCredit(), 40 * COIN);
    BOOST_CHECK_EQUAL(wtx.GetImmatureWatchOnlyCredit(), 10 * COIN);

    const CBlockIndex* matureBlock = chainActive[chainActive.Height() - COINBASE_MATURITY];
    BOOST_REQUIRE(matureBlock);
    wtx.hashBlock = matureBlock->GetBlockHash();
    wtx.MarkDirty();

    BOOST_CHECK_EQUAL(wtx.GetBlocksToMaturity(), 0);
    BOOST_CHECK_EQUAL(wtx.GetCredit(ISMINE_SPENDABLE), 40 * COIN);
    BOOST_CHECK_EQUAL(wtx.GetCredit(ISMINE_WATCH_ONLY), 10 * COIN);
    BOOST_CHECK_EQUAL(wtx.GetAvailableCredit(), 40 * COIN);
    BOOST_CHECK_EQUAL(wtx.GetAvailableCredit(true, ISMINE_WATCH_ONLY), 10 * COIN);
    BOOST_CHECK_EQUAL(wtx.GetImmatureCredit(), 0);
    BOOST_CHECK_EQUAL(wtx.GetImmatureWatchOnlyCredit(), 0);
}

BOOST_AUTO_TEST_SUITE_END()
