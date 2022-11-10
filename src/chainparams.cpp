// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Copyright (c) 2013-2022 The Dimecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <consensus/merkle.h>

#include <tinyformat.h>
#include <util/strencodings.h>
#include <arith_uint256.h>
#include <util/system.h>
#include <assert.h>

#include <chainparamsseeds.h>

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 *
 * CBlock(hash=000000000019d6, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=4a5e1e, nTime=1231006505, nBits=1d00ffff, nNonce=2083236893, vtx=1)
 *   CTransaction(hash=4a5e1e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
 *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73)
 *     CTxOut(nValue=50.00000000, scriptPubKey=0x5F1DF16B2B704C8A578D0B)
 *   vMerkleTree: 4a5e1e
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "BIN COIN START";
    const CScript genesisOutputScript = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

void CChainParams::UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    consensus.vDeployments[d].nStartTime = nStartTime;
    consensus.vDeployments[d].nTimeout = nTimeout;
}

/**
 * Main network
 */
/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */
class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";

        consensus.nSubsidyHalvingInterval = 512000;
        consensus.nFirstPoSBlock = 5000000;
        consensus.nInstantSendKeepLock = 24;
        consensus.nBudgetPaymentsStartBlock = 0;
        consensus.nBudgetPaymentsCycleBlocks = 16616;
        consensus.nBudgetPaymentsWindowBlocks = 100;
        consensus.nBudgetProposalEstablishingTime = 60*60*24;
        consensus.nSuperblockCycle = 43200;
        consensus.nSuperblockStartBlock = consensus.nSuperblockCycle;
        consensus.nGovernanceMinQuorum = 10;
        consensus.nGovernanceFilterElements = 20000;
        consensus.BIP34Height = 10;
        consensus.BIP34Hash = uint256S("0000000000000000000000000000000000000000000000000000000000000000");
        consensus.BIP65Height = consensus.nFirstPoSBlock;
        consensus.BIP66Height = consensus.nFirstPoSBlock;
        consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.posLimit = consensus.powLimit;
        consensus.nPowTargetTimespan = 65536; //! this is a worry
        consensus.nPowTargetSpacing = 64;     //! this is a worry
        consensus.nPosTargetSpacing = consensus.nPowTargetSpacing;
        consensus.nPosTargetTimespan = consensus.nPowTargetTimespan;
        consensus.checkpointPubKey = "045af95d3e64f3166cef9fab4ce87f6a085055b7552bac891c2600f7a90b382053f88b0741282b02c763b1b3352de4dd98d8b32c4664686b6ba050e6c2f8ca0520";
        consensus.nMasternodePaymentsStartBlock = std::numeric_limits<int>::max();
        consensus.nMasternodeMinimumConfirmations = 15;
        consensus.nFoundationPaymentAddress = "7P54NbY76KhSg4j6fWL7FMHyH7wLJTiP5j";
        consensus.nStakeMinAge = 10 * 60;
        consensus.nStakeMaxAge = 60 * 60 * 24 * 30;
        consensus.nStakeMinAmount = 100000 * COIN;
        consensus.nStakeMinDepth = 450;
        consensus.nModifierInterval = 60 * 20;
        consensus.nCoinbaseMaturity = 60;
        consensus.fullSplitDiffHeight = 5075000;
        consensus.fullStakingOverhaul = 5228000;

        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1080;
        consensus.nMinerConfirmationWindow = 1440;

        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1462060800; // May 1st, 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1493596800; // May 1st, 2017

        // Deployment of SegWit (BIP141, BIP143, and BIP147)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 1479168000; // November 15th, 2016.
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 1510704000; // November 15th, 2017.

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0000000000000000000000000000000000000000000000000000000000000000");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0000000000000000000000000000000000000000000000000000000000000000");

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xfe;
        pchMessageStart[1] = 0xa5;
        pchMessageStart[2] = 0x03;
        pchMessageStart[3] = 0xdd;
        nDefaultPort = 11931;
        nPruneAfterHeight = 100000;
        nMaxReorganizationDepth = 100;

        genesis = CreateGenesisBlock(1387807823, 16888732, 0x1e0fffff, 112, 1 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("00000d5a9113f87575c77eb5442845ff8a0014f6e79e2dd2317d88946ef910da"));
        assert(genesis.hashMerkleRoot == uint256S("72596a6a36d42416b5486386c6e2b7e339782ef4eb49fb8a60ec7dc3475da545"));

        vSeeds.emplace_back("seed1.dimecoinnetwork.com", "seed1.dimecoinnetwork.com");         //Primary DNS Seed
        vSeeds.emplace_back("seed2.dimecoinnetwork.com", "seed2.dimecoinnetwork.com");         //Secondary DNS Seed
        vSeeds.emplace_back("node1.dimecoinnetwork.com", "node1.dimecoinnetwork.com");         //Primary node
        vSeeds.emplace_back("node2.dimecoinnetwork.com", "node2.dimecoinnetwork.com");         //Secondary node
        vSeeds.emplace_back("dime-pool.dimecoinnetwork.com", "dime-pool.dimecoinnetwork.com"); //dime-pool.com node

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,15);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,9);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,143);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};
        bech32_hrp = "vx";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;
        nCollateralLevels = { 500000000 };
        nPoolMaxTransactions = 3;
        nFulfilledRequestExpireTime = 60*60;
        strSporkPubKey = "045078e030f9b5131e2fe4de7bf81761e5eb609309f951ca3dd63b85fc702b97c736de0644562e48b75c2555a5765bdc2aecc15264777b99ec2f2643d4521a50e2"; //! 76ekH6a2mnJXmeqxfmSduYXYKv7wZ1oG3F

        checkpointData = {{
            {       0,     uint256S("0x00000d5a9113f87575c77eb5442845ff8a0014f6e79e2dd2317d88946ef910da")},
            {  250000,     uint256S("0x000000001ecc6fd9f2a2e50722762de18bc96599e6d00e3ddc9e2d97d4b177ff")},
            {  500000,     uint256S("0x00000000129e32df9b68e9478fb99ad094ca3311e26b853a07d864c557ae3696")},
            { 1000000,     uint256S("0x000000001e2ef1ef0f9f587150e598faa2e74ff95c0bcdd2026d2688b2bbf13a")},
            { 1500000,     uint256S("0x000000007f0008d7b697589583d58cc0d83a62f7364859711576fe50bca70f06")},
            { 2000000,     uint256S("0x000000000a212f4cfab5d16c26868c37067a4642691b77230bb68a60a10c97e1")},
            { 2500000,     uint256S("0x0000000000010f978067c92689a39ef8c6f17849a6e3c6c86ac8685e8f7f84af")},
            { 3000000,     uint256S("0x000000000000c7552284192bd1e3e7b1b7648ec4868cef082f2aa4945f7bad36")},
            { 3500000,     uint256S("0x000000000002b20eb6313116e798153c6f7650f2ccba4db3f44cefc7910b33fa")},
            { 4000000,     uint256S("0x00000000000a267908d41c43de815f429543ff93fd00ec8fb0a067fc61ab3d36")},
        }};

        chainTxData = ChainTxData{
            1656588520,
            6133748,
            0.03
        };

        /* disable fallback fee on mainnet */
        m_fallback_fee_enabled = true;
    }
};

