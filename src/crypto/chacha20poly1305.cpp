// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/chacha20poly1305.h>

#include <crypto/chacha20.h>
#include <crypto/common.h>
#include <crypto/poly1305.h>
#include <span.h>
#include <support/cleanse.h>

#include <assert.h>
#include <cstddef>

AEADChaCha20Poly1305::AEADChaCha20Poly1305(Span<const unsigned char> key) noexcept : m_chacha20(key)
{
    assert(key.size() == KEYLEN);
}

void AEADChaCha20Poly1305::SetKey(Span<const unsigned char> key) noexcept
{
    assert(key.size() == KEYLEN);
    m_chacha20.SetKey(key);
}

namespace {

#ifndef HAVE_TIMINGSAFE_BCMP
#define HAVE_TIMINGSAFE_BCMP

int timingsafe_bcmp(const unsigned char* b1, const unsigned char* b2, size_t n) noexcept
{
    const unsigned char *p1 = b1, *p2 = b2;
    int ret = 0;
    for (; n > 0; n--)
        ret |= *p1++ ^ *p2++;
    return (ret != 0);
}

#endif

/** Compute poly1305 tag. chacha20 must be set to the right nonce, block 0. Will be at block 1 after. */
void ComputeTag(ChaCha20& chacha20, Span<const unsigned char> aad, Span<const unsigned char> cipher, Span<unsigned char> tag) noexcept
{
    static const unsigned char PADDING[16] = {0};

    // Get block of keystream (use a full 64 byte buffer to avoid the need for chacha20's own buffering).
    unsigned char first_block[ChaCha20::BLOCKLEN];
    chacha20.Keystream(Span<unsigned char>(first_block, ChaCha20::BLOCKLEN));

    // Use the first 32 bytes of the first keystream block as poly1305 key.
    Poly1305 poly1305{Span<const unsigned char>(first_block, Poly1305::KEYLEN)};

    // Compute tag:
    // - Process the padded AAD with Poly1305.
    const unsigned aad_padding_length = (16 - (aad.size() % 16)) % 16;
    poly1305.Update(aad).Update(Span<const unsigned char>(PADDING, aad_padding_length));
    // - Process the padded ciphertext with Poly1305.
    const unsigned cipher_padding_length = (16 - (cipher.size() % 16)) % 16;
    poly1305.Update(cipher).Update(Span<const unsigned char>(PADDING, cipher_padding_length));
    // - Process the AAD and plaintext length with Poly1305.
    unsigned char length_desc[Poly1305::TAGLEN];
    WriteLE64(length_desc, aad.size());
    WriteLE64(length_desc + 8, cipher.size());
    poly1305.Update(Span<const unsigned char>(length_desc, Poly1305::TAGLEN));

    // Output tag.
    poly1305.Finalize(tag);
}

} // namespace

void AEADChaCha20Poly1305::Encrypt(Span<const unsigned char> plain1, Span<const unsigned char> plain2, Span<const unsigned char> aad, Nonce96 nonce, Span<unsigned char> cipher) noexcept
{
    assert(cipher.size() == plain1.size() + plain2.size() + EXPANSION);

    // Encrypt using ChaCha20 (starting at block 1).
    m_chacha20.Seek(nonce, 1);
    m_chacha20.Crypt(plain1, cipher.first(plain1.size()));
    m_chacha20.Crypt(plain2, cipher.subspan(plain1.size()).first(plain2.size()));

    // Seek to block 0, and compute tag using key drawn from there.
    m_chacha20.Seek(nonce, 0);
    ComputeTag(m_chacha20, aad, Span<const unsigned char>(cipher.data(), cipher.size() - EXPANSION), cipher.last(EXPANSION));
}

bool AEADChaCha20Poly1305::Decrypt(Span<const unsigned char> cipher, Span<const unsigned char> aad, Nonce96 nonce, Span<unsigned char> plain1, Span<unsigned char> plain2) noexcept
{
    assert(cipher.size() == plain1.size() + plain2.size() + EXPANSION);

    // Verify tag (using key drawn from block 0).
    m_chacha20.Seek(nonce, 0);
    unsigned char expected_tag[EXPANSION];
    ComputeTag(m_chacha20, aad, cipher.first(cipher.size() - EXPANSION), Span<unsigned char>(expected_tag, EXPANSION));
    if (timingsafe_bcmp(expected_tag, cipher.last(EXPANSION).data(), EXPANSION)) return false;

    // Decrypt (starting at block 1).
    m_chacha20.Crypt(cipher.first(plain1.size()), plain1);
    m_chacha20.Crypt(cipher.subspan(plain1.size()).first(plain2.size()), plain2);
    return true;
}

void AEADChaCha20Poly1305::Keystream(Nonce96 nonce, Span<unsigned char> keystream) noexcept
{
    // Skip the first output block, as it's used for generating the poly1305 key.
    m_chacha20.Seek(nonce, 1);
    m_chacha20.Keystream(keystream);
}

void FSChaCha20Poly1305::NextPacket() noexcept
{
    if (++m_packet_counter == m_rekey_interval) {
        // Generate a full block of keystream, to avoid needing the ChaCha20 buffer, even though
        // we only need KEYLEN (32) bytes.
        unsigned char one_block[ChaCha20::BLOCKLEN];
        m_aead.Keystream({0xFFFFFFFF, m_rekey_counter}, Span<unsigned char>(one_block, ChaCha20::BLOCKLEN));
        // Switch keys.
        m_aead.SetKey(Span<const unsigned char>(one_block, KEYLEN));
        // Wipe the generated keystream (a copy remains inside m_aead, which will be cleaned up
        // once it cycles again, or is destroyed).
        memory_cleanse(one_block, sizeof(one_block));
        // Update counters.
        m_packet_counter = 0;
        ++m_rekey_counter;
    }
}

void FSChaCha20Poly1305::Encrypt(Span<const unsigned char> plain1, Span<const unsigned char> plain2, Span<const unsigned char> aad, Span<unsigned char> cipher) noexcept
{
    m_aead.Encrypt(plain1, plain2, aad, {m_packet_counter, m_rekey_counter}, cipher);
    NextPacket();
}

bool FSChaCha20Poly1305::Decrypt(Span<const unsigned char> cipher, Span<const unsigned char> aad, Span<unsigned char> plain1, Span<unsigned char> plain2) noexcept
{
    bool ret = m_aead.Decrypt(cipher, aad, {m_packet_counter, m_rekey_counter}, plain1, plain2);
    NextPacket();
    return ret;
}
