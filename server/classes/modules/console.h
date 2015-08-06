/* console.h                                               -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 24 Jul 2015, 13:14:02 tquirk
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
 * This file contains the base class for server consoles.  We'll derive
 * classes for Unix-domain files, and for TCP ports.
 *
 * We'll have a server-wide vector of Consoles, each of which will
 * have its own listening port.  Each Console object will have a
 * listening thread, and a vector of active connection threads.
 *
 * If two console threads try to modify things at the same time, there
 * could be some bad drama.  We've got a static pthread_mutex in the
 * console session, with some protected get/drop lock functions.
 *
 * Things to do
 *
 */

#ifndef __INC_CONSOLE_H__
#define __INC_CONSOLE_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>

#include <vector>
#include <string>
#include <iostream>

/* Each accepted session on a console socket will create a new thread
 * which is handled by a ConsoleSession.  The function calls are
 * dispatched through the class ConsoleSession::dispatch method, which
 * should have a lock-unlock of the mutex around the call.
 */
class ConsoleSession
{
  public:
    pthread_t thread_id;
    int sock;
    std::istream *in;
    std::ostream *out;
    static pthread_mutex_t dispatch_lock;

    ConsoleSession(int);
    ~ConsoleSession();

    static void *start(void *);

  protected:
    void get_lock(void);
    void drop_lock(void);

    static std::string dispatch(std::string &);
    static void cleanup(void *);

    std::string get_line(void);
};

class Console
{
  private:
    pthread_t listen_thread;

  protected:
    int console_sock;
    std::vector<ConsoleSession *> sessions;

  public:
    virtual ~Console();

    /* Both can throw int */
    void start(void *(*)(void *));
    void stop(void);
};

class UnixConsole : public Console
{
  private:
    std::string console_fname;

  public:
    UnixConsole(char *);
    ~UnixConsole();

    static void *listener(void *);

  private:
    void open_socket(void);
};

class InetConsole : public Console
{
  private:
    uint16_t port_num;

  public:
    InetConsole(uint16_t, struct addrinfo *);
    ~InetConsole();

    static void *listener(void *);

  private:
    /* Can throw int */
    void open_socket(struct addrinfo *);

    int wrap_request(struct sockaddr_storage *);
};

#endif /* __INC_CONSOLE_H__ */
