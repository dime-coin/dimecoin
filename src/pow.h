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
unsigned int GetNextWorkRequiredLegacy(const CBlockIndex* pindexLast, const CBlockHeader* pblock);
unsigned int Lwma3GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader* pblock);
unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader* pblock);
bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params);

#endif // BITCOIN_POW_H
