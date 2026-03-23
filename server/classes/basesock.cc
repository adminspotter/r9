/* basesock.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *
 * Revision IX game server
 * Copyright (C) 2007-2026  Trinity Annabelle Quirk
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
 * This file contains the socket creation class and the basic user
 * tracking.  The listen socket will encapsulate these, and extend
 * them to a more usable state.
 *
 * Things to do
 *
 */

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#include <sstream>
#include <stdexcept>
#include <system_error>

#include "basesock.h"
#include "log.h"

/* This default constructor is here solely for ease of mocking other
 * classes which depend on the basesock.  It should not be used in
 * production code, thus the log warning.
 */
basesock::basesock()
    : exit_flag(false), exit_lock(), exit_cond(), listen_thread()
{
    std::clog << syslogWarn << "default basesock constructor" << std::endl;
    this->ai = NULL;
    this->sa = NULL;
    this->init();
}

void basesock::init(void)
{
    this->listen_arg = NULL;
    this->listen_started = false;
    this->sock = 0;
    this->port_type = "base";
}

void basesock::create_socket(void)
{
    uid_t uid = geteuid();
    gid_t gid = getegid();
    int do_uid = this->sa->port() <= 1024 && uid != 0;
    int opt = 1, ret;

    if ((this->sock = socket(this->ai->family(),
                             this->ai->socktype(),
                             this->ai->protocol())) < 0)
    {
        std::ostringstream s;

        s << "socket creation failed for " << this->port_type
          << " " << this->sa->str();
        this->sock = 0;
        throw std::system_error(errno, std::generic_category(), s.str());
    }
    setsockopt(this->sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));

    if (do_uid)
    {
        if (getuid() != 0)
        {
            std::ostringstream s;
            s << "can't open " << this->port_type << " "
              << this->sa->str() << " as non-root user";
            close(this->sock);
            this->sock = 0;
            throw std::runtime_error(s.str());
        }
        else
        {
            seteuid(getuid());
            setegid(getgid());
        }
    }
    ret = bind(this->sock, this->sa->sockaddr(), this->sa->size());
    if (do_uid)
    {
        seteuid(uid);
        setegid(gid);
    }
    if (ret < 0)
    {
        std::ostringstream s;

        s << "bind failed for " << this->port_type << " " << this->sa->str();
        close(this->sock);
        this->sock = 0;
        throw std::system_error(errno, std::generic_category(), s.str());
    }

    if (this->ai->socktype() == SOCK_STREAM)
    {
        if (listen(this->sock, basesock::LISTEN_BACKLOG) < 0)
        {
            std::ostringstream s;

            s << "listen failed for " << this->port_type << " "
              << this->sa->str();
            close(this->sock);
            this->sock = 0;
            throw std::system_error(errno, std::generic_category(), s.str());
        }
    }

    std::clog << "created " << this->port_type << " socket "
              << this->sock << " on " << this->sa->str() << std::endl;
}

basesock::basesock(Addrinfo *ai)
    : exit_flag(false), exit_lock(), exit_cond(), listen_thread()
{
    this->ai = ai;
    this->sa = ai->sockaddr();
    this->init();
}

basesock::~basesock()
{
    try { this->stop(); }
    catch (std::exception& e) {}
    if (this->sock)
    {
        close(this->sock);
        this->sock = 0;
    }
    if (this->sa != NULL)
        delete this->sa;
}

void basesock::start(void (*func)(void *), void *arg)
{
    int ret;

    this->create_socket();
    if (arg != NULL)
        this->listen_arg = arg;
    if (!this->listen_started && this->sock > 0)
    {
        this->listen_thread = std::thread(func, this->listen_arg);
        this->listen_started = true;
    }
}

void basesock::stop(void)
{
    this->exit_flag = true;
    this->exit_cond.notify_all();

    if (this->listen_started)
    {
        this->listen_thread.join();
        this->listen_started = false;
    }
}
