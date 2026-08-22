// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Adapted for Dimecoin: uses standard secp256k1 ECDH (CPubKey) instead of
// Bitcoin's EllSwift, and the "dimecoin_v2_shared_secret" salt (with Dimecoin
// network magic). Two Dimecoin nodes running this code interoperate; it is NOT
// wire-compatible with Bitcoin Core's BIP324 (which requires an EllSwift upgrade
// to libsecp256k1). See PR description / discussion #90.

#ifndef BITCOIN_BIP324_H
#define BITCOIN_BIP324_H

#include <array>
#include <cstddef>
#include <memory>

#include <crypto/chacha20.h>
#include <crypto/chacha20poly1305.h>
#include <key.h>
#include <pubkey.h>
#include <span.h>
#include <util/memory.h>

/** The BIP324 packet cipher, encapsulating its key derivation, stream cipher, and AEAD. */
class BIP324Cipher
{
public:
    static constexpr unsigned SESSION_ID_LEN{32};
    static constexpr unsigned GARBAGE_TERMINATOR_LEN{16};
    static constexpr unsigned REKEY_INTERVAL{224};
    static constexpr unsigned LENGTH_LEN{3};
    static constexpr unsigned HEADER_LEN{1};
    static constexpr unsigned EXPANSION = LENGTH_LEN + HEADER_LEN + FSChaCha20Poly1305::EXPANSION;
    static constexpr unsigned char IGNORE_BIT{0x80};

private:
    std::unique_ptr<FSChaCha20> m_send_l_cipher;
    std::unique_ptr<FSChaCha20> m_recv_l_cipher;
    std::unique_ptr<FSChaCha20Poly1305> m_send_p_cipher;
    std::unique_ptr<FSChaCha20Poly1305> m_recv_p_cipher;

    CKey m_key;
    CPubKey m_our_pubkey;

    std::array<unsigned char, SESSION_ID_LEN> m_session_id;
    std::array<unsigned char, GARBAGE_TERMINATOR_LEN> m_send_garbage_terminator;
    std::array<unsigned char, GARBAGE_TERMINATOR_LEN> m_recv_garbage_terminator;

public:
    /** No default constructor; keys must be provided to create a BIP324Cipher. */
    BIP324Cipher() = delete;

    /** Initialize a BIP324 cipher with a specified key (testing only). */
    BIP324Cipher(const CKey& key) noexcept;

    /** Initialize a BIP324 cipher with specified key and public key (testing only). */
    BIP324Cipher(const CKey& key, const CPubKey& pubkey) noexcept;

    /** Retrieve our public key. */
    const CPubKey& GetOurPubKey() const noexcept { return m_our_pubkey; }

    /** Initialize when the other side's public key is received. Can only be called once.
     *
     * initiator is set to true if we are the initiator establishing the v2 P2P connection.
     * self_decrypt is only for testing, and swaps encryption/decryption keys, so that encryption
     * and decryption can be tested without knowing the other side's private key.
     */
    void Initialize(const CPubKey& their_pubkey, bool initiator, bool self_decrypt = false) noexcept;

    /** Determine whether this cipher is fully initialized. */
    explicit operator bool() const noexcept { return static_cast<bool>(m_send_l_cipher); }

    /** Encrypt a packet. Only after Initialize().
     *
     * It must hold that output.size() == contents.size() + EXPANSION.
     */
    void Encrypt(Span<const unsigned char> contents, Span<const unsigned char> aad, bool ignore, Span<unsigned char> output) noexcept;

    /** Decrypt the length of a packet. Only after Initialize().
     *
     * It must hold that input.size() == LENGTH_LEN.
     */
    unsigned DecryptLength(Span<const unsigned char> input) noexcept;

    /** Decrypt a packet. Only after Initialize().
     *
     * It must hold that input.size() + LENGTH_LEN == contents.size() + EXPANSION.
     * Contents.size() must equal the length returned by DecryptLength.
     */
    bool Decrypt(Span<const unsigned char> input, Span<const unsigned char> aad, bool& ignore, Span<unsigned char> contents) noexcept;

    /** Get the Session ID. Only after Initialize(). */
    Span<const unsigned char> GetSessionID() const noexcept { return Span<const unsigned char>(m_session_id.data(), m_session_id.size()); }

    /** Get the Garbage Terminator to send. Only after Initialize(). */
    Span<const unsigned char> GetSendGarbageTerminator() const noexcept { return Span<const unsigned char>(m_send_garbage_terminator.data(), m_send_garbage_terminator.size()); }

    /** Get the expected Garbage Terminator to receive. Only after Initialize(). */
    Span<const unsigned char> GetReceiveGarbageTerminator() const noexcept { return Span<const unsigned char>(m_recv_garbage_terminator.data(), m_recv_garbage_terminator.size()); }
};

#endif // BITCOIN_BIP324_H
