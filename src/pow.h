// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POW_H
#define BITCOIN_POW_H

#include <consensus/params.h>

#include <stdint.h>

class CBlockHeader;
class CBlockIndex;
class uint256;

const CBlockIndex* GetLastBlockIndex(const CBlockIndex* pindex, bool fProofOfStake);
unsigned int DualKGW3(const CBlockIndex* pindexLast, const CBlockHeader* pblock, const Consensus::Params& params);
unsigned int PoSWorkRequired(const CBlockIndex* pindexLast, const Consensus::Params& params);
unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const Consensus::Params& params, bool fProofOfStake);
bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params);

#endif // BITCOIN_POW_H
