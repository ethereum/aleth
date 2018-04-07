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


#include "Hacl_Ed25519.h"

static void Hacl_Bignum_Modulo_carry_top(uint64_t *b)
{
  uint64_t b4 = b[4U];
  uint64_t b0 = b[0U];
  uint64_t b4_ = b4 & (uint64_t)0x7ffffffffffffU;
  uint64_t b0_ = b0 + (uint64_t)19U * (b4 >> (uint32_t)51U);
  b[4U] = b4_;
  b[0U] = b0_;
}

inline static void
Hacl_Bignum_Fproduct_copy_from_wide_(uint64_t *output, FStar_UInt128_t *input)
{
  {
    FStar_UInt128_t xi = input[0U];
    output[0U] = FStar_UInt128_uint128_to_uint64(xi);
  }
  {
    FStar_UInt128_t xi = input[1U];
    output[1U] = FStar_UInt128_uint128_to_uint64(xi);
  }
  {
    FStar_UInt128_t xi = input[2U];
    output[2U] = FStar_UInt128_uint128_to_uint64(xi);
  }
  {
    FStar_UInt128_t xi = input[3U];
    output[3U] = FStar_UInt128_uint128_to_uint64(xi);
  }
  {
    FStar_UInt128_t xi = input[4U];
    output[4U] = FStar_UInt128_uint128_to_uint64(xi);
  }
}

inline static void
Hacl_Bignum_Fproduct_sum_scalar_multiplication_(
  FStar_UInt128_t *output,
  uint64_t *input,
  uint64_t s
)
{
  {
    FStar_UInt128_t xi = output[0U];
    uint64_t yi = input[0U];
    output[0U] = FStar_UInt128_add_mod(xi, FStar_UInt128_mul_wide(yi, s));
  }
  {
    FStar_UInt128_t xi = output[1U];
    uint64_t yi = input[1U];
    output[1U] = FStar_UInt128_add_mod(xi, FStar_UInt128_mul_wide(yi, s));
  }
  {
    FStar_UInt128_t xi = output[2U];
    uint64_t yi = input[2U];
    output[2U] = FStar_UInt128_add_mod(xi, FStar_UInt128_mul_wide(yi, s));
  }
  {
    FStar_UInt128_t xi = output[3U];
    uint64_t yi = input[3U];
    output[3U] = FStar_UInt128_add_mod(xi, FStar_UInt128_mul_wide(yi, s));
  }
  {
    FStar_UInt128_t xi = output[4U];
    uint64_t yi = input[4U];
    output[4U] = FStar_UInt128_add_mod(xi, FStar_UInt128_mul_wide(yi, s));
  }
}

inline static void Hacl_Bignum_Fproduct_carry_wide_(FStar_UInt128_t *tmp)
{
  {
    uint32_t ctr = (uint32_t)0U;
    FStar_UInt128_t tctr = tmp[ctr];
    FStar_UInt128_t tctrp1 = tmp[ctr + (uint32_t)1U];
    uint64_t r0 = FStar_UInt128_uint128_to_uint64(tctr) & (uint64_t)0x7ffffffffffffU;
    FStar_UInt128_t c = FStar_UInt128_shift_right(tctr, (uint32_t)51U);
    tmp[ctr] = FStar_UInt128_uint64_to_uint128(r0);
    tmp[ctr + (uint32_t)1U] = FStar_UInt128_add(tctrp1, c);
  }
  {
    uint32_t ctr = (uint32_t)1U;
    FStar_UInt128_t tctr = tmp[ctr];
    FStar_UInt128_t tctrp1 = tmp[ctr + (uint32_t)1U];
    uint64_t r0 = FStar_UInt128_uint128_to_uint64(tctr) & (uint64_t)0x7ffffffffffffU;
    FStar_UInt128_t c = FStar_UInt128_shift_right(tctr, (uint32_t)51U);
    tmp[ctr] = FStar_UInt128_uint64_to_uint128(r0);
    tmp[ctr + (uint32_t)1U] = FStar_UInt128_add(tctrp1, c);
  }
  {
    uint32_t ctr = (uint32_t)2U;
    FStar_UInt128_t tctr = tmp[ctr];
    FStar_UInt128_t tctrp1 = tmp[ctr + (uint32_t)1U];
    uint64_t r0 = FStar_UInt128_uint128_to_uint64(tctr) & (uint64_t)0x7ffffffffffffU;
    FStar_UInt128_t c = FStar_UInt128_shift_right(tctr, (uint32_t)51U);
    tmp[ctr] = FStar_UInt128_uint64_to_uint128(r0);
    tmp[ctr + (uint32_t)1U] = FStar_UInt128_add(tctrp1, c);
  }
  {
    uint32_t ctr = (uint32_t)3U;
    FStar_UInt128_t tctr = tmp[ctr];
    FStar_UInt128_t tctrp1 = tmp[ctr + (uint32_t)1U];
    uint64_t r0 = FStar_UInt128_uint128_to_uint64(tctr) & (uint64_t)0x7ffffffffffffU;
    FStar_UInt128_t c = FStar_UInt128_shift_right(tctr, (uint32_t)51U);
    tmp[ctr] = FStar_UInt128_uint64_to_uint128(r0);
    tmp[ctr + (uint32_t)1U] = FStar_UInt128_add(tctrp1, c);
  }
}

inline static void Hacl_Bignum_Fmul_shift_reduce(uint64_t *output)
{
  uint64_t tmp = output[4U];
  {
    uint32_t ctr = (uint32_t)5U - (uint32_t)0U - (uint32_t)1U;
    uint64_t z = output[ctr - (uint32_t)1U];
    output[ctr] = z;
  }
  {
    uint32_t ctr = (uint32_t)5U - (uint32_t)1U - (uint32_t)1U;
    uint64_t z = output[ctr - (uint32_t)1U];
    output[ctr] = z;
  }
  {
    uint32_t ctr = (uint32_t)5U - (uint32_t)2U - (uint32_t)1U;
    uint64_t z = output[ctr - (uint32_t)1U];
    output[ctr] = z;
  }
  {
    uint32_t ctr = (uint32_t)5U - (uint32_t)3U - (uint32_t)1U;
    uint64_t z = output[ctr - (uint32_t)1U];
    output[ctr] = z;
  }
  output[0U] = tmp;
  uint64_t b0 = output[0U];
  output[0U] = (uint64_t)19U * b0;
}

static void
Hacl_Bignum_Fmul_mul_shift_reduce_(FStar_UInt128_t *output, uint64_t *input, uint64_t *input21)
{
  {
    uint64_t input2i = input21[0U];
    Hacl_Bignum_Fproduct_sum_scalar_multiplication_(output, input, input2i);
    Hacl_Bignum_Fmul_shift_reduce(input);
  }
  {
    uint64_t input2i = input21[1U];
    Hacl_Bignum_Fproduct_sum_scalar_multiplication_(output, input, input2i);
    Hacl_Bignum_Fmul_shift_reduce(input);
  }
  {
    uint64_t input2i = input21[2U];
    Hacl_Bignum_Fproduct_sum_scalar_multiplication_(output, input, input2i);
    Hacl_Bignum_Fmul_shift_reduce(input);
  }
  {
    uint64_t input2i = input21[3U];
    Hacl_Bignum_Fproduct_sum_scalar_multiplication_(output, input, input2i);
    Hacl_Bignum_Fmul_shift_reduce(input);
  }
  uint32_t i = (uint32_t)4U;
  uint64_t input2i = input21[i];
  Hacl_Bignum_Fproduct_sum_scalar_multiplication_(output, input, input2i);
}

inline static void Hacl_Bignum_Fmul_fmul(uint64_t *output, uint64_t *input, uint64_t *input21)
{
  uint64_t tmp[5U] = { 0U };
  memcpy(tmp, input, (uint32_t)5U * sizeof input[0U]);
  KRML_CHECK_SIZE(FStar_UInt128_uint64_to_uint128((uint64_t)0U), (uint32_t)5U);
  FStar_UInt128_t t[5U];
  for (uint32_t _i = 0U; _i < (uint32_t)5U; ++_i)
    t[_i] = FStar_UInt128_uint64_to_uint128((uint64_t)0U);
  Hacl_Bignum_Fmul_mul_shift_reduce_(t, tmp, input21);
  Hacl_Bignum_Fproduct_carry_wide_(t);
  FStar_UInt128_t b4 = t[4U];
  FStar_UInt128_t b0 = t[0U];
  FStar_UInt128_t
  b4_ = FStar_UInt128_logand(b4, FStar_UInt128_uint64_to_uint128((uint64_t)0x7ffffffffffffU));
  FStar_UInt128_t
  b0_ =
    FStar_UInt128_add(b0,
      FStar_UInt128_mul_wide((uint64_t)19U,
        FStar_UInt128_uint128_to_uint64(FStar_UInt128_shift_right(b4, (uint32_t)51U))));
  t[4U] = b4_;
  t[0U] = b0_;
  Hacl_Bignum_Fproduct_copy_from_wide_(output, t);
  uint64_t i0 = output[0U];
  uint64_t i1 = output[1U];
  uint64_t i0_ = i0 & (uint64_t)0x7ffffffffffffU;
  uint64_t i1_ = i1 + (i0 >> (uint32_t)51U);
  output[0U] = i0_;
  output[1U] = i1_;
}

inline static void Hacl_Bignum_Fsquare_fsquare__(FStar_UInt128_t *tmp, uint64_t *output)
{
  uint64_t r0 = output[0U];
  uint64_t r1 = output[1U];
  uint64_t r2 = output[2U];
  uint64_t r3 = output[3U];
  uint64_t r4 = output[4U];
  uint64_t d0 = r0 * (uint64_t)2U;
  uint64_t d1 = r1 * (uint64_t)2U;
  uint64_t d2 = r2 * (uint64_t)2U * (uint64_t)19U;
  uint64_t d419 = r4 * (uint64_t)19U;
  uint64_t d4 = d419 * (uint64_t)2U;
  FStar_UInt128_t
  s0 =
    FStar_UInt128_add(FStar_UInt128_add(FStar_UInt128_mul_wide(r0, r0),
        FStar_UInt128_mul_wide(d4, r1)),
      FStar_UInt128_mul_wide(d2, r3));
  FStar_UInt128_t
  s1 =
    FStar_UInt128_add(FStar_UInt128_add(FStar_UInt128_mul_wide(d0, r1),
        FStar_UInt128_mul_wide(d4, r2)),
      FStar_UInt128_mul_wide(r3 * (uint64_t)19U, r3));
  FStar_UInt128_t
  s2 =
    FStar_UInt128_add(FStar_UInt128_add(FStar_UInt128_mul_wide(d0, r2),
        FStar_UInt128_mul_wide(r1, r1)),
      FStar_UInt128_mul_wide(d4, r3));
  FStar_UInt128_t
  s3 =
    FStar_UInt128_add(FStar_UInt128_add(FStar_UInt128_mul_wide(d0, r3),
        FStar_UInt128_mul_wide(d1, r2)),
      FStar_UInt128_mul_wide(r4, d419));
  FStar_UInt128_t
  s4 =
    FStar_UInt128_add(FStar_UInt128_add(FStar_UInt128_mul_wide(d0, r4),
        FStar_UInt128_mul_wide(d1, r3)),
      FStar_UInt128_mul_wide(r2, r2));
  tmp[0U] = s0;
  tmp[1U] = s1;
  tmp[2U] = s2;
  tmp[3U] = s3;
  tmp[4U] = s4;
}

inline static void Hacl_Bignum_Fsquare_fsquare_(FStar_UInt128_t *tmp, uint64_t *output)
{
  Hacl_Bignum_Fsquare_fsquare__(tmp, output);
  Hacl_Bignum_Fproduct_carry_wide_(tmp);
  FStar_UInt128_t b4 = tmp[4U];
  FStar_UInt128_t b0 = tmp[0U];
  FStar_UInt128_t
  b4_ = FStar_UInt128_logand(b4, FStar_UInt128_uint64_to_uint128((uint64_t)0x7ffffffffffffU));
  FStar_UInt128_t
  b0_ =
    FStar_UInt128_add(b0,
      FStar_UInt128_mul_wide((uint64_t)19U,
        FStar_UInt128_uint128_to_uint64(FStar_UInt128_shift_right(b4, (uint32_t)51U))));
  tmp[4U] = b4_;
  tmp[0U] = b0_;
  Hacl_Bignum_Fproduct_copy_from_wide_(output, tmp);
  uint64_t i0 = output[0U];
  uint64_t i1 = output[1U];
  uint64_t i0_ = i0 & (uint64_t)0x7ffffffffffffU;
  uint64_t i1_ = i1 + (i0 >> (uint32_t)51U);
  output[0U] = i0_;
  output[1U] = i1_;
}

static void
Hacl_Bignum_Fsquare_fsquare_times_(uint64_t *input, FStar_UInt128_t *tmp, uint32_t count1)
{
  Hacl_Bignum_Fsquare_fsquare_(tmp, input);
  for (uint32_t i = (uint32_t)1U; i < count1; i = i + (uint32_t)1U)
    Hacl_Bignum_Fsquare_fsquare_(tmp, input);
}

inline static void
Hacl_Bignum_Fsquare_fsquare_times(uint64_t *output, uint64_t *input, uint32_t count1)
{
  KRML_CHECK_SIZE(FStar_UInt128_uint64_to_uint128((uint64_t)0U), (uint32_t)5U);
  FStar_UInt128_t t[5U];
  for (uint32_t _i = 0U; _i < (uint32_t)5U; ++_i)
    t[_i] = FStar_UInt128_uint64_to_uint128((uint64_t)0U);
  memcpy(output, input, (uint32_t)5U * sizeof input[0U]);
  Hacl_Bignum_Fsquare_fsquare_times_(output, t, count1);
}

inline static void Hacl_Bignum_Fsquare_fsquare_times_inplace(uint64_t *output, uint32_t count1)
{
  KRML_CHECK_SIZE(FStar_UInt128_uint64_to_uint128((uint64_t)0U), (uint32_t)5U);
  FStar_UInt128_t t[5U];
  for (uint32_t _i = 0U; _i < (uint32_t)5U; ++_i)
    t[_i] = FStar_UInt128_uint64_to_uint128((uint64_t)0U);
  Hacl_Bignum_Fsquare_fsquare_times_(output, t, count1);
}

inline static void Hacl_Bignum_Crecip_crecip(uint64_t *out, uint64_t *z)
{
  uint64_t buf[20U] = { 0U };
  uint64_t *a = buf;
  uint64_t *t00 = buf + (uint32_t)5U;
  uint64_t *b0 = buf + (uint32_t)10U;
  Hacl_Bignum_Fsquare_fsquare_times(a, z, (uint32_t)1U);
  Hacl_Bignum_Fsquare_fsquare_times(t00, a, (uint32_t)2U);
  Hacl_Bignum_Fmul_fmul(b0, t00, z);
  Hacl_Bignum_Fmul_fmul(a, b0, a);
  Hacl_Bignum_Fsquare_fsquare_times(t00, a, (uint32_t)1U);
  Hacl_Bignum_Fmul_fmul(b0, t00, b0);
  Hacl_Bignum_Fsquare_fsquare_times(t00, b0, (uint32_t)5U);
  uint64_t *t01 = buf + (uint32_t)5U;
  uint64_t *b1 = buf + (uint32_t)10U;
  uint64_t *c0 = buf + (uint32_t)15U;
  Hacl_Bignum_Fmul_fmul(b1, t01, b1);
  Hacl_Bignum_Fsquare_fsquare_times(t01, b1, (uint32_t)10U);
  Hacl_Bignum_Fmul_fmul(c0, t01, b1);
  Hacl_Bignum_Fsquare_fsquare_times(t01, c0, (uint32_t)20U);
  Hacl_Bignum_Fmul_fmul(t01, t01, c0);
  Hacl_Bignum_Fsquare_fsquare_times_inplace(t01, (uint32_t)10U);
  Hacl_Bignum_Fmul_fmul(b1, t01, b1);
  Hacl_Bignum_Fsquare_fsquare_times(t01, b1, (uint32_t)50U);
  uint64_t *a0 = buf;
  uint64_t *t0 = buf + (uint32_t)5U;
  uint64_t *b = buf + (uint32_t)10U;
  uint64_t *c = buf + (uint32_t)15U;
  Hacl_Bignum_Fmul_fmul(c, t0, b);
  Hacl_Bignum_Fsquare_fsquare_times(t0, c, (uint32_t)100U);
  Hacl_Bignum_Fmul_fmul(t0, t0, c);
  Hacl_Bignum_Fsquare_fsquare_times_inplace(t0, (uint32_t)50U);
  Hacl_Bignum_Fmul_fmul(t0, t0, b);
  Hacl_Bignum_Fsquare_fsquare_times_inplace(t0, (uint32_t)5U);
  Hacl_Bignum_Fmul_fmul(out, t0, a0);
}

