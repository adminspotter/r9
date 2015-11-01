/* basesock.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 01 Nov 2015, 10:55:57 tquirk
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
 * This file contains the socket creation class and the basic user
 * tracking.  The listen socket will encapsulate these, and extend
 * them to a more usable state.
 *
 * Things to do
 *
 */

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#include <sstream>
#include <stdexcept>

#include "basesock.h"

#include "../log.h"

basesock::basesock(struct addrinfo *ai, uint16_t port)
{
    this->port_num = port;
    this->listen_arg = NULL;
    this->create_socket(ai);
}

basesock::~basesock()
{
    try { this->stop(); }
    catch (std::exception& e) { /* Do nothing */ }
    if (this->sock)
    {
        close(this->sock);
        this->sock = 0;
    }
}

void basesock::create_socket(struct addrinfo *ai)
{
    uid_t uid = geteuid();
    gid_t gid = getegid();
    int do_uid = this->port_num <= 1024 && uid != 0;
    int opt = 1;
    const std::string typestr
        = (ai->ai_socktype == SOCK_DGRAM ? "dgram" : "stream");

    if ((this->sock = socket(ai->ai_family,
                             ai->ai_socktype,
                             ai->ai_protocol)) < 0)
    {
        std::ostringstream s;
        s << "socket creation failed for " << typestr << " port "
          << this->port_num << ": "
          << strerror(errno) << " (" << errno << ")";
        this->sock = 0;
        throw std::runtime_error(s.str());
    }
    setsockopt(this->sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));

    switch (ai->ai_addr->sa_family)
    {
      case AF_INET:
        reinterpret_cast<struct sockaddr_in *>(ai->ai_addr)->sin_port
            = htons(this->port_num);
        break;
      case AF_INET6:
        reinterpret_cast<struct sockaddr_in6 *>(ai->ai_addr)->sin6_port
            = htons(this->port_num);
        break;
      default:
        close(this->sock);
        this->sock = 0;
        std::ostringstream s;
        s << "don't recognize address family " << ai->ai_addr->sa_family
          << ": " << strerror(EINVAL) << " (" << EINVAL << ")";
        throw std::runtime_error(s.str());
    }

    /* If root's gotta open the port, become root, if possible. */
    if (do_uid)
    {
        if (getuid() != 0)
        {
            std::ostringstream s;
            s << "can't open " << typestr << " port "
              << this->port_num << " as non-root user";
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
    if (bind(this->sock,
             (struct sockaddr *)(ai->ai_addr), ai->ai_addrlen) < 0)
    {
        if (do_uid)
        {
            seteuid(uid);
            setegid(gid);
        }
        std::ostringstream s;
        s << "bind failed for " << typestr << " port " << this->port_num << ": "
          << strerror(errno) << " (" << errno << ")";
        close(this->sock);
        this->sock = 0;
        throw std::runtime_error(s.str());
    }
    /* Restore the original euid and egid of the process, if necessary. */
    if (do_uid)
    {
        seteuid(uid);
        setegid(gid);
    }

    if (ai->ai_socktype == SOCK_STREAM)
    {
        if (listen(this->sock, basesock::LISTEN_BACKLOG) < 0)
        {
            std::ostringstream s;
            s << "listen failed for " << typestr << " port "
              << this->port_num << ": "
              << strerror(errno) << " (" << errno << ")";
            close(this->sock);
            this->sock = 0;
            throw std::runtime_error(s.str());
        }
    }

    std::clog << "created " << typestr << " socket "
              << this->sock << " on port "
              << this->port_num << std::endl;
}

void basesock::start(void *(*func)(void *))
{
    int ret;

    if (this->sock == 0)
    {
        std::ostringstream s;
        s << "no socket available to listen for port " << this->port_num << ": "
          << strerror(ENOTSOCK) << " (" << ENOTSOCK << ")";
        throw std::runtime_error(s.str());
    }
    if ((ret = pthread_create(&(this->listen_thread),
                              NULL,
                              func,
                              this->listen_arg)) != 0)
    {
        std::ostringstream s;
        s << "couldn't start listen thread for port " << this->port_num << ": "
          << strerror(ret) << " (" << ret << ")";
        throw std::runtime_error(s.str());
    }
}

void basesock::stop(void)
{
    int ret;

    if ((ret = pthread_cancel(this->listen_thread)) != 0)
    {
        std::ostringstream s;
        s << "couldn't cancel listen thread for port " << this->port_num << ": "
          << strerror(ret) << " (" << ret << ")";
        throw std::runtime_error(s.str());
    }
    sleep(0);
    if ((ret = pthread_join(this->listen_thread, NULL)) != 0)
    {
        std::ostringstream s;
        s << "couldn't join listen thread for port " << this->port_num << ": "
          << strerror(ret) << " (" << ret << ")";
        throw std::runtime_error(s.str());
    }
}

base_user::base_user(uint64_t u, Control *c)
{
    this->init(u, c);
}

base_user::~base_user()
{
}

void base_user::init(uint64_t u, Control *c)
{
    this->userid = u;
    this->control = c;
    this->timestamp = time(NULL);
    this->pending_logout = false;
}

bool base_user::operator<(const base_user& u) const
{
    return (this->userid < u.userid);
}

bool base_user::operator==(const base_user& u) const
{
    return (this->userid == u.userid);
}

const base_user& base_user::operator=(const base_user& u)
{
    this->userid = u.userid;
    this->control = u.control;
    this->timestamp = u.timestamp;
    this->pending_logout = u.pending_logout;
    return *this;
}
