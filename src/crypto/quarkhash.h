#ifndef HASHBLOCK_H
#define HASHBLOCK_H

#include <crypto/sph_blake.h>
#include <crypto/sph_bmw.h>
#include <crypto/sph_groestl.h>
#include <crypto/sph_jh.h>
#include <crypto/sph_keccak.h>
#include <crypto/sph_skein.h>
#include <uint256.h>

#ifndef QT_NO_DEBUG
#include <string>
#endif

#ifdef GLOBALDEFINED
#define GLOBAL
#else
#define GLOBAL extern
#endif

GLOBAL sph_blake512_context z_blake;
GLOBAL sph_bmw512_context z_bmw;
GLOBAL sph_groestl512_context z_groestl;
GLOBAL sph_jh512_context z_jh;
GLOBAL sph_keccak512_context z_keccak;
GLOBAL sph_skein512_context z_skein;

#define fillz()                          \
    do {                                 \
        sph_blake512_init(&z_blake);     \
        sph_bmw512_init(&z_bmw);         \
        sph_groestl512_init(&z_groestl); \
        sph_jh512_init(&z_jh);           \
        sph_keccak512_init(&z_keccak);   \
        sph_skein512_init(&z_skein);     \
    } while (0)

#define ZBLAKE (memcpy(&ctx_blake, &z_blake, sizeof(z_blake)))
#define ZBMW (memcpy(&ctx_bmw, &z_bmw, sizeof(z_bmw)))
#define ZGROESTL (memcpy(&ctx_groestl, &z_groestl, sizeof(z_groestl)))
#define ZJH (memcpy(&ctx_jh, &z_jh, sizeof(z_jh)))
#define ZKECCAK (memcpy(&ctx_keccak, &z_keccak, sizeof(z_keccak)))
#define ZSKEIN (memcpy(&ctx_skein, &z_skein, sizeof(z_skein)))

template <typename T1>
inline uint256 quark_hash(const T1 pbegin, const T1 pend)
{
  sph_blake512_context ctx_blake;
  sph_bmw512_context ctx_bmw;
  sph_groestl512_context ctx_groestl;
  sph_jh512_context ctx_jh;
  sph_keccak512_context ctx_keccak;
  sph_skein512_context ctx_skein;

  uint256 hash[1];
  uint32_t mask = 8;
  uint32_t zero = 0;

  uint32_t hashA[16], hashB[16];

  sph_blake512_init(&ctx_blake);
  sph_blake512(&ctx_blake, static_cast<const void*>(&pbegin[0]), 80);
  sph_blake512_close(&ctx_blake, hashA); // 0

  sph_bmw512_init(&ctx_bmw);
  sph_bmw512(&ctx_bmw, hashA, 64); // 0
  sph_bmw512_close(&ctx_bmw, hashB); // 1

  if ((hashB[0] & mask) != zero) // 1
      {
    sph_groestl512_init(&ctx_groestl);
    sph_groestl512(&ctx_groestl, hashB, 64); // 1
    sph_groestl512_close(&ctx_groestl, hashA); // 2
  } else {
    sph_skein512_init(&ctx_skein);
    sph_skein512(&ctx_skein, hashB, 64); // 1
    sph_skein512_close(&ctx_skein, hashA); // 2
  }

  sph_groestl512_init(&ctx_groestl);
  sph_groestl512(&ctx_groestl, hashA, 64); // 2
  sph_groestl512_close(&ctx_groestl, hashB); // 3

  sph_jh512_init(&ctx_jh);
  sph_jh512(&ctx_jh, hashB, 64); // 3
  sph_jh512_close(&ctx_jh, hashA); // 4

  if ((hashA[0] & mask) != zero) // 4
      {
    sph_blake512_init(&ctx_blake);
    sph_blake512(&ctx_blake, hashA, 64); //
    sph_blake512_close(&ctx_blake, hashB); // 5
  } else {
    sph_bmw512_init(&ctx_bmw);
    sph_bmw512(&ctx_bmw, hashA, 64); // 4
    sph_bmw512_close(&ctx_bmw, hashB); // 5
  }

  sph_keccak512_init(&ctx_keccak);
  sph_keccak512(&ctx_keccak, hashB, 64); // 5
  sph_keccak512_close(&ctx_keccak, hashA); // 6

  sph_skein512_init(&ctx_skein);
  sph_skein512(&ctx_skein, hashA, 64); // 6
  sph_skein512_close(&ctx_skein, hashB); // 7

  if ((hashB[0] & mask) != zero) // 7
      {
    sph_keccak512_init(&ctx_keccak);
    sph_keccak512(&ctx_keccak, hashB, 64); //
    sph_keccak512_close(&ctx_keccak, hashA); // 8
  } else {
    sph_jh512_init(&ctx_jh);
    sph_jh512(&ctx_jh, hashB, 64); // 7
    sph_jh512_close(&ctx_jh, hashA); // 8
  }

  memcpy(&hash[0], hashA, 32);
  return hash[0];
}

#endif // HASHBLOCK_H