inline static void Hacl_Bignum_Crecip_crecip_(uint64_t *out, uint64_t *z)
{
  uint64_t buf[20U] = { 0U };
  uint64_t *a = buf;
  uint64_t *t00 = buf + (uint32_t)5U;
  uint64_t *b0 = buf + (uint32_t)10U;
  Hacl_Bignum_Fsquare_fsquare_times(a, z, (uint32_t)1U);
  Hacl_Bignum_Fsquare_fsquare_times(t00, a, (uint32_t)2U);
  Hacl_Bignum_Fmul_fmul(b0, t00, z);
  Hacl_Bignum_Fmul_fmul(a, b0, a);
  Hacl_Bignum_Fsquare_fsquare_times(t00, a, (uint32_t)1U);
  Hacl_Bignum_Fmul_fmul(b0, t00, b0);
  Hacl_Bignum_Fsquare_fsquare_times(t00, b0, (uint32_t)5U);
  uint64_t *t01 = buf + (uint32_t)5U;
  uint64_t *b1 = buf + (uint32_t)10U;
  uint64_t *c0 = buf + (uint32_t)15U;
  Hacl_Bignum_Fmul_fmul(b1, t01, b1);
  Hacl_Bignum_Fsquare_fsquare_times(t01, b1, (uint32_t)10U);
  Hacl_Bignum_Fmul_fmul(c0, t01, b1);
  Hacl_Bignum_Fsquare_fsquare_times(t01, c0, (uint32_t)20U);
  Hacl_Bignum_Fmul_fmul(t01, t01, c0);
  Hacl_Bignum_Fsquare_fsquare_times_inplace(t01, (uint32_t)10U);
  Hacl_Bignum_Fmul_fmul(b1, t01, b1);
  Hacl_Bignum_Fsquare_fsquare_times(t01, b1, (uint32_t)50U);
  uint64_t *a0 = buf;
  Hacl_Bignum_Fsquare_fsquare_times(a0, z, (uint32_t)1U);
  uint64_t *a1 = buf;
  uint64_t *t0 = buf + (uint32_t)5U;
  uint64_t *b = buf + (uint32_t)10U;
  uint64_t *c = buf + (uint32_t)15U;
  Hacl_Bignum_Fmul_fmul(c, t0, b);
  Hacl_Bignum_Fsquare_fsquare_times(t0, c, (uint32_t)100U);
  Hacl_Bignum_Fmul_fmul(t0, t0, c);
  Hacl_Bignum_Fsquare_fsquare_times_inplace(t0, (uint32_t)50U);
  Hacl_Bignum_Fmul_fmul(t0, t0, b);
  Hacl_Bignum_Fsquare_fsquare_times_inplace(t0, (uint32_t)2U);
  Hacl_Bignum_Fmul_fmul(out, t0, a1);
}

inline static void Hacl_Bignum_fsum(uint64_t *a, uint64_t *b)
{
  {
    uint64_t xi = a[0U];
    uint64_t yi = b[0U];
    a[0U] = xi + yi;
  }
  {
    uint64_t xi = a[1U];
    uint64_t yi = b[1U];
    a[1U] = xi + yi;
  }
  {
    uint64_t xi = a[2U];
    uint64_t yi = b[2U];
    a[2U] = xi + yi;
  }
  {
    uint64_t xi = a[3U];
    uint64_t yi = b[3U];
    a[3U] = xi + yi;
  }
  {
    uint64_t xi = a[4U];
    uint64_t yi = b[4U];
    a[4U] = xi + yi;
  }
}

inline static void Hacl_Bignum_fdifference(uint64_t *a, uint64_t *b)
{
  uint64_t tmp[5U] = { 0U };
  memcpy(tmp, b, (uint32_t)5U * sizeof b[0U]);
  uint64_t b0 = tmp[0U];
  uint64_t b1 = tmp[1U];
  uint64_t b2 = tmp[2U];
  uint64_t b3 = tmp[3U];
  uint64_t b4 = tmp[4U];
  tmp[0U] = b0 + (uint64_t)0x3fffffffffff68U;
  tmp[1U] = b1 + (uint64_t)0x3ffffffffffff8U;
  tmp[2U] = b2 + (uint64_t)0x3ffffffffffff8U;
  tmp[3U] = b3 + (uint64_t)0x3ffffffffffff8U;
  tmp[4U] = b4 + (uint64_t)0x3ffffffffffff8U;
  {
    uint64_t xi = a[0U];
    uint64_t yi = tmp[0U];
    a[0U] = yi - xi;
  }
  {
    uint64_t xi = a[1U];
    uint64_t yi = tmp[1U];
    a[1U] = yi - xi;
  }
  {
    uint64_t xi = a[2U];
    uint64_t yi = tmp[2U];
    a[2U] = yi - xi;
  }
  {
    uint64_t xi = a[3U];
    uint64_t yi = tmp[3U];
    a[3U] = yi - xi;
  }
  {
    uint64_t xi = a[4U];
    uint64_t yi = tmp[4U];
    a[4U] = yi - xi;
  }
}

inline static void Hacl_Bignum_fmul(uint64_t *output, uint64_t *a, uint64_t *b)
{
  Hacl_Bignum_Fmul_fmul(output, a, b);
}

static void Hacl_EC_Format_fexpand(uint64_t *output, uint8_t *input)
{
  uint64_t i0 = load64_le(input);
  uint8_t *x00 = input + (uint32_t)6U;
  uint64_t i1 = load64_le(x00);
  uint8_t *x01 = input + (uint32_t)12U;
  uint64_t i2 = load64_le(x01);
  uint8_t *x02 = input + (uint32_t)19U;
  uint64_t i3 = load64_le(x02);
  uint8_t *x0 = input + (uint32_t)24U;
  uint64_t i4 = load64_le(x0);
  uint64_t output0 = i0 & (uint64_t)0x7ffffffffffffU;
  uint64_t output1 = i1 >> (uint32_t)3U & (uint64_t)0x7ffffffffffffU;
  uint64_t output2 = i2 >> (uint32_t)6U & (uint64_t)0x7ffffffffffffU;
  uint64_t output3 = i3 >> (uint32_t)1U & (uint64_t)0x7ffffffffffffU;
  uint64_t output4 = i4 >> (uint32_t)12U & (uint64_t)0x7ffffffffffffU;
  output[0U] = output0;
  output[1U] = output1;
  output[2U] = output2;
  output[3U] = output3;
  output[4U] = output4;
}

static void Hacl_EC_Format_fcontract_first_carry_pass(uint64_t *input)
{
  uint64_t t0 = input[0U];
  uint64_t t1 = input[1U];
  uint64_t t2 = input[2U];
  uint64_t t3 = input[3U];
  uint64_t t4 = input[4U];
  uint64_t t1_ = t1 + (t0 >> (uint32_t)51U);
  uint64_t t0_ = t0 & (uint64_t)0x7ffffffffffffU;
  uint64_t t2_ = t2 + (t1_ >> (uint32_t)51U);
  uint64_t t1__ = t1_ & (uint64_t)0x7ffffffffffffU;
  uint64_t t3_ = t3 + (t2_ >> (uint32_t)51U);
  uint64_t t2__ = t2_ & (uint64_t)0x7ffffffffffffU;
  uint64_t t4_ = t4 + (t3_ >> (uint32_t)51U);
  uint64_t t3__ = t3_ & (uint64_t)0x7ffffffffffffU;
  input[0U] = t0_;
  input[1U] = t1__;
  input[2U] = t2__;
  input[3U] = t3__;
  input[4U] = t4_;
}

static void Hacl_EC_Format_fcontract_first_carry_full(uint64_t *input)
{
  Hacl_EC_Format_fcontract_first_carry_pass(input);
  Hacl_Bignum_Modulo_carry_top(input);
}

static void Hacl_EC_Format_fcontract_second_carry_pass(uint64_t *input)
{
  uint64_t t0 = input[0U];
  uint64_t t1 = input[1U];
  uint64_t t2 = input[2U];
  uint64_t t3 = input[3U];
  uint64_t t4 = input[4U];
  uint64_t t1_ = t1 + (t0 >> (uint32_t)51U);
  uint64_t t0_ = t0 & (uint64_t)0x7ffffffffffffU;
  uint64_t t2_ = t2 + (t1_ >> (uint32_t)51U);
  uint64_t t1__ = t1_ & (uint64_t)0x7ffffffffffffU;
  uint64_t t3_ = t3 + (t2_ >> (uint32_t)51U);
  uint64_t t2__ = t2_ & (uint64_t)0x7ffffffffffffU;
  uint64_t t4_ = t4 + (t3_ >> (uint32_t)51U);
  uint64_t t3__ = t3_ & (uint64_t)0x7ffffffffffffU;
  input[0U] = t0_;
  input[1U] = t1__;
  input[2U] = t2__;
  input[3U] = t3__;
  input[4U] = t4_;
}

static void Hacl_EC_Format_fcontract_second_carry_full(uint64_t *input)
{
  Hacl_EC_Format_fcontract_second_carry_pass(input);
  Hacl_Bignum_Modulo_carry_top(input);
  uint64_t i0 = input[0U];
  uint64_t i1 = input[1U];
  uint64_t i0_ = i0 & (uint64_t)0x7ffffffffffffU;
  uint64_t i1_ = i1 + (i0 >> (uint32_t)51U);
  input[0U] = i0_;
  input[1U] = i1_;
}

static void Hacl_EC_Format_fcontract_trim(uint64_t *input)
{
  uint64_t a0 = input[0U];
  uint64_t a1 = input[1U];
  uint64_t a2 = input[2U];
  uint64_t a3 = input[3U];
  uint64_t a4 = input[4U];
  uint64_t mask0 = FStar_UInt64_gte_mask(a0, (uint64_t)0x7ffffffffffedU);
  uint64_t mask1 = FStar_UInt64_eq_mask(a1, (uint64_t)0x7ffffffffffffU);
  uint64_t mask2 = FStar_UInt64_eq_mask(a2, (uint64_t)0x7ffffffffffffU);
  uint64_t mask3 = FStar_UInt64_eq_mask(a3, (uint64_t)0x7ffffffffffffU);
  uint64_t mask4 = FStar_UInt64_eq_mask(a4, (uint64_t)0x7ffffffffffffU);
  uint64_t mask = (((mask0 & mask1) & mask2) & mask3) & mask4;
  uint64_t a0_ = a0 - ((uint64_t)0x7ffffffffffedU & mask);
  uint64_t a1_ = a1 - ((uint64_t)0x7ffffffffffffU & mask);
  uint64_t a2_ = a2 - ((uint64_t)0x7ffffffffffffU & mask);
  uint64_t a3_ = a3 - ((uint64_t)0x7ffffffffffffU & mask);
  uint64_t a4_ = a4 - ((uint64_t)0x7ffffffffffffU & mask);
  input[0U] = a0_;
  input[1U] = a1_;
  input[2U] = a2_;
  input[3U] = a3_;
  input[4U] = a4_;
}

static void Hacl_EC_Format_reduce(uint64_t *out)
{
  Hacl_EC_Format_fcontract_first_carry_full(out);
  Hacl_EC_Format_fcontract_second_carry_full(out);
  Hacl_EC_Format_fcontract_trim(out);
}

static void
Hacl_Lib_Create64_make_h64_5(
  uint64_t *b,
  uint64_t s0,
  uint64_t s1,
  uint64_t s2,
  uint64_t s3,
  uint64_t s4
)
{
  b[0U] = s0;
  b[1U] = s1;
  b[2U] = s2;
  b[3U] = s3;
  b[4U] = s4;
}

static void
Hacl_Lib_Create64_make_h64_10(
  uint64_t *b,
  uint64_t s0,
  uint64_t s1,
  uint64_t s2,
  uint64_t s3,
  uint64_t s4,
  uint64_t s5,
  uint64_t s6,
  uint64_t s7,
  uint64_t s8,
  uint64_t s9
)
{
  b[0U] = s0;
  b[1U] = s1;
  b[2U] = s2;
  b[3U] = s3;
  b[4U] = s4;
  b[5U] = s5;
  b[6U] = s6;
  b[7U] = s7;
  b[8U] = s8;
  b[9U] = s9;
}

static void Hacl_Bignum25519_fsum(uint64_t *a, uint64_t *b)
{
  Hacl_Bignum_fsum(a, b);
}

static void Hacl_Bignum25519_fdifference(uint64_t *a, uint64_t *b)
{
  Hacl_Bignum_fdifference(a, b);
}

static void Hacl_Bignum25519_reduce_513(uint64_t *a)
{
  uint64_t t0 = a[0U];
  uint64_t t1 = a[1U];
  uint64_t t2 = a[2U];
  uint64_t t3 = a[3U];
  uint64_t t4 = a[4U];
  uint64_t t1_ = t1 + (t0 >> (uint32_t)51U);
  uint64_t t0_ = t0 & (uint64_t)0x7ffffffffffffU;
  uint64_t t2_ = t2 + (t1_ >> (uint32_t)51U);
  uint64_t t1__ = t1_ & (uint64_t)0x7ffffffffffffU;
  uint64_t t3_ = t3 + (t2_ >> (uint32_t)51U);
  uint64_t t2__ = t2_ & (uint64_t)0x7ffffffffffffU;
  uint64_t t4_ = t4 + (t3_ >> (uint32_t)51U);
  uint64_t t3__ = t3_ & (uint64_t)0x7ffffffffffffU;
  Hacl_Lib_Create64_make_h64_5(a, t0_, t1__, t2__, t3__, t4_);
  Hacl_Bignum_Modulo_carry_top(a);
  uint64_t i0 = a[0U];
  uint64_t i1 = a[1U];
  uint64_t i0_ = i0 & (uint64_t)0x7ffffffffffffU;
  uint64_t i1_ = i1 + (i0 >> (uint32_t)51U);
  a[0U] = i0_;
  a[1U] = i1_;
}

static void Hacl_Bignum25519_fdifference_reduced(uint64_t *a, uint64_t *b)
{
  Hacl_Bignum25519_fdifference(a, b);
  Hacl_Bignum25519_reduce_513(a);
}

static void Hacl_Bignum25519_fmul(uint64_t *out, uint64_t *a, uint64_t *b)
{
  Hacl_Bignum_fmul(out, a, b);
}

static void Hacl_Bignum25519_times_2(uint64_t *out, uint64_t *a)
{
  uint64_t a0 = a[0U];
  uint64_t a1 = a[1U];
  uint64_t a2 = a[2U];
  uint64_t a3 = a[3U];
  uint64_t a4 = a[4U];
  uint64_t o0 = (uint64_t)2U * a0;
  uint64_t o1 = (uint64_t)2U * a1;
  uint64_t o2 = (uint64_t)2U * a2;
  uint64_t o3 = (uint64_t)2U * a3;
  uint64_t o4 = (uint64_t)2U * a4;
  Hacl_Lib_Create64_make_h64_5(out, o0, o1, o2, o3, o4);
}

