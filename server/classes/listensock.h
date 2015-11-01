/* listensock.h                                            -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 01 Nov 2015, 10:55:31 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2015  Trinity Annabelle Quirk
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
 * This file contains a basic listening socket, with a listener thread.
 *
 * Things to do
 *
 */

#ifndef __INC_LISTENSOCK_H__
#define __INC_LISTENSOCK_H__

#include <pthread.h>

#include <map>

#include "basesock.h"
#include "thread_pool.h"
#include "defs.h"

class listen_socket {
  public:
    static const int REAP_TIMEOUT = 15;
    static const int PING_TIMEOUT = 30;
    static const int LINK_DEAD_TIMEOUT = 75;

  protected:
    pthread_t reaper;

  public:
    std::map<uint64_t, base_user *> users;
    ThreadPool<packet_list> *send_pool;
    ThreadPool<access_list> *access_pool;
    basesock sock;

  public:
    listen_socket(struct addrinfo *, uint16_t);
    virtual ~listen_socket();

    void init(void);

    virtual void start(void) = 0;
    virtual void stop(void);

    static void *access_pool_worker(void *);

    virtual void login_user(access_list&);
    virtual void logout_user(access_list&);

    virtual void do_login(uint64_t, Control *, access_list&) = 0;
};

#endif /* __INC_LISTENSOCK_H__ */
