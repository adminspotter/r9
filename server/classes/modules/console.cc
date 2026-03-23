/* console.cc
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
 * This file contains the base console class implementation, along
 * with the console session thread class.
 *
 * Things to do
 *
 */

#include <config.h>

#include <unistd.h>

#include "console.h"
#include "fdstreambuf.h"

#if HAVE_LIBWRAP
#include <tcpd.h>
#include "../config_data.h"

static char string_unknown[] = STRING_UNKNOWN;
#endif /* HAVE_LIBWRAP */

std::mutex ConsoleSession::dispatch_lock;

ConsoleSession::ConsoleSession(int sock)
{
    int ret;

    this->sock = sock;
    this->in = new std::istream(new fdibuf(sock));
    this->out = new std::ostream(new fdobuf(sock));

    this->thread_id = std::thread(ConsoleSession::session_listener,
                                  (void *)this);
}

ConsoleSession::~ConsoleSession()
{
    close(this->sock);
    delete this->in->rdbuf();
    delete this->in;
    delete this->out->rdbuf();
    delete this->out;
    this->thread_id.join();
}

void ConsoleSession::session_listener(void *arg)
{
    ConsoleSession *sess = (ConsoleSession *)arg;

    for (;;)
    {
        std::string str = sess->get_line();

        if (str == "" || str == "exit")
        {
            (*sess->out) << "exiting" << std::endl;
            break;
        }

        {
            std::unique_lock lock(ConsoleSession::dispatch_lock);
            *(sess->out) << ConsoleSession::dispatch(str) << std::endl;
        }
    }
    delete sess;
}

std::string ConsoleSession::dispatch(std::string &command)
{
    return "not implemented";
}

std::string ConsoleSession::get_line(void)
{
    std::string str = "";

    (*this->out) << "r9 ~> ";
    this->out->flush();
    (*this->in) >> str;
    return str;
}

Console::Console(Addrinfo *ai)
    : basesock(ai)
{
    this->port_type = "console";
}

Console::~Console()
{
}

void Console::console_listener(void *arg)
{
    Console *con = (Console *)arg;
    int newsock;
    struct sockaddr_storage ss;
    socklen_t ss_len = sizeof(ss);
    Sockaddr *sa;

    for (;;)
    {
        if ((newsock = accept(con->sock,
                             (struct sockaddr *)(&ss),
                             &ss_len)) < 1)
            break;

        try
        {
            sa = NULL;
            sa = build_sockaddr((struct sockaddr&)ss);

            if (con->wrap_request(sa))
                new ConsoleSession(newsock);
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
}

int Console::wrap_request(Sockaddr *sa)
{
#if HAVE_LIBWRAP
    return hosts_ctl((char *)config.log_prefix.c_str(),
                     (char *)sa->hostname(),
                     (char *)sa->ntop(),
                     string_unknown);
#else
    return 1;
#endif
}

extern "C" Console *console_create(Addrinfo *ai)
{
    Console *c = new Console(ai);
    c->listen_arg = c;
    c->start(Console::console_listener);
    return c;
}

extern "C" void console_destroy(Console *con)
{
    delete con;
}
