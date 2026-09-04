// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <algorithm>
#include <set>
#include <stdexcept>

#include <hash.h>

#include <blockfilter.h>

namespace {

/** Minimal big-endian bit writer used to serialize the Golomb-Rice stream. */
class BitWriter
{
    std::vector<unsigned char>& m_out;
    unsigned char m_cur{0};
    int m_nbits{0};

public:
    explicit BitWriter(std::vector<unsigned char>& out) : m_out(out) {}

    ~BitWriter() { Flush(); }

    void WriteBit(int bit)
    {
        m_cur |= (bit & 1) << (7 - m_nbits);
        if (++m_nbits == 8) {
            m_out.push_back(m_cur);
            m_cur = 0;
            m_nbits = 0;
        }
    }

    void WriteBits(uint64_t n, int count)
    {
        // Big-endian: most significant bit first.
        for (int i = count - 1; i >= 0; --i) {
            WriteBit((n >> i) & 1);
        }
    }

    void Flush()
    {
        if (m_nbits != 0) {
            m_out.push_back(m_cur);
            m_cur = 0;
            m_nbits = 0;
        }
    }
};

/** Big-endian bit reader over a byte buffer. */
class BitReader
{
    const unsigned char* m_data;
    size_t m_size;
    size_t m_pos{0};

public:
    BitReader(const unsigned char* data, size_t size) : m_data(data), m_size(size) {}

    int ReadBit()
    {
        if (m_pos >= m_size * 8) return 0;
        int bit = (m_data[m_pos / 8] >> (7 - (m_pos % 8))) & 1;
        ++m_pos;
        return bit;
    }

    uint64_t ReadBits(int count)
    {
        uint64_t n = 0;
        for (int i = 0; i < count; ++i) {
            n = (n << 1) | ReadBit();
        }
        return n;
    }
};

void WriteCompactSize(std::vector<unsigned char>& out, uint64_t n)
{
    if (n < 253) {
        out.push_back(n);
    } else if (n <= 0xffff) {
        out.push_back(253);
        out.push_back(n & 0xff);
        out.push_back((n >> 8) & 0xff);
    } else if (n <= 0xffffffff) {
        out.push_back(254);
        for (int i = 0; i < 4; ++i) out.push_back((n >> (8 * i)) & 0xff);
    } else {
        out.push_back(255);
        for (int i = 0; i < 8; ++i) out.push_back((n >> (8 * i)) & 0xff);
    }
}

uint64_t ReadCompactSize(const unsigned char*& p, const unsigned char* end)
{
    if (p >= end) throw std::runtime_error("ReadCompactSize: unexpected end of data");
    uint64_t n = *p++;
    if (n == 253) {
        if (p + 2 > end) throw std::runtime_error("ReadCompactSize: truncated size (2 bytes)");
        n = p[0] | (uint64_t(p[1]) << 8);
        p += 2;
    } else if (n == 254) {
        if (p + 4 > end) throw std::runtime_error("ReadCompactSize: truncated size (4 bytes)");
        n = 0;
        for (int i = 0; i < 4; ++i) n |= uint64_t(*p++) << (8 * i);
    } else if (n == 255) {
        if (p + 8 > end) throw std::runtime_error("ReadCompactSize: truncated size (8 bytes)");
        n = 0;
        for (int i = 0; i < 8; ++i) n |= uint64_t(*p++) << (8 * i);
    }
    return n;
}

//! High 64 bits of the 128-bit product a*b, portable to toolchains that lack
//! a native __uint128_t (e.g. MSVC and some 32-bit/embedded compilers).
uint64_t UmulHi(uint64_t a, uint64_t b)
{
    uint64_t a_lo = a & 0xffffffffULL;
    uint64_t a_hi = a >> 32;
    uint64_t b_lo = b & 0xffffffffULL;
    uint64_t b_hi = b >> 32;

    uint64_t lo = a_lo * b_lo;
    uint64_t mid0 = a_hi * b_lo;
    uint64_t mid1 = a_lo * b_hi;
    uint64_t hi = a_hi * b_hi;

    uint64_t carry = ((mid0 & 0xffffffffULL) + (mid1 & 0xffffffffULL) + (lo >> 32)) >> 32;
    return hi + (mid0 >> 32) + (mid1 >> 32) + carry;
}

uint64_t HashToRange(const CScript& element, uint64_t F, uint64_t k0, uint64_t k1)
{
    uint64_t h = CSipHasher(k0, k1).Write(element.data(), element.size()).Finalize();
#if defined(__SIZEOF_INT128__)
    return (uint64_t)(((__uint128_t)h * F) >> 64);
#else
    return UmulHi(h, F);
#endif
}

void GolombEncode(BitWriter& writer, uint64_t value, uint8_t P)
{
    uint64_t q = value >> P;
    while (q-- > 0) writer.WriteBit(1);
    writer.WriteBit(0);
    writer.WriteBits(value & ((uint64_t(1) << P) - 1), P);
}

uint64_t GolombDecode(BitReader& reader, uint8_t P)
{
    uint64_t q = 0;
    while (reader.ReadBit() == 1) ++q;
    uint64_t r = reader.ReadBits(P);
    return (q << P) | r;
}

} // namespace

