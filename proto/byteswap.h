/* byteswap.h
 *   by Trinity Quirk <tquirk@ymb.net>
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
 * These are the definitions for doing network byte order conversion.
 *
 * Things to do
 *
 */

#ifndef __INC_R9_PROTO_BYTESWAP_H__
#define __INC_R9_PROTO_BYTESWAP_H__

#include <config.h>

#if HAVE_ENDIAN_H
#include <endian.h>
#endif /* HAVE_ENDIAN_H */
#if HAVE_BYTESWAP_H
#include <byteswap.h>
#endif /* HAVE_BYTESWAP_H */
#if HAVE_ARCHITECTURE_BYTE_ORDER_H
#include <architecture/byte_order.h>
#define BYTE_SWAP(x) __builtin_bswap64(x)
#else
#define BYTE_SWAP(x) __bswap_64(x)
#endif /* HAVE_ARCHITECTURE_BYTE_ORDER_H */

#if !(HAVE_HTONLL) && !defined(htonll)
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define htonll(x) (x)
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define htonll(x) BYTE_SWAP(x)
#endif /* __BYTE_ORDER__ */
#endif /* HAVE_HTONLL */
#if !(HAVE_NTOHLL) && !defined(ntohll)
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define ntohll(x) (x)
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define ntohll(x) BYTE_SWAP(x)
#endif /* __BYTE_ORDER__ */
#endif /* HAVE_NTOHLL */

#endif /* __INC_R9_PROTO_BYTESWAP_H__ */
