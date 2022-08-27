// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Copyright (c) 2013-2022 The Dimecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>

#include <arith_uint256.h>
#include <chain.h>
#include <chainparams.h>
#include <primitives/block.h>
#include <uint256.h>
#include <util/system.h>

const CBlockIndex* GetLastBlockIndex(const CBlockIndex* pindex, bool fProofOfStake)
{
    while (pindex && pindex->pprev && (pindex->IsProofOfStake() != fProofOfStake))
        pindex = pindex->pprev;
    return pindex;
}

unsigned int GetNextWorkRequiredLegacy(const CBlockIndex* pindexLast)
{
    unsigned int nProofOfWorkLimit = UintToArith256(Params().GetConsensus().powLimit).GetCompact();

    // Genesis block
    if (pindexLast == NULL)
        return nProofOfWorkLimit;

    // Only change once per interval
    int nInterval = Params().GetConsensus().nPowTargetTimespan / Params().GetConsensus().nPowTargetSpacing;
    if ((pindexLast->nHeight + 1) % nInterval != 0) {
        return pindexLast->nBits;
    }

    // Go back by what we want to be nInterval blocks
    const CBlockIndex* pindexFirst = pindexLast;
    for (int i = 0; pindexFirst && i < nInterval - 1; i++)
        pindexFirst = pindexFirst->pprev;
    assert(pindexFirst);

    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - pindexFirst->GetBlockTime();
    LogPrintf("  nActualTimespan = %d before bounds\n", nActualTimespan);
    int64_t LimUp = Params().GetConsensus().nPowTargetTimespan * 100 / 110; // 110% up
    int64_t LimDown = Params().GetConsensus().nPowTargetTimespan * 2; // 200% down
    if (nActualTimespan < LimUp)
        nActualTimespan = LimUp;
    if (nActualTimespan > LimDown)
        nActualTimespan = LimDown;

    // Retarget
    arith_uint256 bnNew;
    arith_uint256 bnOld;
    bnNew.SetCompact(pindexLast->nBits);
    bnOld = bnNew;
    bnNew *= nActualTimespan;
    bnNew /= Params().GetConsensus().nPowTargetTimespan;

    if (bnNew > UintToArith256(Params().GetConsensus().powLimit))
        bnNew = UintToArith256(Params().GetConsensus().powLimit);

    return bnNew.GetCompact();
}

unsigned int Lwma3GetNextWorkRequired(const CBlockIndex* pindexLast)
{
    const int64_t T = Params().GetConsensus().nPowTargetSpacing;
    const int64_t N = 90;
    const int64_t k = N * (N + 1) * T / 2;
    const int64_t height = pindexLast->nHeight;

    if (height < N) {
        return UintToArith256(Params().GetConsensus().powLimit).GetCompact();
    }

    arith_uint256 sumTarget, previousDiff, nextTarget;
    int64_t thisTimestamp, previousTimestamp;
    int64_t t = 0, j = 0, solvetimeSum = 0;

    const CBlockIndex* blockPreviousTimestamp = pindexLast->GetAncestor(height - N);
    previousTimestamp = blockPreviousTimestamp->GetBlockTime();

    // Loop through N most recent blocks.
    for (int64_t i = height - N + 1; i <= height; i++) {
        const CBlockIndex* block = pindexLast->GetAncestor(i);
        thisTimestamp = (block->GetBlockTime() > previousTimestamp) ? block->GetBlockTime() : previousTimestamp + 1;

        int64_t solvetime = std::min(6 * T, thisTimestamp - previousTimestamp);
        previousTimestamp = thisTimestamp;

        j++;
        t += solvetime * j; // Weighted solvetime sum.
        arith_uint256 target;
        target.SetCompact(block->nBits);
        sumTarget += target / (k * N);

        if (i > height - 3) {
            solvetimeSum += solvetime;
        }
        if (i == height) {
            previousDiff = target.SetCompact(block->nBits);
        }
    }

    nextTarget = t * sumTarget;

    if (nextTarget > (previousDiff * 150) / 100) {
        nextTarget = (previousDiff * 150) / 100;
    }

    const int lwma3fixheight = 3358350;
    if (height < lwma3fixheight && ((previousDiff * 67) / 100 > nextTarget)) {
        nextTarget = (previousDiff * 67);
    }

    if (height >= lwma3fixheight && (nextTarget < (previousDiff * 67) / 100)) {
        nextTarget = (previousDiff * 67) / 100;
    }

    if (solvetimeSum < (8 * T) / 10) {
        nextTarget = previousDiff * 100 / 106;
    }

    if (nextTarget > UintToArith256(Params().GetConsensus().powLimit)) {
        nextTarget = UintToArith256(Params().GetConsensus().powLimit);
    }

    return nextTarget.GetCompact();
}

unsigned int GetNextWorkRequiredDual(const CBlockIndex* pindexLast, const Consensus::Params& consensusParams, bool fProofOfStake)
{
    int64_t nTargetSpacing = 60;
    int64_t nTargetTimespan = 150;
    arith_uint256 bnTargetLimit;
    bnTargetLimit = UintToArith256(consensusParams.powLimit);
    if(fProofOfStake)
        bnTargetLimit = UintToArith256(consensusParams.posLimit);

    if (pindexLast == nullptr)
        return bnTargetLimit.GetCompact();

    const CBlockIndex* pindexPrev = GetLastBlockIndex(pindexLast, fProofOfStake);
    if (pindexPrev->pprev == nullptr)
        return bnTargetLimit.GetCompact();

    const CBlockIndex* pindexPrevPrev = GetLastBlockIndex(pindexPrev->pprev, fProofOfStake);
    if (pindexPrevPrev->pprev == nullptr)
        return bnTargetLimit.GetCompact();

    int64_t nActualSpacing = pindexPrev->GetBlockTime() - pindexPrevPrev->GetBlockTime();
    if (nActualSpacing < 0)
        nActualSpacing = nTargetSpacing;

    // ppcoin: target change every block
    // ppcoin: retarget with exponential moving toward target spacing
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexPrev->nBits);

    int64_t nInterval = nTargetTimespan / nTargetSpacing;
    bnNew *= ((nInterval - 1) * nTargetSpacing + nActualSpacing + nActualSpacing);
    bnNew /= ((nInterval + 1) * nTargetSpacing);

    if (bnNew <= 0 || bnNew > bnTargetLimit)
        bnNew = bnTargetLimit;

    return bnNew.GetCompact();
}

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const Consensus::Params& consensusParams, bool fProofOfStake)
{
    if (Params().NetworkIDString() == CBaseChainParams::MAIN && (pindexLast->nHeight + 1 < consensusParams.fullSplitDiffHeight)) {
        const int nHeight = pindexLast->nHeight + 1;
        const int lwma3height = 3310000;
        if (nHeight < lwma3height) {
            return GetNextWorkRequiredLegacy(pindexLast);
        } else {
            return Lwma3GetNextWorkRequired(pindexLast);
        }
    } else {
        return GetNextWorkRequiredDual(pindexLast, consensusParams, fProofOfStake);
    }
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return error("CheckProofOfWork(): nBits below minimum work");

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}
