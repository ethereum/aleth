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


#include "NaCl.h"

static void Hacl_SecretBox_ZeroPad_set_zero_bytes(uint8_t *b)
{
  uint8_t zero1 = (uint8_t)0U;
  b[0U] = zero1;
  b[1U] = zero1;
  b[2U] = zero1;
  b[3U] = zero1;
  b[4U] = zero1;
  b[5U] = zero1;
  b[6U] = zero1;
  b[7U] = zero1;
  b[8U] = zero1;
  b[9U] = zero1;
  b[10U] = zero1;
  b[11U] = zero1;
  b[12U] = zero1;
  b[13U] = zero1;
  b[14U] = zero1;
  b[15U] = zero1;
  b[16U] = zero1;
  b[17U] = zero1;
  b[18U] = zero1;
  b[19U] = zero1;
  b[20U] = zero1;
  b[21U] = zero1;
  b[22U] = zero1;
  b[23U] = zero1;
  b[24U] = zero1;
  b[25U] = zero1;
  b[26U] = zero1;
  b[27U] = zero1;
  b[28U] = zero1;
  b[29U] = zero1;
  b[30U] = zero1;
  b[31U] = zero1;
}

static uint32_t
Hacl_SecretBox_ZeroPad_crypto_secretbox_detached(
  uint8_t *c,
  uint8_t *mac,
  uint8_t *m,
  uint64_t mlen,
  uint8_t *n1,
  uint8_t *k1
)
{
  uint32_t mlen_ = (uint32_t)mlen;
  uint8_t subkey[32U] = { 0U };
  Hacl_Salsa20_hsalsa20(subkey, k1, n1);
  Hacl_Salsa20_salsa20(c, m, mlen_ + (uint32_t)32U, subkey, n1 + (uint32_t)16U, (uint64_t)0U);
  Hacl_Poly1305_64_crypto_onetimeauth(mac, c + (uint32_t)32U, mlen, c);
  Hacl_SecretBox_ZeroPad_set_zero_bytes(c);
  Hacl_SecretBox_ZeroPad_set_zero_bytes(subkey);
  return (uint32_t)0U;
}

static uint32_t
Hacl_SecretBox_ZeroPad_crypto_secretbox_open_detached_decrypt(
  uint8_t *m,
  uint8_t *c,
  uint64_t clen,
  uint8_t *n1,
  uint8_t *subkey,
  uint8_t verify
)
{
  uint32_t clen_ = (uint32_t)clen;
  if (verify == (uint8_t)0U)
  {
    Hacl_Salsa20_salsa20(m, c, clen_ + (uint32_t)32U, subkey, n1 + (uint32_t)16U, (uint64_t)0U);
    Hacl_SecretBox_ZeroPad_set_zero_bytes(subkey);
    Hacl_SecretBox_ZeroPad_set_zero_bytes(m);
    return (uint32_t)0U;
  }
  else
    return (uint32_t)0xffffffffU;
}

static uint32_t
Hacl_SecretBox_ZeroPad_crypto_secretbox_open_detached(
  uint8_t *m,
  uint8_t *c,
  uint8_t *mac,
  uint64_t clen,
  uint8_t *n1,
  uint8_t *k1
)
{
  uint8_t tmp[112U] = { 0U };
  uint8_t *subkey = tmp;
  uint8_t *mackey = tmp + (uint32_t)32U;
  uint8_t *mackey_ = tmp + (uint32_t)64U;
  uint8_t *cmac = tmp + (uint32_t)96U;
  Hacl_Salsa20_hsalsa20(subkey, k1, n1);
  Hacl_Salsa20_salsa20(mackey, mackey_, (uint32_t)32U, subkey, n1 + (uint32_t)16U, (uint64_t)0U);
  Hacl_Poly1305_64_crypto_onetimeauth(cmac, c + (uint32_t)32U, clen, mackey);
  uint8_t result = Hacl_Policies_cmp_bytes(mac, cmac, (uint32_t)16U);
  uint8_t verify = result;
  uint32_t
  z =
    Hacl_SecretBox_ZeroPad_crypto_secretbox_open_detached_decrypt(m,
      c,
      clen,
      n1,
      subkey,
      verify);
  return z;
}

static uint32_t
Hacl_SecretBox_ZeroPad_crypto_secretbox_easy(
  uint8_t *c,
  uint8_t *m,
  uint64_t mlen,
  uint8_t *n1,
  uint8_t *k1
)
{
  uint8_t cmac[16U] = { 0U };
  uint32_t res = Hacl_SecretBox_ZeroPad_crypto_secretbox_detached(c, cmac, m, mlen, n1, k1);
  memcpy(c + (uint32_t)16U, cmac, (uint32_t)16U * sizeof cmac[0U]);
  return res;
}

static uint32_t
Hacl_SecretBox_ZeroPad_crypto_secretbox_open_easy(
  uint8_t *m,
  uint8_t *c,
  uint64_t clen,
  uint8_t *n1,
  uint8_t *k1
)
{
  uint8_t *mac = c;
  return Hacl_SecretBox_ZeroPad_crypto_secretbox_open_detached(m, c, mac, clen, n1, k1);
}

