/* basesock.h                                              -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 15 Jun 2014, 08:53:40 tquirk
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
 * This file contains a basic listening socket, with a listener thread.
 *
 * Changes
 *   12 Sep 2007 TAQ - Created the file.
 *   13 Sep 2007 TAQ - Added create_socket static method.  Moved blank
 *                     constructor and destructor into basesock.cc.
 *   14 Jun 2014 TAQ - This is now much more of a base socket, which will
 *                     be directly usable everyplace we need a listener.
 *                     Added the base_user and listen_socket base classes.
 *
 * Things to do
 *
 */

#ifndef __INC_BASESOCK_H__
#define __INC_BASESOCK_H__

#include <netinet/in.h>
#include <pthread.h>

#include <map>

#include "control.h"
#include "thread_pool.h"
#include "defs.h"

class basesock
{
  public:
    int sock;
    u_int16_t port_num;

    pthread_t listen_thread;
    void *listen_arg;

    static const int LISTEN_BACKLOG = 10;

  private:
    void create_socket(struct addrinfo *);

  public:
    basesock(struct addrinfo *, u_int16_t);
    ~basesock();

    void start(void *(*)(void *));
    void stop(void);
};

/* For use in the socket types which use the basesock */

class base_user {
  public:
    u_int64_t userid;
    Control *control;
    time_t timestamp;
    bool pending_logout;

    base_user(u_int64_t, Control *);
    virtual ~base_user();

  protected:
    void init(u_int64_t, Control *);

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
    pthread_t reaper;

  public:
    std::map<u_int64_t, base_user *> users;
    ThreadPool<packet_list> *send;
    basesock sock;

  public:
    listen_socket(struct addrinfo *, u_int16_t);
    virtual ~listen_socket();

    void init(void);

    virtual void start(void) = 0;

    virtual base_user *login_user(u_int64_t, Control *, access_list&) = 0;
    virtual base_user *logout_user(u_int64_t);
};

#endif /* __INC_BASESOCK_H__ */
