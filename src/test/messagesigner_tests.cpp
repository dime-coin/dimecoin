// Copyright (c) 2013-2022 The Dimecoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <hash.h>
#include <key.h>
#include <messagesigner.h>
#include <validation.h> // For strMessageMagic

#include <test/test_dimecoin.h>

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(messagesigner_tests, BasicTestingSetup)

namespace {
const std::string kNetworkMagic = "DarkCoin Signed Message:\n";
const std::string kAlternateMagic = "Dimecoin Signed Message:\n";

uint256 MagicHash(const std::string& strMagic, const std::string& strMessage)
{
    CHashWriter ss(SER_GETHASH, 0);
    ss << strMagic;
    ss << strMessage;
    return ss.GetHash();
}

std::vector<unsigned char> SignUnderMagic(const CKey& key, const std::string& strMagic, const std::string& strMessage)
{
    std::vector<unsigned char> vchSig;
    BOOST_CHECK(CHashSigner::SignHash(MagicHash(strMagic, strMessage), key, vchSig));
    return vchSig;
}
} // namespace

// The live network -- including the shipped 2.3.0.0 release and every masternode on it -- signs
// with this magic. Changing it makes masternode broadcasts, pings, sporks, governance objects and
// InstantSend locks unverifiable, and makes the node raise the ban score of every peer relaying
// them. It also breaks signmessage/verifymessage interoperability. Pin the value.
BOOST_AUTO_TEST_CASE(message_magic_matches_live_network)
{
    BOOST_CHECK_EQUAL(strMessageMagic, kNetworkMagic);
}

// Signing must stay single-magic. A node emitting the alternate magic would be invisible to the
// whole network, and if it were a masternode it would stop being paid.
BOOST_AUTO_TEST_CASE(signing_uses_only_the_configured_magic)
{
    CKey key;
    key.MakeNewKey(true);
    const CPubKey pubkey = key.GetPubKey();
    const std::string strMessage = "masternode broadcast payload";

    std::vector<unsigned char> vchSig;
    BOOST_CHECK(CMessageSigner::SignMessage(strMessage, vchSig, key));

    std::string strError;
    BOOST_CHECK(CHashSigner::VerifyHash(MagicHash(strMessageMagic, strMessage), pubkey, vchSig, strError));
    BOOST_CHECK(!CHashSigner::VerifyHash(MagicHash(kAlternateMagic, strMessage), pubkey, vchSig, strError));

    BOOST_CHECK(CMessageSigner::VerifyMessage(pubkey, vchSig, strMessage, strError));
}

// Accept-before-emit: verification must already tolerate the alternate magic so that a future
// release can flip signing over to it without stranding anyone.
BOOST_AUTO_TEST_CASE(alternate_magic_is_accepted_on_verify)
{
    CKey key;
    key.MakeNewKey(true);
    const CPubKey pubkey = key.GetPubKey();
    const std::string strMessage = "signed under the alternate magic";

    const std::vector<unsigned char> vchSig = SignUnderMagic(key, kAlternateMagic, strMessage);

    const int64_t nBefore = CMessageSigner::GetLegacyMagicAcceptCount();
    std::string strError;
    BOOST_CHECK(CMessageSigner::VerifyMessage(pubkey, vchSig, strMessage, strError));
    // The counter is the migration signal that gates the emit flip and the eventual removal.
    BOOST_CHECK(CMessageSigner::GetLegacyMagicAcceptCount() > nBefore);
}

// Widening the accepted magics must not weaken the actual signature check.
BOOST_AUTO_TEST_CASE(wrong_key_is_still_rejected_under_every_magic)
{
    CKey key, other;
    key.MakeNewKey(true);
    other.MakeNewKey(true);
    const CPubKey pubkey = key.GetPubKey();
    const std::string strMessage = "not signed by pubkey";

    std::string strError;
    for (const std::string& strMagic : {kNetworkMagic, kAlternateMagic}) {
        const std::vector<unsigned char> vchSig = SignUnderMagic(other, strMagic, strMessage);
        BOOST_CHECK(!CMessageSigner::VerifyMessage(pubkey, vchSig, strMessage, strError));
        BOOST_CHECK(!strError.empty());
    }
}

// A signature is bound to its message: the fallback must not let a signature float between
// different payloads.
BOOST_AUTO_TEST_CASE(signature_does_not_transfer_between_messages)
{
    CKey key;
    key.MakeNewKey(true);
    const CPubKey pubkey = key.GetPubKey();

    std::vector<unsigned char> vchSig;
    BOOST_CHECK(CMessageSigner::SignMessage("original payload", vchSig, key));

    std::string strError;
    BOOST_CHECK(!CMessageSigner::VerifyMessage(pubkey, vchSig, "tampered payload", strError));
}

BOOST_AUTO_TEST_SUITE_END()
