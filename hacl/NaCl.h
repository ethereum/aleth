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
#ifndef __NaCl_H
#define __NaCl_H


#include "Hacl_Salsa20.h"
#include "Hacl_Curve25519.h"
#include "Hacl_Poly1305_64.h"
#include "Hacl_Policies.h"


extern Prims_int NaCl_crypto_box_NONCEBYTES;

extern Prims_int NaCl_crypto_box_PUBLICKEYBYTES;

extern Prims_int NaCl_crypto_box_SECRETKEYBYTES;

extern Prims_int NaCl_crypto_box_MACBYTES;

extern Prims_int NaCl_crypto_secretbox_NONCEBYTES;

extern Prims_int NaCl_crypto_secretbox_KEYBYTES;

extern Prims_int NaCl_crypto_secretbox_MACBYTES;

uint32_t
NaCl_crypto_secretbox_detached(
  uint8_t *c,
  uint8_t *mac,
  uint8_t *m,
  uint64_t mlen,
  uint8_t *n1,
  uint8_t *k1
);

uint32_t
NaCl_crypto_secretbox_open_detached(
  uint8_t *m,
  uint8_t *c,
  uint8_t *mac,
  uint64_t clen,
  uint8_t *n1,
  uint8_t *k1
);

uint32_t
NaCl_crypto_secretbox_easy(uint8_t *c, uint8_t *m, uint64_t mlen, uint8_t *n1, uint8_t *k1);

uint32_t
NaCl_crypto_secretbox_open_easy(
  uint8_t *m,
  uint8_t *c,
  uint64_t clen,
  uint8_t *n1,
  uint8_t *k1
);

uint32_t NaCl_crypto_box_beforenm(uint8_t *k1, uint8_t *pk, uint8_t *sk);

uint32_t
NaCl_crypto_box_detached_afternm(
  uint8_t *c,
  uint8_t *mac,
  uint8_t *m,
  uint64_t mlen,
  uint8_t *n1,
  uint8_t *k1
);

uint32_t
NaCl_crypto_box_detached(
  uint8_t *c,
  uint8_t *mac,
  uint8_t *m,
  uint64_t mlen,
  uint8_t *n1,
  uint8_t *pk,
  uint8_t *sk
);

uint32_t
NaCl_crypto_box_open_detached(
  uint8_t *m,
  uint8_t *c,
  uint8_t *mac,
  uint64_t mlen,
  uint8_t *n1,
  uint8_t *pk,
  uint8_t *sk
);

uint32_t
NaCl_crypto_box_easy_afternm(uint8_t *c, uint8_t *m, uint64_t mlen, uint8_t *n1, uint8_t *k1);

uint32_t
NaCl_crypto_box_easy(
  uint8_t *c,
  uint8_t *m,
  uint64_t mlen,
  uint8_t *n1,
  uint8_t *pk,
  uint8_t *sk
);

uint32_t
NaCl_crypto_box_open_easy(
  uint8_t *m,
  uint8_t *c,
  uint64_t mlen,
  uint8_t *n1,
  uint8_t *pk,
  uint8_t *sk
);

uint32_t
NaCl_crypto_box_open_detached_afternm(
  uint8_t *m,
  uint8_t *c,
  uint8_t *mac,
  uint64_t mlen,
  uint8_t *n1,
  uint8_t *k1
);

uint32_t
NaCl_crypto_box_open_easy_afternm(
  uint8_t *m,
  uint8_t *c,
  uint64_t mlen,
  uint8_t *n1,
  uint8_t *k1
);
#endif
