// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"

#include "random.h"
#include "util.h"
#include "utilstrencodings.h"

#include <assert.h>

#include <boost/assign/list_of.hpp>

using namespace std;
using namespace boost::assign;

struct SeedSpec6 {
    uint8_t addr[16];
    uint16_t port;
};

#include "chainparamsseeds.h"

/**
 * Main network
 */

//! Convert the pnSeeds6 array into usable address objects.
static void convertSeed6(std::vector<CAddress> &vSeedsOut, const SeedSpec6 *data, unsigned int count)
{
    // It'll only connect to one or two seed nodes because once it connects,
    // it'll get a pile of addresses with newer timestamps.
    // Seed nodes are given a random 'last seen time' of between one and two
    // weeks ago.
    const int64_t nOneWeek = 7*24*60*60;
    for (unsigned int i = 0; i < count; i++)
    {
        struct in6_addr ip;
        memcpy(&ip, data[i].addr, sizeof(ip));
        CAddress addr(CService(ip, data[i].port));
        addr.nTime = GetTime() - GetRand(nOneWeek) - nOneWeek;
        vSeedsOut.push_back(addr);
    }
}

static const unsigned int timeMainGenesisBlock = 1387807823;                                            //DIME
uint256 hashMainGenesisBlock("0x00000d5a9113f87575c77eb5442845ff8a0014f6e79e2dd2317d88946ef910da");     //DIME
static uint256 nMainProofOfWorkLimit(~uint256(0) >> 20);

static const int64_t nGenesisBlockRewardCoin = 1 * COIN;
static const int64_t nBlockRewardStartCoin = 1024 * COIN; //DIME
static const int64_t nBlockRewardMinimumCoin = 1 * COIN;

