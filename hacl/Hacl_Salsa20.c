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


#include "Hacl_Salsa20.h"

static void
Hacl_Lib_Create_make_h32_4(uint32_t *b, uint32_t s0, uint32_t s1, uint32_t s2, uint32_t s3)
{
  b[0U] = s0;
  b[1U] = s1;
  b[2U] = s2;
  b[3U] = s3;
}

static void
Hacl_Lib_Create_make_h32_8(
  uint32_t *b,
  uint32_t s0,
  uint32_t s1,
  uint32_t s2,
  uint32_t s3,
  uint32_t s4,
  uint32_t s5,
  uint32_t s6,
  uint32_t s7
)
{
  Hacl_Lib_Create_make_h32_4(b, s0, s1, s2, s3);
  Hacl_Lib_Create_make_h32_4(b + (uint32_t)4U, s4, s5, s6, s7);
}

static void
Hacl_Lib_Create_make_h32_16(
  uint32_t *b,
  uint32_t s0,
  uint32_t s1,
  uint32_t s2,
  uint32_t s3,
  uint32_t s4,
  uint32_t s5,
  uint32_t s6,
  uint32_t s7,
  uint32_t s8,
  uint32_t s9,
  uint32_t s10,
  uint32_t s11,
  uint32_t s12,
  uint32_t s13,
  uint32_t s14,
  uint32_t s15
)
{
  Hacl_Lib_Create_make_h32_8(b, s0, s1, s2, s3, s4, s5, s6, s7);
  Hacl_Lib_Create_make_h32_8(b + (uint32_t)8U, s8, s9, s10, s11, s12, s13, s14, s15);
}

static void
Hacl_Lib_LoadStore32_uint32s_from_le_bytes(uint32_t *output, uint8_t *input, uint32_t len)
{
  for (uint32_t i = (uint32_t)0U; i < len; i = i + (uint32_t)1U)
  {
    uint8_t *x0 = input + (uint32_t)4U * i;
    uint32_t inputi = load32_le(x0);
    output[i] = inputi;
  }
}

static void
Hacl_Lib_LoadStore32_uint32s_to_le_bytes(uint8_t *output, uint32_t *input, uint32_t len)
{
  for (uint32_t i = (uint32_t)0U; i < len; i = i + (uint32_t)1U)
  {
    uint32_t hd1 = input[i];
    uint8_t *x0 = output + (uint32_t)4U * i;
    store32_le(x0, hd1);
  }
}

inline static void Hacl_Impl_Salsa20_setup(uint32_t *st, uint8_t *k, uint8_t *n1, uint64_t c)
{
  uint32_t tmp[10U] = { 0U };
  uint32_t *k_ = tmp;
  uint32_t *n_ = tmp + (uint32_t)8U;
  Hacl_Lib_LoadStore32_uint32s_from_le_bytes(k_, k, (uint32_t)8U);
  Hacl_Lib_LoadStore32_uint32s_from_le_bytes(n_, n1, (uint32_t)2U);
  uint32_t c0 = (uint32_t)c;
  uint32_t c1 = (uint32_t)(c >> (uint32_t)32U);
  uint32_t k0 = k_[0U];
  uint32_t k1 = k_[1U];
  uint32_t k2 = k_[2U];
  uint32_t k3 = k_[3U];
  uint32_t k4 = k_[4U];
  uint32_t k5 = k_[5U];
  uint32_t k6 = k_[6U];
  uint32_t k7 = k_[7U];
  uint32_t n0 = n_[0U];
  uint32_t n11 = n_[1U];
  Hacl_Lib_Create_make_h32_16(st,
    (uint32_t)0x61707865U,
    k0,
    k1,
    k2,
    k3,
    (uint32_t)0x3320646eU,
    n0,
    n11,
    c0,
    c1,
    (uint32_t)0x79622d32U,
    k4,
    k5,
    k6,
    k7,
    (uint32_t)0x6b206574U);
}

inline static void
Hacl_Impl_Salsa20_line(uint32_t *st, uint32_t a, uint32_t b, uint32_t d, uint32_t s)
{
  uint32_t sa = st[a];
  uint32_t sb = st[b];
  uint32_t sd = st[d];
  uint32_t sbd = sb + sd;
  uint32_t sbds = sbd << s | sbd >> ((uint32_t)32U - s);
  st[a] = sa ^ sbds;
}

inline static void
Hacl_Impl_Salsa20_quarter_round(uint32_t *st, uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
  Hacl_Impl_Salsa20_line(st, b, a, d, (uint32_t)7U);
  Hacl_Impl_Salsa20_line(st, c, b, a, (uint32_t)9U);
  Hacl_Impl_Salsa20_line(st, d, c, b, (uint32_t)13U);
  Hacl_Impl_Salsa20_line(st, a, d, c, (uint32_t)18U);
}

