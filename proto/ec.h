/* ec.h
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 28 Feb 2019, 08:52:45 tquirk
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
 * Defines and prototypes for R9's handling of elliptic-curve
 * cryptographic operations.
 *
 * Things to do
 *
 */

#ifndef __INC_R9_PROTO_EC_H__
#define __INC_R9_PROTO_EC_H__

#include <openssl/evp.h>
#include <openssl/ec.h>

#define R9_CURVE  NID_sect571r1

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

EVP_PKEY *generate_ecdh_key(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INC_R9_PROTO_EC_H__ */