static void Hacl_Bignum25519_times_d(uint64_t *out, uint64_t *a)
{
  uint64_t d1[5U] = { 0U };
  Hacl_Lib_Create64_make_h64_5(d1,
    (uint64_t)0x00034dca135978a3U,
    (uint64_t)0x0001a8283b156ebdU,
    (uint64_t)0x0005e7a26001c029U,
    (uint64_t)0x000739c663a03cbbU,
    (uint64_t)0x00052036cee2b6ffU);
  Hacl_Bignum25519_fmul(out, d1, a);
}

static void Hacl_Bignum25519_times_2d(uint64_t *out, uint64_t *a)
{
  uint64_t d2[5U] = { 0U };
  Hacl_Lib_Create64_make_h64_5(d2,
    (uint64_t)0x00069b9426b2f159U,
    (uint64_t)0x00035050762add7aU,
    (uint64_t)0x0003cf44c0038052U,
    (uint64_t)0x0006738cc7407977U,
    (uint64_t)0x0002406d9dc56dffU);
  Hacl_Bignum25519_fmul(out, a, d2);
}

static void Hacl_Bignum25519_fsquare(uint64_t *out, uint64_t *a)
{
  KRML_CHECK_SIZE(FStar_UInt128_uint64_to_uint128((uint64_t)0U), (uint32_t)5U);
  FStar_UInt128_t tmp[5U];
  for (uint32_t _i = 0U; _i < (uint32_t)5U; ++_i)
    tmp[_i] = FStar_UInt128_uint64_to_uint128((uint64_t)0U);
  memcpy(out, a, (uint32_t)5U * sizeof a[0U]);
  Hacl_Bignum_Fsquare_fsquare_(tmp, out);
}

static void Hacl_Bignum25519_inverse(uint64_t *out, uint64_t *a)
{
  Hacl_Bignum_Crecip_crecip(out, a);
}

static void Hacl_Bignum25519_reduce(uint64_t *out)
{
  Hacl_EC_Format_reduce(out);
}

static uint64_t *Hacl_Impl_Ed25519_ExtPoint_getx(uint64_t *p)
{
  return p;
}

static uint64_t *Hacl_Impl_Ed25519_ExtPoint_gety(uint64_t *p)
{
  return p + (uint32_t)5U;
}

static uint64_t *Hacl_Impl_Ed25519_ExtPoint_getz(uint64_t *p)
{
  return p + (uint32_t)10U;
}

static uint64_t *Hacl_Impl_Ed25519_ExtPoint_gett(uint64_t *p)
{
  return p + (uint32_t)15U;
}

static void Hacl_Impl_Ed25519_G_make_g(uint64_t *g1)
{
  uint64_t *gx = Hacl_Impl_Ed25519_ExtPoint_getx(g1);
  uint64_t *gy = Hacl_Impl_Ed25519_ExtPoint_gety(g1);
  uint64_t *gz = Hacl_Impl_Ed25519_ExtPoint_getz(g1);
  uint64_t *gt1 = Hacl_Impl_Ed25519_ExtPoint_gett(g1);
  Hacl_Lib_Create64_make_h64_5(gx,
    (uint64_t)0x00062d608f25d51aU,
    (uint64_t)0x000412a4b4f6592aU,
    (uint64_t)0x00075b7171a4b31dU,
    (uint64_t)0x0001ff60527118feU,
    (uint64_t)0x000216936d3cd6e5U);
  Hacl_Lib_Create64_make_h64_5(gy,
    (uint64_t)0x0006666666666658U,
    (uint64_t)0x0004ccccccccccccU,
    (uint64_t)0x0001999999999999U,
    (uint64_t)0x0003333333333333U,
    (uint64_t)0x0006666666666666U);
  Hacl_Lib_Create64_make_h64_5(gz,
    (uint64_t)0x0000000000000001U,
    (uint64_t)0x0000000000000000U,
    (uint64_t)0x0000000000000000U,
    (uint64_t)0x0000000000000000U,
    (uint64_t)0x0000000000000000U);
  Hacl_Lib_Create64_make_h64_5(gt1,
    (uint64_t)0x00068ab3a5b7dda3U,
    (uint64_t)0x00000eea2a5eadbbU,
    (uint64_t)0x0002af8df483c27eU,
    (uint64_t)0x000332b375274732U,
    (uint64_t)0x00067875f0fd78b7U);
}

static void Hacl_Impl_Store51_store_51_(uint8_t *output, uint64_t *input)
{
  uint64_t t0 = input[0U];
  uint64_t t1 = input[1U];
  uint64_t t2 = input[2U];
  uint64_t t3 = input[3U];
  uint64_t t4 = input[4U];
  uint64_t o0 = t1 << (uint32_t)51U | t0;
  uint64_t o1 = t2 << (uint32_t)38U | t1 >> (uint32_t)13U;
  uint64_t o2 = t3 << (uint32_t)25U | t2 >> (uint32_t)26U;
  uint64_t o3 = t4 << (uint32_t)12U | t3 >> (uint32_t)39U;
  uint8_t *b0 = output;
  uint8_t *b1 = output + (uint32_t)8U;
  uint8_t *b2 = output + (uint32_t)16U;
  uint8_t *b3 = output + (uint32_t)24U;
  store64_le(b0, o0);
  store64_le(b1, o1);
  store64_le(b2, o2);
  store64_le(b3, o3);
}

static uint64_t Hacl_Impl_Ed25519_PointCompress_x_mod_2(uint64_t *x)
{
  uint64_t x0 = x[0U];
  return x0 & (uint64_t)1U;
}

static void Hacl_Impl_Ed25519_PointCompress_point_compress(uint8_t *z, uint64_t *p)
{
  uint64_t tmp[15U] = { 0U };
  uint64_t *x0 = tmp + (uint32_t)5U;
  uint64_t *out0 = tmp + (uint32_t)10U;
  uint64_t *zinv = tmp;
  uint64_t *x = tmp + (uint32_t)5U;
  uint64_t *out = tmp + (uint32_t)10U;
  uint64_t *px = Hacl_Impl_Ed25519_ExtPoint_getx(p);
  uint64_t *py = Hacl_Impl_Ed25519_ExtPoint_gety(p);
  uint64_t *pz = Hacl_Impl_Ed25519_ExtPoint_getz(p);
  Hacl_Bignum25519_inverse(zinv, pz);
  Hacl_Bignum25519_fmul(x, px, zinv);
  Hacl_Bignum25519_reduce(x);
  Hacl_Bignum25519_fmul(out, py, zinv);
  Hacl_Bignum25519_reduce(out);
  uint64_t b = Hacl_Impl_Ed25519_PointCompress_x_mod_2(x0);
  Hacl_Impl_Store51_store_51_(z, out0);
  uint8_t xbyte = (uint8_t)b;
  uint8_t o31 = z[31U];
  z[31U] = o31 + (xbyte << (uint32_t)7U);
}

static void
Hacl_Impl_Ed25519_SwapConditional_swap_conditional_step(
  uint64_t *a_,
  uint64_t *b_,
  uint64_t *a,
  uint64_t *b,
  uint64_t swap1
)
{
  uint64_t a0 = a[0U];
  uint64_t a1 = a[1U];
  uint64_t a2 = a[2U];
  uint64_t a3 = a[3U];
  uint64_t a4 = a[4U];
  uint64_t b0 = b[0U];
  uint64_t b1 = b[1U];
  uint64_t b2 = b[2U];
  uint64_t b3 = b[3U];
  uint64_t b4 = b[4U];
  uint64_t x0 = swap1 & (a0 ^ b0);
  uint64_t x1 = swap1 & (a1 ^ b1);
  uint64_t x2 = swap1 & (a2 ^ b2);
  uint64_t x3 = swap1 & (a3 ^ b3);
  uint64_t x4 = swap1 & (a4 ^ b4);
  uint64_t a0_ = a0 ^ x0;
  uint64_t b0_ = b0 ^ x0;
  uint64_t a1_ = a1 ^ x1;
  uint64_t b1_ = b1 ^ x1;
  uint64_t a2_ = a2 ^ x2;
  uint64_t b2_ = b2 ^ x2;
  uint64_t a3_ = a3 ^ x3;
  uint64_t b3_ = b3 ^ x3;
  uint64_t a4_ = a4 ^ x4;
  uint64_t b4_ = b4 ^ x4;
  Hacl_Lib_Create64_make_h64_5(a_, a0_, a1_, a2_, a3_, a4_);
  Hacl_Lib_Create64_make_h64_5(b_, b0_, b1_, b2_, b3_, b4_);
}

static void
Hacl_Impl_Ed25519_SwapConditional_swap_conditional(
  uint64_t *a_,
  uint64_t *b_,
  uint64_t *a,
  uint64_t *b,
  uint64_t iswap
)
{
  uint64_t swap1 = (uint64_t)0U - iswap;
  Hacl_Impl_Ed25519_SwapConditional_swap_conditional_step(Hacl_Impl_Ed25519_ExtPoint_getx(a_),
    Hacl_Impl_Ed25519_ExtPoint_getx(b_),
    Hacl_Impl_Ed25519_ExtPoint_getx(a),
    Hacl_Impl_Ed25519_ExtPoint_getx(b),
    swap1);
  Hacl_Impl_Ed25519_SwapConditional_swap_conditional_step(Hacl_Impl_Ed25519_ExtPoint_gety(a_),
    Hacl_Impl_Ed25519_ExtPoint_gety(b_),
    Hacl_Impl_Ed25519_ExtPoint_gety(a),
    Hacl_Impl_Ed25519_ExtPoint_gety(b),
    swap1);
  Hacl_Impl_Ed25519_SwapConditional_swap_conditional_step(Hacl_Impl_Ed25519_ExtPoint_getz(a_),
    Hacl_Impl_Ed25519_ExtPoint_getz(b_),
    Hacl_Impl_Ed25519_ExtPoint_getz(a),
    Hacl_Impl_Ed25519_ExtPoint_getz(b),
    swap1);
  Hacl_Impl_Ed25519_SwapConditional_swap_conditional_step(Hacl_Impl_Ed25519_ExtPoint_gett(a_),
    Hacl_Impl_Ed25519_ExtPoint_gett(b_),
    Hacl_Impl_Ed25519_ExtPoint_gett(a),
    Hacl_Impl_Ed25519_ExtPoint_gett(b),
    swap1);
}

static void
Hacl_Impl_Ed25519_SwapConditional_swap_conditional_inplace(
  uint64_t *a,
  uint64_t *b,
  uint64_t iswap
)
{
  uint64_t swap1 = (uint64_t)0U - iswap;
  Hacl_Impl_Ed25519_SwapConditional_swap_conditional_step(Hacl_Impl_Ed25519_ExtPoint_getx(a),
    Hacl_Impl_Ed25519_ExtPoint_getx(b),
    Hacl_Impl_Ed25519_ExtPoint_getx(a),
    Hacl_Impl_Ed25519_ExtPoint_getx(b),
    swap1);
  Hacl_Impl_Ed25519_SwapConditional_swap_conditional_step(Hacl_Impl_Ed25519_ExtPoint_gety(a),
    Hacl_Impl_Ed25519_ExtPoint_gety(b),
    Hacl_Impl_Ed25519_ExtPoint_gety(a),
    Hacl_Impl_Ed25519_ExtPoint_gety(b),
    swap1);
  Hacl_Impl_Ed25519_SwapConditional_swap_conditional_step(Hacl_Impl_Ed25519_ExtPoint_getz(a),
    Hacl_Impl_Ed25519_ExtPoint_getz(b),
    Hacl_Impl_Ed25519_ExtPoint_getz(a),
    Hacl_Impl_Ed25519_ExtPoint_getz(b),
    swap1);
  Hacl_Impl_Ed25519_SwapConditional_swap_conditional_step(Hacl_Impl_Ed25519_ExtPoint_gett(a),
    Hacl_Impl_Ed25519_ExtPoint_gett(b),
    Hacl_Impl_Ed25519_ExtPoint_gett(a),
    Hacl_Impl_Ed25519_ExtPoint_gett(b),
    swap1);
}

static void Hacl_Impl_Ed25519_SwapConditional_copy(uint64_t *output, uint64_t *input)
{
  memcpy(output, input, (uint32_t)20U * sizeof input[0U]);
}

static void Hacl_Impl_Ed25519_PointAdd_point_add(uint64_t *out, uint64_t *p, uint64_t *q1)
{
  uint64_t tmp[30U] = { 0U };
  uint64_t *tmp1 = tmp;
  uint64_t *tmp20 = tmp + (uint32_t)5U;
  uint64_t *tmp30 = tmp + (uint32_t)10U;
  uint64_t *tmp40 = tmp + (uint32_t)15U;
  uint64_t *x1 = Hacl_Impl_Ed25519_ExtPoint_getx(p);
  uint64_t *y1 = Hacl_Impl_Ed25519_ExtPoint_gety(p);
  uint64_t *x2 = Hacl_Impl_Ed25519_ExtPoint_getx(q1);
  uint64_t *y2 = Hacl_Impl_Ed25519_ExtPoint_gety(q1);
  memcpy(tmp1, x1, (uint32_t)5U * sizeof x1[0U]);
  memcpy(tmp20, x2, (uint32_t)5U * sizeof x2[0U]);
  Hacl_Bignum25519_fdifference_reduced(tmp1, y1);
  Hacl_Bignum25519_fdifference(tmp20, y2);
  Hacl_Bignum25519_fmul(tmp30, tmp1, tmp20);
  memcpy(tmp1, y1, (uint32_t)5U * sizeof y1[0U]);
  memcpy(tmp20, y2, (uint32_t)5U * sizeof y2[0U]);
  Hacl_Bignum25519_fsum(tmp1, x1);
  Hacl_Bignum25519_fsum(tmp20, x2);
  Hacl_Bignum25519_fmul(tmp40, tmp1, tmp20);
  uint64_t *tmp10 = tmp;
  uint64_t *tmp21 = tmp + (uint32_t)5U;
  uint64_t *tmp31 = tmp + (uint32_t)10U;
  uint64_t *tmp50 = tmp + (uint32_t)20U;
  uint64_t *tmp60 = tmp + (uint32_t)25U;
  uint64_t *z1 = Hacl_Impl_Ed25519_ExtPoint_getz(p);
  uint64_t *t1 = Hacl_Impl_Ed25519_ExtPoint_gett(p);
  uint64_t *z2 = Hacl_Impl_Ed25519_ExtPoint_getz(q1);
  uint64_t *t2 = Hacl_Impl_Ed25519_ExtPoint_gett(q1);
  Hacl_Bignum25519_times_2d(tmp10, t1);
  Hacl_Bignum25519_fmul(tmp21, tmp10, t2);
  Hacl_Bignum25519_times_2(tmp10, z1);
  Hacl_Bignum25519_fmul(tmp50, tmp10, z2);
  memcpy(tmp10, tmp31, (uint32_t)5U * sizeof tmp31[0U]);
  memcpy(tmp60, tmp21, (uint32_t)5U * sizeof tmp21[0U]);
  uint64_t *tmp11 = tmp;
  uint64_t *tmp2 = tmp + (uint32_t)5U;
  uint64_t *tmp3 = tmp + (uint32_t)10U;
  uint64_t *tmp41 = tmp + (uint32_t)15U;
  uint64_t *tmp51 = tmp + (uint32_t)20U;
  uint64_t *tmp61 = tmp + (uint32_t)25U;
  Hacl_Bignum25519_fdifference_reduced(tmp11, tmp41);
  Hacl_Bignum25519_fdifference(tmp61, tmp51);
  Hacl_Bignum25519_fsum(tmp51, tmp2);
  Hacl_Bignum25519_fsum(tmp41, tmp3);
  uint64_t *tmp12 = tmp;
  uint64_t *tmp4 = tmp + (uint32_t)15U;
  uint64_t *tmp5 = tmp + (uint32_t)20U;
  uint64_t *tmp6 = tmp + (uint32_t)25U;
  uint64_t *x3 = Hacl_Impl_Ed25519_ExtPoint_getx(out);
  uint64_t *y3 = Hacl_Impl_Ed25519_ExtPoint_gety(out);
  uint64_t *z3 = Hacl_Impl_Ed25519_ExtPoint_getz(out);
  uint64_t *t3 = Hacl_Impl_Ed25519_ExtPoint_gett(out);
  Hacl_Bignum25519_fmul(x3, tmp12, tmp6);
  Hacl_Bignum25519_fmul(y3, tmp5, tmp4);
  Hacl_Bignum25519_fmul(t3, tmp12, tmp4);
  Hacl_Bignum25519_fmul(z3, tmp5, tmp6);
}

