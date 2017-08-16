/* listensock.h                                            -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 16 Aug 2017, 08:17:48 tquirk
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

class listen_socket;

class base_user : public Control {
  public:
    uint64_t sequence;
    time_t timestamp;
    bool pending_logout;

  protected:
    listen_socket *parent;

  public:
    base_user(uint64_t, GameObject *, listen_socket *);
    virtual ~base_user();

    virtual const base_user& operator=(const base_user&);

    void send_ping(void);
    void send_ack(uint8_t, uint8_t);
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

    virtual std::string port_type(void) = 0;

    virtual void start(void);
    virtual void stop(void);

    static void *access_pool_worker(void *);
    static void *reaper_worker(void *);

    virtual void login_user(access_list&);
    virtual uint64_t get_userid(login_request&);
    virtual void connect_user(base_user *, access_list&);

    virtual void logout_user(access_list&);

    virtual void do_login(uint64_t, Control *, access_list&) = 0;
    virtual void do_logout(base_user *) = 0;

    virtual void connect_user(base_user *, access_list&);
    virtual void disconnect_user(base_user *);
};

#endif /* __INC_LISTENSOCK_H__ */
