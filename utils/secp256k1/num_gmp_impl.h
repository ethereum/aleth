/**********************************************************************
 * Copyright (c) 2013, 2014 Pieter Wuille                             *
 * Distributed under the MIT software license, see the accompanying   *
 * file COPYING or http://www.opensource.org/licenses/mit-license.php.*
 **********************************************************************/

#ifndef _SECP256K1_NUM_REPR_IMPL_H_
#define _SECP256K1_NUM_REPR_IMPL_H_

#include <string.h>
#include <stdlib.h>
#include <gmp.h>

#include "util.h"
#include "num.h"

#ifdef VERIFY
static void secp256k1_num_sanity(const secp256k1_num_t *a) {
    VERIFY_CHECK(a->limbs == 1 || (a->limbs > 1 && a->data[a->limbs-1] != 0));
}
#else
#define secp256k1_num_sanity(a) do { } while(0)
#endif

static void secp256k1_num_get_bin(unsigned char *r, unsigned int rlen, const secp256k1_num_t *a) {
    unsigned char tmp[65];
    int len = 0;
    int shift = 0;
    if (a->limbs>1 || a->data[0] != 0) {
        len = mpn_get_str(tmp, 256, (mp_limb_t*)a->data, a->limbs);
    }
    while (shift < len && tmp[shift] == 0) shift++;
    VERIFY_CHECK(len-shift <= (int)rlen);
    memset(r, 0, rlen - len + shift);
    if (len > shift) {
        memcpy(r + rlen - len + shift, tmp + shift, len - shift);
    }
    memset(tmp, 0, sizeof(tmp));
}

static void secp256k1_num_set_bin(secp256k1_num_t *r, const unsigned char *a, unsigned int alen) {
    int len;
    VERIFY_CHECK(alen > 0);
    VERIFY_CHECK(alen <= 64);
    len = mpn_set_str(r->data, a, alen, 256);
    if (len == 0) {
        r->data[0] = 0;
        len = 1;
    }
    VERIFY_CHECK(len <= NUM_LIMBS*2);
    r->limbs = len;
    r->neg = 0;
    while (r->limbs > 1 && r->data[r->limbs-1]==0) {
        r->limbs--;
    }
}

static void secp256k1_num_mod_inverse(secp256k1_num_t *r, const secp256k1_num_t *a, const secp256k1_num_t *m) {
    int i;
    mp_limb_t g[NUM_LIMBS+1];
    mp_limb_t u[NUM_LIMBS+1];
    mp_limb_t v[NUM_LIMBS+1];
    mp_size_t sn;
    mp_size_t gn;
    secp256k1_num_sanity(a);
    secp256k1_num_sanity(m);

    /** mpn_gcdext computes: (G,S) = gcdext(U,V), where
     *  * G = gcd(U,V)
     *  * G = U*S + V*T
     *  * U has equal or more limbs than V, and V has no padding
     *  If we set U to be (a padded version of) a, and V = m:
     *    G = a*S + m*T
     *    G = a*S mod m
     *  Assuming G=1:
     *    S = 1/a mod m
     */
    VERIFY_CHECK(m->limbs <= NUM_LIMBS);
    VERIFY_CHECK(m->data[m->limbs-1] != 0);
    for (i = 0; i < m->limbs; i++) {
        u[i] = (i < a->limbs) ? a->data[i] : 0;
        v[i] = m->data[i];
    }
    sn = NUM_LIMBS+1;
    gn = mpn_gcdext(g, r->data, &sn, u, m->limbs, v, m->limbs);
    VERIFY_CHECK(gn == 1);
    VERIFY_CHECK(g[0] == 1);
    r->neg = a->neg ^ m->neg;
    if (sn < 0) {
        mpn_sub(r->data, m->data, m->limbs, r->data, -sn);
        r->limbs = m->limbs;
        while (r->limbs > 1 && r->data[r->limbs-1]==0) {
            r->limbs--;
        }
    } else {
        r->limbs = sn;
    }
    memset(g, 0, sizeof(g));
    memset(u, 0, sizeof(u));
    memset(v, 0, sizeof(v));
}

#endif
