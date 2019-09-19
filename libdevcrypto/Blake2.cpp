/// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.

#include "Blake2.h"


#include <libdevcore/Exceptions.h>
#include <libdevcore/Log.h>

namespace dev
{
namespace crypto
{
namespace
{
DEV_SIMPLE_EXCEPTION(InvalidInputSize);

enum blake2b_constant
{
    BLAKE2B_BLOCKBYTES = 128,
};

typedef struct blake2b_state__
{
    uint64_t h[8];
    uint64_t t[2];
    uint64_t f[2];
    uint8_t buf[BLAKE2B_BLOCKBYTES];
    size_t buflen;
    size_t outlen;
    uint8_t last_node;
} blake2b_state;

static const uint64_t blake2b_IV[8] = {0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL,
    0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL, 0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL,
    0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL};

static const uint8_t blake2b_sigma[12][16] = {
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
    {14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3},
    {11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4},
    {7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8},
    {9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13},
    {2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9},
    {12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11},
    {13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10},
    {6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5},
    {10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0},
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
    {14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3}};

static inline uint64_t load64(const void* src)
{
#if defined(NATIVE_LITTLE_ENDIAN)
    uint64_t w;
    memcpy(&w, src, sizeof w);
    return w;
#else
    const uint8_t* p = (const uint8_t*)src;
    return ((uint64_t)(p[0]) << 0) | ((uint64_t)(p[1]) << 8) | ((uint64_t)(p[2]) << 16) |
           ((uint64_t)(p[3]) << 24) | ((uint64_t)(p[4]) << 32) | ((uint64_t)(p[5]) << 40) |
           ((uint64_t)(p[6]) << 48) | ((uint64_t)(p[7]) << 56);
#endif
}

static inline void store64(void* dst, uint64_t w)
{
#if defined(NATIVE_LITTLE_ENDIAN)
    memcpy(dst, &w, sizeof w);
#else
    uint8_t* p = (uint8_t*)dst;
    p[0] = (uint8_t)(w >> 0);
    p[1] = (uint8_t)(w >> 8);
    p[2] = (uint8_t)(w >> 16);
    p[3] = (uint8_t)(w >> 24);
    p[4] = (uint8_t)(w >> 32);
    p[5] = (uint8_t)(w >> 40);
    p[6] = (uint8_t)(w >> 48);
    p[7] = (uint8_t)(w >> 56);
#endif
}

static inline uint64_t rotr64(const uint64_t w, const unsigned c)
{
    return (w >> c) | (w << (64 - c));
}

inline void G(uint8_t r, uint8_t i, uint64_t& a, uint64_t& b, uint64_t& c, uint64_t& d, uint64_t* m)
{
    a = a + b + m[blake2b_sigma[r][2 * i + 0]];
    d = rotr64(d ^ a, 32);
    c = c + d;
    b = rotr64(b ^ c, 24);
    a = a + b + m[blake2b_sigma[r][2 * i + 1]];
    d = rotr64(d ^ a, 16);
    c = c + d;
    b = rotr64(b ^ c, 63);
}

inline void ROUND(uint32_t round, uint64_t* v, uint64_t* m)
{
    uint8_t const r = round % 10;
    G(r, 0, v[0], v[4], v[8], v[12], m);
    G(r, 1, v[1], v[5], v[9], v[13], m);
    G(r, 2, v[2], v[6], v[10], v[14], m);
    G(r, 3, v[3], v[7], v[11], v[15], m);
    G(r, 4, v[0], v[5], v[10], v[15], m);
    G(r, 5, v[1], v[6], v[11], v[12], m);
    G(r, 6, v[2], v[7], v[8], v[13], m);
    G(r, 7, v[3], v[4], v[9], v[14], m);
}


static void blake2b_compress(
    uint32_t _rounds, blake2b_state* S, const uint8_t block[BLAKE2B_BLOCKBYTES])
{
    uint64_t m[16];
    uint64_t v[16];
    size_t i;

    for (i = 0; i < 16; ++i)
    {
        m[i] = load64(block + i * sizeof(m[i]));
    }

    for (i = 0; i < 8; ++i)
    {
        v[i] = S->h[i];
    }

    v[8] = blake2b_IV[0];
    v[9] = blake2b_IV[1];
    v[10] = blake2b_IV[2];
    v[11] = blake2b_IV[3];
    v[12] = blake2b_IV[4] ^ S->t[0];
    v[13] = blake2b_IV[5] ^ S->t[1];
    v[14] = blake2b_IV[6] ^ S->f[0];
    v[15] = blake2b_IV[7] ^ S->f[1];

    for (uint32_t r = 0; r < _rounds; ++r)
        ROUND(r, v, m);

    for (i = 0; i < 8; ++i)
    {
        S->h[i] = S->h[i] ^ v[i] ^ v[i + 8];
    }
}


}  // namespace


bytes blake2FCompression(uint32_t _rounds, bytesConstRef _stateVector, bytesConstRef _t0,
    bytesConstRef _t1, bool _lastBlock, bytesConstRef _messageBlockVector)
{
    if (_stateVector.size() != 64)
        BOOST_THROW_EXCEPTION(InvalidInputSize());
    if (_messageBlockVector.size() != 128)
        BOOST_THROW_EXCEPTION(InvalidInputSize());
    if (_t0.size() != 8 || _t1.size() != 8)
        BOOST_THROW_EXCEPTION(InvalidInputSize());


    blake2b_state s{};
    for (size_t i = 0; i < 8; ++i)
        s.h[i] = load64(&_stateVector[i * sizeof(s.h[i])]);

    s.t[0] = load64(_t0.data());
    s.t[1] = load64(_t1.data());
    s.f[0] = _lastBlock ? (uint64_t)-1 : 0;
    s.f[1] = 0;

    uint8_t block[BLAKE2B_BLOCKBYTES];
    std::copy(_messageBlockVector.begin(), _messageBlockVector.end(), &block[0]);

    blake2b_compress(_rounds, &s, block);

    bytes result(64);
    for (size_t i = 0; i < 8; ++i)
        store64(&result[i * sizeof(s.h[i])], s.h[i]);

    return result;
}


}  // namespace crypto
}  // namespace dev