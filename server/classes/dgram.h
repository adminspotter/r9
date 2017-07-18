/* dgram.h                                                 -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 18 Jul 2017, 07:33:12 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2017  Trinity Annabelle Quirk
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
 * Things to do
 *   - Consider whether the socks and users maps should become unordered
 *     maps instead.
 *
 */

#ifndef __INC_DGRAM_H__
#define __INC_DGRAM_H__

#include <cstdint>
#include <map>
#include <functional>

#include "control.h"
#include "listensock.h"
#include "sockaddr.h"

/* Since we can't use a Sockaddr directly in one of our map keys,
 * we'll need to use pointers to them, to leverage the polymorphism
 * here.  We need a compare functor to deref the pointers for the
 * less-than comparison that's required by the map.
 */
class less_sockaddr
    : public std::function<bool(const Sockaddr *, const Sockaddr *)>
{
  public:
    bool operator()(const Sockaddr *a, const Sockaddr *b) const
        {
            return (*a < *b);
        }
};

class dgram_user : public base_user
{
  public:
    Sockaddr *sa;

    dgram_user(uint64_t, Control *);

    const dgram_user& operator=(const dgram_user&);
};

class dgram_socket : public listen_socket
{
  public:
    std::map<Sockaddr *, dgram_user *, less_sockaddr> socks;

    typedef std::map<Sockaddr *,
                     dgram_user *,
                     less_sockaddr>::iterator socks_iterator;

  public:
    dgram_socket(struct addrinfo *);
    ~dgram_socket();

    std::string port_type(void) override;

    void start(void) override;

    void do_login(uint64_t, Control *, access_list&) override;
    void do_logout(base_user *) override;

    static void *dgram_listen_worker(void *);
    static void *dgram_reaper_worker(void *);
    static void *dgram_send_worker(void *);
};

#endif /* __INC_DGRAM_H__ */