/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */
static Checkpoints::MapCheckpoints mapCheckpoints =
        boost::assign::map_list_of
        (       0,     uint256("0x00000d5a9113f87575c77eb5442845ff8a0014f6e79e2dd2317d88946ef910da"))
	    (   20000,     uint256("0x0000008e1581ca6fc2a796401be25dd68bd444a286a322f1f7354b2f102e2e26"))
	    (   30000,     uint256("0x00000034a4be19182d0e8e56bad69541be2e6fa3274043e6c0fb90ad4a436820"))
	    (   40000,     uint256("0x0000004227fb1fc8dfd51919cdc080ede60c3c8590b38e6c88b3d733d805b779"))
	    (   50000,     uint256("0x0000001f6750308a0683f327488c0393e2b255ad0aa61d88716cd10bd8d44b5f"))
	    (   60000,     uint256("0x00000007b938fb5231039fb2f5330cce860e3db7a6542d411bfa796e0c88b20d"))
	    (   70000,     uint256("0x00000001a9f5cb9dd8a033b0b112bd946795bfd9266bff087eebce699e028696"))
	    (   80000,     uint256("0x00000000f87ac340b2ac8f3a33659b27857a1af7a4a2c69b6c6ec85af54b155b"))
	    (   90000,     uint256("0x0000000054e78ffd1a9bed31df6833bd5d86b79f3b18d45dc8c6e8455a5ac6c3"))
	    (  100000,     uint256("0x00000000539760ef8dd9d933743b6281e8337359ecfa35917f924e52156c7566"))
	    (  109346,     uint256("0x0000000003a8062398ad2479884ee028ca0b7cc2daa143b3173afa7068976b74"))
	    (  250000,     uint256("0x000000001ecc6fd9f2a2e50722762de18bc96599e6d00e3ddc9e2d97d4b177ff"))
    	(  350000,     uint256("0x000000001929a0b35aa3199a45d6f5a4a936b3347ae5cea2cea2b8b5ffdab731"))
        (  500000,     uint256("0x00000000129e32df9b68e9478fb99ad094ca3311e26b853a07d864c557ae3696"))
        (  600000,     uint256("0x000000000c12b6426aafd24960e86704c7a2eb4b73f96211f00070d939f3bbba"))
        (  700000,     uint256("0x000000002954073d358a9c13de05711b7be77d669b02c40435931e801cd1c4ed"))
        (  800000,     uint256("0x00000000063b01cae9db85adab32585e02f8c31ac9c26c22a5372408db32ef72"))
        (  900000,     uint256("0x0000000018b771af50cb42842fdf16276458b27f20c8e95595e05bacd5c377ab"))
        ( 1000000,     uint256("0x000000001e2ef1ef0f9f587150e598faa2e74ff95c0bcdd2026d2688b2bbf13a"))
        ( 1100000,     uint256("0x00000000355853adaf8f538c6395b6ab08eb37ada54996b3ac3ff56f1e1cd372"))
        ( 1200000,     uint256("0x0000000003edb0a84251f028361def13930b5ce87adc34680100d3a0f8563418"))
        ( 1300000,     uint256("0x000000015c75a460f24f920db11fd4f6a98fbe5242253bc7c98e3f2900d05586"))
        ( 1400000,     uint256("0x000000002c8fcd56cf4b0a53aa0d7dbf0050ad73fcc218d4015fb70070863d7e"))
        ( 1500000,     uint256("0x000000007f0008d7b697589583d58cc0d83a62f7364859711576fe50bca70f06"))
        ( 1550000,     uint256("0x000000004952592fc7376464d63412f01d2877587a19a08432edd788b005723e"))
        ( 1700000,     uint256("0x00000003b6cd157bfe643cd3c624ec32096709d2bea77191def23ab62b61b73d"))
        ( 1800000,     uint256("0x00000000002f3679c71961e3d989244a2f4e4e9cb75ccb7ff7ac3c500d25ee19"))
        ( 1900000,     uint256("0x0000000001acf206f3c7e4d181f550281cae2735ef1a427f089f45f3651c5717"))
        ( 2000000,     uint256("0x000000000a212f4cfab5d16c26868c37067a4642691b77230bb68a60a10c97e1"))
        ( 2100000,     uint256("0x000000002be14a1a595300b753bb65ac332bfa9715db3535c09d02b0122d8c82"))
        ( 2200000,     uint256("0x0000000028908252525e08a5039a6a2f7f4dc5dae8715b30e86af9286134411f"))
        ( 2300000,     uint256("0x0000000000dac114de7426040d12269c3b48724a64a186d1fb82b0be5dd3f07b"))
        ( 2400000,     uint256("0x0000000000cb3e9c9c2082e80465ea6780320fb3481d67964bcd6965a2077380"))
        ( 2500000,     uint256("0x0000000000010f978067c92689a39ef8c6f17849a6e3c6c86ac8685e8f7f84af"))
        ( 2600000,     uint256("0x0000000000154d18cff372412d641e4f4a17797e1eb5a1ff714910b31dad1632"))
      //new checkpoints added in version 1.9.0.0
        ( 2700000,     uint256("0x000000000005fb1c672edbd26aad6e86db15dc6b7f2ad9bf0bdefdd7a2fe74c5"))
        ( 2800000,     uint256("0x000000000001ee0e0a348d177511ed2d79b4ec98359242c0b386e1931d42489a"))     
        ( 2900000,     uint256("0x000000000002c4539330ab986e1fa4f2fe720559d8858c381859f3b896cac7a5"))
      //new checkpoints added in version 1.9.0.2
        ( 3000000,     uint256("0x000000000000c7552284192bd1e3e7b1b7648ec4868cef082f2aa4945f7bad36"))
        ( 3100000,     uint256("0x00000000000e46ee7f527b6ba4b557ef49a9c17ae4a852a7e0fe3023048288f4"))
      //new checkpoint added in version 1.9.1.0
        ( 3200000,     uint256("0x0000000000015dfaf2bf3f4c05192f61baa66bd682df1359bf77bfa07c2c9e0c"))   
        ;