static void Hacl_Impl_Ed25519_PointDouble_point_double_step_1(uint64_t *p, uint64_t *tmp)
{
  uint64_t *tmp1 = tmp;
  uint64_t *tmp2 = tmp + (uint32_t)5U;
  uint64_t *tmp3 = tmp + (uint32_t)10U;
  uint64_t *tmp4 = tmp + (uint32_t)15U;
  uint64_t *x1 = Hacl_Impl_Ed25519_ExtPoint_getx(p);
  uint64_t *y1 = Hacl_Impl_Ed25519_ExtPoint_gety(p);
  uint64_t *z1 = Hacl_Impl_Ed25519_ExtPoint_getz(p);
  Hacl_Bignum25519_fsquare(tmp1, x1);
  Hacl_Bignum25519_fsquare(tmp2, y1);
  Hacl_Bignum25519_fsquare(tmp3, z1);
  Hacl_Bignum25519_times_2(tmp4, tmp3);
  memcpy(tmp3, tmp1, (uint32_t)5U * sizeof tmp1[0U]);
  Hacl_Bignum25519_fsum(tmp3, tmp2);
  Hacl_Bignum25519_reduce_513(tmp3);
}

static void Hacl_Impl_Ed25519_PointDouble_point_double_step_2(uint64_t *p, uint64_t *tmp)
{
  uint64_t *tmp1 = tmp;
  uint64_t *tmp2 = tmp + (uint32_t)5U;
  uint64_t *tmp3 = tmp + (uint32_t)10U;
  uint64_t *tmp4 = tmp + (uint32_t)15U;
  uint64_t *tmp5 = tmp + (uint32_t)20U;
  uint64_t *tmp6 = tmp + (uint32_t)25U;
  uint64_t *x1 = Hacl_Impl_Ed25519_ExtPoint_getx(p);
  uint64_t *y1 = Hacl_Impl_Ed25519_ExtPoint_gety(p);
  memcpy(tmp5, x1, (uint32_t)5U * sizeof x1[0U]);
  Hacl_Bignum25519_fsum(tmp5, y1);
  Hacl_Bignum25519_fsquare(tmp6, tmp5);
  memcpy(tmp5, tmp3, (uint32_t)5U * sizeof tmp3[0U]);
  Hacl_Bignum25519_fdifference(tmp6, tmp5);
  Hacl_Bignum25519_fdifference_reduced(tmp2, tmp1);
  Hacl_Bignum25519_reduce_513(tmp4);
  Hacl_Bignum25519_fsum(tmp4, tmp2);
}

static void
Hacl_Impl_Ed25519_PointDouble_point_double_(uint64_t *out, uint64_t *p, uint64_t *tmp)
{
  uint64_t *tmp2 = tmp + (uint32_t)5U;
  uint64_t *tmp3 = tmp + (uint32_t)10U;
  uint64_t *tmp4 = tmp + (uint32_t)15U;
  uint64_t *tmp6 = tmp + (uint32_t)25U;
  uint64_t *x3 = Hacl_Impl_Ed25519_ExtPoint_getx(out);
  uint64_t *y3 = Hacl_Impl_Ed25519_ExtPoint_gety(out);
  uint64_t *z3 = Hacl_Impl_Ed25519_ExtPoint_getz(out);
  uint64_t *t3 = Hacl_Impl_Ed25519_ExtPoint_gett(out);
  Hacl_Impl_Ed25519_PointDouble_point_double_step_1(p, tmp);
  Hacl_Impl_Ed25519_PointDouble_point_double_step_2(p, tmp);
  Hacl_Bignum25519_fmul(x3, tmp4, tmp6);
  Hacl_Bignum25519_fmul(y3, tmp2, tmp3);
  Hacl_Bignum25519_fmul(t3, tmp3, tmp6);
  Hacl_Bignum25519_fmul(z3, tmp4, tmp2);
}

static void Hacl_Impl_Ed25519_PointDouble_point_double(uint64_t *out, uint64_t *p)
{
  uint64_t tmp[30U] = { 0U };
  Hacl_Impl_Ed25519_PointDouble_point_double_(out, p, tmp);
}

static uint8_t Hacl_Impl_Ed25519_Ladder_Step_ith_bit(uint8_t *k1, uint32_t i)
{
  uint32_t q1 = i >> (uint32_t)3U;
  uint32_t r = i & (uint32_t)7U;
  uint8_t kq = k1[q1];
  return kq >> r & (uint8_t)1U;
}

static void
Hacl_Impl_Ed25519_Ladder_Step_swap_cond_inplace(uint64_t *p, uint64_t *q1, uint64_t iswap)
{
  Hacl_Impl_Ed25519_SwapConditional_swap_conditional_inplace(p, q1, iswap);
}

static void
Hacl_Impl_Ed25519_Ladder_Step_swap_cond(
  uint64_t *p_,
  uint64_t *q_,
  uint64_t *p,
  uint64_t *q1,
  uint64_t iswap
)
{
  Hacl_Impl_Ed25519_SwapConditional_swap_conditional(p_, q_, p, q1, iswap);
}

static void
Hacl_Impl_Ed25519_Ladder_Step_loop_step_1(uint64_t *b, uint8_t *k1, uint32_t ctr, uint8_t i)
{
  uint64_t *nq = b;
  uint64_t *nqpq = b + (uint32_t)20U;
  uint64_t bit = (uint64_t)i;
  Hacl_Impl_Ed25519_Ladder_Step_swap_cond_inplace(nq, nqpq, bit);
}

static void Hacl_Impl_Ed25519_Ladder_Step_loop_step_2(uint64_t *b, uint8_t *k1, uint32_t ctr)
{
  uint64_t *nq = b;
  uint64_t *nqpq = b + (uint32_t)20U;
  uint64_t *nq2 = b + (uint32_t)40U;
  uint64_t *nqpq2 = b + (uint32_t)60U;
  Hacl_Impl_Ed25519_PointDouble_point_double(nq2, nq);
  Hacl_Impl_Ed25519_PointAdd_point_add(nqpq2, nq, nqpq);
}

static void
Hacl_Impl_Ed25519_Ladder_Step_loop_step_3(uint64_t *b, uint8_t *k1, uint32_t ctr, uint8_t i)
{
  uint64_t *nq = b;
  uint64_t *nqpq = b + (uint32_t)20U;
  uint64_t *nq2 = b + (uint32_t)40U;
  uint64_t *nqpq2 = b + (uint32_t)60U;
  uint64_t bit = (uint64_t)i;
  Hacl_Impl_Ed25519_Ladder_Step_swap_cond(nq, nqpq, nq2, nqpq2, bit);
}

static void Hacl_Impl_Ed25519_Ladder_Step_loop_step(uint64_t *b, uint8_t *k1, uint32_t ctr)
{
  uint8_t bit = Hacl_Impl_Ed25519_Ladder_Step_ith_bit(k1, ctr);
  Hacl_Impl_Ed25519_Ladder_Step_loop_step_1(b, k1, ctr, bit);
  Hacl_Impl_Ed25519_Ladder_Step_loop_step_2(b, k1, ctr);
  Hacl_Impl_Ed25519_Ladder_Step_loop_step_3(b, k1, ctr, bit);
}

static void Hacl_Impl_Ed25519_Ladder_point_mul_(uint64_t *b, uint8_t *k1)
{
  for (uint32_t i = (uint32_t)0U; i < (uint32_t)256U; i = i + (uint32_t)1U)
    Hacl_Impl_Ed25519_Ladder_Step_loop_step(b, k1, (uint32_t)256U - i - (uint32_t)1U);
}

static void Hacl_Impl_Ed25519_Ladder_make_point_inf(uint64_t *b)
{
  uint64_t *x = b;
  uint64_t *y = b + (uint32_t)5U;
  uint64_t *z = b + (uint32_t)10U;
  uint64_t *t = b + (uint32_t)15U;
  uint64_t zero1 = (uint64_t)0U;
  Hacl_Lib_Create64_make_h64_5(x, zero1, zero1, zero1, zero1, zero1);
  uint64_t zero10 = (uint64_t)0U;
  uint64_t one10 = (uint64_t)1U;
  Hacl_Lib_Create64_make_h64_5(y, one10, zero10, zero10, zero10, zero10);
  uint64_t zero11 = (uint64_t)0U;
  uint64_t one1 = (uint64_t)1U;
  Hacl_Lib_Create64_make_h64_5(z, one1, zero11, zero11, zero11, zero11);
  uint64_t zero12 = (uint64_t)0U;
  Hacl_Lib_Create64_make_h64_5(t, zero12, zero12, zero12, zero12, zero12);
}

static void Hacl_Impl_Ed25519_Ladder_point_mul(uint64_t *result, uint8_t *scalar, uint64_t *q1)
{
  uint64_t b[80U] = { 0U };
  uint64_t *nq = b;
  uint64_t *nqpq = b + (uint32_t)20U;
  Hacl_Impl_Ed25519_Ladder_make_point_inf(nq);
  Hacl_Impl_Ed25519_SwapConditional_copy(nqpq, q1);
  Hacl_Impl_Ed25519_Ladder_point_mul_(b, scalar);
  Hacl_Impl_Ed25519_SwapConditional_copy(result, nq);
}

static void
Hacl_Hash_Lib_LoadStore_uint64s_from_be_bytes(uint64_t *output, uint8_t *input, uint32_t len1)
{
  for (uint32_t i = (uint32_t)0U; i < len1; i = i + (uint32_t)1U)
  {
    uint8_t *x0 = input + (uint32_t)8U * i;
    uint64_t inputi = load64_be(x0);
    output[i] = inputi;
  }
}

