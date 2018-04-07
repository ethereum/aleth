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
#ifndef __Hacl_Ed25519_H
#define __Hacl_Ed25519_H





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

typedef struct
{
  void *fst;
  void *snd;
}
K___FStar_Seq_Base_seq_uint64_t_FStar_Seq_Base_seq_uint64_t;

typedef uint64_t *Hacl_EC_Point_point;

typedef uint8_t *Hacl_EC_Format_uint8_p;

typedef uint64_t Hacl_Lib_Create64_h64;

typedef uint64_t Hacl_Bignum25519_limb;

typedef uint64_t *Hacl_Bignum25519_felem;

typedef void *Hacl_Bignum25519_seqelem;

typedef uint64_t *Hacl_Impl_Ed25519_ExtPoint_point;

typedef uint8_t *Hacl_Impl_Store51_uint8_p;

typedef uint64_t *Hacl_Impl_Store51_felem;

typedef uint8_t *Hacl_Impl_Ed25519_PointCompress_hint8_p;

typedef uint64_t *Hacl_Impl_Ed25519_PointCompress_hint64_p;

typedef uint64_t *Hacl_Impl_Ed25519_SwapConditional_felem;

typedef uint8_t *Hacl_Impl_Ed25519_Ladder_Step_uint8_p;

typedef uint64_t *Hacl_Impl_Ed25519_Ladder_elemB;

typedef uint8_t *Hacl_Hash_Lib_LoadStore_uint8_p;

typedef uint8_t Hacl_Hash_Lib_Create_uint8_t;

typedef uint32_t Hacl_Hash_Lib_Create_uint32_t;

typedef uint64_t Hacl_Hash_Lib_Create_uint64_t;

typedef uint8_t Hacl_Hash_Lib_Create_uint8_ht;

typedef uint32_t Hacl_Hash_Lib_Create_uint32_ht;

typedef uint64_t Hacl_Hash_Lib_Create_uint64_ht;

typedef uint8_t *Hacl_Hash_Lib_Create_uint8_p;

typedef uint32_t *Hacl_Hash_Lib_Create_uint32_p;

typedef uint64_t *Hacl_Hash_Lib_Create_uint64_p;

typedef uint8_t Hacl_Impl_SHA2_512_Lemmas_uint8_t;

typedef uint32_t Hacl_Impl_SHA2_512_Lemmas_uint32_t;

typedef uint64_t Hacl_Impl_SHA2_512_Lemmas_uint64_t;

typedef uint8_t Hacl_Impl_SHA2_512_Lemmas_uint8_ht;

typedef uint32_t Hacl_Impl_SHA2_512_Lemmas_uint32_ht;

typedef uint64_t Hacl_Impl_SHA2_512_Lemmas_uint64_ht;

typedef FStar_UInt128_t Hacl_Impl_SHA2_512_Lemmas_uint128_ht;

typedef uint64_t *Hacl_Impl_SHA2_512_Lemmas_uint64_p;

typedef uint8_t *Hacl_Impl_SHA2_512_Lemmas_uint8_p;

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

typedef uint8_t *Hacl_Impl_Ed25519_SecretExpand_hint8_p;

typedef uint8_t *Hacl_Impl_Ed25519_SecretToPublic_hint8_p;

typedef Prims_nat Hacl_Impl_Ed25519_Verify_Lemmas_u51;

typedef uint8_t *Hacl_Impl_Ed25519_PointEqual_uint8_p;

typedef uint64_t *Hacl_Impl_Ed25519_PointEqual_felem;

typedef uint32_t Hacl_Impl_Load56_u32;

typedef uint8_t Hacl_Impl_Load56_h8;

typedef uint64_t Hacl_Impl_Load56_h64;

typedef uint8_t *Hacl_Impl_Load56_hint8_p;

typedef uint64_t *Hacl_Impl_Ed25519_RecoverX_elemB;

typedef uint32_t Hacl_Impl_Load51_u32;

typedef uint8_t Hacl_Impl_Load51_h8;

typedef uint64_t Hacl_Impl_Load51_h64;

typedef uint8_t *Hacl_Impl_Load51_hint8_p;

typedef uint8_t *Hacl_Impl_Store56_hint8_p;

typedef uint64_t *Hacl_Impl_Store56_qelem;

typedef uint8_t *Hacl_Impl_SHA512_Ed25519_1_hint8_p;

typedef uint8_t *Hacl_Impl_SHA512_Ed25519_hint8_p;

typedef uint8_t *Hacl_Impl_Sha512_hint8_p;

typedef FStar_UInt128_t Hacl_Lib_Create128_h128;

typedef uint64_t *Hacl_Impl_BignumQ_Mul_qelemB;

typedef uint64_t Hacl_Impl_BignumQ_Mul_h64;

typedef uint8_t *Hacl_Impl_Ed25519_Verify_Steps_uint8_p;

typedef uint64_t *Hacl_Impl_Ed25519_Verify_Steps_felem;

typedef uint8_t *Hacl_Impl_Ed25519_Verify_uint8_p;

typedef uint64_t *Hacl_Impl_Ed25519_Verify_felem;

typedef uint8_t *Hacl_Impl_Ed25519_Sign_Steps_hint8_p;

typedef uint8_t *Hacl_Impl_Ed25519_Sign_hint8_p;

typedef uint8_t *Hacl_Ed25519_uint8_p;

typedef uint8_t *Hacl_Ed25519_hint8_p;

void Hacl_Ed25519_sign(uint8_t *signature, uint8_t *secret, uint8_t *msg, uint32_t len1);

bool Hacl_Ed25519_verify(uint8_t *pubkey, uint8_t *msg, uint32_t len1, uint8_t *signature);

void Hacl_Ed25519_secret_to_public(uint8_t *out, uint8_t *secret);
#endif
