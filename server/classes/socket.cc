/* socket.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 07 Aug 2024, 09:14:44 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2024  Trinity Annabelle Quirk
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
 * This file contains a socket creation factory.
 *
 * Things to do
 *
 */

#include <string>
#include <stdexcept>

#include "dgram.h"
#include "stream.h"

listen_socket *socket_create(struct addrinfo *ai)
{
    /* We don't actually have a unix-domain listening socket type. */
    if (ai->ai_family == AF_UNIX)
    {
        std::string s = "Unix-domain listening sockets are not supported.";
        throw std::runtime_error(s);
    }

    if (ai->ai_socktype == SOCK_STREAM)
        return new stream_socket(ai);
    return new dgram_socket(ai);
}
