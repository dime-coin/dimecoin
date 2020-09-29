// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>

#include <util.h>
#include <chain.h>
#include <uint256.h>
#include <chainparams.h>
#include <arith_uint256.h>
#include <primitives/block.h>

const CBlockIndex* GetLastBlockIndex(const CBlockIndex* pindex, bool fProofOfStake)
{
    while (pindex && pindex->pprev && (pindex->IsProofOfStake() != fProofOfStake))
        pindex = pindex->pprev;
    return pindex;
}

unsigned int DualKGW3(const CBlockIndex* pindexLast, const Consensus::Params& params)
{
    const CBlockIndex* BlockLastSolved = GetLastBlockIndex(pindexLast, false);
    const CBlockIndex* BlockReading = GetLastBlockIndex(pindexLast, false);
    int64_t PastBlocksMass = 0;
    int64_t PastRateActualSeconds = 0;
    int64_t PastRateTargetSeconds = 0;
    double PastRateAdjustmentRatio = double(1);
    arith_uint256 PastDifficultyAverage;
    arith_uint256 PastDifficultyAveragePrev;
    double EventHorizonDeviation;
    double EventHorizonDeviationFast;
    double EventHorizonDeviationSlow;
    static const int64_t Blocktime = params.nPowTargetSpacing;
    static const unsigned int timeDaySeconds = 86400;
    uint64_t pastSecondsMin = timeDaySeconds * 0.025;
    uint64_t pastSecondsMax = timeDaySeconds * 7;
    uint64_t PastBlocksMin = pastSecondsMin / Blocktime;
    uint64_t PastBlocksMax = pastSecondsMax / Blocktime;
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);

    if (BlockLastSolved == NULL || BlockLastSolved->nHeight == 0 ||
            (uint64_t)BlockLastSolved->nHeight < PastBlocksMin)
        return bnPowLimit.GetCompact();

    for (unsigned int i = 1; BlockReading && BlockReading->nHeight > 0; i++) {
        if (PastBlocksMax > 0 && i > PastBlocksMax) break;
        PastBlocksMass++;
        PastDifficultyAverage.SetCompact(BlockReading->nBits);
        if (i > 1) {
            if (PastDifficultyAverage >= PastDifficultyAveragePrev)
                PastDifficultyAverage =
                        ((PastDifficultyAverage - PastDifficultyAveragePrev) / i) +
                        PastDifficultyAveragePrev;
            else
                PastDifficultyAverage =
                        PastDifficultyAveragePrev -
                        ((PastDifficultyAveragePrev - PastDifficultyAverage) / i);
        }
        PastDifficultyAveragePrev = PastDifficultyAverage;
        PastRateActualSeconds =
                BlockLastSolved->GetBlockTime() - BlockReading->GetBlockTime();
        PastRateTargetSeconds = Blocktime * PastBlocksMass;
        PastRateAdjustmentRatio = double(1);
        if (PastRateActualSeconds < 0) PastRateActualSeconds = 0;
        if (PastRateActualSeconds != 0 && PastRateTargetSeconds != 0)
            PastRateAdjustmentRatio =
                    double(PastRateTargetSeconds) / double(PastRateActualSeconds);
        EventHorizonDeviation =
                1 + (0.7084 * pow((double(PastBlocksMass) / double(72)), -1.228));
        EventHorizonDeviationFast = EventHorizonDeviation;
        EventHorizonDeviationSlow = 1 / EventHorizonDeviation;

        if (PastBlocksMass >= PastBlocksMin) {
            if ((PastRateAdjustmentRatio <= EventHorizonDeviationSlow) ||
                    (PastRateAdjustmentRatio >= EventHorizonDeviationFast)) {
                assert(BlockReading);
                break;
            }
        }
        if (BlockReading->pprev == NULL) {
            assert(BlockReading);
            break;
        }
        BlockReading = BlockReading->pprev;
    }

    arith_uint256 kgw_dual1(PastDifficultyAverage);
    arith_uint256 kgw_dual2;
    kgw_dual2.SetCompact(pindexLast->nBits);
    if (PastRateActualSeconds != 0 && PastRateTargetSeconds != 0) {
        kgw_dual1 *= PastRateActualSeconds;
        kgw_dual1 /= PastRateTargetSeconds;
    }
    int64_t nActualTime1 =
            pindexLast->GetBlockTime() - pindexLast->pprev->GetBlockTime();
    int64_t nActualTimespanshort = nActualTime1;

    if (nActualTime1 < 0) nActualTime1 = Blocktime;
    if (nActualTime1 < Blocktime / 3) nActualTime1 = Blocktime / 3;
    if (nActualTime1 > Blocktime * 3) nActualTime1 = Blocktime * 3;
    kgw_dual2 *= nActualTime1;
    kgw_dual2 /= Blocktime;
    arith_uint256 bnNew;
    bnNew = ((kgw_dual2 + kgw_dual1) / 2);

    if (nActualTimespanshort < Blocktime / 6) {
        const int nLongShortNew1 = 85;
        const int nLongShortNew2 = 100;
        bnNew = bnNew * nLongShortNew1;
        bnNew = bnNew / nLongShortNew2;
    }

    if (bnNew > bnPowLimit) bnNew = bnPowLimit;

    return bnNew.GetCompact();
}

unsigned int PoSWorkRequired(const CBlockIndex* pindexLast, const Consensus::Params& params)
{
    const CBlockIndex* LastPoSBlock = GetLastBlockIndex(pindexLast, true);
    const arith_uint256 bnPosLimit = UintToArith256(params.posLimit);
    int64_t nTargetSpacing = Params().GetConsensus().nPosTargetSpacing;
    int64_t nTargetTimespan = Params().GetConsensus().nPosTargetTimespan;

    int64_t nActualSpacing = 0;
    if (LastPoSBlock->nHeight != 0)
        nActualSpacing = LastPoSBlock->GetBlockTime() - LastPoSBlock->pprev->GetBlockTime();

    if (nActualSpacing < 0)
        nActualSpacing = 1;

    // ppcoin: target change every block
    // ppcoin: retarget with exponential moving toward target spacing
    arith_uint256 bnNew;
    bnNew.SetCompact(LastPoSBlock->nBits);

    int64_t nInterval = nTargetTimespan / nTargetSpacing;
    bnNew *= ((nInterval - 1) * nTargetSpacing + nActualSpacing + nActualSpacing);
    bnNew /= ((nInterval + 1) * nTargetSpacing);

    if (bnNew <= 0 || bnNew > bnPosLimit)
        bnNew = bnPosLimit;

    return bnNew.GetCompact();
}

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const Consensus::Params& params, bool fProofOfStake)
{
    unsigned int nBits = 0;
    if(fProofOfStake)
        nBits = PoSWorkRequired(pindexLast, params);
    else
        nBits = DualKGW3(pindexLast, params);
    return nBits;
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
