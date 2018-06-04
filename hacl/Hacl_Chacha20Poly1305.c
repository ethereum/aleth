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


#include "Hacl_Chacha20Poly1305.h"

Prims_int Hacl_Chacha20Poly1305_noncelen = (krml_checked_int_t)12;

Prims_int Hacl_Chacha20Poly1305_keylen = (krml_checked_int_t)32;

Prims_int Hacl_Chacha20Poly1305_maclen = (krml_checked_int_t)16;

static void
Hacl_Chacha20Poly1305_aead_encrypt_poly(
  uint8_t *c,
  uint32_t mlen,
  uint8_t *mac,
  uint8_t *aad1,
  uint32_t aadlen,
  uint8_t *tmp
)
{
  uint8_t *b = tmp;
  uint8_t *lb = tmp + (uint32_t)64U;
  uint8_t *mk = b;
  uint8_t *key_s = mk + (uint32_t)16U;
  uint64_t tmp1[6U] = { 0U };
  Hacl_Impl_Poly1305_64_State_poly1305_state
  st = AEAD_Poly1305_64_mk_state(tmp1, tmp1 + (uint32_t)3U);
  (void)AEAD_Poly1305_64_poly1305_blocks_init(st, aad1, aadlen, mk);
  (void)AEAD_Poly1305_64_poly1305_blocks_continue(st, c, mlen);
  AEAD_Poly1305_64_poly1305_blocks_finish(st, lb, mac, key_s);
}

void Hacl_Chacha20Poly1305_encode_length(uint8_t *lb, uint32_t aad_len, uint32_t mlen)
{
  store64_le(lb, (uint64_t)aad_len);
  uint8_t *x0 = lb + (uint32_t)8U;
  store64_le(x0, (uint64_t)mlen);
}

uint32_t
Hacl_Chacha20Poly1305_aead_encrypt_(
  uint8_t *c,
  uint8_t *mac,
  uint8_t *m,
  uint32_t mlen,
  uint8_t *aad1,
  uint32_t aadlen,
  uint8_t *k1,
  uint8_t *n1
)
{
  uint8_t tmp[80U] = { 0U };
  uint8_t *b = tmp;
  uint8_t *lb = tmp + (uint32_t)64U;
  Hacl_Chacha20Poly1305_encode_length(lb, aadlen, mlen);
  Hacl_Chacha20_chacha20(c, m, mlen, k1, n1, (uint32_t)1U);
  Hacl_Chacha20_chacha20_key_block(b, k1, n1, (uint32_t)0U);
  Hacl_Chacha20Poly1305_aead_encrypt_poly(c, mlen, mac, aad1, aadlen, tmp);
  return (uint32_t)0U;
}

uint32_t
Hacl_Chacha20Poly1305_aead_encrypt(
  uint8_t *c,
  uint8_t *mac,
  uint8_t *m,
  uint32_t mlen,
  uint8_t *aad1,
  uint32_t aadlen,
  uint8_t *k1,
  uint8_t *n1
)
{
  uint32_t z = Hacl_Chacha20Poly1305_aead_encrypt_(c, mac, m, mlen, aad1, aadlen, k1, n1);
  return z;
}

uint32_t
Hacl_Chacha20Poly1305_aead_decrypt(
  uint8_t *m,
  uint8_t *c,
  uint32_t mlen,
  uint8_t *mac,
  uint8_t *aad1,
  uint32_t aadlen,
  uint8_t *k1,
  uint8_t *n1
)
{
  uint8_t tmp[96U] = { 0U };
  uint8_t *b = tmp;
  uint8_t *lb = tmp + (uint32_t)64U;
  Hacl_Chacha20Poly1305_encode_length(lb, aadlen, mlen);
  uint8_t *rmac = tmp + (uint32_t)80U;
  Hacl_Chacha20_chacha20_key_block(b, k1, n1, (uint32_t)0U);
  Hacl_Chacha20Poly1305_aead_encrypt_poly(c, mlen, rmac, aad1, aadlen, tmp);
  uint8_t result = Hacl_Policies_cmp_bytes(mac, rmac, (uint32_t)16U);
  uint8_t verify = result;
  uint32_t res;
  if (verify == (uint8_t)0U)
  {
    Hacl_Chacha20_chacha20(m, c, mlen, k1, n1, (uint32_t)1U);
    res = (uint32_t)0U;
  }
  else
    res = (uint32_t)1U;
  return res;
}

