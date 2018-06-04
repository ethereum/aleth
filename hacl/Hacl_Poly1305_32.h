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
#ifndef __Hacl_Poly1305_32_H
#define __Hacl_Poly1305_32_H





typedef uint32_t Hacl_Bignum_Constants_limb;

typedef uint64_t Hacl_Bignum_Constants_wide;

typedef uint64_t Hacl_Bignum_Wide_t;

typedef uint32_t Hacl_Bignum_Limb_t;

typedef void *Hacl_Impl_Poly1305_32_State_log_t;

typedef uint8_t *Hacl_Impl_Poly1305_32_State_uint8_p;

typedef uint32_t *Hacl_Impl_Poly1305_32_State_bigint;

typedef void *Hacl_Impl_Poly1305_32_State_seqelem;

typedef uint32_t *Hacl_Impl_Poly1305_32_State_elemB;

typedef uint8_t *Hacl_Impl_Poly1305_32_State_wordB;

typedef uint8_t *Hacl_Impl_Poly1305_32_State_wordB_16;

typedef struct 
{
  uint32_t *r;
  uint32_t *h;
}
Hacl_Impl_Poly1305_32_State_poly1305_state;

typedef void *Hacl_Impl_Poly1305_32_log_t;

typedef uint32_t *Hacl_Impl_Poly1305_32_bigint;

typedef uint8_t *Hacl_Impl_Poly1305_32_uint8_p;

typedef uint32_t *Hacl_Impl_Poly1305_32_elemB;

typedef uint8_t *Hacl_Impl_Poly1305_32_wordB;

typedef uint8_t *Hacl_Impl_Poly1305_32_wordB_16;

typedef uint8_t *Hacl_Poly1305_32_uint8_p;

typedef uint64_t Hacl_Poly1305_32_uint64_t;

void *Hacl_Poly1305_32_op_String_Access(FStar_Monotonic_HyperStack_mem h, uint8_t *b);

typedef uint8_t *Hacl_Poly1305_32_key;

typedef Hacl_Impl_Poly1305_32_State_poly1305_state Hacl_Poly1305_32_state;

Hacl_Impl_Poly1305_32_State_poly1305_state
Hacl_Poly1305_32_mk_state(uint32_t *r, uint32_t *acc);

void Hacl_Poly1305_32_init(Hacl_Impl_Poly1305_32_State_poly1305_state st, uint8_t *k1);

extern void *Hacl_Poly1305_32_empty_log;

void Hacl_Poly1305_32_update_block(Hacl_Impl_Poly1305_32_State_poly1305_state st, uint8_t *m);

void
Hacl_Poly1305_32_update(
  Hacl_Impl_Poly1305_32_State_poly1305_state st,
  uint8_t *m,
  uint32_t len1
);

void
Hacl_Poly1305_32_update_last(
  Hacl_Impl_Poly1305_32_State_poly1305_state st,
  uint8_t *m,
  uint32_t len1
);

void
Hacl_Poly1305_32_finish(
  Hacl_Impl_Poly1305_32_State_poly1305_state st,
  uint8_t *mac,
  uint8_t *k1
);

void
Hacl_Poly1305_32_crypto_onetimeauth(
  uint8_t *output,
  uint8_t *input,
  uint64_t len1,
  uint8_t *k1
);
#endif