/*
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";

        consensus.nSubsidyHalvingInterval = 512000;
        consensus.nFirstPoSBlock = 100;
        consensus.nInstantSendKeepLock = 24;
        consensus.nBudgetPaymentsStartBlock = 0;
        consensus.nBudgetPaymentsCycleBlocks = 16616;
        consensus.nBudgetPaymentsWindowBlocks = 100;
        consensus.nBudgetProposalEstablishingTime = 60*60*24;
        consensus.nSuperblockCycle = 43200;
        consensus.nSuperblockStartBlock = consensus.nSuperblockCycle;
        consensus.nGovernanceMinQuorum = 10;
        consensus.nGovernanceFilterElements = 20000;
        consensus.BIP34Height = std::numeric_limits<int>::max();
        consensus.BIP34Hash = uint256S("0000000000000000000000000000000000000000000000000000000000000000");
        consensus.BIP65Height = consensus.nFirstPoSBlock;
        consensus.BIP66Height = consensus.nFirstPoSBlock;
        consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.posLimit = consensus.powLimit;
        consensus.nPowTargetTimespan = 65536; //! this is a worry
        consensus.nPowTargetSpacing = 64;     //! this is a worry
        consensus.nPosTargetSpacing = consensus.nPowTargetSpacing;
        consensus.nPosTargetTimespan = consensus.nPowTargetTimespan;
        consensus.checkpointPubKey = "04fcac7bbdcf17dfd5b4fae8e22a16b88820a5d7105981a613f7205cf5676480b36553e9539cf22ca0f6b3fd84de7600e60ec15b260d61f6c1baabdb508ee60976";
        consensus.nMasternodePaymentsStartBlock = 2300;
        consensus.nMasternodeMinimumConfirmations = 15;
        consensus.nFoundationPaymentAddress = "7KXXXXXXXXXXXXXXXXXXXXXXXXXXYMjbje";
        consensus.nStakeMinAge = 10 * 60;
        consensus.nStakeMaxAge = 60 * 60 * 24 * 30;
        consensus.nStakeMinAmount = 0 * COIN;
        consensus.nStakeMinDepth = 15;
        consensus.nModifierInterval = 60 * 20;
        consensus.nCoinbaseMaturity = 15;
        consensus.fullSplitDiffHeight = 0;
        consensus.fullStakingOverhaul = 95000;

        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1080;
        consensus.nMinerConfirmationWindow = 1440;

        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1462060800; // May 1st, 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1493596800; // May 1st, 2017

        // Deployment of SegWit (BIP141, BIP143, and BIP147)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 1479168000; // November 15th, 2016.
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 1510704000; // November 15th, 2017.

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0000000000000000000000000000000000000000000000000000000000000000");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0000000000000000000000000000000000000000000000000000000000000000");

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0x78;
        pchMessageStart[1] = 0x92;
        pchMessageStart[2] = 0x30;
        pchMessageStart[3] = 0x39;
        nDefaultPort = 21931;
        nPruneAfterHeight = 100000;
        nMaxReorganizationDepth = 100;

        genesis = CreateGenesisBlock(1636592000, 803251, 0x1e0fffff, 1, 0 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("00000d3b251f80cbbff32cf05e0d50cfbec979e9309d3677e2726988203ba0f1"));
        assert(genesis.hashMerkleRoot == uint256S("558288e9f2dbdd2c5a9ed64d2962a5679b83bda205394564609cfddbbaab6193"));

        vFixedSeeds.clear();
        vSeeds.clear();

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,15);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,9);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,143);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};
        bech32_hrp = "vx";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;
        nCollateralLevels = { 100000 };
        nPoolMaxTransactions = 3;
        nFulfilledRequestExpireTime = 60*60;
        strSporkPubKey = "045078e030f9b5131e2fe4de7bf81761e5eb609309f951ca3dd63b85fc702b97c736de0644562e48b75c2555a5765bdc2aecc15264777b99ec2f2643d4521a50e2"; //! 76ekH6a2mnJXmeqxfmSduYXYKv7wZ1oG3F

        checkpointData = {{
            {       0,     uint256S("0x00000d3b251f80cbbff32cf05e0d50cfbec979e9309d3677e2726988203ba0f1")},
        }};

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        /* disable fallback fee on mainnet */
        m_fallback_fee_enabled = true;
    }
};