inline static void Hacl_Impl_Salsa20_double_round(uint32_t *st)
{
  Hacl_Impl_Salsa20_quarter_round(st, (uint32_t)0U, (uint32_t)4U, (uint32_t)8U, (uint32_t)12U);
  Hacl_Impl_Salsa20_quarter_round(st, (uint32_t)5U, (uint32_t)9U, (uint32_t)13U, (uint32_t)1U);
  Hacl_Impl_Salsa20_quarter_round(st, (uint32_t)10U, (uint32_t)14U, (uint32_t)2U, (uint32_t)6U);
  Hacl_Impl_Salsa20_quarter_round(st, (uint32_t)15U, (uint32_t)3U, (uint32_t)7U, (uint32_t)11U);
  Hacl_Impl_Salsa20_quarter_round(st, (uint32_t)0U, (uint32_t)1U, (uint32_t)2U, (uint32_t)3U);
  Hacl_Impl_Salsa20_quarter_round(st, (uint32_t)5U, (uint32_t)6U, (uint32_t)7U, (uint32_t)4U);
  Hacl_Impl_Salsa20_quarter_round(st, (uint32_t)10U, (uint32_t)11U, (uint32_t)8U, (uint32_t)9U);
  Hacl_Impl_Salsa20_quarter_round(st,
    (uint32_t)15U,
    (uint32_t)12U,
    (uint32_t)13U,
    (uint32_t)14U);
}

inline static void Hacl_Impl_Salsa20_rounds(uint32_t *st)
{
  for (uint32_t i = (uint32_t)0U; i < (uint32_t)10U; i = i + (uint32_t)1U)
    Hacl_Impl_Salsa20_double_round(st);
}

inline static void Hacl_Impl_Salsa20_sum_states(uint32_t *st, uint32_t *st_)
{
  for (uint32_t i = (uint32_t)0U; i < (uint32_t)16U; i = i + (uint32_t)1U)
  {
    uint32_t xi = st[i];
    uint32_t yi = st_[i];
    st[i] = xi + yi;
  }
}

inline static void Hacl_Impl_Salsa20_copy_state(uint32_t *st, uint32_t *st_)
{
  memcpy(st, st_, (uint32_t)16U * sizeof st_[0U]);
}

inline static void Hacl_Impl_Salsa20_salsa20_core(uint32_t *k, uint32_t *st, uint64_t ctr)
{
  uint32_t c0 = (uint32_t)ctr;
  uint32_t c1 = (uint32_t)(ctr >> (uint32_t)32U);
  st[8U] = c0;
  st[9U] = c1;
  Hacl_Impl_Salsa20_copy_state(k, st);
  Hacl_Impl_Salsa20_rounds(k);
  Hacl_Impl_Salsa20_sum_states(k, st);
}

inline static void
Hacl_Impl_Salsa20_salsa20_block(uint8_t *stream_block, uint32_t *st, uint64_t ctr)
{
  uint32_t st_[16U] = { 0U };
  Hacl_Impl_Salsa20_salsa20_core(st_, st, ctr);
  Hacl_Lib_LoadStore32_uint32s_to_le_bytes(stream_block, st_, (uint32_t)16U);
}

inline static void Hacl_Impl_Salsa20_init(uint32_t *st, uint8_t *k, uint8_t *n1)
{
  Hacl_Impl_Salsa20_setup(st, k, n1, (uint64_t)0U);
}

static void
Hacl_Impl_Salsa20_update_last(
  uint8_t *output,
  uint8_t *plain,
  uint32_t len,
  uint32_t *st,
  uint64_t ctr
)
{
  uint8_t block[64U] = { 0U };
  Hacl_Impl_Salsa20_salsa20_block(block, st, ctr);
  uint8_t *mask = block;
  for (uint32_t i = (uint32_t)0U; i < len; i = i + (uint32_t)1U)
  {
    uint8_t xi = plain[i];
    uint8_t yi = mask[i];
    output[i] = xi ^ yi;
  }
}

static void
Hacl_Impl_Salsa20_update(uint8_t *output, uint8_t *plain, uint32_t *st, uint64_t ctr)
{
  uint32_t b[48U] = { 0U };
  uint32_t *k = b;
  uint32_t *ib = b + (uint32_t)16U;
  uint32_t *ob = b + (uint32_t)32U;
  Hacl_Impl_Salsa20_salsa20_core(k, st, ctr);
  Hacl_Lib_LoadStore32_uint32s_from_le_bytes(ib, plain, (uint32_t)16U);
  for (uint32_t i = (uint32_t)0U; i < (uint32_t)16U; i = i + (uint32_t)1U)
  {
    uint32_t xi = ib[i];
    uint32_t yi = k[i];
    ob[i] = xi ^ yi;
  }
  Hacl_Lib_LoadStore32_uint32s_to_le_bytes(output, ob, (uint32_t)16U);
}

