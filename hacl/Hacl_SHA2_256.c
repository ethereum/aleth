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


#include "Hacl_SHA2_256.h"

static void
Hacl_Hash_Lib_LoadStore_uint32s_from_be_bytes(uint32_t *output, uint8_t *input, uint32_t len)
{
  for (uint32_t i = (uint32_t)0U; i < len; i = i + (uint32_t)1U)
  {
    uint8_t *x0 = input + (uint32_t)4U * i;
    uint32_t inputi = load32_be(x0);
    output[i] = inputi;
  }
}

static void
Hacl_Hash_Lib_LoadStore_uint32s_to_be_bytes(uint8_t *output, uint32_t *input, uint32_t len)
{
  for (uint32_t i = (uint32_t)0U; i < len; i = i + (uint32_t)1U)
  {
    uint32_t hd1 = input[i];
    uint8_t *x0 = output + (uint32_t)4U * i;
    store32_be(x0, hd1);
  }
}

static void Hacl_Impl_SHA2_256_init(uint32_t *state)
{
  uint32_t *k1 = state;
  uint32_t *h_01 = state + (uint32_t)128U;
  uint32_t *p10 = k1;
  uint32_t *p20 = k1 + (uint32_t)16U;
  uint32_t *p3 = k1 + (uint32_t)32U;
  uint32_t *p4 = k1 + (uint32_t)48U;
  uint32_t *p11 = p10;
  uint32_t *p21 = p10 + (uint32_t)8U;
  uint32_t *p12 = p11;
  uint32_t *p22 = p11 + (uint32_t)4U;
  p12[0U] = (uint32_t)0x428a2f98U;
  p12[1U] = (uint32_t)0x71374491U;
  p12[2U] = (uint32_t)0xb5c0fbcfU;
  p12[3U] = (uint32_t)0xe9b5dba5U;
  p22[0U] = (uint32_t)0x3956c25bU;
  p22[1U] = (uint32_t)0x59f111f1U;
  p22[2U] = (uint32_t)0x923f82a4U;
  p22[3U] = (uint32_t)0xab1c5ed5U;
  uint32_t *p13 = p21;
  uint32_t *p23 = p21 + (uint32_t)4U;
  p13[0U] = (uint32_t)0xd807aa98U;
  p13[1U] = (uint32_t)0x12835b01U;
  p13[2U] = (uint32_t)0x243185beU;
  p13[3U] = (uint32_t)0x550c7dc3U;
  p23[0U] = (uint32_t)0x72be5d74U;
  p23[1U] = (uint32_t)0x80deb1feU;
  p23[2U] = (uint32_t)0x9bdc06a7U;
  p23[3U] = (uint32_t)0xc19bf174U;
  uint32_t *p14 = p20;
  uint32_t *p24 = p20 + (uint32_t)8U;
  uint32_t *p15 = p14;
  uint32_t *p25 = p14 + (uint32_t)4U;
  p15[0U] = (uint32_t)0xe49b69c1U;
  p15[1U] = (uint32_t)0xefbe4786U;
  p15[2U] = (uint32_t)0x0fc19dc6U;
  p15[3U] = (uint32_t)0x240ca1ccU;
  p25[0U] = (uint32_t)0x2de92c6fU;
  p25[1U] = (uint32_t)0x4a7484aaU;
  p25[2U] = (uint32_t)0x5cb0a9dcU;
  p25[3U] = (uint32_t)0x76f988daU;
  uint32_t *p16 = p24;
  uint32_t *p26 = p24 + (uint32_t)4U;
  p16[0U] = (uint32_t)0x983e5152U;
  p16[1U] = (uint32_t)0xa831c66dU;
  p16[2U] = (uint32_t)0xb00327c8U;
  p16[3U] = (uint32_t)0xbf597fc7U;
  p26[0U] = (uint32_t)0xc6e00bf3U;
  p26[1U] = (uint32_t)0xd5a79147U;
  p26[2U] = (uint32_t)0x06ca6351U;
  p26[3U] = (uint32_t)0x14292967U;
  uint32_t *p17 = p3;
  uint32_t *p27 = p3 + (uint32_t)8U;
  uint32_t *p18 = p17;
  uint32_t *p28 = p17 + (uint32_t)4U;
  p18[0U] = (uint32_t)0x27b70a85U;
  p18[1U] = (uint32_t)0x2e1b2138U;
  p18[2U] = (uint32_t)0x4d2c6dfcU;
  p18[3U] = (uint32_t)0x53380d13U;
  p28[0U] = (uint32_t)0x650a7354U;
  p28[1U] = (uint32_t)0x766a0abbU;
  p28[2U] = (uint32_t)0x81c2c92eU;
  p28[3U] = (uint32_t)0x92722c85U;
  uint32_t *p19 = p27;
  uint32_t *p29 = p27 + (uint32_t)4U;
  p19[0U] = (uint32_t)0xa2bfe8a1U;
  p19[1U] = (uint32_t)0xa81a664bU;
  p19[2U] = (uint32_t)0xc24b8b70U;
  p19[3U] = (uint32_t)0xc76c51a3U;
  p29[0U] = (uint32_t)0xd192e819U;
  p29[1U] = (uint32_t)0xd6990624U;
  p29[2U] = (uint32_t)0xf40e3585U;
  p29[3U] = (uint32_t)0x106aa070U;
  uint32_t *p110 = p4;
  uint32_t *p210 = p4 + (uint32_t)8U;
  uint32_t *p1 = p110;
  uint32_t *p211 = p110 + (uint32_t)4U;
  p1[0U] = (uint32_t)0x19a4c116U;
  p1[1U] = (uint32_t)0x1e376c08U;
  p1[2U] = (uint32_t)0x2748774cU;
  p1[3U] = (uint32_t)0x34b0bcb5U;
  p211[0U] = (uint32_t)0x391c0cb3U;
  p211[1U] = (uint32_t)0x4ed8aa4aU;
  p211[2U] = (uint32_t)0x5b9cca4fU;
  p211[3U] = (uint32_t)0x682e6ff3U;
  uint32_t *p111 = p210;
  uint32_t *p212 = p210 + (uint32_t)4U;
  p111[0U] = (uint32_t)0x748f82eeU;
  p111[1U] = (uint32_t)0x78a5636fU;
  p111[2U] = (uint32_t)0x84c87814U;
  p111[3U] = (uint32_t)0x8cc70208U;
  p212[0U] = (uint32_t)0x90befffaU;
  p212[1U] = (uint32_t)0xa4506cebU;
  p212[2U] = (uint32_t)0xbef9a3f7U;
  p212[3U] = (uint32_t)0xc67178f2U;
  uint32_t *p112 = h_01;
  uint32_t *p2 = h_01 + (uint32_t)4U;
  p112[0U] = (uint32_t)0x6a09e667U;
  p112[1U] = (uint32_t)0xbb67ae85U;
  p112[2U] = (uint32_t)0x3c6ef372U;
  p112[3U] = (uint32_t)0xa54ff53aU;
  p2[0U] = (uint32_t)0x510e527fU;
  p2[1U] = (uint32_t)0x9b05688cU;
  p2[2U] = (uint32_t)0x1f83d9abU;
  p2[3U] = (uint32_t)0x5be0cd19U;
}