static const Checkpoints::CCheckpointData data = {
        &mapCheckpoints,
        1539877199, // * UNIX timestamp of last checkpoint block 3100000
        4173777,    // * total number of transactions between genesis and last checkpoint 3200000
                    //   (the tx=... number in the SetBestChain debug.log lines)
        2880.0      // * estimated number of transactions per day after checkpoint
    };

static Checkpoints::MapCheckpoints mapCheckpointsTestnet =
        boost::assign::map_list_of
        ( 0  , uint256("0x0000025aeaa593ebf9bb305fd1d00b4972d87aacf688b1f48434ff5f12488a35")) //DIME testnet
        ( 238, uint256("0x00000ee17c7e6fec2bbb3e022a23d35b41bebf03d028f44fba23596742dc4378")) 
        ;
static const Checkpoints::CCheckpointData dataTestnet = {
        &mapCheckpointsTestnet,
        1538258716,
        240,
        2880
    };

static Checkpoints::MapCheckpoints mapCheckpointsRegtest =
        boost::assign::map_list_of
        ( 0, uint256("0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206"))
        ;
static const Checkpoints::CCheckpointData dataRegtest = {
        &mapCheckpointsRegtest,
        0,
        0,
        0
    };

class CMainParams : public CChainParams {
public:
    CMainParams() {
        networkID = CBaseChainParams::MAIN;
        strNetworkID = "main";
        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 4-byte int at any alignment.
         */
        pchMessageStart[0] = 0xfe;
        pchMessageStart[1] = 0xa5;
        pchMessageStart[2] = 0x03;
        pchMessageStart[3] = 0xdd;
        vAlertPubKey = ParseHex("04e7fc3ea64e8fc816371c8ecb26852f9b864e584da18464999accfebb627e3196cc4266d778f2ba3394752a76027b7e1d9e981b1098b6441f26034314bd9a8a26");
        nDefaultPort = 11931; //DIME dns seed port
        bnProofOfWorkLimit = nMainProofOfWorkLimit;
        nSubsidyHalvingInterval = 512000; //DIME
        nEnforceBlockUpgradeMajority = 7500;
        nRejectBlockOutdatedMajority = 9000;
        nToCheckBlockUpgradeMajority = 10000;
        nMinerThreads = 0;
        nTargetTimespan = 65536; // DIME : about 18 hour
        nTargetSpacing = 64;     // DIME : 64 seconds
        nMaxTipAge = 96 * 60 * 60; //DIME
        strCheckpointPubKey = "045af95d3e64f3166cef9fab4ce87f6a085055b7552bac891c2600f7a90b382053f88b0741282b02c763b1b3352de4dd98d8b32c4664686b6ba050e6c2f8ca0520"; // ACP
        //LWMA3 implementation
        nLwma3Height = 9999999; 
        nLwmaAveragingWindow = 90;
        //LWMA3 - end

        /**
         * Build the genesis block. Note that the output of the genesis coinbase cannot
         * be spent as it did not originally exist in the database.
         *
         * CBlock(hash=000000000019d6, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=4a5e1e, nTime=1231006505, nBits=1d00ffff, nNonce=2083236893, vtx=1)
         *   CTransaction(hash=4a5e1e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
         *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73)
         *     CTxOut(nValue=50.00000000, scriptPubKey=0x5F1DF16B2B704C8A578D0B)
         *   vMerkleTree: 4a5e1e
         */
        //const char* pszTimestamp = "21 July 2013, The Guardian, Tesco boss says cheap food era is over"
        const char* pszTimestamp = "BIN COIN START"; //DIME
        CMutableTransaction txNew;
        txNew.vin.resize(1);
        txNew.vout.resize(1);
        txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
        txNew.vout[0].nValue = nGenesisBlockRewardCoin;
        txNew.vout[0].scriptPubKey = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;
        genesis.vtx.push_back(txNew);
        genesis.hashPrevBlock = 0;
        genesis.hashMerkleRoot = genesis.BuildMerkleTree();
        genesis.nVersion = 112;
        genesis.nTime    = timeMainGenesisBlock;
        genesis.nBits    = nMainProofOfWorkLimit.GetCompact();
        genesis.nNonce   = 16888732; //DIME

        assert(genesis.hashMerkleRoot == uint256("0x72596a6a36d42416b5486386c6e2b7e339782ef4eb49fb8a60ec7dc3475da545")); //DIME

        hashGenesisBlock = genesis.GetHash();
        assert(hashGenesisBlock == hashMainGenesisBlock);

        vSeeds.push_back(CDNSSeedData("seed1.dimecoinnetwork.com", "seed1.dimecoinnetwork.com"));         //Primary DNS Seed
        vSeeds.push_back(CDNSSeedData("seed2.dimecoinnetwork.com", "seed2.dimecoinnetwork.com"));         //Secondary DNS Seed
        vSeeds.push_back(CDNSSeedData("node1.dimecoinnetwork.com", "node1.dimecoinnetwork.com"));         //Primary node
        vSeeds.push_back(CDNSSeedData("node2.dimecoinnetwork.com", "node2.dimecoinnetwork.com"));         //Secondary node
        vSeeds.push_back(CDNSSeedData("dime-pool.dimecoinnetwork.com", "dime-pool.dimecoinnetwork.com")); //dime-pool.com node 
 
/* used if using -std=c++11
        base58Prefixes[PUBKEY_ADDRESS] = {15};
        base58Prefixes[SCRIPT_ADDRESS] = {9};
        base58Prefixes[SECRET_KEY] =     {143};
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04,0x88,0xB2,0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04,0x88,0xAD,0xE4};
*/
        base58Prefixes[PUBKEY_ADDRESS] = list_of(15);
        base58Prefixes[SCRIPT_ADDRESS] = list_of(9);
        base58Prefixes[SECRET_KEY] =     list_of(143);
        base58Prefixes[EXT_PUBLIC_KEY] = list_of(0x04)(0x88)(0xB2)(0x1E);
        base58Prefixes[EXT_SECRET_KEY] = list_of(0x04)(0x88)(0xAD)(0xE4);

        convertSeed6(vFixedSeeds, pnSeed6_main, ARRAYLEN(pnSeed6_main));

        fRequireRPCPassword = true;
        fMiningRequiresPeers = true;
        fAllowMinDifficultyBlocks = false;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;
        fSkipProofOfWorkCheck = false;
        fTestnetToBeDeprecatedFieldRPC = false;
    }

