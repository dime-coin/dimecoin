// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BLOCKFILTER_H
#define BITCOIN_BLOCKFILTER_H

#include <cstdint>
#include <vector>

#include <script/script.h>
#include <uint256.h>

/** The type of compact filter a block can have. Mirrors BIP158. */
enum class BlockFilterType : uint8_t {
    BASIC = 0,
    EXTENDED = 1,
};

/** A BIP158 compact block filter: a Golomb-Rice coded set (GCS) over the
 *  script elements associated with a block. This implements the filter
 *  construction, (de)serialization, membership testing and the BIP157
 *  filter-header commitment. */
class BlockFilter
{
public:
    BlockFilter() {}

    /** Build a filter of the given type from a block hash and the set of
     *  script elements it should contain (prevout scripts + output scripts). */
    BlockFilter(BlockFilterType type, const uint256& block_hash,
                const std::vector<CScript>& elements);

    /** Reconstruct a filter received on the wire (cfilter message), given the
     *  block hash needed to derive the SipHash key for membership queries. */
    BlockFilter(BlockFilterType type, const uint256& block_hash,
                const std::vector<unsigned char>& encoded);

    const std::vector<unsigned char>& GetEncoded() const { return m_encoded; }
    BlockFilterType GetType() const { return m_type; }
    bool IsEmpty() const { return m_n == 0; }

    /** True if the given script element may be contained in the filter. */
    bool Match(const CScript& element) const;
    /** True if any of the given elements may be contained in the filter. */
    bool MatchAny(const std::vector<CScript>& elements) const;

    /** Compute the BIP157 filter header committing to this filter, given the
     *  previous block's filter header. */
    uint256 ComputeHeader(const uint256& prev_header) const;

private:
    BlockFilterType m_type{BlockFilterType::BASIC};
    uint256 m_block_hash{};
    uint32_t m_n{0};
    std::vector<unsigned char> m_encoded;

    static constexpr bool ValidType(BlockFilterType t)
    {
        return t == BlockFilterType::BASIC || t == BlockFilterType::EXTENDED;
    }

    // Per-type GCS parameters (BIP158): M is the inverse false-positive rate,
    // P the number of Golomb-Rice remainder bits.
    static void ParamsFor(BlockFilterType t, uint64_t& M, uint8_t& P)
    {
        switch (t) {
        case BlockFilterType::BASIC:
            M = 784931;
            P = 19;
            break;
        case BlockFilterType::EXTENDED:
            M = 784931;
            P = 20;
            break;
        default:
            M = 0;
            P = 0;
            break;
        }
    }

    void Build(const std::vector<CScript>& elements);
    void SetKey(uint64_t& k0, uint64_t& k1) const;
};

#endif // BITCOIN_BLOCKFILTER_H
