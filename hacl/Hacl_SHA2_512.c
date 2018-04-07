/* MIT License
 *
 * Copyright (c) 2016-2017 INRIA and Microsoft Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#include "Hacl_SHA2_512.h"

static void
Hacl_Hash_Lib_LoadStore_uint64s_from_be_bytes(uint64_t *output, uint8_t *input, uint32_t len)
{
  for (uint32_t i = (uint32_t)0U; i < len; i = i + (uint32_t)1U)
  {
    uint8_t *x0 = input + (uint32_t)8U * i;
    uint64_t inputi = load64_be(x0);
    output[i] = inputi;
  }
}

static void
Hacl_Hash_Lib_LoadStore_uint64s_to_be_bytes(uint8_t *output, uint64_t *input, uint32_t len)
{
  for (uint32_t i = (uint32_t)0U; i < len; i = i + (uint32_t)1U)
  {
    uint64_t hd1 = input[i];
    uint8_t *x0 = output + (uint32_t)8U * i;
    store64_be(x0, hd1);
  }
}

static void Hacl_Impl_SHA2_512_init(uint64_t *state)
{
  uint64_t *k1 = state;
  uint64_t *h_01 = state + (uint32_t)160U;
  uint64_t *p10 = k1;
  uint64_t *p20 = k1 + (uint32_t)16U;
  uint64_t *p3 = k1 + (uint32_t)32U;
  uint64_t *p4 = k1 + (uint32_t)48U;
  uint64_t *p5 = k1 + (uint32_t)64U;
  uint64_t *p11 = p10;
  uint64_t *p21 = p10 + (uint32_t)8U;
  uint64_t *p12 = p11;
  uint64_t *p22 = p11 + (uint32_t)4U;
  p12[0U] = (uint64_t)0x428a2f98d728ae22U;
  p12[1U] = (uint64_t)0x7137449123ef65cdU;
  p12[2U] = (uint64_t)0xb5c0fbcfec4d3b2fU;
  p12[3U] = (uint64_t)0xe9b5dba58189dbbcU;
  p22[0U] = (uint64_t)0x3956c25bf348b538U;
  p22[1U] = (uint64_t)0x59f111f1b605d019U;
  p22[2U] = (uint64_t)0x923f82a4af194f9bU;
  p22[3U] = (uint64_t)0xab1c5ed5da6d8118U;
  uint64_t *p13 = p21;
  uint64_t *p23 = p21 + (uint32_t)4U;
  p13[0U] = (uint64_t)0xd807aa98a3030242U;
  p13[1U] = (uint64_t)0x12835b0145706fbeU;
  p13[2U] = (uint64_t)0x243185be4ee4b28cU;
  p13[3U] = (uint64_t)0x550c7dc3d5ffb4e2U;
  p23[0U] = (uint64_t)0x72be5d74f27b896fU;
  p23[1U] = (uint64_t)0x80deb1fe3b1696b1U;
  p23[2U] = (uint64_t)0x9bdc06a725c71235U;
  p23[3U] = (uint64_t)0xc19bf174cf692694U;
  uint64_t *p14 = p20;
  uint64_t *p24 = p20 + (uint32_t)8U;
  uint64_t *p15 = p14;
  uint64_t *p25 = p14 + (uint32_t)4U;
  p15[0U] = (uint64_t)0xe49b69c19ef14ad2U;
  p15[1U] = (uint64_t)0xefbe4786384f25e3U;
  p15[2U] = (uint64_t)0x0fc19dc68b8cd5b5U;
  p15[3U] = (uint64_t)0x240ca1cc77ac9c65U;
  p25[0U] = (uint64_t)0x2de92c6f592b0275U;
  p25[1U] = (uint64_t)0x4a7484aa6ea6e483U;
  p25[2U] = (uint64_t)0x5cb0a9dcbd41fbd4U;
  p25[3U] = (uint64_t)0x76f988da831153b5U;
  uint64_t *p16 = p24;
  uint64_t *p26 = p24 + (uint32_t)4U;
  p16[0U] = (uint64_t)0x983e5152ee66dfabU;
  p16[1U] = (uint64_t)0xa831c66d2db43210U;
  p16[2U] = (uint64_t)0xb00327c898fb213fU;
  p16[3U] = (uint64_t)0xbf597fc7beef0ee4U;
  p26[0U] = (uint64_t)0xc6e00bf33da88fc2U;
  p26[1U] = (uint64_t)0xd5a79147930aa725U;
  p26[2U] = (uint64_t)0x06ca6351e003826fU;
  p26[3U] = (uint64_t)0x142929670a0e6e70U;
  uint64_t *p17 = p3;
  uint64_t *p27 = p3 + (uint32_t)8U;
  uint64_t *p18 = p17;
  uint64_t *p28 = p17 + (uint32_t)4U;
  p18[0U] = (uint64_t)0x27b70a8546d22ffcU;
  p18[1U] = (uint64_t)0x2e1b21385c26c926U;
  p18[2U] = (uint64_t)0x4d2c6dfc5ac42aedU;
  p18[3U] = (uint64_t)0x53380d139d95b3dfU;
  p28[0U] = (uint64_t)0x650a73548baf63deU;
  p28[1U] = (uint64_t)0x766a0abb3c77b2a8U;
  p28[2U] = (uint64_t)0x81c2c92e47edaee6U;
  p28[3U] = (uint64_t)0x92722c851482353bU;
  uint64_t *p19 = p27;
  uint64_t *p29 = p27 + (uint32_t)4U;
  p19[0U] = (uint64_t)0xa2bfe8a14cf10364U;
  p19[1U] = (uint64_t)0xa81a664bbc423001U;
  p19[2U] = (uint64_t)0xc24b8b70d0f89791U;
  p19[3U] = (uint64_t)0xc76c51a30654be30U;
  p29[0U] = (uint64_t)0xd192e819d6ef5218U;
  p29[1U] = (uint64_t)0xd69906245565a910U;
  p29[2U] = (uint64_t)0xf40e35855771202aU;
  p29[3U] = (uint64_t)0x106aa07032bbd1b8U;
  uint64_t *p110 = p4;
  uint64_t *p210 = p4 + (uint32_t)8U;
  uint64_t *p111 = p110;
  uint64_t *p211 = p110 + (uint32_t)4U;
  p111[0U] = (uint64_t)0x19a4c116b8d2d0c8U;
  p111[1U] = (uint64_t)0x1e376c085141ab53U;
  p111[2U] = (uint64_t)0x2748774cdf8eeb99U;
  p111[3U] = (uint64_t)0x34b0bcb5e19b48a8U;
  p211[0U] = (uint64_t)0x391c0cb3c5c95a63U;
  p211[1U] = (uint64_t)0x4ed8aa4ae3418acbU;
  p211[2U] = (uint64_t)0x5b9cca4f7763e373U;
  p211[3U] = (uint64_t)0x682e6ff3d6b2b8a3U;
  uint64_t *p112 = p210;
  uint64_t *p212 = p210 + (uint32_t)4U;
  p112[0U] = (uint64_t)0x748f82ee5defb2fcU;
  p112[1U] = (uint64_t)0x78a5636f43172f60U;
  p112[2U] = (uint64_t)0x84c87814a1f0ab72U;
  p112[3U] = (uint64_t)0x8cc702081a6439ecU;
  p212[0U] = (uint64_t)0x90befffa23631e28U;
  p212[1U] = (uint64_t)0xa4506cebde82bde9U;
  p212[2U] = (uint64_t)0xbef9a3f7b2c67915U;
  p212[3U] = (uint64_t)0xc67178f2e372532bU;
  uint64_t *p113 = p5;
  uint64_t *p213 = p5 + (uint32_t)8U;
  uint64_t *p1 = p113;
  uint64_t *p214 = p113 + (uint32_t)4U;
  p1[0U] = (uint64_t)0xca273eceea26619cU;
  p1[1U] = (uint64_t)0xd186b8c721c0c207U;
  p1[2U] = (uint64_t)0xeada7dd6cde0eb1eU;
  p1[3U] = (uint64_t)0xf57d4f7fee6ed178U;
  p214[0U] = (uint64_t)0x06f067aa72176fbaU;
  p214[1U] = (uint64_t)0x0a637dc5a2c898a6U;
  p214[2U] = (uint64_t)0x113f9804bef90daeU;
  p214[3U] = (uint64_t)0x1b710b35131c471bU;
  uint64_t *p114 = p213;
  uint64_t *p215 = p213 + (uint32_t)4U;
  p114[0U] = (uint64_t)0x28db77f523047d84U;
  p114[1U] = (uint64_t)0x32caab7b40c72493U;
  p114[2U] = (uint64_t)0x3c9ebe0a15c9bebcU;
  p114[3U] = (uint64_t)0x431d67c49c100d4cU;
  p215[0U] = (uint64_t)0x4cc5d4becb3e42b6U;
  p215[1U] = (uint64_t)0x597f299cfc657e2aU;
  p215[2U] = (uint64_t)0x5fcb6fab3ad6faecU;
  p215[3U] = (uint64_t)0x6c44198c4a475817U;
  uint64_t *p115 = h_01;
  uint64_t *p2 = h_01 + (uint32_t)4U;
  p115[0U] = (uint64_t)0x6a09e667f3bcc908U;
  p115[1U] = (uint64_t)0xbb67ae8584caa73bU;
  p115[2U] = (uint64_t)0x3c6ef372fe94f82bU;
  p115[3U] = (uint64_t)0xa54ff53a5f1d36f1U;
  p2[0U] = (uint64_t)0x510e527fade682d1U;
  p2[1U] = (uint64_t)0x9b05688c2b3e6c1fU;
  p2[2U] = (uint64_t)0x1f83d9abfb41bd6bU;
  p2[3U] = (uint64_t)0x5be0cd19137e2179U;
}

static void Hacl_Impl_SHA2_512_update(uint64_t *state, uint8_t *data)
{
  KRML_CHECK_SIZE((uint64_t)(uint32_t)0U, (uint32_t)16U);
  uint64_t data_w[16U];
  for (uint32_t _i = 0U; _i < (uint32_t)16U; ++_i)
    data_w[_i] = (uint64_t)(uint32_t)0U;
  Hacl_Hash_Lib_LoadStore_uint64s_from_be_bytes(data_w, data, (uint32_t)16U);
  uint64_t *hash_w = state + (uint32_t)160U;
  uint64_t *ws_w = state + (uint32_t)80U;
  uint64_t *k_w = state;
  uint64_t *counter_w = state + (uint32_t)168U;
  for (uint32_t i = (uint32_t)0U; i < (uint32_t)16U; i = i + (uint32_t)1U)
  {
    uint64_t b = data_w[i];
    ws_w[i] = b;
  }
  for (uint32_t i = (uint32_t)16U; i < (uint32_t)80U; i = i + (uint32_t)1U)
  {
    uint64_t t16 = ws_w[i - (uint32_t)16U];
    uint64_t t15 = ws_w[i - (uint32_t)15U];
    uint64_t t7 = ws_w[i - (uint32_t)7U];
    uint64_t t2 = ws_w[i - (uint32_t)2U];
    ws_w[i] =
      ((t2 >> (uint32_t)19U | t2 << ((uint32_t)64U - (uint32_t)19U))
      ^ ((t2 >> (uint32_t)61U | t2 << ((uint32_t)64U - (uint32_t)61U)) ^ t2 >> (uint32_t)6U))
      +
        t7
        +
          ((t15 >> (uint32_t)1U | t15 << ((uint32_t)64U - (uint32_t)1U))
          ^ ((t15 >> (uint32_t)8U | t15 << ((uint32_t)64U - (uint32_t)8U)) ^ t15 >> (uint32_t)7U))
          + t16;
  }
  uint64_t hash_0[8U] = { 0U };
  memcpy(hash_0, hash_w, (uint32_t)8U * sizeof hash_w[0U]);
  for (uint32_t i = (uint32_t)0U; i < (uint32_t)80U; i = i + (uint32_t)1U)
  {
    uint64_t a = hash_0[0U];
    uint64_t b = hash_0[1U];
    uint64_t c = hash_0[2U];
    uint64_t d = hash_0[3U];
    uint64_t e = hash_0[4U];
    uint64_t f1 = hash_0[5U];
    uint64_t g = hash_0[6U];
    uint64_t h = hash_0[7U];
    uint64_t k_t = k_w[i];
    uint64_t ws_t = ws_w[i];
    uint64_t
    t1 =
      h
      +
        ((e >> (uint32_t)14U | e << ((uint32_t)64U - (uint32_t)14U))
        ^
          ((e >> (uint32_t)18U | e << ((uint32_t)64U - (uint32_t)18U))
          ^ (e >> (uint32_t)41U | e << ((uint32_t)64U - (uint32_t)41U))))
      + ((e & f1) ^ (~e & g))
      + k_t
      + ws_t;
    uint64_t
    t2 =
      ((a >> (uint32_t)28U | a << ((uint32_t)64U - (uint32_t)28U))
      ^
        ((a >> (uint32_t)34U | a << ((uint32_t)64U - (uint32_t)34U))
        ^ (a >> (uint32_t)39U | a << ((uint32_t)64U - (uint32_t)39U))))
      + ((a & b) ^ ((a & c) ^ (b & c)));
    uint64_t x1 = t1 + t2;
    uint64_t x5 = d + t1;
    uint64_t *p1 = hash_0;
    uint64_t *p2 = hash_0 + (uint32_t)4U;
    p1[0U] = x1;
    p1[1U] = a;
    p1[2U] = b;
    p1[3U] = c;
    p2[0U] = x5;
    p2[1U] = e;
    p2[2U] = f1;
    p2[3U] = g;
  }
  for (uint32_t i = (uint32_t)0U; i < (uint32_t)8U; i = i + (uint32_t)1U)
  {
    uint64_t xi = hash_w[i];
    uint64_t yi = hash_0[i];
    hash_w[i] = xi + yi;
  }
  uint64_t c0 = counter_w[0U];
  uint64_t one1 = (uint64_t)(uint32_t)1U;
  counter_w[0U] = c0 + one1;
}

static void Hacl_Impl_SHA2_512_update_multi(uint64_t *state, uint8_t *data, uint32_t n1)
{
  for (uint32_t i = (uint32_t)0U; i < n1; i = i + (uint32_t)1U)
  {
    uint8_t *b = data + i * (uint32_t)128U;
    Hacl_Impl_SHA2_512_update(state, b);
  }
}

static void Hacl_Impl_SHA2_512_update_last(uint64_t *state, uint8_t *data, uint64_t len)
{
  uint8_t blocks[256U] = { 0U };
  uint32_t nb;
  if (len < (uint64_t)112U)
    nb = (uint32_t)1U;
  else
    nb = (uint32_t)2U;
  uint8_t *final_blocks;
  if (len < (uint64_t)112U)
    final_blocks = blocks + (uint32_t)128U;
  else
    final_blocks = blocks;
  memcpy(final_blocks, data, (uint32_t)len * sizeof data[0U]);
  uint64_t n1 = state[168U];
  uint8_t *padding = final_blocks + (uint32_t)len;
  FStar_UInt128_t
  encodedlen =
    FStar_UInt128_shift_left(FStar_UInt128_add(FStar_UInt128_mul_wide(n1, (uint64_t)(uint32_t)128U),
        FStar_UInt128_uint64_to_uint128(len)),
      (uint32_t)3U);
  uint32_t
  pad0len = ((uint32_t)256U - ((uint32_t)len + (uint32_t)16U + (uint32_t)1U)) % (uint32_t)128U;
  uint8_t *buf1 = padding;
  uint8_t *buf2 = padding + (uint32_t)1U + pad0len;
  buf1[0U] = (uint8_t)0x80U;
  store128_be(buf2, encodedlen);
  Hacl_Impl_SHA2_512_update_multi(state, final_blocks, nb);
}

static void Hacl_Impl_SHA2_512_finish(uint64_t *state, uint8_t *hash1)
{
  uint64_t *hash_w = state + (uint32_t)160U;
  Hacl_Hash_Lib_LoadStore_uint64s_to_be_bytes(hash1, hash_w, (uint32_t)8U);
}

static void Hacl_Impl_SHA2_512_hash(uint8_t *hash1, uint8_t *input, uint32_t len)
{
  KRML_CHECK_SIZE((uint64_t)(uint32_t)0U, (uint32_t)169U);
  uint64_t state[169U];
  for (uint32_t _i = 0U; _i < (uint32_t)169U; ++_i)
    state[_i] = (uint64_t)(uint32_t)0U;
  uint32_t n1 = len / (uint32_t)128U;
  uint32_t r = len % (uint32_t)128U;
  uint8_t *input_blocks = input;
  uint8_t *input_last = input + n1 * (uint32_t)128U;
  Hacl_Impl_SHA2_512_init(state);
  Hacl_Impl_SHA2_512_update_multi(state, input_blocks, n1);
  Hacl_Impl_SHA2_512_update_last(state, input_last, (uint64_t)r);
  Hacl_Impl_SHA2_512_finish(state, hash1);
}

uint32_t Hacl_SHA2_512_size_word = (uint32_t)8U;

uint32_t Hacl_SHA2_512_size_hash_w = (uint32_t)8U;

uint32_t Hacl_SHA2_512_size_block_w = (uint32_t)16U;

uint32_t Hacl_SHA2_512_size_hash = (uint32_t)64U;

uint32_t Hacl_SHA2_512_size_block = (uint32_t)128U;

uint32_t Hacl_SHA2_512_size_k_w = (uint32_t)80U;

uint32_t Hacl_SHA2_512_size_ws_w = (uint32_t)80U;

uint32_t Hacl_SHA2_512_size_whash_w = (uint32_t)8U;

uint32_t Hacl_SHA2_512_size_count_w = (uint32_t)1U;

uint32_t Hacl_SHA2_512_size_len_8 = (uint32_t)16U;

uint32_t Hacl_SHA2_512_size_state = (uint32_t)169U;

uint32_t Hacl_SHA2_512_pos_k_w = (uint32_t)0U;

uint32_t Hacl_SHA2_512_pos_ws_w = (uint32_t)80U;

uint32_t Hacl_SHA2_512_pos_whash_w = (uint32_t)160U;

uint32_t Hacl_SHA2_512_pos_count_w = (uint32_t)168U;

void Hacl_SHA2_512_init(uint64_t *state)
{
  Hacl_Impl_SHA2_512_init(state);
}

void Hacl_SHA2_512_update(uint64_t *state, uint8_t *data)
{
  Hacl_Impl_SHA2_512_update(state, data);
}

void Hacl_SHA2_512_update_multi(uint64_t *state, uint8_t *data, uint32_t n1)
{
  Hacl_Impl_SHA2_512_update_multi(state, data, n1);
}

void Hacl_SHA2_512_update_last(uint64_t *state, uint8_t *data, uint64_t len)
{
  Hacl_Impl_SHA2_512_update_last(state, data, len);
}

void Hacl_SHA2_512_finish(uint64_t *state, uint8_t *hash1)
{
  Hacl_Impl_SHA2_512_finish(state, hash1);
}

void Hacl_SHA2_512_hash(uint8_t *hash1, uint8_t *input, uint32_t len)
{
  Hacl_Impl_SHA2_512_hash(hash1, input, len);
}