    const Checkpoints::CCheckpointData& Checkpoints() const
    {
        return data;
    }
};
static CMainParams mainParams;

/**
 * Testnet (v3)
 */
class CTestNetParams : public CMainParams {
public:
    CTestNetParams() {
        networkID = CBaseChainParams::TESTNET;
        strNetworkID = "test";
        pchMessageStart[0] = 0x01;
        pchMessageStart[1] = 0x1A;
        pchMessageStart[2] = 0x39;
        pchMessageStart[3] = 0xF7;
        vAlertPubKey = ParseHex("04c5aca36066b187e3cd2ff178309c652a4196cc230e8018ce2afed658bd0c073cba4c93b1c119f51ab65a5af8851c3bda386c35bcf45b77a34050a0b6e042f7da"); //DIME
        nDefaultPort = 21931; //DIME Testnet port
        nEnforceBlockUpgradeMajority = 51;
        nRejectBlockOutdatedMajority = 75;
        nToCheckBlockUpgradeMajority = 100;
        nMinerThreads = 0;
        nTargetTimespan = 65536; // DIME : ten minutes
        nTargetSpacing = 64;     // DIME : 30 seconds
        nMaxTipAge = 0x7fffffff;
        strCheckpointPubKey = "042f99f07597141a8675be9bb36666b332ad8ba4a3b7e2ae913f0d2f8521342a7fe88ae9cd5d13faf16bedd6d43ad66c48d0f9609be878c6420395076c956898b8"; // ACP
        nLwma3Height = 30000; //LWMA implementation

        //! Modify the testnet genesis block so the timestamp is valid for a later start.
        genesis.nTime = 1514764800; //new testnet 1/01/2018
        genesis.nNonce = 146813;
        hashGenesisBlock = genesis.GetHash();


/* create a genesis block with a hash below first nbits
        while (hashGenesisBlock > uint256("00000fffff000000000000000000000000000000000000000000000000000000"))
        {

                if (hashGenesisBlock < uint256("00000fffff000000000000000000000000000000000000000000000000000000"))
                    break;
                if ((genesis.nNonce & 0xFFF) == 0)
                {
                    printf("nonce %08X: hash = %s (target = %s)\n", genesis.nNonce, hashGenesisBlock.ToString().c_str(), uint256("00000fffff000000000000000000000000000000000000000000000000000000").ToString().c_str());
                }
                ++genesis.nNonce;
                if (genesis.nNonce == 0)
                {
                    printf("NONCE WRAPPED, incrementing time\n");
                    ++genesis.nTime;
                }
                hashGenesisBlock = genesis.GetHash();

            printf("block.nTime = %u \n", genesis.nTime);
            printf("block.nNonce = %u \n", genesis.nNonce);
            printf("block.GetHash = %s\n", hashGenesisBlock.ToString().c_str());
        } */

        assert(hashGenesisBlock == uint256("0x0000025aeaa593ebf9bb305fd1d00b4972d87aacf688b1f48434ff5f12488a35")); //DIME

        vFixedSeeds.clear();
        vSeeds.clear();

/* used if using -std=c++11
        base58Prefixes[PUBKEY_ADDRESS] = {119};
        base58Prefixes[SCRIPT_ADDRESS] = {199};
        base58Prefixes[SECRET_KEY]     = {247};
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04,0x35,0x87,0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04,0x35,0x83,0x94};
*/

        base58Prefixes[PUBKEY_ADDRESS] = list_of(119);
        base58Prefixes[SCRIPT_ADDRESS] = list_of(199);
        base58Prefixes[SECRET_KEY] =     list_of(247);
        base58Prefixes[EXT_PUBLIC_KEY] = list_of(0x04)(0x35)(0x87)(0xCF);
        base58Prefixes[EXT_SECRET_KEY] = list_of(0x04)(0x35)(0x83)(0x94);

        convertSeed6(vFixedSeeds, pnSeed6_test, ARRAYLEN(pnSeed6_test));

        fRequireRPCPassword = true;
        fMiningRequiresPeers = true;
        fAllowMinDifficultyBlocks = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;
        fTestnetToBeDeprecatedFieldRPC = true;
    }
    const Checkpoints::CCheckpointData& Checkpoints() const
    {
        return dataTestnet;
    }
};
static CTestNetParams testNetParams;

