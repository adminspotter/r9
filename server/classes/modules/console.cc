/* console.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 03 Dec 2015, 16:41:40 tquirk
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
 * This file contains the base console class implementation, along
 * with the console session thread class.
 *
 * Things to do
 *   - See if we can use the basesock, rather than mostly reimplementing it.
 *
 */

#include <config.h>

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include <sstream>
#include <stdexcept>

#include "console.h"
#include "fdstreambuf.h"

#if HAVE_LIBWRAP
#include <tcpd.h>
#include "../../config_data.h"
#endif /* HAVE_LIBWRAP */

pthread_mutex_t ConsoleSession::dispatch_lock = PTHREAD_MUTEX_INITIALIZER;

ConsoleSession::ConsoleSession(int sock)
{
    int ret;

    this->sock = sock;
    this->in = new std::istream(new fdibuf(sock));
    this->out = new std::ostream(new fdobuf(sock));

    if ((ret = pthread_create(&(this->thread_id), NULL,
                              ConsoleSession::session_listener,
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

void *ConsoleSession::session_listener(void *arg)
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

    delete sess->in->rdbuf();
    delete sess->in;
    delete sess->out->rdbuf();
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

Console::Console(struct addrinfo *ai)
    : basesock(ai), sessions()
{
}

Console::~Console()
{
    Console::sessions_iterator i;

    for (i = this->sessions.begin(); i != this->sessions.end(); ++i)
        delete *i;
    this->sessions.erase(this->sessions.begin(), this->sessions.end());
}

void *Console::console_listener(void *arg)
{
    Console *con = (Console *)arg;
    int newsock;
    struct sockaddr_storage ss;
    socklen_t ss_len = sizeof(ss);
    Sockaddr *sa;
    ConsoleSession *sess = NULL;

    while ((newsock = accept(con->sock,
                             reinterpret_cast<struct sockaddr *>(&ss),
                             &ss_len)) != -1)
    {
        try
        {
            sa = NULL;
            sa = build_sockaddr((struct sockaddr&)ss);

            if (con->wrap_request(sa))
            {
                sess = new ConsoleSession(newsock);
                con->sessions.push_back(sess);
            }
            else
                close(newsock);
        }
        catch (std::exception& e)
        {
            close(newsock);
            continue;
        }
        if (sa != NULL)
            delete sa;
        ss_len = sizeof(ss);
    }
    pthread_exit(NULL);
    return NULL;
}

int Console::wrap_request(Sockaddr *sa)
{
#if HAVE_LIBWRAP
    return hosts_ctl((char *)config.log_prefix.c_str(),
                     (char *)sa->hostname(),
                     (char *)sa->ntop(),
                     STRING_UNKNOWN);
#else
    return 1;
#endif
}
