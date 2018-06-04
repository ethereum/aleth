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
#ifndef __Hacl_SHA2_512_H
#define __Hacl_SHA2_512_H





typedef uint8_t Hacl_Hash_Lib_Create_uint8_t;

typedef uint32_t Hacl_Hash_Lib_Create_uint32_t;

typedef uint64_t Hacl_Hash_Lib_Create_uint64_t;

typedef uint8_t Hacl_Hash_Lib_Create_uint8_ht;

typedef uint32_t Hacl_Hash_Lib_Create_uint32_ht;

typedef uint64_t Hacl_Hash_Lib_Create_uint64_ht;

typedef uint8_t *Hacl_Hash_Lib_Create_uint8_p;

typedef uint32_t *Hacl_Hash_Lib_Create_uint32_p;

typedef uint64_t *Hacl_Hash_Lib_Create_uint64_p;

typedef uint8_t *Hacl_Hash_Lib_LoadStore_uint8_p;

typedef uint8_t Hacl_Impl_SHA2_512_uint8_t;

typedef uint32_t Hacl_Impl_SHA2_512_uint32_t;

typedef uint64_t Hacl_Impl_SHA2_512_uint64_t;

typedef uint8_t Hacl_Impl_SHA2_512_uint8_ht;

typedef uint32_t Hacl_Impl_SHA2_512_uint32_ht;

typedef uint64_t Hacl_Impl_SHA2_512_uint64_ht;

typedef FStar_UInt128_t Hacl_Impl_SHA2_512_uint128_ht;

typedef uint64_t *Hacl_Impl_SHA2_512_uint64_p;

typedef uint8_t *Hacl_Impl_SHA2_512_uint8_p;

typedef uint8_t Hacl_SHA2_512_uint8_t;

typedef uint32_t Hacl_SHA2_512_uint32_t;

typedef uint64_t Hacl_SHA2_512_uint64_t;

typedef uint8_t Hacl_SHA2_512_uint8_ht;

typedef uint32_t Hacl_SHA2_512_uint32_ht;

typedef uint64_t Hacl_SHA2_512_uint64_ht;

typedef FStar_UInt128_t Hacl_SHA2_512_uint128_ht;

typedef uint64_t *Hacl_SHA2_512_uint64_p;

typedef uint8_t *Hacl_SHA2_512_uint8_p;

extern uint32_t Hacl_SHA2_512_size_word;

extern uint32_t Hacl_SHA2_512_size_hash_w;

extern uint32_t Hacl_SHA2_512_size_block_w;

extern uint32_t Hacl_SHA2_512_size_hash;

extern uint32_t Hacl_SHA2_512_size_block;

extern uint32_t Hacl_SHA2_512_size_k_w;

extern uint32_t Hacl_SHA2_512_size_ws_w;

extern uint32_t Hacl_SHA2_512_size_whash_w;

extern uint32_t Hacl_SHA2_512_size_count_w;

extern uint32_t Hacl_SHA2_512_size_len_8;

extern uint32_t Hacl_SHA2_512_size_state;

extern uint32_t Hacl_SHA2_512_pos_k_w;

extern uint32_t Hacl_SHA2_512_pos_ws_w;

extern uint32_t Hacl_SHA2_512_pos_whash_w;

extern uint32_t Hacl_SHA2_512_pos_count_w;

void Hacl_SHA2_512_init(uint64_t *state);

void Hacl_SHA2_512_update(uint64_t *state, uint8_t *data);

void Hacl_SHA2_512_update_multi(uint64_t *state, uint8_t *data, uint32_t n1);

void Hacl_SHA2_512_update_last(uint64_t *state, uint8_t *data, uint64_t len);

void Hacl_SHA2_512_finish(uint64_t *state, uint8_t *hash1);

void Hacl_SHA2_512_hash(uint8_t *hash1, uint8_t *input, uint32_t len);
#endif
