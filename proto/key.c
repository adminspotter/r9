/* key.c
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 12 Apr 2018, 08:12:18 tquirk
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
    return NULL;
}

size_t pkey_to_string(EVP_PKEY *key, unsigned char **string, size_t len)
{
    BIO *bo = NULL;
    size_t actual_len = 0;

    if ((bo = BIO_new(BIO_s_mem())) == NULL)
        return 0;

    if (PEM_write_bio_PUBKEY(bo, key) == 1)
        actual_len = BIO_read(bo, string, len);

    BIO_free(bo);
    return actual_len;
}
