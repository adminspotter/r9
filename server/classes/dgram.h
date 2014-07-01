/* dgram.h                                                 -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 01 Jul 2014, 17:58:28 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2014  Trinity Annabelle Quirk
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
 * This file contains the datagram socket object.
 *
 * Changes
 *   08 Sep 2007 TAQ - Created the file from the ashes of udpserver.c.
 *   09 Sep 2007 TAQ - Renamed dgram_user_list to dgram_user.  Added
 *                     operator methods to the dgram_user.
 *   13 Sep 2007 TAQ - Made port and sock public for the send_pool_workers.
 *   16 Sep 2007 TAQ - Added timestamp member to user class.
 *   22 Oct 2007 TAQ - Added a private reaper thread member.
 *   02 Dec 2007 TAQ - Added pending_logout member to user class.
 *   14 Jun 2014 TAQ - Quite a bit of restructuring.  The basesock is now
 *                     a has-a, rather than an is-a relationship, and the
 *                     base classes have changed.
 *   15 Jun 2014 TAQ - Moved send worker into the class.  Sockaddr is now
 *                     a class hierarchy.
 *   17 Jun 2014 TAQ - The socks member now uses a pointer as a key, and
 *                     takes the comparator argument.
 *
 * Things to do
 *   - Consider whether the socks and users maps should become unordered
 *     maps instead.
 *
 */

#ifndef __INC_DGRAM_H__
#define __INC_DGRAM_H__

#include <map>

#include "control.h"
#include "basesock.h"
#include "sockaddr.h"

class dgram_user : public base_user
{
  public:
    Sockaddr *sa;

    dgram_user(u_int64_t, Control *);

    const dgram_user& operator=(const dgram_user&);
};

class dgram_socket : public listen_socket
{
  public:
    std::map<Sockaddr *, dgram_user *, less_sockaddr> socks;

  public:
    dgram_socket(struct addrinfo *, u_int16_t);
    ~dgram_socket();

    void start(void);

    void do_login(u_int64_t, Control *, access_list&);

    static void *dgram_listen_worker(void *);
    static void *dgram_reaper_worker(void *);
    static void *dgram_send_worker(void *);
};

#endif /* __INC_DGRAM_H__ */