static uint32_t Hacl_Box_ZeroPad_crypto_box_beforenm(uint8_t *k1, uint8_t *pk, uint8_t *sk)
{
  uint8_t tmp[48U] = { 0U };
  uint8_t *hsalsa_k = tmp;
  uint8_t *hsalsa_n = tmp + (uint32_t)32U;
  Hacl_Curve25519_crypto_scalarmult(hsalsa_k, sk, pk);
  Hacl_Salsa20_hsalsa20(k1, hsalsa_k, hsalsa_n);
  return (uint32_t)0U;
}

static uint32_t
Hacl_Box_ZeroPad_crypto_box_detached_afternm(
  uint8_t *c,
  uint8_t *mac,
  uint8_t *m,
  uint64_t mlen,
  uint8_t *n1,
  uint8_t *k1
)
{
  return Hacl_SecretBox_ZeroPad_crypto_secretbox_detached(c, mac, m, mlen, n1, k1);
}

static uint32_t
Hacl_Box_ZeroPad_crypto_box_detached(
  uint8_t *c,
  uint8_t *mac,
  uint8_t *m,
  uint64_t mlen,
  uint8_t *n1,
  uint8_t *pk,
  uint8_t *sk
)
{
  uint8_t key[80U] = { 0U };
  uint8_t *k1 = key;
  uint8_t *subkey = key + (uint32_t)32U;
  uint8_t *hsalsa_n = key + (uint32_t)64U;
  Hacl_Curve25519_crypto_scalarmult(k1, sk, pk);
  Hacl_Salsa20_hsalsa20(subkey, k1, hsalsa_n);
  uint32_t z = Hacl_SecretBox_ZeroPad_crypto_secretbox_detached(c, mac, m, mlen, n1, subkey);
  return z;
}

static uint32_t
Hacl_Box_ZeroPad_crypto_box_open_detached(
  uint8_t *m,
  uint8_t *c,
  uint8_t *mac,
  uint64_t mlen,
  uint8_t *n1,
  uint8_t *pk,
  uint8_t *sk
)
{
  uint8_t key[80U] = { 0U };
  uint8_t *k1 = key;
  uint8_t *subkey = key + (uint32_t)32U;
  uint8_t *hsalsa_n = key + (uint32_t)64U;
  Hacl_Curve25519_crypto_scalarmult(k1, sk, pk);
  Hacl_Salsa20_hsalsa20(subkey, k1, hsalsa_n);
  uint32_t
  z = Hacl_SecretBox_ZeroPad_crypto_secretbox_open_detached(m, c, mac, mlen, n1, subkey);
  return z;
}

static uint32_t
Hacl_Box_ZeroPad_crypto_box_easy_afternm(
  uint8_t *c,
  uint8_t *m,
  uint64_t mlen,
  uint8_t *n1,
  uint8_t *k1
)
{
  uint8_t cmac[16U] = { 0U };
  uint32_t z = Hacl_Box_ZeroPad_crypto_box_detached_afternm(c, cmac, m, mlen, n1, k1);
  memcpy(c + (uint32_t)16U, cmac, (uint32_t)16U * sizeof cmac[0U]);
  return z;
}

static uint32_t
Hacl_Box_ZeroPad_crypto_box_easy(
  uint8_t *c,
  uint8_t *m,
  uint64_t mlen,
  uint8_t *n1,
  uint8_t *pk,
  uint8_t *sk
)
{
  uint8_t cmac[16U] = { 0U };
  uint32_t res = Hacl_Box_ZeroPad_crypto_box_detached(c, cmac, m, mlen, n1, pk, sk);
  memcpy(c + (uint32_t)16U, cmac, (uint32_t)16U * sizeof cmac[0U]);
  return res;
}

static uint32_t
Hacl_Box_ZeroPad_crypto_box_open_easy(
  uint8_t *m,
  uint8_t *c,
  uint64_t mlen,
  uint8_t *n1,
  uint8_t *pk,
  uint8_t *sk
)
{
  uint8_t *mac = c + (uint32_t)16U;
  return Hacl_Box_ZeroPad_crypto_box_open_detached(m, c, mac, mlen, n1, pk, sk);
}

static uint32_t
Hacl_Box_ZeroPad_crypto_box_open_detached_afternm(
  uint8_t *m,
  uint8_t *c,
  uint8_t *mac,
  uint64_t mlen,
  uint8_t *n1,
  uint8_t *k1
)
{
  return Hacl_SecretBox_ZeroPad_crypto_secretbox_open_detached(m, c, mac, mlen, n1, k1);
}

static uint32_t
Hacl_Box_ZeroPad_crypto_box_open_easy_afternm(
  uint8_t *m,
  uint8_t *c,
  uint64_t mlen,
  uint8_t *n1,
  uint8_t *k1
)
{
  uint8_t *mac = c;
  uint32_t t = Hacl_Box_ZeroPad_crypto_box_open_detached_afternm(m, c, mac, mlen, n1, k1);
  return t;
}

