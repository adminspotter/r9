/* basesock.h                                              -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 04 Nov 2015, 06:48:30 tquirk
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
 * This file contains a base socket type.
 *
 * To create a unix-domain basesock will require a little hackery:
 * create an addrinfo struct in the typical way (probably use
 * localhost as the base address) and reset the ai_addr pointer in the
 * struct to point to a struct sockaddr_un.  Based on the way we grab
 * the arguments, it should work fine.
 *
 * Things to do
 *
 */

#ifndef __INC_BASESOCK_H__
#define __INC_BASESOCK_H__

#include <netinet/in.h>
#include <netdb.h>

#include <cstdint>

#include "sockaddr.h"

class basesock
{
  private:
    bool thread_started;

  public:
    Sockaddr *sa;
    int sock;

    pthread_t listen_thread;
    void *listen_arg;

    static const int LISTEN_BACKLOG = 10;

  protected:
    virtual void create_socket(struct addrinfo *);
    virtual std::string get_port_string(void);

  public:
    basesock(struct addrinfo *);
    virtual ~basesock();

    void start(void *(*)(void *));
    void stop(void);
};

#endif /* __INC_BASESOCK_H__ */
