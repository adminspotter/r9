/* key.c
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 13 Apr 2019, 14:28:25 tquirk
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
 * Functions for loading and saving public keys.
 *
 * Things to do
 *
 */

#include "key.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <openssl/pem.h>

EVP_PKEY *string_to_pkey(const unsigned char *string, size_t len)
{
    EVP_PKEY *priv_key = NULL;
    BIO *bo = NULL;

    if ((bo = BIO_new(BIO_s_mem())) == NULL)
        return NULL;

    if (BIO_write(bo, string, len) == len)
        PEM_read_bio_PrivateKey(bo, &priv_key, NULL, NULL);

    BIO_free(bo);
    return priv_key;
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

EVP_PKEY *pub_string_to_pkey(const unsigned char *string, size_t len)
{
    EVP_PKEY *pub_key = NULL;
    BIO *bo = NULL;

    if ((bo = BIO_new(BIO_s_mem())) == NULL)
        return NULL;

    if (BIO_write(bo, string, len) == len)
        PEM_read_bio_PUBKEY(bo, &pub_key, NULL, NULL);

    BIO_free(bo);
    return pub_key;
}

size_t pkey_to_pub_string(EVP_PKEY *key, unsigned char **string, size_t len)
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

EVP_PKEY *pub_der_to_pkey(const unsigned char *string, size_t len)
{
    EVP_PKEY *pub_key = EVP_PKEY_new();
    EC_KEY *ec_key = NULL;
    BIO *bo = NULL;

    if ((pub_key = EVP_PKEY_new()) == NULL)
        return NULL;

    if ((bo = BIO_new(BIO_s_mem())) == NULL)
        goto BAILOUT1;

    if (BIO_write(bo, string, len) == len
        && d2i_EC_PUBKEY_bio(bo, &ec_key))
        EVP_PKEY_assign_EC_KEY(pub_key, ec_key);
    else
        goto BAILOUT2;

    BIO_free(bo);
    return pub_key;

  BAILOUT2:
    BIO_free(bo);
  BAILOUT1:
    EVP_PKEY_free(pub_key);
    return NULL;
}

size_t pkey_to_pub_der(EVP_PKEY *key, unsigned char **string, size_t len)
{
    BIO *bo = NULL;
    size_t actual_len = 0;

    if ((bo = BIO_new(BIO_s_mem())) == NULL)
        return 0;

    if (i2d_EC_PUBKEY_bio(bo, EVP_PKEY_get1_EC_KEY(key)) == 1)
        actual_len = BIO_read(bo, string, len);

    BIO_free(bo);
    return actual_len;
}

EVP_PKEY *file_to_pkey(const char *fname, unsigned char *passphrase)
{
    EVP_PKEY *key = NULL;
    FILE *fp = fopen(fname, "r");

    if (fp != NULL)
    {
        key = PEM_read_PrivateKey(fp, &key, NULL, passphrase);
        fclose(fp);
    }
    return key;
}

int pkey_to_file(EVP_PKEY *key,
                 const char *fname,
                 unsigned char *passphrase)
{
    int fd;
    FILE *fp = NULL;
    int result = 0;

    if ((fd = creat(fname, S_IRUSR | S_IWUSR)) != 0
        && (fp = fdopen(fd, "w")) != NULL)
    {
        EVP_CIPHER *cipher = NULL;
        int pp_len = 0;

        if (passphrase != NULL && (pp_len = strlen((char *)passphrase)) != 0)
            cipher = (EVP_CIPHER *)EVP_aes_192_cbc();
        result = PEM_write_PrivateKey(fp, key, cipher,
                                      passphrase, pp_len,
                                      NULL, NULL);
        fclose(fp);
    }
    return result;
}
