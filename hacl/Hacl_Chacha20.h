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
#ifndef __Hacl_Chacha20_H
#define __Hacl_Chacha20_H





typedef uint32_t Hacl_Impl_Xor_Lemmas_u32;

typedef uint8_t Hacl_Impl_Xor_Lemmas_u8;

typedef uint8_t *Hacl_Lib_LoadStore32_uint8_p;

typedef uint32_t Hacl_Impl_Chacha20_u32;

typedef uint32_t Hacl_Impl_Chacha20_h32;

typedef uint8_t *Hacl_Impl_Chacha20_uint8_p;

typedef uint32_t *Hacl_Impl_Chacha20_state;

typedef uint32_t Hacl_Impl_Chacha20_idx;

typedef struct 
{
  void *k;
  void *n;
}
Hacl_Impl_Chacha20_log_t_;

typedef void *Hacl_Impl_Chacha20_log_t;

typedef uint32_t Hacl_Lib_Create_h32;

typedef uint8_t *Hacl_Chacha20_uint8_p;

typedef uint32_t Hacl_Chacha20_uint32_t;

void Hacl_Chacha20_chacha20_key_block(uint8_t *block, uint8_t *k, uint8_t *n1, uint32_t ctr);

/*
  This function implements Chacha20

  val chacha20 :
  output:uint8_p ->
  plain:uint8_p{ disjoint output plain } ->
  len:uint32_t{ v len = length output /\ v len = length plain } ->
  key:uint8_p{ length key = 32 } ->
  nonce:uint8_p{ length nonce = 12 } ->
  ctr:uint32_t{ v ctr + length plain / 64 < pow2 32 } ->
  Stack unit
    (requires
      fun h -> live h output /\ live h plain /\ live h nonce /\ live h key)
    (ensures
      fun h0 _ h1 ->
        live h1 output /\ live h0 plain /\ modifies_1 output h0 h1 /\
        live h0 nonce /\
        live h0 key /\
        h1.[ output ] ==
        chacha20_encrypt_bytes h0.[ key ] h0.[ nonce ] (v ctr) h0.[ plain ])
*/
void
Hacl_Chacha20_chacha20(
  uint8_t *output,
  uint8_t *plain,
  uint32_t len,
  uint8_t *k,
  uint8_t *n1,
  uint32_t ctr
);
#endif
