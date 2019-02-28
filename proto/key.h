/* key.h
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 28 Feb 2019, 08:53:11 tquirk
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
 * Prototypes for loading and saving public keys.
 *
 * Things to do
 *
 */

#ifndef __INC_R9_PROTO_KEY_H__
#define __INC_R9_PROTO_KEY_H__

#include <openssl/evp.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

EVP_PKEY *string_to_pkey(const unsigned char *, size_t);
size_t pkey_to_string(EVP_PKEY *, unsigned char **, size_t);

EVP_PKEY *file_to_pkey(const char *, unsigned char *);
int pkey_to_file(EVP_PKEY *, const char *, unsigned char *);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INC_R9_PROTO_KEY_H__ */
