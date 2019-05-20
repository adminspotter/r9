/* ec.c
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 28 Feb 2019, 08:52:36 tquirk
 *
 * Revision IX game protocol
 * Copyright (C) 2019  Trinity Annabelle Quirk
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

#include "ec.h"

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
