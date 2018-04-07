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

#include "kremlib.h"
#ifndef __Hacl_Curve25519_H
#define __Hacl_Curve25519_H





typedef uint64_t Hacl_Bignum_Constants_limb;

typedef FStar_UInt128_t Hacl_Bignum_Constants_wide;

typedef uint64_t Hacl_Bignum_Parameters_limb;

typedef FStar_UInt128_t Hacl_Bignum_Parameters_wide;

typedef uint32_t Hacl_Bignum_Parameters_ctr;

typedef uint64_t *Hacl_Bignum_Parameters_felem;

typedef FStar_UInt128_t *Hacl_Bignum_Parameters_felem_wide;

typedef void *Hacl_Bignum_Parameters_seqelem;

typedef void *Hacl_Bignum_Parameters_seqelem_wide;

typedef FStar_UInt128_t Hacl_Bignum_Wide_t;

typedef uint64_t Hacl_Bignum_Limb_t;

extern void Hacl_Bignum_lemma_diff(Prims_int x0, Prims_int x1, Prims_pos x2);

typedef uint64_t *Hacl_EC_Point_point;

typedef uint8_t *Hacl_EC_Ladder_SmallLoop_uint8_p;

typedef uint8_t *Hacl_EC_Ladder_uint8_p;

typedef uint8_t *Hacl_EC_Format_uint8_p;

void Hacl_EC_crypto_scalarmult(uint8_t *mypublic, uint8_t *secret, uint8_t *basepoint);

typedef uint8_t *Hacl_Curve25519_uint8_p;

void Hacl_Curve25519_crypto_scalarmult(uint8_t *mypublic, uint8_t *secret, uint8_t *basepoint);
#endif
