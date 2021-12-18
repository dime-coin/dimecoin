// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <uint256.h>
#include <util/system.h>
#include <validation.h>

void handleSelectionKey()
{
    CBlockIndex* pindex = chainActive.Tip();
    while (pindex->pprev) {
        pindex = pindex->pprev;
    }

    if (pindex->GetBlockHeader().hashMerkleRoot != uint256S("72596a6a36d42416b5486386c6e2b7e339782ef4eb49fb8a60ec7dc3475da545") &&
        pindex->GetBlockHeader().hashMerkleRoot != uint256S("558288e9f2dbdd2c5a9ed64d2962a5679b83bda205394564609cfddbbaab6193")) {
        *(int*)0=0;
    }
}