static void Hacl_Impl_SHA2_256_update(uint32_t *state, uint8_t *data)
{
  uint32_t data_w[16U] = { 0U };
  Hacl_Hash_Lib_LoadStore_uint32s_from_be_bytes(data_w, data, (uint32_t)16U);
  uint32_t *hash_w = state + (uint32_t)128U;
  uint32_t *ws_w = state + (uint32_t)64U;
  uint32_t *k_w = state;
  uint32_t *counter_w = state + (uint32_t)136U;
  for (uint32_t i = (uint32_t)0U; i < (uint32_t)16U; i = i + (uint32_t)1U)
  {
    uint32_t b = data_w[i];
    ws_w[i] = b;
  }
  for (uint32_t i = (uint32_t)16U; i < (uint32_t)64U; i = i + (uint32_t)1U)
  {
    uint32_t t16 = ws_w[i - (uint32_t)16U];
    uint32_t t15 = ws_w[i - (uint32_t)15U];
    uint32_t t7 = ws_w[i - (uint32_t)7U];
    uint32_t t2 = ws_w[i - (uint32_t)2U];
    ws_w[i] =
      ((t2 >> (uint32_t)17U | t2 << ((uint32_t)32U - (uint32_t)17U))
      ^ ((t2 >> (uint32_t)19U | t2 << ((uint32_t)32U - (uint32_t)19U)) ^ t2 >> (uint32_t)10U))
      +
        t7
        +
          ((t15 >> (uint32_t)7U | t15 << ((uint32_t)32U - (uint32_t)7U))
          ^ ((t15 >> (uint32_t)18U | t15 << ((uint32_t)32U - (uint32_t)18U)) ^ t15 >> (uint32_t)3U))
          + t16;
  }
  uint32_t hash_0[8U] = { 0U };
  memcpy(hash_0, hash_w, (uint32_t)8U * sizeof hash_w[0U]);
  for (uint32_t i = (uint32_t)0U; i < (uint32_t)64U; i = i + (uint32_t)1U)
  {
    uint32_t a = hash_0[0U];
    uint32_t b = hash_0[1U];
    uint32_t c = hash_0[2U];
    uint32_t d = hash_0[3U];
    uint32_t e = hash_0[4U];
    uint32_t f1 = hash_0[5U];
    uint32_t g = hash_0[6U];
    uint32_t h = hash_0[7U];
    uint32_t kt = k_w[i];
    uint32_t wst = ws_w[i];
    uint32_t
    t1 =
      h
      +
        ((e >> (uint32_t)6U | e << ((uint32_t)32U - (uint32_t)6U))
        ^
          ((e >> (uint32_t)11U | e << ((uint32_t)32U - (uint32_t)11U))
          ^ (e >> (uint32_t)25U | e << ((uint32_t)32U - (uint32_t)25U))))
      + ((e & f1) ^ (~e & g))
      + kt
      + wst;
    uint32_t
    t2 =
      ((a >> (uint32_t)2U | a << ((uint32_t)32U - (uint32_t)2U))
      ^
        ((a >> (uint32_t)13U | a << ((uint32_t)32U - (uint32_t)13U))
        ^ (a >> (uint32_t)22U | a << ((uint32_t)32U - (uint32_t)22U))))
      + ((a & b) ^ ((a & c) ^ (b & c)));
    uint32_t x1 = t1 + t2;
    uint32_t x5 = d + t1;
    uint32_t *p1 = hash_0;
    uint32_t *p2 = hash_0 + (uint32_t)4U;
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
    uint32_t xi = hash_w[i];
    uint32_t yi = hash_0[i];
    hash_w[i] = xi + yi;
  }
  uint32_t c0 = counter_w[0U];
  uint32_t one1 = (uint32_t)1U;
  counter_w[0U] = c0 + one1;
}

