// Copyright (c) 2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <vector>

#include <blockfilter.h>
#include <util/strencodings.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(blockfilter_tests)

// BIP158 test vectors (genesis block, height 0) and the empty block
// (height 1414221) from bip-0158/testnet-19.json.
BOOST_AUTO_TEST_CASE(bip158_genesis)
{
    uint256 block_hash = uint256S("000000000933ea01ad0ee984209779baaec3ced90fa3f408719526f8d77f4943");
    CScript script(ParseHex("4104678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5fac"));

    BlockFilter filter(BlockFilterType::BASIC, block_hash, std::vector<CScript>{script});

    BOOST_CHECK_EQUAL(HexStr(filter.GetEncoded()), "019dfca8");
    BOOST_CHECK_EQUAL(filter.ComputeHeader(uint256()).GetHex(),
                      "21584579b7eb08997773e5aeff3a7f932700042d0ed2a6129012b7d7ae81b750");
    BOOST_CHECK(filter.Match(script));
    BOOST_CHECK(!filter.Match(CScript(ParseHex("6a"))));
}

BOOST_AUTO_TEST_CASE(bip158_empty)
{
    uint256 block_hash = uint256S("0000000000000027b2b3b3381f114f674f481544ff2be37ae3788d7e078383b1");

    BlockFilter filter(BlockFilterType::BASIC, block_hash, std::vector<CScript>{});

    BOOST_CHECK_EQUAL(HexStr(filter.GetEncoded()), "00");
    BOOST_CHECK(filter.IsEmpty());
}

BOOST_AUTO_TEST_CASE(bip158_roundtrip)
{
    std::vector<CScript> elements;
    elements.emplace_back(ParseHex("76a914abcabcabcabcabcabcabcabcabcabcabcabcab88ac"));
    elements.emplace_back(ParseHex("a914abcabcabcabcabcabcabcabcabcabcabcabcab87"));

    BlockFilter filter(BlockFilterType::BASIC, uint256S("11"), elements);

    BOOST_CHECK(filter.Match(elements[0]));
    BOOST_CHECK(filter.Match(elements[1]));
    BOOST_CHECK(!filter.Match(CScript(ParseHex("00"))));
    BOOST_CHECK(filter.MatchAny(elements));
}

BOOST_AUTO_TEST_SUITE_END()
