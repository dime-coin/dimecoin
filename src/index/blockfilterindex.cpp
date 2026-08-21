// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <index/blockfilterindex.h>
#include <undo.h>
#include <util/system.h>
#include <validation.h>

#include <logging.h>

namespace {
static constexpr char DB_FILTER = 'F';
static constexpr char DB_HEADER = 'H';

std::vector<unsigned char> MakeKey(char type, const uint256& hash)
{
    std::vector<unsigned char> key;
    key.reserve(1 + 32);
    key.push_back(type);
    key.insert(key.end(), hash.begin(), hash.end());
    return key;
}
} // namespace

BlockFilterIndexDB::BlockFilterIndexDB(size_t n_cache_size, bool f_memory, bool f_wipe)
    : BaseIndex::DB(GetDataDir() / "indexes" / "blockfilter", n_cache_size, f_memory, f_wipe)
{
}

bool BlockFilterIndexDB::WriteFilter(const uint256& block_hash, const std::vector<unsigned char>& filter)
{
    return Write(MakeKey(DB_FILTER, block_hash), filter);
}

bool BlockFilterIndexDB::ReadFilter(const uint256& block_hash, std::vector<unsigned char>& filter) const
{
    return Read(MakeKey(DB_FILTER, block_hash), filter);
}

bool BlockFilterIndexDB::WriteHeader(const uint256& block_hash, const uint256& header)
{
    return Write(MakeKey(DB_HEADER, block_hash), header);
}

bool BlockFilterIndexDB::ReadHeader(const uint256& block_hash, uint256& header) const
{
    return Read(MakeKey(DB_HEADER, block_hash), header);
}

std::unique_ptr<CBlockFilterIndex> g_filter_index;

CBlockFilterIndex::CBlockFilterIndex(size_t n_cache_size, bool f_memory, bool f_wipe)
    : m_db(std::make_unique<BlockFilterIndexDB>(n_cache_size, f_memory, f_wipe))
{
}

bool CBlockFilterIndex::WriteBlock(const CBlock& block, const CBlockIndex* pindex)
{
    // Read the block undo data to recover the scripts of spent outputs, which
    // are no longer present in the UTXO set at BlockConnected time.
    CBlockUndo block_undo;
    if (!ReadBlockUndo(pindex, block_undo)) {
        LogPrintf("*** %s: failed to read block undo for %s\n", __func__, pindex->GetBlockHash().ToString());
        return false;
    }

    std::vector<CScript> elements;
    int undo_idx = 0;
    for (const CTransactionRef& tx : block.vtx) {
        if (!tx->IsCoinBase()) {
            const CTxUndo& tx_undo = block_undo.vtxundo[undo_idx++];
            for (const Coin& prevout : tx_undo.vprevout) {
                if (!prevout.out.scriptPubKey.empty()) {
                    elements.push_back(prevout.out.scriptPubKey);
                }
            }
        }
        for (const CTxOut& out : tx->vout) {
            // Exclude OP_RETURN outputs so the filter can later be committed to
            // via a soft-fork without a circular dependency.
            if (out.scriptPubKey.empty() || out.scriptPubKey[0] == OP_RETURN) {
                continue;
            }
            elements.push_back(out.scriptPubKey);
        }
    }

    uint256 block_hash = block.GetHash();
    BlockFilter filter(BlockFilterType::BASIC, block_hash, elements);

    uint256 prev_header;
    if (pindex->pprev) {
        uint256 prev_hash = pindex->pprev->GetBlockHash();
        if (!m_db->ReadHeader(prev_hash, prev_header)) {
            // Previous header not yet indexed; leave prev_header null. The
            // header chain remains consistent once the index catches up.
            prev_header.SetNull();
        }
    }
    uint256 header = filter.ComputeHeader(prev_header);

    return m_db->WriteFilter(block_hash, filter.GetEncoded()) &&
           m_db->WriteHeader(block_hash, header);
}

bool CBlockFilterIndex::GetFilter(const uint256& block_hash, uint256& block_hash_out,
                                  std::vector<unsigned char>& filter) const
{
    if (!m_db->ReadFilter(block_hash, filter)) {
        return false;
    }
    block_hash_out = block_hash;
    return true;
}

bool CBlockFilterIndex::GetHeader(const uint256& block_hash, uint256& block_hash_out,
                                  uint256& header) const
{
    if (!m_db->ReadHeader(block_hash, header)) {
        return false;
    }
    block_hash_out = block_hash;
    return true;
}
