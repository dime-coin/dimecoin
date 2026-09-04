// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INDEX_BLOCKFILTERINDEX_H
#define BITCOIN_INDEX_BLOCKFILTERINDEX_H

#include <blockfilter.h>
#include <index/base.h>

/** Default for -blockfilterindex, controlling whether the BIP158 filter index
 *  is maintained for serving BIP157 light clients. */
static constexpr bool DEFAULT_BLOCKFILTERINDEX{false};

/** Map a block hash to its serialized BIP158 compact filter. */
class BlockFilterIndexDB : public BaseIndex::DB
{
public:
    BlockFilterIndexDB(size_t n_cache_size, bool f_memory = false, bool f_wipe = false);

    bool WriteFilter(const uint256& block_hash, const std::vector<unsigned char>& filter);
    bool ReadFilter(const uint256& block_hash, std::vector<unsigned char>& filter) const;

    bool WriteHeader(const uint256& block_hash, const uint256& header);
    bool ReadHeader(const uint256& block_hash, uint256& header) const;
};

/** Maintains a BIP158 compact block filter (BASIC type) for every block in the
 *  active chain, so the node can serve BIP157 light clients. */
class CBlockFilterIndex final : public BaseIndex
{
private:
    const std::unique_ptr<BlockFilterIndexDB> m_db;

protected:
    DB& GetDB() const override { return *m_db; }
    const char* GetName() const override { return "blockfilterindex"; }
    bool WriteBlock(const CBlock& block, const CBlockIndex* pindex) override;

public:
    explicit CBlockFilterIndex(size_t n_cache_size, bool f_memory = false, bool f_wipe = false);

    /** Look up the BASIC filter for a block. Returns false if not indexed. */
    bool GetFilter(const uint256& block_hash, uint256& block_hash_out,
                   std::vector<unsigned char>& filter) const;

    /** Look up the BIP157 filter header for a block. */
    bool GetHeader(const uint256& block_hash, uint256& block_hash_out, uint256& header) const;
};

/** Global instance, created in init.cpp. Null when -blockfilterindex=0. */
extern std::unique_ptr<CBlockFilterIndex> g_filter_index;

#endif // BITCOIN_INDEX_BLOCKFILTERINDEX_H
