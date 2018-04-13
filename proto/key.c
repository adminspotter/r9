/* key.c
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 13 Apr 2018, 09:48:02 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2018  Trinity Annabelle Quirk
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
 * Functions for loading and saving public keys.
 *
 * Things to do
 *
 */

#include "key.h"

#include <openssl/pem.h>

EVP_PKEY *string_to_pkey(const unsigned char *string, size_t len)
{
    EVP_PKEY *pub_key = NULL;
    BIO *bo = NULL;

    if ((bo = BIO_new(BIO_s_mem())) == NULL)
        return NULL;

    if (BIO_write(bo, string, len) == len)
        PEM_read_bio_PrivateKey(bo, &pub_key, NULL, NULL);

    BIO_free(bo);
    return pub_key;
}

size_t pkey_to_string(EVP_PKEY *key, unsigned char **string, size_t len)
{
    BIO *bo = NULL;
    size_t actual_len = 0;

    if ((bo = BIO_new(BIO_s_mem())) == NULL)
        return 0;

    if (PEM_write_bio_PrivateKey(bo, key,
                                 NULL,
                                 NULL, 0,
                                 NULL, NULL) == 1)
        actual_len = BIO_read(bo, string, len);

    BIO_free(bo);
    return actual_len;
}
