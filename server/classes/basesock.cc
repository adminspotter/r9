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
{
    std::clog << syslogWarn << "default basesock constructor" << std::endl;
    this->sa = NULL;
    this->listen_arg = NULL;
    this->thread_started = false;
    this->sock = 0;
}

basesock::basesock(Addrinfo *ai)
{
    this->sa = ai->sockaddr();
    this->listen_arg = NULL;
    this->thread_started = false;
    this->create_socket(ai);
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

void basesock::create_socket(Addrinfo *ai)
{
    uid_t uid = geteuid();
    gid_t gid = getegid();
    int do_uid = this->sa->port() <= 1024 && uid != 0;
    int opt = 1, ret;
    const std::string typestr
        = (ai->socktype() == SOCK_DGRAM ? "dgram " : "stream ");

    if ((this->sock = socket(ai->family(),
                             ai->socktype(),
                             ai->protocol())) < 0)
    {
        std::ostringstream s;

        s << "socket creation failed for " << typestr
          << this->get_port_string();
        this->sock = 0;
        throw std::system_error(errno, std::generic_category(), s.str());
    }
    setsockopt(this->sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));

    if (do_uid)
    {
        if (getuid() != 0)
        {
            std::ostringstream s;
            s << "can't open " << typestr << this->get_port_string()
              << " as non-root user";
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
    ret = bind(this->sock, this->sa->sockaddr(), ai->addrlen());
    if (do_uid)
    {
        seteuid(uid);
        setegid(gid);
    }
    if (ret < 0)
    {
        std::ostringstream s;

        s << "bind failed for " << typestr << this->get_port_string();
        close(this->sock);
        this->sock = 0;
        throw std::system_error(errno, std::generic_category(), s.str());
    }

    if (ai->socktype() == SOCK_STREAM)
    {
        if (listen(this->sock, basesock::LISTEN_BACKLOG) < 0)
        {
            std::ostringstream s;

            s << "listen failed for " << typestr << this->get_port_string();
            close(this->sock);
            this->sock = 0;
            throw std::system_error(errno, std::generic_category(), s.str());
        }
    }

    std::clog << "created " << typestr << "socket "
              << this->sock << " on " << this->get_port_string() << std::endl;
}

std::string basesock::port_type(void)
{
    return "base";
}

std::string basesock::get_port_string(void)
{
    std::ostringstream s;

    Sockaddr_un *sun = dynamic_cast<Sockaddr_un *>(this->sa);
    if (sun != NULL)
        s << sun->ntop();
    else
        s << "port " << this->sa->port();
    return s.str();
}

void basesock::start(void *(*func)(void *))
{
    int ret;

    if (!this->thread_started && this->sock > 0)
    {
        if ((ret = pthread_create(&(this->listen_thread),
                                  NULL,
                                  func,
                                  this->listen_arg)) != 0)
        {
            std::ostringstream s;

            s << "couldn't start listen thread for " << this->get_port_string();
            throw std::system_error(ret, std::generic_category(), s.str());
        }
        this->thread_started = true;
    }
}

void basesock::stop(void)
{
    int ret;

    if (this->thread_started)
    {
        if ((ret = pthread_cancel(this->listen_thread)) != 0)
        {
            std::ostringstream s;

            s << "couldn't cancel listen thread for "
              << this->get_port_string();
            throw std::system_error(ret, std::generic_category(), s.str());
        }
        sleep(0);
        if ((ret = pthread_join(this->listen_thread, NULL)) != 0)
        {
            std::ostringstream s;

            s << "couldn't join listen thread for " << this->get_port_string();
            throw std::system_error(ret, std::generic_category(), s.str());
        }
        this->thread_started = false;
    }
}
