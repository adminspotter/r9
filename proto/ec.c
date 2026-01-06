/* ec.c
 *   by Trinity Quirk <tquirk@ymb.net>
 *
 * Revision IX game protocol
 * Copyright (C) 2018-2025  Trinity Annabelle Quirk
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *
 * Functions for R9's handling of elliptic-curve cryptographic
 * operations.
 *
 * Things to do
 *
 */

#include <string.h>

#include <openssl/opensslv.h>
#include <openssl/ec.h>

#include "ec.h"

#if OPENSSL_VERSION_MAJOR >= 3
#include <openssl/core_names.h>
#endif /* OPENSSL_VERSION_MAJOR */

EVP_PKEY *generate_ecdh_key(void)
{
    EVP_PKEY_CTX *param_ctx, *key_ctx;
    EVP_PKEY *key = NULL, *params = NULL;

    if ((param_ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL)) == NULL)
        goto BAILOUT1;

    if (EVP_PKEY_paramgen_init(param_ctx) != 1
        || EVP_PKEY_CTX_set_ec_paramgen_curve_nid(param_ctx, R9_CURVE) != 1
        || EVP_PKEY_paramgen(param_ctx, &params) != 1)
        goto BAILOUT2;

    if ((key_ctx = EVP_PKEY_CTX_new(params, NULL)) == NULL)
        goto BAILOUT3;
    if (EVP_PKEY_keygen_init(key_ctx) == 1)
        EVP_PKEY_keygen(key_ctx, &key);

    EVP_PKEY_CTX_free(key_ctx);
  BAILOUT3:
    EVP_PKEY_free(params);
  BAILOUT2:
    EVP_PKEY_CTX_free(param_ctx);

  BAILOUT1:
    return key;
}

int pkey_to_public_key(const EVP_PKEY *pkey, uint8_t *keybuf, size_t keybuf_sz)
{
#if OPENSSL_VERSION_MAJOR >= 3
    int expected = i2d_PublicKey(pkey, NULL);
    if (keybuf_sz >= expected)
        return i2d_PublicKey(pkey, &keybuf);
    return 0;
#else
    EC_KEY *key = NULL;
    int keylen = 0;

    if ((key = (EC_KEY *)EVP_PKEY_get0(pkey)) != NULL)
        if ((keylen = i2o_ECPublicKey(key, NULL)) != 0 && keybuf_sz >= keylen)
        {
            memset(keybuf, 0, keybuf_sz);
            i2o_ECPublicKey(key, &keybuf);
            return keylen;
        }
    return 0;
#endif /* OPENSSL_VERSION_MAJOR */
}

EVP_PKEY *public_key_to_pkey(const uint8_t *keybuf, size_t keybuf_sz)
{
#if OPENSSL_VERSION_MAJOR >= 3
    EVP_PKEY_CTX *pkey_ctx = EVP_PKEY_CTX_new_from_name(NULL, "ec", NULL);
    OSSL_PARAM params[3];
    EVP_PKEY *key = NULL;

    if (EVP_PKEY_fromdata_init(pkey_ctx) != 1)
        goto CLEANUP;

    params[0] = OSSL_PARAM_construct_utf8_string(OSSL_PKEY_PARAM_GROUP_NAME,
                                                 R9_CURVE_NAME,
                                                 0);
    params[1] = OSSL_PARAM_construct_octet_string(OSSL_PKEY_PARAM_PUB_KEY,
                                                  (void*)keybuf,
                                                  keybuf_sz);
    params[2] = OSSL_PARAM_construct_end();

    /* If this blows up, key should still be NULL. */
    EVP_PKEY_fromdata(pkey_ctx, &key, EVP_PKEY_PUBLIC_KEY, params);

  CLEANUP:
    EVP_PKEY_CTX_free(pkey_ctx);
    return key;
#else
    EC_KEY *key = NULL;
    EC_GROUP *grp = NULL;
    EVP_PKEY *pkey = NULL;

    if (keybuf == NULL || keybuf_sz == 0 || (key = EC_KEY_new()) == NULL)
        return pkey;
    if ((grp = EC_GROUP_new_by_curve_name(R9_CURVE)) == NULL)
        goto BAILOUT1;
    if (EC_KEY_set_group(key, grp) == 0)
        goto BAILOUT2;
    if ((pkey = EVP_PKEY_new()) == NULL
        || o2i_ECPublicKey(&key,
                           (const unsigned char **)&keybuf,
                           keybuf_sz) == NULL
        || EVP_PKEY_assign_EC_KEY(pkey, key) == 0)
        goto BAILOUT3;
    EC_GROUP_free(grp);
    return pkey;

  BAILOUT3:
    EVP_PKEY_free(pkey);
  BAILOUT2:
    EC_GROUP_free(grp);
  BAILOUT1:
    EC_KEY_free(key);
    return NULL;
#endif /* OPENSSL_VERSION_MAJOR */
}
