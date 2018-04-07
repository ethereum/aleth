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
#ifndef __Hacl_Salsa20_H
#define __Hacl_Salsa20_H





typedef uint32_t Hacl_Impl_Xor_Lemmas_u32;

typedef uint8_t Hacl_Impl_Xor_Lemmas_u8;

typedef uint32_t Hacl_Lib_Create_h32;

typedef uint8_t *Hacl_Lib_LoadStore32_uint8_p;

typedef uint32_t Hacl_Impl_Salsa20_u32;

typedef uint32_t Hacl_Impl_Salsa20_h32;

typedef uint8_t *Hacl_Impl_Salsa20_uint8_p;

typedef uint32_t *Hacl_Impl_Salsa20_state;

typedef uint32_t Hacl_Impl_Salsa20_idx;

typedef struct 
{
  void *k;
  void *n;
}
Hacl_Impl_Salsa20_log_t_;

typedef void *Hacl_Impl_Salsa20_log_t;

typedef uint32_t Hacl_Impl_HSalsa20_h32;

typedef uint32_t Hacl_Impl_HSalsa20_u32;

typedef uint8_t *Hacl_Impl_HSalsa20_uint8_p;

typedef uint32_t *Hacl_Impl_HSalsa20_state;

typedef uint8_t *Hacl_Salsa20_uint8_p;

typedef uint32_t Hacl_Salsa20_uint32_t;

typedef uint32_t *Hacl_Salsa20_state;

void
Hacl_Salsa20_salsa20(
  uint8_t *output,
  uint8_t *plain,
  uint32_t len,
  uint8_t *k,
  uint8_t *n1,
  uint64_t ctr
);

void Hacl_Salsa20_hsalsa20(uint8_t *output, uint8_t *key, uint8_t *nonce);
#endif