/*
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";

        consensus.nSubsidyHalvingInterval = 512000;
        consensus.nFirstPoSBlock = 100;
        consensus.nInstantSendKeepLock = 24;
        consensus.nBudgetPaymentsStartBlock = 0;
        consensus.nBudgetPaymentsCycleBlocks = 16616;
        consensus.nBudgetPaymentsWindowBlocks = 100;
        consensus.nBudgetProposalEstablishingTime = 60*60*24;
        consensus.nSuperblockCycle = 43200;
        consensus.nSuperblockStartBlock = consensus.nSuperblockCycle;
        consensus.nGovernanceMinQuorum = 10;
        consensus.nGovernanceFilterElements = 20000;
        consensus.BIP34Height = std::numeric_limits<int>::max();
        consensus.BIP34Hash = uint256S("0000000000000000000000000000000000000000000000000000000000000000");
        consensus.BIP65Height = consensus.nFirstPoSBlock;
        consensus.BIP66Height = consensus.nFirstPoSBlock;
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.posLimit = consensus.powLimit;
        consensus.nPowTargetTimespan = 65536; //! this is a worry
        consensus.nPowTargetSpacing = 64;     //! this is a worry
        consensus.nPosTargetSpacing = consensus.nPowTargetSpacing;
        consensus.nPosTargetTimespan = consensus.nPowTargetTimespan;
        consensus.checkpointPubKey = "04fcac7bbdcf17dfd5b4fae8e22a16b88820a5d7105981a613f7205cf5676480b36553e9539cf22ca0f6b3fd84de7600e60ec15b260d61f6c1baabdb508ee60976";
        consensus.nMasternodePaymentsStartBlock = 2300;
        consensus.nMasternodeMinimumConfirmations = 15;
        consensus.nFoundationPaymentAddress = "7KXXXXXXXXXXXXXXXXXXXXXXXXXXYMjbje";
        consensus.nStakeMinAge = 10 * 60;
        consensus.nStakeMaxAge = 60 * 60 * 24 * 30;
        consensus.nStakeMinAmount = 0 * COIN;
        consensus.nStakeMinDepth = 15;
        consensus.nModifierInterval = 60 * 20;
        consensus.nCoinbaseMaturity = 15;
        consensus.fullSplitDiffHeight = 0;

        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1080;
        consensus.nMinerConfirmationWindow = 1440;

        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // Deployment of BIP68, BIP112, and BIP113.
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].bit = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nStartTime = 1462060800; // May 1st, 2016
        consensus.vDeployments[Consensus::DEPLOYMENT_CSV].nTimeout = 1493596800; // May 1st, 2017

        // Deployment of SegWit (BIP141, BIP143, and BIP147)
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].bit = 1;
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nStartTime = 1479168000; // November 15th, 2016.
        consensus.vDeployments[Consensus::DEPLOYMENT_SEGWIT].nTimeout = 1510704000; // November 15th, 2017.

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0000000000000000000000000000000000000000000000000000000000000000");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0000000000000000000000000000000000000000000000000000000000000000");

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0x78;
        pchMessageStart[1] = 0x92;
        pchMessageStart[2] = 0x30;
        pchMessageStart[3] = 0x39;
        nDefaultPort = 21931;
        nPruneAfterHeight = 100000;
        nMaxReorganizationDepth = 100;

        genesis = CreateGenesisBlock(1636592000, 1, 0x207fffff, 1, 0 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("36e44ebfce5fa314e5de80c28a05f74aa46abd7e3bd0750c7b9618508f4414e5"));
        assert(genesis.hashMerkleRoot == uint256S("558288e9f2dbdd2c5a9ed64d2962a5679b83bda205394564609cfddbbaab6193"));

        vFixedSeeds.clear();
        vSeeds.clear();

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,15);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,9);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,143);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};
        bech32_hrp = "vx";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;
        nCollateralLevels = { 100000 };
        nPoolMaxTransactions = 3;
        nFulfilledRequestExpireTime = 60*60;
        strSporkPubKey = "045078e030f9b5131e2fe4de7bf81761e5eb609309f951ca3dd63b85fc702b97c736de0644562e48b75c2555a5765bdc2aecc15264777b99ec2f2643d4521a50e2"; //! 76ekH6a2mnJXmeqxfmSduYXYKv7wZ1oG3F

        checkpointData = {{
            { 0, genesis.GetHash() }
        }};

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        /* disable fallback fee on mainnet */
        m_fallback_fee_enabled = true;
    }
};

static std::unique_ptr<CChainParams> globalChainParams;

const CChainParams &Params() {
    assert(globalChainParams);
    return *globalChainParams;
}

std::unique_ptr<CChainParams> CreateChainParams(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
        return std::unique_ptr<CChainParams>(new CMainParams());
    else if (chain == CBaseChainParams::TESTNET)
        return std::unique_ptr<CChainParams>(new CTestNetParams());
    else if (chain == CBaseChainParams::REGTEST)
        return std::unique_ptr<CChainParams>(new CRegTestParams());
    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(network);
}

void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    globalChainParams->UpdateVersionBitsParameters(d, nStartTime, nTimeout);
}
