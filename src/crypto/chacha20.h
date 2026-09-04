// Copyright (c) 2017-2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_CRYPTO_CHACHA20_H
#define BITCOIN_CRYPTO_CHACHA20_H

#include <stdint.h>
#include <stdlib.h>

#include <array>
#include <utility>

#include <span.h>

/** A PRNG class for ChaCha20. */
class ChaCha20
{
private:
    uint32_t input[16];

public:
    ChaCha20();
    ChaCha20(const unsigned char* key, size_t keylen);
    void SetKey(const unsigned char* key, size_t keylen);
    void SetIV(uint64_t iv);
    void Seek(uint64_t pos);
    void Output(unsigned char* output, size_t bytes);

    /** Expected key length in constructor and SetKey (BIP324 additions). */
    static constexpr unsigned KEYLEN = 32;

    /** Block size (inputs/outputs to Keystream / Crypt should be multiples of this). */
    static constexpr unsigned BLOCKLEN = 64;

    /** 96-bit nonce, as in RFC8439 Section 2.3 (32-bit fixed part + 64-bit part). */
    using Nonce96 = std::pair<uint32_t, uint64_t>;

    /** Initialize a cipher with a specified 32-byte key. */
    ChaCha20(Span<const unsigned char> key) noexcept;

    /** Destructor to clean up private memory. */
    ~ChaCha20();

    /** Set 32-byte key, and seek to nonce 0 and block position 0. */
    void SetKey(Span<const unsigned char> key) noexcept;

    /** Set the 96-bit nonce and 32-bit block counter.
     *
     * block_counter selects a position to seek to (to byte BLOCKLEN*block_counter). After 256 GiB,
     * the block counter overflows, and nonce.first is incremented.
     */
    void Seek(Nonce96 nonce, uint32_t block_counter) noexcept;

    /** outputs the keystream into out, whose length must be a multiple of BLOCKLEN. */
    void Keystream(Span<unsigned char> out) noexcept;

    /** en/deciphers the message <in> and write the result into <out>.
     *
     * The size of in and out must be equal.
     */
    void Crypt(Span<const unsigned char> in, Span<unsigned char> out) noexcept;
};

/** Forward-secure ChaCha20 (BIP324).
 *
 * This implements a stream cipher that automatically transitions to a new stream with a new key
 * and new nonce after a predefined number of encryptions or decryptions.
 */
class FSChaCha20
{
private:
    /** Internal stream cipher. */
    ChaCha20 m_chacha20;

    /** The number of encryptions/decryptions before a rekey happens. */
    const uint32_t m_rekey_interval;

    /** The number of encryptions/decryptions since the last rekey. */
    uint32_t m_chunk_counter{0};

    /** The number of rekey operations that have happened. */
    uint64_t m_rekey_counter{0};

public:
    /** Length of keys expected by the constructor. */
    static constexpr unsigned KEYLEN = 32;

    // No copy or move to protect the secret.
    FSChaCha20(const FSChaCha20&) = delete;
    FSChaCha20(FSChaCha20&&) = delete;
    FSChaCha20& operator=(const FSChaCha20&) = delete;
    FSChaCha20& operator=(FSChaCha20&&) = delete;

    /** Construct an FSChaCha20 cipher that rekeys every rekey_interval Crypt() calls. */
    FSChaCha20(Span<const unsigned char> key, uint32_t rekey_interval) noexcept;

    /** Encrypt or decrypt a chunk. */
    void Crypt(Span<const unsigned char> input, Span<unsigned char> output) noexcept;
};

#endif // BITCOIN_CRYPTO_CHACHA20_H