static void
Hacl_Impl_Salsa20_salsa20_counter_mode_blocks(
  uint8_t *output,
  uint8_t *plain,
  uint32_t len,
  uint32_t *st,
  uint64_t ctr
)
{
  for (uint32_t i = (uint32_t)0U; i < len; i = i + (uint32_t)1U)
  {
    uint8_t *b = plain + (uint32_t)64U * i;
    uint8_t *o = output + (uint32_t)64U * i;
    Hacl_Impl_Salsa20_update(o, b, st, ctr + (uint64_t)i);
  }
}

static void
Hacl_Impl_Salsa20_salsa20_counter_mode(
  uint8_t *output,
  uint8_t *plain,
  uint32_t len,
  uint32_t *st,
  uint64_t ctr
)
{
  uint32_t blocks_len = len >> (uint32_t)6U;
  uint32_t part_len = len & (uint32_t)0x3fU;
  uint8_t *output_ = output;
  uint8_t *plain_ = plain;
  uint8_t *output__ = output + (uint32_t)64U * blocks_len;
  uint8_t *plain__ = plain + (uint32_t)64U * blocks_len;
  Hacl_Impl_Salsa20_salsa20_counter_mode_blocks(output_, plain_, blocks_len, st, ctr);
  if (part_len > (uint32_t)0U)
    Hacl_Impl_Salsa20_update_last(output__, plain__, part_len, st, ctr + (uint64_t)blocks_len);
}

static void
Hacl_Impl_Salsa20_salsa20(
  uint8_t *output,
  uint8_t *plain,
  uint32_t len,
  uint8_t *k,
  uint8_t *n1,
  uint64_t ctr
)
{
  uint32_t buf[16U] = { 0U };
  uint32_t *st = buf;
  Hacl_Impl_Salsa20_init(st, k, n1);
  Hacl_Impl_Salsa20_salsa20_counter_mode(output, plain, len, st, ctr);
}

inline static void Hacl_Impl_HSalsa20_setup(uint32_t *st, uint8_t *k, uint8_t *n1)
{
  uint32_t tmp[12U] = { 0U };
  uint32_t *k_ = tmp;
  uint32_t *n_ = tmp + (uint32_t)8U;
  Hacl_Lib_LoadStore32_uint32s_from_le_bytes(k_, k, (uint32_t)8U);
  Hacl_Lib_LoadStore32_uint32s_from_le_bytes(n_, n1, (uint32_t)4U);
  uint32_t k0 = k_[0U];
  uint32_t k1 = k_[1U];
  uint32_t k2 = k_[2U];
  uint32_t k3 = k_[3U];
  uint32_t k4 = k_[4U];
  uint32_t k5 = k_[5U];
  uint32_t k6 = k_[6U];
  uint32_t k7 = k_[7U];
  uint32_t n0 = n_[0U];
  uint32_t n11 = n_[1U];
  uint32_t n2 = n_[2U];
  uint32_t n3 = n_[3U];
  Hacl_Lib_Create_make_h32_16(st,
    (uint32_t)0x61707865U,
    k0,
    k1,
    k2,
    k3,
    (uint32_t)0x3320646eU,
    n0,
    n11,
    n2,
    n3,
    (uint32_t)0x79622d32U,
    k4,
    k5,
    k6,
    k7,
    (uint32_t)0x6b206574U);
}

static void
Hacl_Impl_HSalsa20_crypto_core_hsalsa20(uint8_t *output, uint8_t *nonce, uint8_t *key)
{
  uint32_t tmp[24U] = { 0U };
  uint32_t *st = tmp;
  uint32_t *hs = tmp + (uint32_t)16U;
  Hacl_Impl_HSalsa20_setup(st, key, nonce);
  Hacl_Impl_Salsa20_rounds(st);
  uint32_t hs0 = st[0U];
  uint32_t hs1 = st[5U];
  uint32_t hs2 = st[10U];
  uint32_t hs3 = st[15U];
  uint32_t hs4 = st[6U];
  uint32_t hs5 = st[7U];
  uint32_t hs6 = st[8U];
  uint32_t hs7 = st[9U];
  Hacl_Lib_Create_make_h32_8(hs, hs0, hs1, hs2, hs3, hs4, hs5, hs6, hs7);
  Hacl_Lib_LoadStore32_uint32s_to_le_bytes(output, hs, (uint32_t)8U);
}

void
Hacl_Salsa20_salsa20(
  uint8_t *output,
  uint8_t *plain,
  uint32_t len,
  uint8_t *k,
  uint8_t *n1,
  uint64_t ctr
)
{
  Hacl_Impl_Salsa20_salsa20(output, plain, len, k, n1, ctr);
}

void Hacl_Salsa20_hsalsa20(uint8_t *output, uint8_t *key, uint8_t *nonce)
{
  Hacl_Impl_HSalsa20_crypto_core_hsalsa20(output, nonce, key);
}