static void
Hacl_Hash_Lib_LoadStore_uint64s_to_be_bytes(uint8_t *output, uint64_t *input, uint32_t len1)
{
  for (uint32_t i = (uint32_t)0U; i < len1; i = i + (uint32_t)1U)
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
    uint64_t d1 = hash_0[3U];
    uint64_t e = hash_0[4U];
    uint64_t f1 = hash_0[5U];
    uint64_t g1 = hash_0[6U];
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
      + ((e & f1) ^ (~e & g1))
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
    uint64_t x5 = d1 + t1;
    uint64_t *p1 = hash_0;
    uint64_t *p2 = hash_0 + (uint32_t)4U;
    p1[0U] = x1;
    p1[1U] = a;
    p1[2U] = b;
    p1[3U] = c;
    p2[0U] = x5;
    p2[1U] = e;
    p2[2U] = f1;
    p2[3U] = g1;
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

static void Hacl_Impl_SHA2_512_update_last(uint64_t *state, uint8_t *data, uint64_t len1)
{
  uint8_t blocks[256U] = { 0U };
  uint32_t nb;
  if (len1 < (uint64_t)112U)
    nb = (uint32_t)1U;
  else
    nb = (uint32_t)2U;
  uint8_t *final_blocks;
  if (len1 < (uint64_t)112U)
    final_blocks = blocks + (uint32_t)128U;
  else
    final_blocks = blocks;
  memcpy(final_blocks, data, (uint32_t)len1 * sizeof data[0U]);
  uint64_t n1 = state[168U];
  uint8_t *padding = final_blocks + (uint32_t)len1;
  FStar_UInt128_t
  encodedlen =
    FStar_UInt128_shift_left(FStar_UInt128_add(FStar_UInt128_mul_wide(n1, (uint64_t)(uint32_t)128U),
        FStar_UInt128_uint64_to_uint128(len1)),
      (uint32_t)3U);
  uint32_t
  pad0len = ((uint32_t)256U - ((uint32_t)len1 + (uint32_t)16U + (uint32_t)1U)) % (uint32_t)128U;
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

static void Hacl_Impl_SHA2_512_hash(uint8_t *hash1, uint8_t *input, uint32_t len1)
{
  KRML_CHECK_SIZE((uint64_t)(uint32_t)0U, (uint32_t)169U);
  uint64_t state[169U];
  for (uint32_t _i = 0U; _i < (uint32_t)169U; ++_i)
    state[_i] = (uint64_t)(uint32_t)0U;
  uint32_t n1 = len1 / (uint32_t)128U;
  uint32_t r = len1 % (uint32_t)128U;
  uint8_t *input_blocks = input;
  uint8_t *input_last = input + n1 * (uint32_t)128U;
  Hacl_Impl_SHA2_512_init(state);
  Hacl_Impl_SHA2_512_update_multi(state, input_blocks, n1);
  Hacl_Impl_SHA2_512_update_last(state, input_last, (uint64_t)r);
  Hacl_Impl_SHA2_512_finish(state, hash1);
}

static void Hacl_SHA2_512_hash(uint8_t *hash1, uint8_t *input, uint32_t len1)
{
  Hacl_Impl_SHA2_512_hash(hash1, input, len1);
}

static void Hacl_Impl_Ed25519_SecretExpand_secret_expand(uint8_t *expanded, uint8_t *secret)
{
  Hacl_SHA2_512_hash(expanded, secret, (uint32_t)32U);
  uint8_t *h_low = expanded;
  uint8_t h_low0 = h_low[0U];
  uint8_t h_low31 = h_low[31U];
  h_low[0U] = h_low0 & (uint8_t)0xf8U;
  h_low[31U] = (h_low31 & (uint8_t)127U) | (uint8_t)64U;
}

static void Hacl_Impl_Ed25519_SecretToPublic_point_mul_g(uint64_t *result, uint8_t *scalar)
{
  uint64_t g1[20U] = { 0U };
  Hacl_Impl_Ed25519_G_make_g(g1);
  Hacl_Impl_Ed25519_Ladder_point_mul(result, scalar, g1);
}

static void
Hacl_Impl_Ed25519_SecretToPublic_secret_to_public_(
  uint8_t *out,
  uint8_t *secret,
  uint8_t *expanded_secret
)
{
  uint8_t *a = expanded_secret;
  uint64_t res[20U] = { 0U };
  Hacl_Impl_Ed25519_SecretToPublic_point_mul_g(res, a);
  Hacl_Impl_Ed25519_PointCompress_point_compress(out, res);
}

static void Hacl_Impl_Ed25519_SecretToPublic_secret_to_public(uint8_t *out, uint8_t *secret)
{
  uint8_t expanded[64U] = { 0U };
  Hacl_Impl_Ed25519_SecretExpand_secret_expand(expanded, secret);
  Hacl_Impl_Ed25519_SecretToPublic_secret_to_public_(out, secret, expanded);
}

static bool Hacl_Impl_Ed25519_PointEqual_gte_q(uint64_t *s)
{
  uint64_t s0 = s[0U];
  uint64_t s1 = s[1U];
  uint64_t s2 = s[2U];
  uint64_t s3 = s[3U];
  uint64_t s4 = s[4U];
  if (s4 > (uint64_t)0x00000010000000U)
    return true;
  else if (s4 < (uint64_t)0x00000010000000U)
    return false;
  else if (s3 > (uint64_t)0x00000000000000U)
    return true;
  else if (s2 > (uint64_t)0x000000000014deU)
    return true;
  else if (s2 < (uint64_t)0x000000000014deU)
    return false;
  else if (s1 > (uint64_t)0xf9dea2f79cd658U)
    return true;
  else if (s1 < (uint64_t)0xf9dea2f79cd658U)
    return false;
  else if (s0 >= (uint64_t)0x12631a5cf5d3edU)
    return true;
  else
    return false;
}

static bool Hacl_Impl_Ed25519_PointEqual_eq(uint64_t *a, uint64_t *b)
{
  uint64_t a0 = a[0U];
  uint64_t a1 = a[1U];
  uint64_t a2 = a[2U];
  uint64_t a3 = a[3U];
  uint64_t a4 = a[4U];
  uint64_t b0 = b[0U];
  uint64_t b1 = b[1U];
  uint64_t b2 = b[2U];
  uint64_t b3 = b[3U];
  uint64_t b4 = b[4U];
  bool z = a0 == b0 && a1 == b1 && a2 == b2 && a3 == b3 && a4 == b4;
  return z;
}

static bool
Hacl_Impl_Ed25519_PointEqual_point_equal_1(uint64_t *p, uint64_t *q1, uint64_t *tmp)
{
  uint64_t *pxqz = tmp;
  uint64_t *qxpz = tmp + (uint32_t)5U;
  Hacl_Bignum25519_fmul(pxqz,
    Hacl_Impl_Ed25519_ExtPoint_getx(p),
    Hacl_Impl_Ed25519_ExtPoint_getz(q1));
  Hacl_Bignum25519_reduce(pxqz);
  Hacl_Bignum25519_fmul(qxpz,
    Hacl_Impl_Ed25519_ExtPoint_getx(q1),
    Hacl_Impl_Ed25519_ExtPoint_getz(p));
  Hacl_Bignum25519_reduce(qxpz);
  bool b = Hacl_Impl_Ed25519_PointEqual_eq(pxqz, qxpz);
  return b;
}

static bool
Hacl_Impl_Ed25519_PointEqual_point_equal_2(uint64_t *p, uint64_t *q1, uint64_t *tmp)
{
  uint64_t *pyqz = tmp + (uint32_t)10U;
  uint64_t *qypz = tmp + (uint32_t)15U;
  Hacl_Bignum25519_fmul(pyqz,
    Hacl_Impl_Ed25519_ExtPoint_gety(p),
    Hacl_Impl_Ed25519_ExtPoint_getz(q1));
  Hacl_Bignum25519_reduce(pyqz);
  Hacl_Bignum25519_fmul(qypz,
    Hacl_Impl_Ed25519_ExtPoint_gety(q1),
    Hacl_Impl_Ed25519_ExtPoint_getz(p));
  Hacl_Bignum25519_reduce(qypz);
  bool b = Hacl_Impl_Ed25519_PointEqual_eq(pyqz, qypz);
  return b;
}

static bool Hacl_Impl_Ed25519_PointEqual_point_equal_(uint64_t *p, uint64_t *q1, uint64_t *tmp)
{
  bool b = Hacl_Impl_Ed25519_PointEqual_point_equal_1(p, q1, tmp);
  if (b == true)
    return Hacl_Impl_Ed25519_PointEqual_point_equal_2(p, q1, tmp);
  else
    return false;
}

static bool Hacl_Impl_Ed25519_PointEqual_point_equal(uint64_t *p, uint64_t *q1)
{
  uint64_t tmp[20U] = { 0U };
  bool res = Hacl_Impl_Ed25519_PointEqual_point_equal_(p, q1, tmp);
  return res;
}

static void Hacl_Impl_Load56_load_64_bytes(uint64_t *out, uint8_t *b)
{
  uint8_t *b80 = b;
  uint64_t z = load64_le(b80);
  uint64_t z_ = z & (uint64_t)0xffffffffffffffU;
  uint64_t b0 = z_;
  uint8_t *b81 = b + (uint32_t)7U;
  uint64_t z0 = load64_le(b81);
  uint64_t z_0 = z0 & (uint64_t)0xffffffffffffffU;
  uint64_t b1 = z_0;
  uint8_t *b82 = b + (uint32_t)14U;
  uint64_t z1 = load64_le(b82);
  uint64_t z_1 = z1 & (uint64_t)0xffffffffffffffU;
  uint64_t b2 = z_1;
  uint8_t *b83 = b + (uint32_t)21U;
  uint64_t z2 = load64_le(b83);
  uint64_t z_2 = z2 & (uint64_t)0xffffffffffffffU;
  uint64_t b3 = z_2;
  uint8_t *b84 = b + (uint32_t)28U;
  uint64_t z3 = load64_le(b84);
  uint64_t z_3 = z3 & (uint64_t)0xffffffffffffffU;
  uint64_t b4 = z_3;
  uint8_t *b85 = b + (uint32_t)35U;
  uint64_t z4 = load64_le(b85);
  uint64_t z_4 = z4 & (uint64_t)0xffffffffffffffU;
  uint64_t b5 = z_4;
  uint8_t *b86 = b + (uint32_t)42U;
  uint64_t z5 = load64_le(b86);
  uint64_t z_5 = z5 & (uint64_t)0xffffffffffffffU;
  uint64_t b6 = z_5;
  uint8_t *b87 = b + (uint32_t)49U;
  uint64_t z6 = load64_le(b87);
  uint64_t z_6 = z6 & (uint64_t)0xffffffffffffffU;
  uint64_t b7 = z_6;
  uint8_t *b8 = b + (uint32_t)56U;
  uint64_t z7 = load64_le(b8);
  uint64_t z_7 = z7 & (uint64_t)0xffffffffffffffU;
  uint64_t b88 = z_7;
  uint8_t b63 = b[63U];
  uint64_t b9 = (uint64_t)b63;
  Hacl_Lib_Create64_make_h64_10(out, b0, b1, b2, b3, b4, b5, b6, b7, b88, b9);
}

static void Hacl_Impl_Load56_load_32_bytes(uint64_t *out, uint8_t *b)
{
  uint8_t *b80 = b;
  uint64_t z = load64_le(b80);
  uint64_t z_ = z & (uint64_t)0xffffffffffffffU;
  uint64_t b0 = z_;
  uint8_t *b81 = b + (uint32_t)7U;
  uint64_t z0 = load64_le(b81);
  uint64_t z_0 = z0 & (uint64_t)0xffffffffffffffU;
  uint64_t b1 = z_0;
  uint8_t *b82 = b + (uint32_t)14U;
  uint64_t z1 = load64_le(b82);
  uint64_t z_1 = z1 & (uint64_t)0xffffffffffffffU;
  uint64_t b2 = z_1;
  uint8_t *b8 = b + (uint32_t)21U;
  uint64_t z2 = load64_le(b8);
  uint64_t z_2 = z2 & (uint64_t)0xffffffffffffffU;
  uint64_t b3 = z_2;
  uint8_t *x0 = b + (uint32_t)28U;
  uint32_t b4 = load32_le(x0);
  uint64_t b41 = (uint64_t)b4;
  Hacl_Lib_Create64_make_h64_5(out, b0, b1, b2, b3, b41);
}

inline static void Hacl_Impl_Ed25519_Pow2_252m2_pow2_252m2(uint64_t *out, uint64_t *z)
{
  Hacl_Bignum_Crecip_crecip_(out, z);
}

static bool Hacl_Impl_Ed25519_RecoverX_is_0(uint64_t *x)
{
  uint64_t x0 = x[0U];
  uint64_t x1 = x[1U];
  uint64_t x2 = x[2U];
  uint64_t x3 = x[3U];
  uint64_t x4 = x[4U];
  return
    x0
    == (uint64_t)0U
    && x1 == (uint64_t)0U
    && x2 == (uint64_t)0U
    && x3 == (uint64_t)0U
    && x4 == (uint64_t)0U;
}

static void
Hacl_Impl_Ed25519_RecoverX_recover_x_step_5(uint64_t *x, uint64_t sign1, uint64_t *tmp)
{
  uint64_t *x3 = tmp + (uint32_t)5U;
  uint64_t *t0 = tmp + (uint32_t)10U;
  uint64_t x0 = x3[0U];
  uint64_t x00 = x0 & (uint64_t)1U;
  if (!(x00 == sign1))
  {
    uint64_t zero1 = (uint64_t)0U;
    Hacl_Lib_Create64_make_h64_5(t0, zero1, zero1, zero1, zero1, zero1);
    Hacl_Bignum25519_fdifference(x3, t0);
    Hacl_Bignum25519_reduce_513(x3);
    Hacl_Bignum25519_reduce(x3);
  }
  memcpy(x, x3, (uint32_t)5U * sizeof x3[0U]);
}

static bool
Hacl_Impl_Ed25519_RecoverX_recover_x_(uint64_t *x, uint64_t *y, uint64_t sign1, uint64_t *tmp)
{
  uint64_t *x20 = tmp;
  uint64_t x0 = y[0U];
  uint64_t x1 = y[1U];
  uint64_t x2 = y[2U];
  uint64_t x30 = y[3U];
  uint64_t x4 = y[4U];
  bool
  b =
    x0
    >= (uint64_t)0x7ffffffffffedU
    && x1 == (uint64_t)0x7ffffffffffffU
    && x2 == (uint64_t)0x7ffffffffffffU
    && x30 == (uint64_t)0x7ffffffffffffU
    && x4 == (uint64_t)0x7ffffffffffffU;
  if (b)
    return false;
  else
  {
    uint64_t tmp0[25U] = { 0U };
    uint64_t *one10 = tmp0;
    uint64_t *y2 = tmp0 + (uint32_t)5U;
    uint64_t *dyyi = tmp0 + (uint32_t)10U;
    uint64_t *dyy = tmp0 + (uint32_t)15U;
    uint64_t zero10 = (uint64_t)0U;
    uint64_t one1 = (uint64_t)1U;
    Hacl_Lib_Create64_make_h64_5(one10, one1, zero10, zero10, zero10, zero10);
    Hacl_Bignum25519_fsquare(y2, y);
    Hacl_Bignum25519_times_d(dyy, y2);
    Hacl_Bignum25519_fsum(dyy, one10);
    Hacl_Bignum25519_reduce_513(dyy);
    Hacl_Bignum25519_inverse(dyyi, dyy);
    Hacl_Bignum25519_fdifference(one10, y2);
    Hacl_Bignum25519_fmul(x20, dyyi, one10);
    Hacl_Bignum25519_reduce(x20);
    bool x2_is_0 = Hacl_Impl_Ed25519_RecoverX_is_0(x20);
    uint8_t z;
    if (x2_is_0)
    {
      uint8_t ite;
      if (sign1 == (uint64_t)0U)
      {
        uint64_t zero1 = (uint64_t)0U;
        Hacl_Lib_Create64_make_h64_5(x, zero1, zero1, zero1, zero1, zero1);
        ite = (uint8_t)1U;
      }
      else
        ite = (uint8_t)0U;
      z = ite;
    }
    else
      z = (uint8_t)2U;
    if (z == (uint8_t)0U)
      return false;
    else if (z == (uint8_t)1U)
      return true;
    else
    {
      uint64_t *x2 = tmp;
      uint64_t *x30 = tmp + (uint32_t)5U;
      uint64_t *t00 = tmp + (uint32_t)10U;
      uint64_t *t10 = tmp + (uint32_t)15U;
      Hacl_Impl_Ed25519_Pow2_252m2_pow2_252m2(x30, x2);
      Hacl_Bignum25519_fsquare(t00, x30);
      memcpy(t10, x2, (uint32_t)5U * sizeof x2[0U]);
      Hacl_Bignum25519_fdifference(t10, t00);
      Hacl_Bignum25519_reduce_513(t10);
      Hacl_Bignum25519_reduce(t10);
      bool t1_is_0 = Hacl_Impl_Ed25519_RecoverX_is_0(t10);
      if (!t1_is_0)
      {
        uint64_t sqrt_m1[5U] = { 0U };
        Hacl_Lib_Create64_make_h64_5(sqrt_m1,
          (uint64_t)0x00061b274a0ea0b0U,
          (uint64_t)0x0000d5a5fc8f189dU,
          (uint64_t)0x0007ef5e9cbd0c60U,
          (uint64_t)0x00078595a6804c9eU,
          (uint64_t)0x0002b8324804fc1dU);
        Hacl_Bignum25519_fmul(x30, x30, sqrt_m1);
      }
      Hacl_Bignum25519_reduce(x30);
      uint64_t *x20 = tmp;
      uint64_t *x3 = tmp + (uint32_t)5U;
      uint64_t *t0 = tmp + (uint32_t)10U;
      uint64_t *t1 = tmp + (uint32_t)15U;
      Hacl_Bignum25519_fsquare(t0, x3);
      memcpy(t1, x20, (uint32_t)5U * sizeof x20[0U]);
      Hacl_Bignum25519_fdifference(t1, t0);
      Hacl_Bignum25519_reduce_513(t1);
      Hacl_Bignum25519_reduce(t1);
      bool z1 = Hacl_Impl_Ed25519_RecoverX_is_0(t1);
      if (z1 == false)
        return false;
      else
      {
        Hacl_Impl_Ed25519_RecoverX_recover_x_step_5(x, sign1, tmp);
        return true;
      }
    }
  }
}

static bool Hacl_Impl_Ed25519_RecoverX_recover_x(uint64_t *x, uint64_t *y, uint64_t sign1)
{
  uint64_t tmp[20U] = { 0U };
  bool res = Hacl_Impl_Ed25519_RecoverX_recover_x_(x, y, sign1, tmp);
  return res;
}

static void Hacl_Impl_Load51_load_51(uint64_t *output, uint8_t *input)
{
  Hacl_EC_Format_fexpand(output, input);
}

static bool Hacl_Impl_Ed25519_PointDecompress_point_decompress(uint64_t *out, uint8_t *s)
{
  uint64_t tmp[10U] = { 0U };
  uint64_t *y0 = tmp;
  uint64_t *x0 = tmp + (uint32_t)5U;
  uint64_t *y = tmp;
  uint64_t *x = tmp + (uint32_t)5U;
  uint8_t s31 = s[31U];
  uint64_t sign1 = (uint64_t)(s31 >> (uint32_t)7U);
  Hacl_Impl_Load51_load_51(y, s);
  bool z = Hacl_Impl_Ed25519_RecoverX_recover_x(x, y, sign1);
  bool z0 = z;
  bool res;
  if (z0 == false)
    res = false;
  else
  {
    uint64_t *outx = Hacl_Impl_Ed25519_ExtPoint_getx(out);
    uint64_t *outy = Hacl_Impl_Ed25519_ExtPoint_gety(out);
    uint64_t *outz = Hacl_Impl_Ed25519_ExtPoint_getz(out);
    uint64_t *outt = Hacl_Impl_Ed25519_ExtPoint_gett(out);
    memcpy(outx, x0, (uint32_t)5U * sizeof x0[0U]);
    memcpy(outy, y0, (uint32_t)5U * sizeof y0[0U]);
    uint64_t zero1 = (uint64_t)0U;
    uint64_t one1 = (uint64_t)1U;
    Hacl_Lib_Create64_make_h64_5(outz, one1, zero1, zero1, zero1, zero1);
    Hacl_Bignum25519_fmul(outt, x0, y0);
    res = true;
  }
  return res;
}

static void Hacl_Impl_Store56_store_56(uint8_t *out, uint64_t *b)
{
  uint64_t b0 = b[0U];
  uint64_t b1 = b[1U];
  uint64_t b2 = b[2U];
  uint64_t b3 = b[3U];
  uint64_t b4 = b[4U];
  uint32_t b41 = (uint32_t)b4;
  uint8_t *b8 = out;
  store64_le(b8, b0);
  uint8_t *b80 = out + (uint32_t)7U;
  store64_le(b80, b1);
  uint8_t *b81 = out + (uint32_t)14U;
  store64_le(b81, b2);
  uint8_t *b82 = out + (uint32_t)21U;
  store64_le(b82, b3);
  uint8_t *x0 = out + (uint32_t)28U;
  store32_le(x0, b41);
}

static void
Hacl_Impl_SHA512_Ed25519_2_hash_block_and_rest(
  uint8_t *out,
  uint8_t *block,
  uint8_t *msg,
  uint32_t len1
)
{
  uint32_t nblocks = len1 >> (uint32_t)7U;
  uint64_t rest = (uint64_t)(len1 & (uint32_t)127U);
  uint64_t st[169U] = { 0U };
  Hacl_Impl_SHA2_512_init(st);
  Hacl_Impl_SHA2_512_update(st, block);
  Hacl_Impl_SHA2_512_update_multi(st, msg, nblocks);
  Hacl_Impl_SHA2_512_update_last(st, msg + (uint32_t)128U * nblocks, rest);
  Hacl_Impl_SHA2_512_finish(st, out);
}

static void
Hacl_Impl_SHA512_Ed25519_1_copy_bytes(uint8_t *output, uint8_t *input, uint32_t len1)
{
  memcpy(output, input, len1 * sizeof input[0U]);
}

static void
Hacl_Impl_SHA512_Ed25519_1_concat_2(uint8_t *out, uint8_t *pre, uint8_t *msg, uint32_t len1)
{
  Hacl_Impl_SHA512_Ed25519_1_copy_bytes(out, pre, (uint32_t)32U);
  Hacl_Impl_SHA512_Ed25519_1_copy_bytes(out + (uint32_t)32U, msg, len1);
}

static void
Hacl_Impl_SHA512_Ed25519_1_concat_3(
  uint8_t *out,
  uint8_t *pre,
  uint8_t *pre2,
  uint8_t *msg,
  uint32_t len1
)
{
  Hacl_Impl_SHA512_Ed25519_1_copy_bytes(out, pre, (uint32_t)32U);
  Hacl_Impl_SHA512_Ed25519_1_copy_bytes(out + (uint32_t)32U, pre2, (uint32_t)32U);
  Hacl_Impl_SHA512_Ed25519_1_copy_bytes(out + (uint32_t)64U, msg, len1);
}

static void
Hacl_Impl_SHA512_Ed25519_1_sha512_pre_msg_1(
  uint8_t *h,
  uint8_t *prefix,
  uint8_t *input,
  uint32_t len1
)
{
  uint8_t block[128U] = { 0U };
  uint8_t *block_ = block;
  Hacl_Impl_SHA512_Ed25519_1_concat_2(block_, prefix, input, len1);
  Hacl_Impl_SHA2_512_hash(h, block_, len1 + (uint32_t)32U);
}

static void
Hacl_Impl_SHA512_Ed25519_1_sha512_pre_pre2_msg_1(
  uint8_t *h,
  uint8_t *prefix,
  uint8_t *prefix2,
  uint8_t *input,
  uint32_t len1
)
{
  uint8_t block[128U] = { 0U };
  uint8_t *block_ = block;
  Hacl_Impl_SHA512_Ed25519_1_concat_3(block_, prefix, prefix2, input, len1);
  Hacl_Impl_SHA2_512_hash(h, block_, len1 + (uint32_t)64U);
}

static void
Hacl_Impl_SHA512_Ed25519_3_sha512_pre_msg_2(
  uint8_t *h,
  uint8_t *prefix,
  uint8_t *input,
  uint32_t len1
)
{
  uint8_t *input11 = input;
  uint8_t *input21 = input + (uint32_t)96U;
  uint8_t block[128U] = { 0U };
  Hacl_Impl_SHA512_Ed25519_1_concat_2(block, prefix, input11, (uint32_t)96U);
  Hacl_Impl_SHA512_Ed25519_2_hash_block_and_rest(h, block, input21, len1 - (uint32_t)96U);
}

static void
Hacl_Impl_SHA512_Ed25519_3_sha512_pre_msg(
  uint8_t *h,
  uint8_t *prefix,
  uint8_t *input,
  uint32_t len1
)
{
  if (len1 <= (uint32_t)96U)
    Hacl_Impl_SHA512_Ed25519_1_sha512_pre_msg_1(h, prefix, input, len1);
  else
    Hacl_Impl_SHA512_Ed25519_3_sha512_pre_msg_2(h, prefix, input, len1);
}

static void
Hacl_Impl_SHA512_Ed25519_3_sha512_pre_pre2_msg_2(
  uint8_t *h,
  uint8_t *prefix,
  uint8_t *prefix2,
  uint8_t *input,
  uint32_t len1
)
{
  uint8_t *input11 = input;
  uint8_t *input21 = input + (uint32_t)64U;
  uint8_t block[128U] = { 0U };
  Hacl_Impl_SHA512_Ed25519_1_concat_3(block, prefix, prefix2, input11, (uint32_t)64U);
  Hacl_Impl_SHA512_Ed25519_2_hash_block_and_rest(h, block, input21, len1 - (uint32_t)64U);
}

static void
Hacl_Impl_SHA512_Ed25519_3_sha512_pre_pre2_msg(
  uint8_t *h,
  uint8_t *prefix,
  uint8_t *prefix2,
  uint8_t *input,
  uint32_t len1
)
{
  if (len1 <= (uint32_t)64U)
    Hacl_Impl_SHA512_Ed25519_1_sha512_pre_pre2_msg_1(h, prefix, prefix2, input, len1);
  else
    Hacl_Impl_SHA512_Ed25519_3_sha512_pre_pre2_msg_2(h, prefix, prefix2, input, len1);
}

static void
Hacl_Impl_SHA512_Ed25519_sha512_pre_msg(
  uint8_t *h,
  uint8_t *prefix,
  uint8_t *input,
  uint32_t len1
)
{
  Hacl_Impl_SHA512_Ed25519_3_sha512_pre_msg(h, prefix, input, len1);
}

static void
Hacl_Impl_SHA512_Ed25519_sha512_pre_pre2_msg(
  uint8_t *h,
  uint8_t *prefix,
  uint8_t *prefix2,
  uint8_t *input,
  uint32_t len1
)
{
  Hacl_Impl_SHA512_Ed25519_3_sha512_pre_pre2_msg(h, prefix, prefix2, input, len1);
}

static void
Hacl_Impl_Sha512_sha512_pre_msg(uint8_t *h, uint8_t *prefix, uint8_t *input, uint32_t len1)
{
  Hacl_Impl_SHA512_Ed25519_sha512_pre_msg(h, prefix, input, len1);
}

static void
Hacl_Impl_Sha512_sha512_pre_pre2_msg(
  uint8_t *h,
  uint8_t *prefix,
  uint8_t *prefix2,
  uint8_t *input,
  uint32_t len1
)
{
  Hacl_Impl_SHA512_Ed25519_sha512_pre_pre2_msg(h, prefix, prefix2, input, len1);
}

static void
Hacl_Lib_Create128_make_h128_9(
  FStar_UInt128_t *b,
  FStar_UInt128_t s0,
  FStar_UInt128_t s1,
  FStar_UInt128_t s2,
  FStar_UInt128_t s3,
  FStar_UInt128_t s4,
  FStar_UInt128_t s5,
  FStar_UInt128_t s6,
  FStar_UInt128_t s7,
  FStar_UInt128_t s8
)
{
  b[0U] = s0;
  b[1U] = s1;
  b[2U] = s2;
  b[3U] = s3;
  b[4U] = s4;
  b[5U] = s5;
  b[6U] = s6;
  b[7U] = s7;
  b[8U] = s8;
}

static void Hacl_Impl_BignumQ_Mul_make_m(uint64_t *m1)
{
  Hacl_Lib_Create64_make_h64_5(m1,
    (uint64_t)0x12631a5cf5d3edU,
    (uint64_t)0xf9dea2f79cd658U,
    (uint64_t)0x000000000014deU,
    (uint64_t)0x00000000000000U,
    (uint64_t)0x00000010000000U);
}

static void Hacl_Impl_BignumQ_Mul_make_mu(uint64_t *m1)
{
  Hacl_Lib_Create64_make_h64_5(m1,
    (uint64_t)0x9ce5a30a2c131bU,
    (uint64_t)0x215d086329a7edU,
    (uint64_t)0xffffffffeb2106U,
    (uint64_t)0xffffffffffffffU,
    (uint64_t)0x00000fffffffffU);
}

static void Hacl_Impl_BignumQ_Mul_choose(uint64_t *z, uint64_t *x, uint64_t *y, uint64_t b)
{
  uint64_t mask = b - (uint64_t)1U;
  uint64_t x0 = x[0U];
  uint64_t x1 = x[1U];
  uint64_t x2 = x[2U];
  uint64_t x3 = x[3U];
  uint64_t x4 = x[4U];
  uint64_t y0 = y[0U];
  uint64_t y1 = y[1U];
  uint64_t y2 = y[2U];
  uint64_t y3 = y[3U];
  uint64_t y4 = y[4U];
  uint64_t z0 = ((y0 ^ x0) & mask) ^ x0;
  uint64_t z1 = ((y1 ^ x1) & mask) ^ x1;
  uint64_t z2 = ((y2 ^ x2) & mask) ^ x2;
  uint64_t z3 = ((y3 ^ x3) & mask) ^ x3;
  uint64_t z4 = ((y4 ^ x4) & mask) ^ x4;
  Hacl_Lib_Create64_make_h64_5(z, z0, z1, z2, z3, z4);
}

static uint64_t Hacl_Impl_BignumQ_Mul_lt(uint64_t a, uint64_t b)
{
  return (a - b) >> (uint32_t)63U;
}

static uint64_t Hacl_Impl_BignumQ_Mul_shiftl_56(uint64_t b)
{
  return b << (uint32_t)56U;
}

static void Hacl_Impl_BignumQ_Mul_sub_mod_264(uint64_t *z, uint64_t *x, uint64_t *y)
{
  uint64_t x0 = x[0U];
  uint64_t x1 = x[1U];
  uint64_t x2 = x[2U];
  uint64_t x3 = x[3U];
  uint64_t x4 = x[4U];
  uint64_t y0 = y[0U];
  uint64_t y1 = y[1U];
  uint64_t y2 = y[2U];
  uint64_t y3 = y[3U];
  uint64_t y4 = y[4U];
  uint64_t b = Hacl_Impl_BignumQ_Mul_lt(x0, y0);
  uint64_t t0 = Hacl_Impl_BignumQ_Mul_shiftl_56(b) + x0 - y0;
  uint64_t y11 = y1 + b;
  uint64_t b1 = Hacl_Impl_BignumQ_Mul_lt(x1, y11);
  uint64_t t1 = Hacl_Impl_BignumQ_Mul_shiftl_56(b1) + x1 - y11;
  uint64_t y21 = y2 + b1;
  uint64_t b2 = Hacl_Impl_BignumQ_Mul_lt(x2, y21);
  uint64_t t2 = Hacl_Impl_BignumQ_Mul_shiftl_56(b2) + x2 - y21;
  uint64_t y31 = y3 + b2;
  uint64_t b3 = Hacl_Impl_BignumQ_Mul_lt(x3, y31);
  uint64_t t3 = Hacl_Impl_BignumQ_Mul_shiftl_56(b3) + x3 - y31;
  uint64_t y41 = y4 + b3;
  uint64_t b4 = Hacl_Impl_BignumQ_Mul_lt(x4, y41);
  uint64_t t4 = (b4 << (uint32_t)40U) + x4 - y41;
  Hacl_Lib_Create64_make_h64_5(z, t0, t1, t2, t3, t4);
}

static void Hacl_Impl_BignumQ_Mul_subm_conditional(uint64_t *z, uint64_t *x)
{
  uint64_t tmp[5U] = { 0U };
  uint64_t x0 = x[0U];
  uint64_t x1 = x[1U];
  uint64_t x2 = x[2U];
  uint64_t x3 = x[3U];
  uint64_t x4 = x[4U];
  Hacl_Lib_Create64_make_h64_5(tmp, x0, x1, x2, x3, x4);
  uint64_t y0 = (uint64_t)0x12631a5cf5d3edU;
  uint64_t y1 = (uint64_t)0xf9dea2f79cd658U;
  uint64_t y2 = (uint64_t)0x000000000014deU;
  uint64_t y3 = (uint64_t)0x00000000000000U;
  uint64_t y4 = (uint64_t)0x00000010000000U;
  uint64_t b = Hacl_Impl_BignumQ_Mul_lt(x0, y0);
  uint64_t t0 = Hacl_Impl_BignumQ_Mul_shiftl_56(b) + x0 - y0;
  uint64_t y11 = y1 + b;
  uint64_t b1 = Hacl_Impl_BignumQ_Mul_lt(x1, y11);
  uint64_t t1 = Hacl_Impl_BignumQ_Mul_shiftl_56(b1) + x1 - y11;
  uint64_t y21 = y2 + b1;
  uint64_t b2 = Hacl_Impl_BignumQ_Mul_lt(x2, y21);
  uint64_t t2 = Hacl_Impl_BignumQ_Mul_shiftl_56(b2) + x2 - y21;
  uint64_t y31 = y3 + b2;
  uint64_t b3 = Hacl_Impl_BignumQ_Mul_lt(x3, y31);
  uint64_t t3 = Hacl_Impl_BignumQ_Mul_shiftl_56(b3) + x3 - y31;
  uint64_t y41 = y4 + b3;
  uint64_t b4 = Hacl_Impl_BignumQ_Mul_lt(x4, y41);
  uint64_t t4 = Hacl_Impl_BignumQ_Mul_shiftl_56(b4) + x4 - y41;
  Hacl_Lib_Create64_make_h64_5(z, t0, t1, t2, t3, t4);
  Hacl_Impl_BignumQ_Mul_choose(z, tmp, z, b4);
}

static void Hacl_Impl_BignumQ_Mul_low_mul_5(uint64_t *z, uint64_t *x, uint64_t *y)
{
  uint64_t x0 = x[0U];
  uint64_t x1 = x[1U];
  uint64_t x2 = x[2U];
  uint64_t x3 = x[3U];
  uint64_t x4 = x[4U];
  uint64_t y0 = y[0U];
  uint64_t y1 = y[1U];
  uint64_t y2 = y[2U];
  uint64_t y3 = y[3U];
  uint64_t y4 = y[4U];
  FStar_UInt128_t xy00 = FStar_UInt128_mul_wide(x0, y0);
  FStar_UInt128_t xy01 = FStar_UInt128_mul_wide(x0, y1);
  FStar_UInt128_t xy02 = FStar_UInt128_mul_wide(x0, y2);
  FStar_UInt128_t xy03 = FStar_UInt128_mul_wide(x0, y3);
  FStar_UInt128_t xy04 = FStar_UInt128_mul_wide(x0, y4);
  FStar_UInt128_t xy10 = FStar_UInt128_mul_wide(x1, y0);
  FStar_UInt128_t xy11 = FStar_UInt128_mul_wide(x1, y1);
  FStar_UInt128_t xy12 = FStar_UInt128_mul_wide(x1, y2);
  FStar_UInt128_t xy13 = FStar_UInt128_mul_wide(x1, y3);
  FStar_UInt128_t xy20 = FStar_UInt128_mul_wide(x2, y0);
  FStar_UInt128_t xy21 = FStar_UInt128_mul_wide(x2, y1);
  FStar_UInt128_t xy22 = FStar_UInt128_mul_wide(x2, y2);
  FStar_UInt128_t xy30 = FStar_UInt128_mul_wide(x3, y0);
  FStar_UInt128_t xy31 = FStar_UInt128_mul_wide(x3, y1);
  FStar_UInt128_t xy40 = FStar_UInt128_mul_wide(x4, y0);
  FStar_UInt128_t x5 = xy00;
  FStar_UInt128_t carry1 = FStar_UInt128_shift_right(x5, (uint32_t)56U);
  uint64_t t = FStar_UInt128_uint128_to_uint64(x5) & (uint64_t)0xffffffffffffffU;
  uint64_t t0 = t;
  FStar_UInt128_t x6 = FStar_UInt128_add(FStar_UInt128_add(xy01, xy10), carry1);
  FStar_UInt128_t carry2 = FStar_UInt128_shift_right(x6, (uint32_t)56U);
  uint64_t t1 = FStar_UInt128_uint128_to_uint64(x6) & (uint64_t)0xffffffffffffffU;
  uint64_t t11 = t1;
  FStar_UInt128_t
  x7 = FStar_UInt128_add(FStar_UInt128_add(FStar_UInt128_add(xy02, xy11), xy20), carry2);
  FStar_UInt128_t carry3 = FStar_UInt128_shift_right(x7, (uint32_t)56U);
  uint64_t t2 = FStar_UInt128_uint128_to_uint64(x7) & (uint64_t)0xffffffffffffffU;
  uint64_t t21 = t2;
  FStar_UInt128_t
  x8 =
    FStar_UInt128_add(FStar_UInt128_add(FStar_UInt128_add(FStar_UInt128_add(xy03, xy12), xy21),
        xy30),
      carry3);
  FStar_UInt128_t carry4 = FStar_UInt128_shift_right(x8, (uint32_t)56U);
  uint64_t t3 = FStar_UInt128_uint128_to_uint64(x8) & (uint64_t)0xffffffffffffffU;
  uint64_t t31 = t3;
  uint64_t
  t4 =
    FStar_UInt128_uint128_to_uint64(FStar_UInt128_add(FStar_UInt128_add(FStar_UInt128_add(FStar_UInt128_add(FStar_UInt128_add(xy04,
                xy13),
              xy22),
            xy31),
          xy40),
        carry4))
    & (uint64_t)0xffffffffffU;
  Hacl_Lib_Create64_make_h64_5(z, t0, t11, t21, t31, t4);
}

static void Hacl_Impl_BignumQ_Mul_mul_5(FStar_UInt128_t *z, uint64_t *x, uint64_t *y)
{
  uint64_t x0 = x[0U];
  uint64_t x1 = x[1U];
  uint64_t x2 = x[2U];
  uint64_t x3 = x[3U];
  uint64_t x4 = x[4U];
  uint64_t y0 = y[0U];
  uint64_t y1 = y[1U];
  uint64_t y2 = y[2U];
  uint64_t y3 = y[3U];
  uint64_t y4 = y[4U];
  FStar_UInt128_t xy00 = FStar_UInt128_mul_wide(x0, y0);
  FStar_UInt128_t xy01 = FStar_UInt128_mul_wide(x0, y1);
  FStar_UInt128_t xy02 = FStar_UInt128_mul_wide(x0, y2);
  FStar_UInt128_t xy03 = FStar_UInt128_mul_wide(x0, y3);
  FStar_UInt128_t xy04 = FStar_UInt128_mul_wide(x0, y4);
  FStar_UInt128_t xy10 = FStar_UInt128_mul_wide(x1, y0);
  FStar_UInt128_t xy11 = FStar_UInt128_mul_wide(x1, y1);
  FStar_UInt128_t xy12 = FStar_UInt128_mul_wide(x1, y2);
  FStar_UInt128_t xy13 = FStar_UInt128_mul_wide(x1, y3);
  FStar_UInt128_t xy14 = FStar_UInt128_mul_wide(x1, y4);
  FStar_UInt128_t xy20 = FStar_UInt128_mul_wide(x2, y0);
  FStar_UInt128_t xy21 = FStar_UInt128_mul_wide(x2, y1);
  FStar_UInt128_t xy22 = FStar_UInt128_mul_wide(x2, y2);
  FStar_UInt128_t xy23 = FStar_UInt128_mul_wide(x2, y3);
  FStar_UInt128_t xy24 = FStar_UInt128_mul_wide(x2, y4);
  FStar_UInt128_t xy30 = FStar_UInt128_mul_wide(x3, y0);
  FStar_UInt128_t xy31 = FStar_UInt128_mul_wide(x3, y1);
  FStar_UInt128_t xy32 = FStar_UInt128_mul_wide(x3, y2);
  FStar_UInt128_t xy33 = FStar_UInt128_mul_wide(x3, y3);
  FStar_UInt128_t xy34 = FStar_UInt128_mul_wide(x3, y4);
  FStar_UInt128_t xy40 = FStar_UInt128_mul_wide(x4, y0);
  FStar_UInt128_t xy41 = FStar_UInt128_mul_wide(x4, y1);
  FStar_UInt128_t xy42 = FStar_UInt128_mul_wide(x4, y2);
  FStar_UInt128_t xy43 = FStar_UInt128_mul_wide(x4, y3);
  FStar_UInt128_t xy44 = FStar_UInt128_mul_wide(x4, y4);
  FStar_UInt128_t z0 = xy00;
  FStar_UInt128_t z1 = FStar_UInt128_add(xy01, xy10);
  FStar_UInt128_t z2 = FStar_UInt128_add(FStar_UInt128_add(xy02, xy11), xy20);
  FStar_UInt128_t
  z3 = FStar_UInt128_add(FStar_UInt128_add(FStar_UInt128_add(xy03, xy12), xy21), xy30);
  FStar_UInt128_t
  z4 =
    FStar_UInt128_add(FStar_UInt128_add(FStar_UInt128_add(FStar_UInt128_add(xy04, xy13), xy22),
        xy31),
      xy40);
  FStar_UInt128_t
  z5 = FStar_UInt128_add(FStar_UInt128_add(FStar_UInt128_add(xy14, xy23), xy32), xy41);
  FStar_UInt128_t z6 = FStar_UInt128_add(FStar_UInt128_add(xy24, xy33), xy42);
  FStar_UInt128_t z7 = FStar_UInt128_add(xy34, xy43);
  FStar_UInt128_t z8 = xy44;
  Hacl_Lib_Create128_make_h128_9(z, z0, z1, z2, z3, z4, z5, z6, z7, z8);
}

static void Hacl_Impl_BignumQ_Mul_carry(uint64_t *out, FStar_UInt128_t *z)
{
  FStar_UInt128_t z0 = z[0U];
  FStar_UInt128_t z1 = z[1U];
  FStar_UInt128_t z2 = z[2U];
  FStar_UInt128_t z3 = z[3U];
  FStar_UInt128_t z4 = z[4U];
  FStar_UInt128_t z5 = z[5U];
  FStar_UInt128_t z6 = z[6U];
  FStar_UInt128_t z7 = z[7U];
  FStar_UInt128_t z8 = z[8U];
  FStar_UInt128_t x = z0;
  FStar_UInt128_t y = z1;
  FStar_UInt128_t carry1 = FStar_UInt128_shift_right(x, (uint32_t)56U);
  uint64_t t = FStar_UInt128_uint128_to_uint64(x) & (uint64_t)0xffffffffffffffU;
  uint64_t x0 = t;
  FStar_UInt128_t z1_ = FStar_UInt128_add(y, carry1);
  FStar_UInt128_t x1 = z1_;
  FStar_UInt128_t y1 = z2;
  FStar_UInt128_t carry2 = FStar_UInt128_shift_right(x1, (uint32_t)56U);
  uint64_t t1 = FStar_UInt128_uint128_to_uint64(x1) & (uint64_t)0xffffffffffffffU;
  uint64_t x11 = t1;
  FStar_UInt128_t z2_ = FStar_UInt128_add(y1, carry2);
  FStar_UInt128_t x2 = z2_;
  FStar_UInt128_t y2 = z3;
  FStar_UInt128_t carry3 = FStar_UInt128_shift_right(x2, (uint32_t)56U);
  uint64_t t2 = FStar_UInt128_uint128_to_uint64(x2) & (uint64_t)0xffffffffffffffU;
  uint64_t x21 = t2;
  FStar_UInt128_t z3_ = FStar_UInt128_add(y2, carry3);
  FStar_UInt128_t x3 = z3_;
  FStar_UInt128_t y3 = z4;
  FStar_UInt128_t carry4 = FStar_UInt128_shift_right(x3, (uint32_t)56U);
  uint64_t t3 = FStar_UInt128_uint128_to_uint64(x3) & (uint64_t)0xffffffffffffffU;
  uint64_t x31 = t3;
  FStar_UInt128_t z4_ = FStar_UInt128_add(y3, carry4);
  FStar_UInt128_t x4 = z4_;
  FStar_UInt128_t y4 = z5;
  FStar_UInt128_t carry5 = FStar_UInt128_shift_right(x4, (uint32_t)56U);
  uint64_t t4 = FStar_UInt128_uint128_to_uint64(x4) & (uint64_t)0xffffffffffffffU;
  uint64_t x41 = t4;
  FStar_UInt128_t z5_ = FStar_UInt128_add(y4, carry5);
  FStar_UInt128_t x5 = z5_;
  FStar_UInt128_t y5 = z6;
  FStar_UInt128_t carry6 = FStar_UInt128_shift_right(x5, (uint32_t)56U);
  uint64_t t5 = FStar_UInt128_uint128_to_uint64(x5) & (uint64_t)0xffffffffffffffU;
  uint64_t x51 = t5;
  FStar_UInt128_t z6_ = FStar_UInt128_add(y5, carry6);
  FStar_UInt128_t x6 = z6_;
  FStar_UInt128_t y6 = z7;
  FStar_UInt128_t carry7 = FStar_UInt128_shift_right(x6, (uint32_t)56U);
  uint64_t t6 = FStar_UInt128_uint128_to_uint64(x6) & (uint64_t)0xffffffffffffffU;
  uint64_t x61 = t6;
  FStar_UInt128_t z7_ = FStar_UInt128_add(y6, carry7);
  FStar_UInt128_t x7 = z7_;
  FStar_UInt128_t y7 = z8;
  FStar_UInt128_t carry8 = FStar_UInt128_shift_right(x7, (uint32_t)56U);
  uint64_t t7 = FStar_UInt128_uint128_to_uint64(x7) & (uint64_t)0xffffffffffffffU;
  uint64_t x71 = t7;
  FStar_UInt128_t z8_ = FStar_UInt128_add(y7, carry8);
  FStar_UInt128_t x8 = z8_;
  FStar_UInt128_t y8 = FStar_UInt128_uint64_to_uint128((uint64_t)0U);
  FStar_UInt128_t carry9 = FStar_UInt128_shift_right(x8, (uint32_t)56U);
  uint64_t t8 = FStar_UInt128_uint128_to_uint64(x8) & (uint64_t)0xffffffffffffffU;
  uint64_t x81 = t8;
  FStar_UInt128_t z9_ = FStar_UInt128_add(y8, carry9);
  uint64_t x9 = FStar_UInt128_uint128_to_uint64(z9_);
  Hacl_Lib_Create64_make_h64_10(out, x0, x11, x21, x31, x41, x51, x61, x71, x81, x9);
}

static void Hacl_Impl_BignumQ_Mul_mod_264(uint64_t *r, uint64_t *t)
{
  uint64_t x0 = t[0U];
  uint64_t x1 = t[1U];
  uint64_t x2 = t[2U];
  uint64_t x3 = t[3U];
  uint64_t x4 = t[4U];
  uint64_t x4_ = x4 & (uint64_t)0xffffffffffU;
  Hacl_Lib_Create64_make_h64_5(r, x0, x1, x2, x3, x4_);
}

static void Hacl_Impl_BignumQ_Mul_div_248(uint64_t *out, uint64_t *t)
{
  uint64_t x4 = t[4U];
  uint64_t x5 = t[5U];
  uint64_t x6 = t[6U];
  uint64_t x7 = t[7U];
  uint64_t x8 = t[8U];
  uint64_t x9 = t[9U];
  uint64_t z0 = (x5 & (uint64_t)0xffffffU) << (uint32_t)32U | x4 >> (uint32_t)24U;
  uint64_t z1 = (x6 & (uint64_t)0xffffffU) << (uint32_t)32U | x5 >> (uint32_t)24U;
  uint64_t z2 = (x7 & (uint64_t)0xffffffU) << (uint32_t)32U | x6 >> (uint32_t)24U;
  uint64_t z3 = (x8 & (uint64_t)0xffffffU) << (uint32_t)32U | x7 >> (uint32_t)24U;
  uint64_t z4 = (x9 & (uint64_t)0xffffffU) << (uint32_t)32U | x8 >> (uint32_t)24U;
  Hacl_Lib_Create64_make_h64_5(out, z0, z1, z2, z3, z4);
}

static void Hacl_Impl_BignumQ_Mul_div_264(uint64_t *out, uint64_t *t)
{
  uint64_t x4 = t[4U];
  uint64_t x5 = t[5U];
  uint64_t x6 = t[6U];
  uint64_t x7 = t[7U];
  uint64_t x8 = t[8U];
  uint64_t x9 = t[9U];
  uint64_t z0 = (x5 & (uint64_t)0xffffffffffU) << (uint32_t)16U | x4 >> (uint32_t)40U;
  uint64_t z1 = (x6 & (uint64_t)0xffffffffffU) << (uint32_t)16U | x5 >> (uint32_t)40U;
  uint64_t z2 = (x7 & (uint64_t)0xffffffffffU) << (uint32_t)16U | x6 >> (uint32_t)40U;
  uint64_t z3 = (x8 & (uint64_t)0xffffffffffU) << (uint32_t)16U | x7 >> (uint32_t)40U;
  uint64_t z4 = (x9 & (uint64_t)0xffffffffffU) << (uint32_t)16U | x8 >> (uint32_t)40U;
  Hacl_Lib_Create64_make_h64_5(out, z0, z1, z2, z3, z4);
}

static void
Hacl_Impl_BignumQ_Mul_barrett_reduction__1(
  FStar_UInt128_t *qmu,
  uint64_t *t,
  uint64_t *mu1,
  uint64_t *tmp
)
{
  uint64_t *q1 = tmp;
  uint64_t *qmu_ = tmp + (uint32_t)10U;
  uint64_t *qmu_264 = tmp + (uint32_t)20U;
  Hacl_Impl_BignumQ_Mul_div_248(q1, t);
  Hacl_Impl_BignumQ_Mul_mul_5(qmu, q1, mu1);
  Hacl_Impl_BignumQ_Mul_carry(qmu_, qmu);
  Hacl_Impl_BignumQ_Mul_div_264(qmu_264, qmu_);
}

static void
Hacl_Impl_BignumQ_Mul_barrett_reduction__2(uint64_t *t, uint64_t *m1, uint64_t *tmp)
{
  uint64_t *qmul = tmp;
  uint64_t *r = tmp + (uint32_t)5U;
  uint64_t *qmu_264 = tmp + (uint32_t)20U;
  uint64_t *s = tmp + (uint32_t)25U;
  Hacl_Impl_BignumQ_Mul_mod_264(r, t);
  Hacl_Impl_BignumQ_Mul_low_mul_5(qmul, qmu_264, m1);
  Hacl_Impl_BignumQ_Mul_sub_mod_264(s, r, qmul);
}

static void
Hacl_Impl_BignumQ_Mul_barrett_reduction__(
  uint64_t *z,
  uint64_t *t,
  uint64_t *m1,
  uint64_t *mu1,
  uint64_t *tmp
)
{
  uint64_t *s = tmp + (uint32_t)25U;
  KRML_CHECK_SIZE(FStar_UInt128_uint64_to_uint128((uint64_t)0U), (uint32_t)9U);
  FStar_UInt128_t qmu[9U];
  for (uint32_t _i = 0U; _i < (uint32_t)9U; ++_i)
    qmu[_i] = FStar_UInt128_uint64_to_uint128((uint64_t)0U);
  Hacl_Impl_BignumQ_Mul_barrett_reduction__1(qmu, t, mu1, tmp);
  Hacl_Impl_BignumQ_Mul_barrett_reduction__2(t, m1, tmp);
  Hacl_Impl_BignumQ_Mul_subm_conditional(z, s);
}

static void Hacl_Impl_BignumQ_Mul_barrett_reduction_(uint64_t *z, uint64_t *t)
{
  uint64_t tmp[40U] = { 0U };
  uint64_t *m1 = tmp;
  uint64_t *mu1 = tmp + (uint32_t)5U;
  uint64_t *tmp1 = tmp + (uint32_t)10U;
  Hacl_Impl_BignumQ_Mul_make_m(m1);
  Hacl_Impl_BignumQ_Mul_make_mu(mu1);
  Hacl_Impl_BignumQ_Mul_barrett_reduction__(z, t, m1, mu1, tmp1);
}

static void Hacl_Impl_BignumQ_Mul_barrett_reduction(uint64_t *z, uint64_t *t)
{
  Hacl_Impl_BignumQ_Mul_barrett_reduction_(z, t);
}

static void Hacl_Impl_BignumQ_Mul_mul_modq(uint64_t *out, uint64_t *x, uint64_t *y)
{
  uint64_t z_[10U] = { 0U };
  KRML_CHECK_SIZE(FStar_UInt128_uint64_to_uint128((uint64_t)0U), (uint32_t)9U);
  FStar_UInt128_t z[9U];
  for (uint32_t _i = 0U; _i < (uint32_t)9U; ++_i)
    z[_i] = FStar_UInt128_uint64_to_uint128((uint64_t)0U);
  Hacl_Impl_BignumQ_Mul_mul_5(z, x, y);
  Hacl_Impl_BignumQ_Mul_carry(z_, z);
  Hacl_Impl_BignumQ_Mul_barrett_reduction_(out, z_);
}

static void Hacl_Impl_BignumQ_Mul_add_modq_(uint64_t *out, uint64_t *x, uint64_t *y)
{
  uint64_t tmp[5U] = { 0U };
  uint64_t x0 = x[0U];
  uint64_t x1 = x[1U];
  uint64_t x2 = x[2U];
  uint64_t x3 = x[3U];
  uint64_t x4 = x[4U];
  uint64_t y0 = y[0U];
  uint64_t y1 = y[1U];
  uint64_t y2 = y[2U];
  uint64_t y3 = y[3U];
  uint64_t y4 = y[4U];
  uint64_t z0 = x0 + y0;
  uint64_t z1 = x1 + y1;
  uint64_t z2 = x2 + y2;
  uint64_t z3 = x3 + y3;
  uint64_t z4 = x4 + y4;
  uint64_t x5 = z0;
  uint64_t y5 = z1;
  uint64_t carry1 = x5 >> (uint32_t)56U;
  uint64_t t = x5 & (uint64_t)0xffffffffffffffU;
  uint64_t x01 = t;
  uint64_t z1_ = y5 + carry1;
  uint64_t x6 = z1_;
  uint64_t y6 = z2;
  uint64_t carry2 = x6 >> (uint32_t)56U;
  uint64_t t1 = x6 & (uint64_t)0xffffffffffffffU;
  uint64_t x11 = t1;
  uint64_t z2_ = y6 + carry2;
  uint64_t x7 = z2_;
  uint64_t y7 = z3;
  uint64_t carry3 = x7 >> (uint32_t)56U;
  uint64_t t2 = x7 & (uint64_t)0xffffffffffffffU;
  uint64_t x21 = t2;
  uint64_t z3_ = y7 + carry3;
  uint64_t x8 = z3_;
  uint64_t y8 = z4;
  uint64_t carry4 = x8 >> (uint32_t)56U;
  uint64_t t3 = x8 & (uint64_t)0xffffffffffffffU;
  uint64_t x31 = t3;
  uint64_t x41 = y8 + carry4;
  Hacl_Lib_Create64_make_h64_5(tmp, x01, x11, x21, x31, x41);
  Hacl_Impl_BignumQ_Mul_subm_conditional(out, tmp);
}

static void Hacl_Impl_BignumQ_Mul_add_modq(uint64_t *out, uint64_t *x, uint64_t *y)
{
  Hacl_Impl_BignumQ_Mul_add_modq_(out, x, y);
}

static void
Hacl_Impl_SHA512_ModQ_sha512_modq_pre_(
  uint64_t *out,
  uint8_t *prefix,
  uint8_t *input,
  uint32_t len1,
  uint64_t *tmp
)
{
  uint8_t hash1[64U] = { 0U };
  Hacl_Impl_Sha512_sha512_pre_msg(hash1, prefix, input, len1);
  Hacl_Impl_Load56_load_64_bytes(tmp, hash1);
  Hacl_Impl_BignumQ_Mul_barrett_reduction(out, tmp);
}

static void
Hacl_Impl_SHA512_ModQ_sha512_modq_pre(
  uint64_t *out,
  uint8_t *prefix,
  uint8_t *input,
  uint32_t len1
)
{
  uint64_t tmp[10U] = { 0U };
  Hacl_Impl_SHA512_ModQ_sha512_modq_pre_(out, prefix, input, len1, tmp);
}

static void
Hacl_Impl_SHA512_ModQ_sha512_modq_pre_pre2_(
  uint64_t *out,
  uint8_t *prefix,
  uint8_t *prefix2,
  uint8_t *input,
  uint32_t len1,
  uint64_t *tmp
)
{
  uint8_t hash1[64U] = { 0U };
  Hacl_Impl_Sha512_sha512_pre_pre2_msg(hash1, prefix, prefix2, input, len1);
  Hacl_Impl_Load56_load_64_bytes(tmp, hash1);
  Hacl_Impl_BignumQ_Mul_barrett_reduction(out, tmp);
}

static void
Hacl_Impl_SHA512_ModQ_sha512_modq_pre_pre2(
  uint64_t *out,
  uint8_t *prefix,
  uint8_t *prefix2,
  uint8_t *input,
  uint32_t len1
)
{
  uint64_t tmp[10U] = { 0U };
  Hacl_Impl_SHA512_ModQ_sha512_modq_pre_pre2_(out, prefix, prefix2, input, len1, tmp);
}

static bool Hacl_Impl_Ed25519_Verify_Steps_verify_step_1(uint64_t *r_, uint8_t *signature)
{
  uint8_t *rs = signature;
  bool b_ = Hacl_Impl_Ed25519_PointDecompress_point_decompress(r_, rs);
  return b_;
}

static void
Hacl_Impl_Ed25519_Verify_Steps_verify_step_2(
  uint8_t *r,
  uint8_t *msg,
  uint32_t len1,
  uint8_t *rs,
  uint8_t *public
)
{
  uint64_t r_[5U] = { 0U };
  Hacl_Impl_SHA512_ModQ_sha512_modq_pre_pre2(r_, rs, public, msg, len1);
  Hacl_Impl_Store56_store_56(r, r_);
}

static void Hacl_Impl_Ed25519_Verify_Steps_point_mul_g(uint64_t *result, uint8_t *scalar)
{
  uint64_t g1[20U] = { 0U };
  Hacl_Impl_Ed25519_G_make_g(g1);
  Hacl_Impl_Ed25519_Ladder_point_mul(result, scalar, g1);
}

static bool
Hacl_Impl_Ed25519_Verify_Steps_verify_step_4(
  uint8_t *s,
  uint8_t *h_,
  uint64_t *a_,
  uint64_t *r_
)
{
  uint64_t tmp[60U] = { 0U };
  uint64_t *hA = tmp;
  uint64_t *rhA = tmp + (uint32_t)20U;
  uint64_t *sB = tmp + (uint32_t)40U;
  Hacl_Impl_Ed25519_Verify_Steps_point_mul_g(sB, s);
  Hacl_Impl_Ed25519_Ladder_point_mul(hA, h_, a_);
  Hacl_Impl_Ed25519_PointAdd_point_add(rhA, r_, hA);
  bool b = Hacl_Impl_Ed25519_PointEqual_point_equal(sB, rhA);
  return b;
}

static bool
Hacl_Impl_Ed25519_Verify_verify__(
  uint8_t *public,
  uint8_t *msg,
  uint32_t len1,
  uint8_t *signature,
  uint64_t *tmp,
  uint8_t *tmp_
)
{
  uint64_t *a_ = tmp;
  uint64_t *r_ = tmp + (uint32_t)20U;
  uint64_t *s = tmp + (uint32_t)40U;
  uint8_t *h_ = tmp_;
  bool b = Hacl_Impl_Ed25519_PointDecompress_point_decompress(a_, public);
  if (b)
  {
    uint8_t *rs = signature;
    bool b_ = Hacl_Impl_Ed25519_Verify_Steps_verify_step_1(r_, signature);
    if (b_)
    {
      Hacl_Impl_Load56_load_32_bytes(s, signature + (uint32_t)32U);
      bool b__ = Hacl_Impl_Ed25519_PointEqual_gte_q(s);
      if (b__)
        return false;
      else
      {
        Hacl_Impl_Ed25519_Verify_Steps_verify_step_2(h_, msg, len1, rs, public);
        bool
        b1 = Hacl_Impl_Ed25519_Verify_Steps_verify_step_4(signature + (uint32_t)32U, h_, a_, r_);
        return b1;
      }
    }
    else
      return false;
  }
  else
    return false;
}

static bool
Hacl_Impl_Ed25519_Verify_verify_(
  uint8_t *public,
  uint8_t *msg,
  uint32_t len1,
  uint8_t *signature
)
{
  uint64_t tmp[45U] = { 0U };
  uint8_t tmp_[32U] = { 0U };
  bool res = Hacl_Impl_Ed25519_Verify_verify__(public, msg, len1, signature, tmp, tmp_);
  return res;
}

static bool
Hacl_Impl_Ed25519_Verify_verify(
  uint8_t *public,
  uint8_t *msg,
  uint32_t len1,
  uint8_t *signature
)
{
  return Hacl_Impl_Ed25519_Verify_verify_(public, msg, len1, signature);
}

static void Hacl_Impl_Ed25519_Sign_Steps_point_mul_g(uint64_t *result, uint8_t *scalar)
{
  uint64_t g1[20U] = { 0U };
  Hacl_Impl_Ed25519_G_make_g(g1);
  Hacl_Impl_Ed25519_Ladder_point_mul(result, scalar, g1);
}

static void Hacl_Impl_Ed25519_Sign_Steps_point_mul_g_compress(uint8_t *out, uint8_t *s)
{
  uint64_t tmp[20U] = { 0U };
  Hacl_Impl_Ed25519_Sign_Steps_point_mul_g(tmp, s);
  Hacl_Impl_Ed25519_PointCompress_point_compress(out, tmp);
}

static void
Hacl_Impl_Ed25519_Sign_Steps_copy_bytes(uint8_t *output, uint8_t *input, uint32_t len1)
{
  memcpy(output, input, len1 * sizeof input[0U]);
}

static void Hacl_Impl_Ed25519_Sign_Steps_sign_step_1(uint8_t *secret, uint8_t *tmp_bytes)
{
  uint8_t *a__ = tmp_bytes + (uint32_t)96U;
  uint8_t *apre = tmp_bytes + (uint32_t)224U;
  uint8_t *a = apre;
  Hacl_Impl_Ed25519_SecretExpand_secret_expand(apre, secret);
  Hacl_Impl_Ed25519_Sign_Steps_point_mul_g_compress(a__, a);
}

static void
Hacl_Impl_Ed25519_Sign_Steps_sign_step_2(
  uint8_t *msg,
  uint32_t len1,
  uint8_t *tmp_bytes,
  uint64_t *tmp_ints
)
{
  uint64_t *r = tmp_ints + (uint32_t)20U;
  uint8_t *apre = tmp_bytes + (uint32_t)224U;
  uint8_t *prefix = apre + (uint32_t)32U;
  Hacl_Impl_SHA512_ModQ_sha512_modq_pre(r, prefix, msg, len1);
}

static void
Hacl_Impl_Ed25519_Sign_Steps_sign_step_4(
  uint8_t *msg,
  uint32_t len1,
  uint8_t *tmp_bytes,
  uint64_t *tmp_ints
)
{
  uint64_t *h = tmp_ints + (uint32_t)60U;
  uint8_t *a__ = tmp_bytes + (uint32_t)96U;
  uint8_t *rs_ = tmp_bytes + (uint32_t)160U;
  Hacl_Impl_SHA512_ModQ_sha512_modq_pre_pre2(h, rs_, a__, msg, len1);
}

static void Hacl_Impl_Ed25519_Sign_Steps_sign_step_5(uint8_t *tmp_bytes, uint64_t *tmp_ints)
{
  uint64_t *r = tmp_ints + (uint32_t)20U;
  uint64_t *aq = tmp_ints + (uint32_t)45U;
  uint64_t *ha = tmp_ints + (uint32_t)50U;
  uint64_t *s = tmp_ints + (uint32_t)55U;
  uint64_t *h = tmp_ints + (uint32_t)60U;
  uint8_t *s_ = tmp_bytes + (uint32_t)192U;
  uint8_t *a = tmp_bytes + (uint32_t)224U;
  Hacl_Impl_Load56_load_32_bytes(aq, a);
  Hacl_Impl_BignumQ_Mul_mul_modq(ha, h, aq);
  Hacl_Impl_BignumQ_Mul_add_modq(s, r, ha);
  Hacl_Impl_Store56_store_56(s_, s);
}

static void Hacl_Impl_Ed25519_Sign_append_to_sig(uint8_t *signature, uint8_t *a, uint8_t *b)
{
  Hacl_Impl_Ed25519_Sign_Steps_copy_bytes(signature, a, (uint32_t)32U);
  Hacl_Impl_Ed25519_Sign_Steps_copy_bytes(signature + (uint32_t)32U, b, (uint32_t)32U);
}

static void
Hacl_Impl_Ed25519_Sign_sign_(uint8_t *signature, uint8_t *secret, uint8_t *msg, uint32_t len1)
{
  uint8_t tmp_bytes[352U] = { 0U };
  uint64_t tmp_ints[65U] = { 0U };
  uint8_t *rs_ = tmp_bytes + (uint32_t)160U;
  uint8_t *s_ = tmp_bytes + (uint32_t)192U;
  Hacl_Impl_Ed25519_Sign_Steps_sign_step_1(secret, tmp_bytes);
  Hacl_Impl_Ed25519_Sign_Steps_sign_step_2(msg, len1, tmp_bytes, tmp_ints);
  uint8_t rb[32U] = { 0U };
  uint64_t *r = tmp_ints + (uint32_t)20U;
  uint8_t *rs_0 = tmp_bytes + (uint32_t)160U;
  Hacl_Impl_Store56_store_56(rb, r);
  Hacl_Impl_Ed25519_Sign_Steps_point_mul_g_compress(rs_0, rb);
  Hacl_Impl_Ed25519_Sign_Steps_sign_step_4(msg, len1, tmp_bytes, tmp_ints);
  Hacl_Impl_Ed25519_Sign_Steps_sign_step_5(tmp_bytes, tmp_ints);
  Hacl_Impl_Ed25519_Sign_append_to_sig(signature, rs_, s_);
}

static void
Hacl_Impl_Ed25519_Sign_sign(uint8_t *signature, uint8_t *secret, uint8_t *msg, uint32_t len1)
{
  Hacl_Impl_Ed25519_Sign_sign_(signature, secret, msg, len1);
}

void Hacl_Ed25519_sign(uint8_t *signature, uint8_t *secret, uint8_t *msg, uint32_t len1)
{
  Hacl_Impl_Ed25519_Sign_sign(signature, secret, msg, len1);
}

bool Hacl_Ed25519_verify(uint8_t *pubkey, uint8_t *msg, uint32_t len1, uint8_t *signature)
{
  return Hacl_Impl_Ed25519_Verify_verify(pubkey, msg, len1, signature);
}

void Hacl_Ed25519_secret_to_public(uint8_t *out, uint8_t *secret)
{
  Hacl_Impl_Ed25519_SecretToPublic_secret_to_public(out, secret);
}