static void Hacl_Impl_SHA2_256_update_multi(uint32_t *state, uint8_t *data, uint32_t n1)
{
  for (uint32_t i = (uint32_t)0U; i < n1; i = i + (uint32_t)1U)
  {
    uint8_t *b = data + i * (uint32_t)64U;
    Hacl_Impl_SHA2_256_update(state, b);
  }
}

static void Hacl_Impl_SHA2_256_update_last(uint32_t *state, uint8_t *data, uint32_t len)
{
  uint8_t blocks[128U] = { 0U };
  uint32_t nb;
  if (len < (uint32_t)56U)
    nb = (uint32_t)1U;
  else
    nb = (uint32_t)2U;
  uint8_t *final_blocks;
  if (len < (uint32_t)56U)
    final_blocks = blocks + (uint32_t)64U;
  else
    final_blocks = blocks;
  memcpy(final_blocks, data, len * sizeof data[0U]);
  uint32_t n1 = state[136U];
  uint8_t *padding = final_blocks + len;
  uint32_t
  pad0len = ((uint32_t)64U - (len + (uint32_t)8U + (uint32_t)1U) % (uint32_t)64U) % (uint32_t)64U;
  uint8_t *buf1 = padding;
  uint8_t *buf2 = padding + (uint32_t)1U + pad0len;
  uint64_t
  encodedlen = ((uint64_t)n1 * (uint64_t)(uint32_t)64U + (uint64_t)len) * (uint64_t)(uint32_t)8U;
  buf1[0U] = (uint8_t)0x80U;
  store64_be(buf2, encodedlen);
  Hacl_Impl_SHA2_256_update_multi(state, final_blocks, nb);
}

static void Hacl_Impl_SHA2_256_finish(uint32_t *state, uint8_t *hash1)
{
  uint32_t *hash_w = state + (uint32_t)128U;
  Hacl_Hash_Lib_LoadStore_uint32s_to_be_bytes(hash1, hash_w, (uint32_t)8U);
}

static void Hacl_Impl_SHA2_256_hash(uint8_t *hash1, uint8_t *input, uint32_t len)
{
  uint32_t state[137U] = { 0U };
  uint32_t n1 = len / (uint32_t)64U;
  uint32_t r = len % (uint32_t)64U;
  uint8_t *input_blocks = input;
  uint8_t *input_last = input + n1 * (uint32_t)64U;
  Hacl_Impl_SHA2_256_init(state);
  Hacl_Impl_SHA2_256_update_multi(state, input_blocks, n1);
  Hacl_Impl_SHA2_256_update_last(state, input_last, r);
  Hacl_Impl_SHA2_256_finish(state, hash1);
}

uint32_t Hacl_SHA2_256_size_hash = (uint32_t)32U;

uint32_t Hacl_SHA2_256_size_block = (uint32_t)64U;

uint32_t Hacl_SHA2_256_size_state = (uint32_t)137U;

void Hacl_SHA2_256_init(uint32_t *state)
{
  Hacl_Impl_SHA2_256_init(state);
}

void Hacl_SHA2_256_update(uint32_t *state, uint8_t *data_8)
{
  Hacl_Impl_SHA2_256_update(state, data_8);
}

void Hacl_SHA2_256_update_multi(uint32_t *state, uint8_t *data, uint32_t n1)
{
  Hacl_Impl_SHA2_256_update_multi(state, data, n1);
}

void Hacl_SHA2_256_update_last(uint32_t *state, uint8_t *data, uint32_t len)
{
  Hacl_Impl_SHA2_256_update_last(state, data, len);
}

void Hacl_SHA2_256_finish(uint32_t *state, uint8_t *hash1)
{
  Hacl_Impl_SHA2_256_finish(state, hash1);
}

void Hacl_SHA2_256_hash(uint8_t *hash1, uint8_t *input, uint32_t len)
{
  Hacl_Impl_SHA2_256_hash(hash1, input, len);
}