void BlockFilter::SetKey(uint64_t& k0, uint64_t& k1) const
{
    unsigned char key[16];
    memcpy(key, m_block_hash.begin(), 16);
    k0 = ReadLE64(key);
    k1 = ReadLE64(key + 8);
}

void BlockFilter::Build(const std::vector<CScript>& elements)
{
    uint64_t M;
    uint8_t P;
    ParamsFor(m_type, M, P);

    uint64_t k0, k1;
    SetKey(k0, k1);

    // Deduplicate: a GCS is a set.
    std::set<CScript> uniq(elements.begin(), elements.end());

    std::vector<uint64_t> values;
    values.reserve(uniq.size());
    for (const auto& e : uniq) {
        values.push_back(HashToRange(e, uint64_t(uniq.size()) * M, k0, k1));
    }
    std::sort(values.begin(), values.end());

    m_n = values.size();

    std::vector<unsigned char> body;
    {
        BitWriter writer(body);
        uint64_t last = 0;
        for (uint64_t v : values) {
            GolombEncode(writer, v - last, P);
            last = v;
        }
    }

    m_encoded.clear();
    WriteCompactSize(m_encoded, m_n);
    m_encoded.insert(m_encoded.end(), body.begin(), body.end());
}

BlockFilter::BlockFilter(BlockFilterType type, const uint256& block_hash,
                         const std::vector<CScript>& elements)
    : m_type(type), m_block_hash(block_hash)
{
    Build(elements);
}

BlockFilter::BlockFilter(BlockFilterType type, const uint256& block_hash,
                         const std::vector<unsigned char>& encoded)
    : m_type(type), m_block_hash(block_hash), m_encoded(encoded)
{
    if (m_encoded.empty()) {
        m_n = 0;
        return;
    }
    const unsigned char* p = m_encoded.data();
    const unsigned char* end = p + m_encoded.size();
    try {
        m_n = ReadCompactSize(p, end);
    } catch (const std::exception&) {
        m_n = 0;
    }
}

bool BlockFilter::Match(const CScript& element) const
{
    if (m_n == 0) return false;

    uint64_t M;
    uint8_t P;
    ParamsFor(m_type, M, P);

    uint64_t k0, k1;
    SetKey(k0, k1);

    uint64_t target = HashToRange(element, uint64_t(m_n) * M, k0, k1);

    const unsigned char* p = m_encoded.data();
    const unsigned char* end = p + m_encoded.size();
    ReadCompactSize(p, end); // skip N

    BitReader reader(p, end - p);
    uint64_t last = 0;
    for (uint32_t i = 0; i < m_n; ++i) {
        uint64_t v = last + GolombDecode(reader, P);
        last = v;
        if (v == target) return true;
        if (v > target) break;
    }
    return false;
}

bool BlockFilter::MatchAny(const std::vector<CScript>& elements) const
{
    if (m_n == 0) return false;

    uint64_t M;
    uint8_t P;
    ParamsFor(m_type, M, P);

    uint64_t k0, k1;
    SetKey(k0, k1);

    std::vector<uint64_t> targets;
    targets.reserve(elements.size());
    for (const auto& e : elements) {
        targets.push_back(HashToRange(e, uint64_t(m_n) * M, k0, k1));
    }
    std::sort(targets.begin(), targets.end());

    const unsigned char* p = m_encoded.data();
    const unsigned char* end = p + m_encoded.size();
    ReadCompactSize(p, end);

    BitReader reader(p, end - p);
    uint64_t value = 0;
    size_t idx = 0;
    for (uint32_t i = 0; i < m_n; ++i) {
        value += GolombDecode(reader, P);
        while (true) {
            if (targets[idx] == value) return true;
            if (targets[idx] > value) break;
            if (++idx == targets.size()) return false;
        }
    }
    return false;
}

uint256 BlockFilter::ComputeHeader(const uint256& prev_header) const
{
    uint256 filter_hash = Hash(m_encoded.begin(), m_encoded.end());
    return Hash(filter_hash.begin(), filter_hash.end(),
                prev_header.begin(), prev_header.end());
}
