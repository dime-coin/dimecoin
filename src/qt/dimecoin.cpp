// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <sync.h>
#include <uint256.h>
#include <util/system.h>
#include <validation.h>

#include <cstdlib>

void handleSelectionKey()
{
    uint256 merkleRoot;
    {
        LOCK(cs_main);
        const CBlockIndex* pgenesis = chainActive.Genesis();
        if (pgenesis == nullptr) {
            // Chain not loaded yet - nothing to verify, and dereferencing the tip
            // here is what used to crash on an empty chain.
            return;
        }
        merkleRoot = pgenesis->GetBlockHeader().hashMerkleRoot;
    }

    if (merkleRoot != uint256S("72596a6a36d42416b5486386c6e2b7e339782ef4eb49fb8a60ec7dc3475da545") &&
        merkleRoot != uint256S("558288e9f2dbdd2c5a9ed64d2962a5679b83bda205394564609cfddbbaab6193")) {
        LogPrintf("%s: genesis merkle root %s matches no known Dimecoin genesis - aborting\n",
                  __func__, merkleRoot.ToString());
        std::abort();
    }
}
