// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Adapted for Dimecoin (standard secp256k1 ECDH instead of EllSwift).

#include <bip324.h>
#include <key.h>
#include <pubkey.h>
#include <span.h>

#include <test/test_dimecoin.h>

#include <vector>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(bip324_tests, BasicTestingSetup)

static std::vector<unsigned char> ToVec(const Span<const unsigned char>& s)
{
    return std::vector<unsigned char>(s.begin(), s.end());
}

BOOST_AUTO_TEST_CASE(bip324_handshake_and_roundtrip)
{
    CKey ki, kr;
    ki.MakeNewKey(true);
    kr.MakeNewKey(true);

    BIP324Cipher ci(ki), cr(kr);
    ci.Initialize(kr.GetPubKey(), true /* initiator */);
    cr.Initialize(ki.GetPubKey(), false /* responder */);

    // Both sides must be fully initialized.
    BOOST_CHECK(static_cast<bool>(ci));
    BOOST_CHECK(static_cast<bool>(cr));

    // The session id and garbage terminators are deterministic from the shared ECDH secret.
    BOOST_CHECK(ToVec(ci.GetSessionID()) == ToVec(cr.GetSessionID()));
    BOOST_CHECK(ToVec(ci.GetSendGarbageTerminator()) == ToVec(cr.GetReceiveGarbageTerminator()));
    BOOST_CHECK(ToVec(ci.GetReceiveGarbageTerminator()) == ToVec(cr.GetSendGarbageTerminator()));

    // Roundtrip a packet from initiator -> responder.
    std::vector<unsigned char> contents = {'h', 'e', 'l', 'l', 'o', ' ', 'v', '2'};
    std::vector<unsigned char> aad = {'a', 'a', 'd'};
    std::vector<unsigned char> output(contents.size() + BIP324Cipher::EXPANSION);
    ci.Encrypt(Span<const unsigned char>(contents.data(), contents.size()),
               Span<const unsigned char>(aad.data(), aad.size()),
               false,
               Span<unsigned char>(output.data(), output.size()));

    unsigned len = cr.DecryptLength(Span<const unsigned char>(output.data(), BIP324Cipher::LENGTH_LEN));
    BOOST_CHECK_EQUAL(len, contents.size());

    std::vector<unsigned char> decrypted(len);
    bool ignore = false;
    bool ok = cr.Decrypt(Span<const unsigned char>(output.data() + BIP324Cipher::LENGTH_LEN, output.size() - BIP324Cipher::LENGTH_LEN),
                          Span<const unsigned char>(aad.data(), aad.size()),
                          ignore,
                          Span<unsigned char>(decrypted.data(), decrypted.size()));
    BOOST_CHECK(ok);
    BOOST_CHECK(!ignore);
    BOOST_CHECK(decrypted == contents);

    // A wrong AAD must cause decryption to fail.
    std::vector<unsigned char> wrong_aad = {'x', 'a', 'd'};
    bool ignore2 = false;
    bool ok2 = cr.Decrypt(Span<const unsigned char>(output.data() + BIP324Cipher::LENGTH_LEN, output.size() - BIP324Cipher::LENGTH_LEN),
                           Span<const unsigned char>(wrong_aad.data(), wrong_aad.size()),
                           ignore2,
                           Span<unsigned char>(decrypted.data(), decrypted.size()));
    BOOST_CHECK(!ok2);
}

BOOST_AUTO_TEST_CASE(bip324_self_decrypt)
{
    // With self_decrypt, the same cipher can encrypt and decrypt (handy for testing).
    CKey k;
    k.MakeNewKey(true);
    BIP324Cipher c(k, k.GetPubKey());
    c.Initialize(k.GetPubKey(), true, true /* self_decrypt */);
    BOOST_CHECK(static_cast<bool>(c));

    std::vector<unsigned char> contents = {'s', 'e', 'l', 'f'};
    std::vector<unsigned char> output(contents.size() + BIP324Cipher::EXPANSION);
    c.Encrypt(Span<const unsigned char>(contents.data(), contents.size()), {}, false,
              Span<unsigned char>(output.data(), output.size()));

    unsigned len = c.DecryptLength(Span<const unsigned char>(output.data(), BIP324Cipher::LENGTH_LEN));
    BOOST_CHECK_EQUAL(len, contents.size());
    std::vector<unsigned char> decrypted(len);
    bool ignore = false;
    bool ok = c.Decrypt(Span<const unsigned char>(output.data() + BIP324Cipher::LENGTH_LEN, output.size() - BIP324Cipher::LENGTH_LEN), {}, ignore,
                         Span<unsigned char>(decrypted.data(), decrypted.size()));
    BOOST_CHECK(ok);
    BOOST_CHECK(decrypted == contents);
}

BOOST_AUTO_TEST_SUITE_END()