Prims_int NaCl_crypto_box_NONCEBYTES = (krml_checked_int_t)24;

Prims_int NaCl_crypto_box_PUBLICKEYBYTES = (krml_checked_int_t)32;

Prims_int NaCl_crypto_box_SECRETKEYBYTES = (krml_checked_int_t)32;

Prims_int NaCl_crypto_box_MACBYTES = (krml_checked_int_t)16;

Prims_int NaCl_crypto_secretbox_NONCEBYTES = (krml_checked_int_t)24;

Prims_int NaCl_crypto_secretbox_KEYBYTES = (krml_checked_int_t)32;

Prims_int NaCl_crypto_secretbox_MACBYTES = (krml_checked_int_t)16;

uint32_t
NaCl_crypto_secretbox_detached(
  uint8_t *c,
  uint8_t *mac,
  uint8_t *m,
  uint64_t mlen,
  uint8_t *n1,
  uint8_t *k1
)
{
  return Hacl_SecretBox_ZeroPad_crypto_secretbox_detached(c, mac, m, mlen, n1, k1);
}

uint32_t
NaCl_crypto_secretbox_open_detached(
  uint8_t *m,
  uint8_t *c,
  uint8_t *mac,
  uint64_t clen,
  uint8_t *n1,
  uint8_t *k1
)
{
  return Hacl_SecretBox_ZeroPad_crypto_secretbox_open_detached(m, c, mac, clen, n1, k1);
}

uint32_t
NaCl_crypto_secretbox_easy(uint8_t *c, uint8_t *m, uint64_t mlen, uint8_t *n1, uint8_t *k1)
{
  return Hacl_SecretBox_ZeroPad_crypto_secretbox_easy(c, m, mlen, n1, k1);
}

uint32_t
NaCl_crypto_secretbox_open_easy(
  uint8_t *m,
  uint8_t *c,
  uint64_t clen,
  uint8_t *n1,
  uint8_t *k1
)
{
  return Hacl_SecretBox_ZeroPad_crypto_secretbox_open_easy(m, c, clen, n1, k1);
}

uint32_t NaCl_crypto_box_beforenm(uint8_t *k1, uint8_t *pk, uint8_t *sk)
{
  return Hacl_Box_ZeroPad_crypto_box_beforenm(k1, pk, sk);
}

uint32_t
NaCl_crypto_box_detached_afternm(
  uint8_t *c,
  uint8_t *mac,
  uint8_t *m,
  uint64_t mlen,
  uint8_t *n1,
  uint8_t *k1
)
{
  return Hacl_Box_ZeroPad_crypto_box_detached_afternm(c, mac, m, mlen, n1, k1);
}

uint32_t
NaCl_crypto_box_detached(
  uint8_t *c,
  uint8_t *mac,
  uint8_t *m,
  uint64_t mlen,
  uint8_t *n1,
  uint8_t *pk,
  uint8_t *sk
)
{
  return Hacl_Box_ZeroPad_crypto_box_detached(c, mac, m, mlen, n1, pk, sk);
}

uint32_t
NaCl_crypto_box_open_detached(
  uint8_t *m,
  uint8_t *c,
  uint8_t *mac,
  uint64_t mlen,
  uint8_t *n1,
  uint8_t *pk,
  uint8_t *sk
)
{
  return Hacl_Box_ZeroPad_crypto_box_open_detached(m, c, mac, mlen, n1, pk, sk);
}

uint32_t
NaCl_crypto_box_easy_afternm(uint8_t *c, uint8_t *m, uint64_t mlen, uint8_t *n1, uint8_t *k1)
{
  return Hacl_Box_ZeroPad_crypto_box_easy_afternm(c, m, mlen, n1, k1);
}

uint32_t
NaCl_crypto_box_easy(
  uint8_t *c,
  uint8_t *m,
  uint64_t mlen,
  uint8_t *n1,
  uint8_t *pk,
  uint8_t *sk
)
{
  return Hacl_Box_ZeroPad_crypto_box_easy(c, m, mlen, n1, pk, sk);
}

uint32_t
NaCl_crypto_box_open_easy(
  uint8_t *m,
  uint8_t *c,
  uint64_t mlen,
  uint8_t *n1,
  uint8_t *pk,
  uint8_t *sk
)
{
  return Hacl_Box_ZeroPad_crypto_box_open_easy(m, c, mlen, n1, pk, sk);
}

uint32_t
NaCl_crypto_box_open_detached_afternm(
  uint8_t *m,
  uint8_t *c,
  uint8_t *mac,
  uint64_t mlen,
  uint8_t *n1,
  uint8_t *k1
)
{
  return Hacl_Box_ZeroPad_crypto_box_open_detached_afternm(m, c, mac, mlen, n1, k1);
}

uint32_t
NaCl_crypto_box_open_easy_afternm(
  uint8_t *m,
  uint8_t *c,
  uint64_t mlen,
  uint8_t *n1,
  uint8_t *k1
)
{
  return Hacl_Box_ZeroPad_crypto_box_open_easy_afternm(m, c, mlen, n1, k1);
}

