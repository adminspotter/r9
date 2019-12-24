/* defs.h                                                  -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 14 Dec 2019, 10:01:47 tquirk
 *
 * Revision IX game server
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
 * Some basic definitions and typedefs for the server classes.
 *
 * Things to do
 *   - If we can get rid of this file entirely, that would probably be
 *     a good thing.
 *
 */

#ifndef __INC_DEFS_H__
#define __INC_DEFS_H__

#include <cstdint>
#include <string>

#include "../../proto/proto.h"

/* Eliminate the multiple-include problems */
class base_user;
class Sockaddr;

typedef struct packet_list_tag
{
    packet buf;
    base_user *who;
}
packet_list;

typedef struct access_list_tag
{
    packet buf;
    union
    {
        struct
        {
            union
            {
                Sockaddr *dgram;
                int stream;
            }
            who;
        }
        login;
        struct
        {
            uint64_t who;
        }
        logout;
    }
    what;
}
access_list;

#endif /* __INC_DEFS_H__ */