/**
 * Regression test
 */
class CRegTestParams : public CTestNetParams {
public:
    CRegTestParams() {
        networkID = CBaseChainParams::REGTEST;
        strNetworkID = "regtest";
        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbf;
        pchMessageStart[2] = 0xb5;
        pchMessageStart[3] = 0xda;
        nSubsidyHalvingInterval = 150;
        nEnforceBlockUpgradeMajority = 750;
        nRejectBlockOutdatedMajority = 950;
        nToCheckBlockUpgradeMajority = 1000;
        nMinerThreads = 1;
        nTargetTimespan = 10 * 60; // ten minutes
        nTargetSpacing = 30; // 30 seconds
        bnProofOfWorkLimit = ~uint256(0) >> 1;
        nMaxTipAge = 24 * 60 * 60;
        strCheckpointPubKey = ""; //ACP
        genesis.nTime = 1296688602;
        genesis.nBits = 0x207fffff;
        genesis.nNonce = 2;
        hashGenesisBlock = genesis.GetHash();
        nDefaultPort = 18444;
        // assert(hashGenesisBlock == uint256("0x0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206"));

        vFixedSeeds.clear(); //! Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();  //! Regtest mode doesn't have any DNS seeds.

        fRequireRPCPassword = false;
        fMiningRequiresPeers = false;
        fAllowMinDifficultyBlocks = true;
        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;
        fTestnetToBeDeprecatedFieldRPC = false;
    }
    const Checkpoints::CCheckpointData& Checkpoints() const
    {
        return dataRegtest;
    }
};
static CRegTestParams regTestParams;

