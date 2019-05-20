/* encrypt.c
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 17 Mar 2019, 08:31:17 tquirk
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
 * Functions for symmetric encryption and decryption.
 *
 * Things to do
 *
 */

#include <openssl/conf.h>
#include <openssl/err.h>

#include "encrypt.h"

int r9_encrypt(const unsigned char *plaintext, int plaintext_len,
               const unsigned char *key, const unsigned char *iv,
               unsigned char *ciphertext)
{
    EVP_CIPHER_CTX *ctx;
    int len, ciphertext_len = 0;

    if ((ctx = EVP_CIPHER_CTX_new()) == NULL)
        goto BAILOUT1;

    if (EVP_EncryptInit_ex(ctx, R9_SYMMETRIC_ALGO, NULL, key, iv) != 1
        || EVP_EncryptUpdate(ctx,
                             ciphertext, &len,
                             plaintext, plaintext_len) != 1)
        goto BAILOUT2;
    ciphertext_len = len;

    if (EVP_EncryptFinal_ex(ctx, ciphertext + len, &len) == 1)
        ciphertext_len += len;
    else
        /* We'll return length 0 on any error. */
        ciphertext_len = 0;

  BAILOUT2:
    EVP_CIPHER_CTX_free(ctx);
  BAILOUT1:
    return ciphertext_len;
}

int r9_decrypt(const unsigned char *ciphertext, int ciphertext_len,
               const unsigned char *key, const unsigned char *iv,
               unsigned char *plaintext)
{
    EVP_CIPHER_CTX *ctx;
    int len, plaintext_len = 0;

    if ((ctx = EVP_CIPHER_CTX_new()) == NULL)
        goto BAILOUT1;

    if (EVP_DecryptInit_ex(ctx, R9_SYMMETRIC_ALGO, NULL, key, iv) != 1
        || EVP_DecryptUpdate(ctx,
                             plaintext, &len,
                             ciphertext, ciphertext_len) != 1)
        goto BAILOUT2;
    plaintext_len = len;

    if (EVP_DecryptFinal_ex(ctx, plaintext + len, &len) == 1)
        plaintext_len += len;
    else
        /* We'll return length 0 on any error. */
        plaintext_len = 0;

  BAILOUT2:
    EVP_CIPHER_CTX_free(ctx);
  BAILOUT1:
    return plaintext_len;
}
