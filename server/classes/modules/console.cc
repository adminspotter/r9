/* console.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 10 Jul 2014, 14:25:26 trinityquirk
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
 * This file contains the base console class implementation, along
 * with the console session thread class.
 *
 * Changes
 *   25 May 2014 TAQ - Created the file.  Having the console(s) handled by
 *                     some C stuff, in weird ways, is just silly.  We can
 *                     handle things much more extensibly with some open-
 *                     ended C++ objects.
 *   31 May 2014 TAQ - Added some more utility functions, copied from
 *                     console.c.
 *   07 Jun 2014 TAQ - Removed a couple remnants of a previous bad idea.
 *                     Created the new ConsoleSession, which solves the
 *                     file descriptor leaking problem.
 *   27 Jun 2014 TAQ - We now log to std::clog.  Trying out the GNU
 *                     stdio_filebuf from libstdc++, in order to use streams
 *                     on our session sockets.
 *   06 Jul 2014 TAQ - A blank constructor in the Console base class was
 *                     preventing the derived classes from linking properly,
 *                     so it's gone.
 *   09 Jul 2014 TAQ - We're now doing no syslogging, and only throwing
 *                     standard exceptions.
 *
 * Things to do
 *   - See if we can use the basesock, rather than mostly reimplementing it.
 *
 */

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include <sstream>
#include <stdexcept>
#include <ext/stdio_filebuf.h>

#include "console.h"

pthread_mutex_t ConsoleSession::dispatch_lock = PTHREAD_MUTEX_INITIALIZER;

ConsoleSession::ConsoleSession(int sock)
{
    int ret;

    this->sock = sock;
    __gnu_cxx::stdio_filebuf<char> inbuf(sock, std::ios::in);
    this->in = new std::istream(&inbuf);
    __gnu_cxx::stdio_filebuf<char> outbuf(sock, std::ios::out);
    this->out = new std::ostream(&outbuf);

    if ((ret = pthread_create(&(this->thread_id), NULL,
                              ConsoleSession::start,
                              (void *)this)) != 0)
    {
        std::ostringstream s;
        s << "error creating console thread: "
          << strerror(ret) << " (" << ret << ")";
        throw std::runtime_error(s.str());
    }
}

ConsoleSession::~ConsoleSession()
{
    ConsoleSession::cleanup(this);
}

void ConsoleSession::get_lock(void)
{
    pthread_mutex_lock(&ConsoleSession::dispatch_lock);
}

void ConsoleSession::drop_lock(void)
{
    pthread_mutex_unlock(&ConsoleSession::dispatch_lock);
}

void *ConsoleSession::start(void *arg)
{
    ConsoleSession *sess = (ConsoleSession *)arg;
    int done = 0, old_cancel_type;

    pthread_cleanup_push(ConsoleSession::cleanup, (void *)sess);

    while (!done)
    {
        std::string str = sess->get_line();

	/* Do we need to exit? */
	if (str == "exit")
	{
	    (*sess->out) << "exiting" << std::endl;
	    done = 1;
            break;
	}

        sess->get_lock();
        *(sess->out) << ConsoleSession::dispatch(str) << std::endl;
        sess->drop_lock();
    }

    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &old_cancel_type);
    pthread_cleanup_pop(1);
    pthread_setcanceltype(old_cancel_type, NULL);
    pthread_exit(NULL);

    return NULL;
}

std::string ConsoleSession::dispatch(std::string &command)
{
    return std::string("not implemented");
}

/* So we don't leak file descriptors when we get cancelled */
void ConsoleSession::cleanup(void *arg)
{
    ConsoleSession *sess = (ConsoleSession *)arg;

    delete sess->in;
    delete sess->out;
    if (sess->sock)
    {
        close(sess->sock);
        sess->sock = 0;
    }
}

std::string ConsoleSession::get_line(void)
{
    std::string str = "";

    (*this->out) << "r9 ~> ";
    this->out->flush();
    (*this->in) >> str;
    if (this->in->eof())
        str = "exit";
    return str;
}

Console::~Console()
{
    try { this->stop(); }
    catch (std::exception& e) { /* Do nothing */ }
    if (this->console_sock)
    {
        close(this->console_sock);
    }
}

void Console::start(void *(*func)(void *))
{
    int ret;

    if (this->console_sock == 0)
    {
        std::ostringstream s;
        s << "no socket available to listen";
        throw std::runtime_error(s.str());
    }
    if ((ret = pthread_create(&(this->listen_thread),
                              NULL,
                              func,
                              (void *)this)) != 0)
    {
        std::ostringstream s;
        s << "couldn't start listen thread: "
          << strerror(ret) << " (" << ret << ")";
        throw std::runtime_error(s.str());
    }
}

void Console::stop(void)
{
    int ret;

    if ((ret = pthread_cancel(this->listen_thread)) != 0)
    {
        std::ostringstream s;
        s << "couldn't terminate console thread: "
          << strerror(ret) << " (" << ret << ")";
        throw std::runtime_error(s.str());
    }
    close(this->console_sock);
    this->console_sock = 0;
    while (sessions.size())
    {
        ConsoleSession *sess = sessions.back();
        pthread_cancel(sess->thread_id);
        sleep(0);
        pthread_join(sess->thread_id, NULL);
        delete sess;
        sessions.pop_back();
    }
}
