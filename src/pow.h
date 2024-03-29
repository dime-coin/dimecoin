// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Copyright (c) 2013-2022 The Dimecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POW_H
#define BITCOIN_POW_H

#include <chainparams.h>
#include <consensus/params.h>

#include <stdint.h>

class CBlockHeader;
class CBlockIndex;
class uint256;

unsigned int GetNextWorkRequiredDual(const CBlockIndex* pindexLast, const Consensus::Params& consensusParams, bool fProofOfStake);

const CBlockIndex* GetLastBlockIndex(const CBlockIndex* pindex, bool fProofOfStake);
unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const Consensus::Params& consensusParams = Params().GetConsensus(), bool fProofOfStake = false);
bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params);

#endif // BITCOIN_POW_H
