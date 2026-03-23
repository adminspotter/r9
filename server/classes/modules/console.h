/* console.h                                               -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *
 * Revision IX game server
 * Copyright (C) 2014-2026  Trinity Annabelle Quirk
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
 * This file contains the base class for server consoles.  We'll derive
 * classes for Unix-domain files, and for TCP ports.
 *
 * We'll have a server-wide vector of Consoles, each of which will
 * have its own listening port.  Each Console object will have its own
 * listening thread.
 *
 * If two console threads try to modify things at the same time, there
 * could be some bad drama.  We've got a static mutex in the console
 * session to try to limit such collisions.
 *
 * Things to do
 *
 */

#ifndef __INC_CONSOLE_H__
#define __INC_CONSOLE_H__

#include <string>
#include <thread>
#include <mutex>
#include <iostream>

#include "../basesock.h"

class ConsoleSession
{
  public:
    std::thread thread_id;
    int sock;
    std::istream *in;
    std::ostream *out;
    static std::mutex dispatch_lock;

    ConsoleSession(int);
    ~ConsoleSession();

    static void session_listener(void *);

  protected:
    static std::string dispatch(std::string &);

    std::string get_line(void);
};

class Console : public basesock
{
  public:
    Console(Addrinfo *);
    virtual ~Console();

    int wrap_request(Sockaddr *);

    static void *console_listener(void *);
};

typedef Console *console_create_t(Addrinfo *);
typedef void console_destroy_t(Console *);

#endif /* __INC_CONSOLE_H__ */
