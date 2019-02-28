/* dh.h
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 28 Feb 2019, 08:52:08 tquirk
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
 * Structures and function declarations for Diffie-Hellman key exchange.
 *
 * Things to do
 *
 */

#ifndef __INC_R9_PROTO_DH_H__
#define __INC_R9_PROTO_DH_H__

#include <openssl/evp.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

struct dh_message
{
    unsigned char *message;
    size_t message_len;
};

struct dh_message *dh_shared_secret(EVP_PKEY *, EVP_PKEY *);
struct dh_message *digest_message(const struct dh_message *);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INC_R9_PROTO_DH_H__ */
