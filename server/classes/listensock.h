/* listensock.h                                            -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 17 Jul 2017, 22:30:03 tquirk
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
#include "control.h"
#include "thread_pool.h"
#include "defs.h"

/* For use in the socket types which use the basesock */

class base_user {
  public:
    uint64_t userid, sequence;
    Control *control;
    time_t timestamp;
    bool pending_logout;

    base_user(uint64_t, Control *);
    virtual ~base_user();

  protected:
    void init(uint64_t, Control *);

  public:
    virtual bool operator<(const base_user&) const;
    virtual bool operator==(const base_user&) const;

    virtual const base_user& operator=(const base_user&);
};

class listen_socket {
  public:
    static const int REAP_TIMEOUT = 15;
    static const int PING_TIMEOUT = 30;
    static const int LINK_DEAD_TIMEOUT = 75;

  protected:
    bool reaper_running;
    pthread_t reaper;

  public:
    std::map<uint64_t, base_user *> users;

    typedef std::map<uint64_t, base_user *>::iterator users_iterator;

    ThreadPool<packet_list> *send_pool;
    ThreadPool<access_list> *access_pool;
    basesock sock;

  public:
    listen_socket(struct addrinfo *);
    virtual ~listen_socket();

    void init(void);

    virtual void start(void) = 0;
    virtual void stop(void);

    static void *access_pool_worker(void *);

    virtual void login_user(access_list&);
    virtual uint64_t get_userid(login_request&);
    virtual void connect_user(base_user *, access_list&);

    virtual void logout_user(access_list&);

    virtual void do_login(uint64_t, Control *, access_list&) = 0;

    virtual void send_ping(Control *);
    virtual void send_ack(Control *, uint8_t, uint8_t);
};

#endif /* __INC_LISTENSOCK_H__ */