/**
 * Unit test
 */
class CUnitTestParams : public CMainParams, public CModifiableParams {
public:
    CUnitTestParams() {
        networkID = CBaseChainParams::UNITTEST;
        strNetworkID = "unittest";
        nDefaultPort = 18445;
        vFixedSeeds.clear(); //! Unit test mode doesn't have any fixed seeds.
        vSeeds.clear();  //! Unit test mode doesn't have any DNS seeds.

        fRequireRPCPassword = false;
        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = true;
        fAllowMinDifficultyBlocks = false;
        fMineBlocksOnDemand = true;
    }

    const Checkpoints::CCheckpointData& Checkpoints() const
    {
        // UnitTest share the same checkpoints as MAIN
        return data;
    }

    //! Published setters to allow changing values in unit test cases
    virtual void setSubsidyHalvingInterval(int anSubsidyHalvingInterval)  { nSubsidyHalvingInterval=anSubsidyHalvingInterval; }
    virtual void setEnforceBlockUpgradeMajority(int anEnforceBlockUpgradeMajority)  { nEnforceBlockUpgradeMajority=anEnforceBlockUpgradeMajority; }
    virtual void setRejectBlockOutdatedMajority(int anRejectBlockOutdatedMajority)  { nRejectBlockOutdatedMajority=anRejectBlockOutdatedMajority; }
    virtual void setToCheckBlockUpgradeMajority(int anToCheckBlockUpgradeMajority)  { nToCheckBlockUpgradeMajority=anToCheckBlockUpgradeMajority; }
    virtual void setDefaultConsistencyChecks(bool afDefaultConsistencyChecks)  { fDefaultConsistencyChecks=afDefaultConsistencyChecks; }
    virtual void setAllowMinDifficultyBlocks(bool afAllowMinDifficultyBlocks) {  fAllowMinDifficultyBlocks=afAllowMinDifficultyBlocks; }
    virtual void setSkipProofOfWorkCheck(bool afSkipProofOfWorkCheck) { fSkipProofOfWorkCheck = afSkipProofOfWorkCheck; }
};
static CUnitTestParams unitTestParams;


static CChainParams *pCurrentParams = 0;

CModifiableParams *ModifiableParams()
{
   assert(pCurrentParams);
   assert(pCurrentParams==&unitTestParams);
   return (CModifiableParams*)&unitTestParams;
}

const CChainParams &Params() {
    assert(pCurrentParams);
    return *pCurrentParams;
}

CChainParams &Params(CBaseChainParams::Network network) {
    switch (network) {
        case CBaseChainParams::MAIN:
            return mainParams;
        case CBaseChainParams::TESTNET:
            return testNetParams;
        case CBaseChainParams::REGTEST:
            return regTestParams;
        case CBaseChainParams::UNITTEST:
            return unitTestParams;
        default:
            assert(false && "Unimplemented network");
            return mainParams;
    }
}

void SelectParams(CBaseChainParams::Network network) {
    SelectBaseParams(network);
    pCurrentParams = &Params(network);
}

bool SelectParamsFromCommandLine()
{
    CBaseChainParams::Network network = NetworkIdFromCommandLine();
    if (network == CBaseChainParams::MAX_NETWORK_TYPES)
        return false;

    SelectParams(network);
    return true;
}
