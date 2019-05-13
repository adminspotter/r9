/* dh.c
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 12 May 2019, 20:33:00 tquirk
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
 * Functions which generate shared secrets for Diffie-Hellman key
 * exchange.
 *
 * Things to do
 *
 */

#include "dh.h"

struct dh_message *dh_shared_secret(EVP_PKEY *priv_key, EVP_PKEY *peer_key)
{
    EVP_PKEY_CTX *derive_ctx;
    struct dh_message *msg = NULL, *digest = NULL;

    if ((msg = OPENSSL_malloc(sizeof(struct dh_message))) == NULL)
        return NULL;

    if ((derive_ctx = EVP_PKEY_CTX_new(priv_key, NULL)) == NULL)
        goto BAILOUT1;

    if (EVP_PKEY_derive_init(derive_ctx) != 1
        || EVP_PKEY_derive_set_peer(derive_ctx, peer_key) != 1
        || EVP_PKEY_derive(derive_ctx, NULL, &msg->message_len) != 1)
        goto BAILOUT2;

    if ((msg->message = OPENSSL_malloc(msg->message_len)) == NULL)
        goto BAILOUT2;
    else if (EVP_PKEY_derive(derive_ctx, msg->message, &msg->message_len) != 1)
        goto BAILOUT3;

    EVP_PKEY_CTX_free(derive_ctx);
    digest = digest_message(msg);
    OPENSSL_free(msg->message);
    OPENSSL_free(msg);
    return digest;

  BAILOUT3:
    OPENSSL_free(msg->message);
  BAILOUT2:
    EVP_PKEY_CTX_free(derive_ctx);
  BAILOUT1:
    OPENSSL_free(msg);
    return NULL;
}

struct dh_message *digest_message(const struct dh_message *msg)
{
    EVP_MD_CTX *digest_ctx;
    struct dh_message *digest = NULL;

    if ((digest = OPENSSL_malloc(sizeof(struct dh_message))) == NULL)
        return NULL;
    digest->message_len = EVP_MD_size(EVP_sha256());
    if ((digest->message = OPENSSL_malloc(digest->message_len)) == NULL)
        goto BAILOUT1;

    if ((digest_ctx = EVP_MD_CTX_create()) == NULL)
        goto BAILOUT2;

    if (EVP_DigestInit_ex(digest_ctx, EVP_sha256(), NULL) != 1
        || EVP_DigestUpdate(digest_ctx, msg->message, msg->message_len) != 1)
        goto BAILOUT3;

    if (EVP_DigestFinal_ex(digest_ctx,
                           digest->message,
                           (unsigned int *)&digest->message_len) != 1)
        goto BAILOUT3;

    EVP_MD_CTX_destroy(digest_ctx);
    return digest;

  BAILOUT3:
    EVP_MD_CTX_destroy(digest_ctx);
  BAILOUT2:
    OPENSSL_free(digest->message);
  BAILOUT1:
    OPENSSL_free(digest);
    return NULL;
}
