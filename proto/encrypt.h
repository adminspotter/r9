/* encrypt.h
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 12 Jun 2019, 06:26:32 tquirk
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
 * Defines and function declarations for encryption and decryption.
 *
 * Things to do
 *
 */

#ifndef __INC_R9_PROTO_ENCRYPT_H__
#define __INC_R9_PROTO_ENCRYPT_H__

#include <stdint.h>

#include <openssl/evp.h>

#define R9_SYMMETRIC_KEY_SZ      256
#define R9_SYMMETRIC_ALGO        EVP_aes_256_ctr()
#define R9_SYMMETRIC_KEY_BUF_SZ  R9_SYMMETRIC_KEY_SZ / 8
#define R9_SYMMETRIC_IV_BUF_SZ   R9_SYMMETRIC_KEY_BUF_SZ / 2

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

int r9_encrypt(const unsigned char *, int,
               const unsigned char *, unsigned char *, uint64_t,
               unsigned char *);
int r9_decrypt(const unsigned char *, int,
               const unsigned char *, unsigned char *, uint64_t,
               unsigned char *);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INC_R9_PROTO_ENCRYPT_H__ */
