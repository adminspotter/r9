/* inet_console.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 24 Jul 2015, 15:09:47 tquirk
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
 * This file contains the inet port console.
 *
 * Changes
 *   31 May 2014 TAQ - Created the file.
 *   09 Jul 2014 TAQ - De-syslogged, and we only throw runtime_errors now.
 *   10 Jul 2014 TAQ - Missed one catch of an int, now a runtime_error.  Also
 *                     when a constructor throws an exception, memory is
 *                     automatically cleaned up.
 *   02 May 2015 TAQ - Moved most of the hostname-grabbing into wrap_request,
 *                     since that's the only place it's used.  Switched to
 *                     hosts_ctl, since it does a lot more stuff automatically.
 *   24 Jul 2015 TAQ - Converted to stdint types.
 *
 * Things to do
 *
 */

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#ifdef HAVE_LIBWRAP
#include <netdb.h>
#include <tcpd.h>
#endif

#include <cstdint>
#include <cstring>
#include <sstream>
#include <stdexcept>

#include "console.h"

InetConsole::InetConsole(uint16_t port, struct addrinfo *ai)
{
    this->port_num = port;
    this->open_socket(ai);
    this->start(InetConsole::listener);
}

InetConsole::~InetConsole()
{
}

void InetConsole::open_socket(struct addrinfo *ai)
{
    uid_t uid = geteuid();
    gid_t gid = getegid();
    int do_uid = this->port_num <= 1024 && uid != 0;
    int opt = 1;

    if ((this->console_sock = socket(ai->ai_family,
                                     ai->ai_socktype,
                                     ai->ai_protocol)) < 0)
    {
        std::ostringstream s;
        s << "socket creation failed for console port " << port_num << ": "
          << strerror(errno) << " (" << errno << ")";
        throw std::runtime_error(s.str());
    }
    setsockopt(this->console_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));

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
        close(this->console_sock);
        throw EINVAL;
    }

    /* If root's gotta open the port, become root, if possible. */
    if (do_uid)
    {
        if (getuid() != 0)
        {
            close(this->console_sock);
            std::ostringstream s;
            s << "can't open console port " << this->port_num
              << " as non-root user",
            throw std::runtime_error(s.str());
        }
        else
        {
            seteuid(getuid());
            setegid(getgid());
        }
    }
    if (bind(this->console_sock,
             (struct sockaddr *)(ai->ai_addr), ai->ai_addrlen) < 0)
    {
         if (do_uid)
        {
            seteuid(uid);
            setegid(gid);
        }
        close(this->console_sock);
        std::ostringstream s;
        s << "bind failed for console port " << this->port_num << ": "
          << strerror(errno) << " (" << errno << ")";
        throw std::runtime_error(s.str());
    }
    /* Restore the original euid and egid of the process, if necessary. */
    if (do_uid)
    {
        seteuid(uid);
        setegid(gid);
    }

    if (listen(this->console_sock, 5) < 0)
    {
        std::ostringstream s;
        s << "listen failed for console port " << this->port_num << ": "
          << strerror(errno) << " (" << errno << ")";
        close(this->console_sock);
        throw std::runtime_error(s.str());
    }
}

void *InetConsole::listener(void *arg)
{
    InetConsole *con = (InetConsole *)arg;
    int newsock;
    struct sockaddr_storage ss;
    socklen_t ss_len = sizeof(ss);
    ConsoleSession *sess = NULL;

    while ((newsock = accept(con->console_sock,
                             reinterpret_cast<struct sockaddr *>(&ss),
                             &ss_len)) != -1)
    {
        try
        {
            if (con->wrap_request(&ss))
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
        ss_len = sizeof(ss);
    }
    pthread_exit(NULL);
    return NULL;
}

int InetConsole::wrap_request(struct sockaddr_storage *ss)
{
#ifdef HAVE_LIBWRAP
    char hostname[1024];
    char ipaddr[INET6_ADDRSTRLEN];
    void *addr;
    int retval;

    if ((retval = getnameinfo(ss, sizeof(sockaddr_storage),
                              hostname, sizeof(hostname),
                              NULL, 0, 0)))
    {
        std::ostringstream s;
        s << "could not get hostname: "
          << gai_strerror(retval) << " (" << retval << ")";
        throw std::runtime_error(s.str());
    }

    switch (ss.ss_family)
    {
      case AF_INET:
        addr = (void *)&(reinterpret_cast<struct sockaddr_in *>(&ss))->sin_addr;
        break;
      case AF_INET6:
        addr = (void *)&(reinterpret_cast<struct sockaddr_in6 *>(&ss))->sin6_addr;
        break;
      default:
        addr = NULL;
    }
    if (!inet_ntop(ss.ss_family, addr, ipaddr, sizeof(ipaddr)))
    {
        std::ostringstream s;
        s << "could not convert IP to printable format: "
          << strerror(errno) << " (" << errno << ")";
        throw std::runtime_error(s.str());
    }

    return hosts_ctl(config.log_prefix.c_str(), hostname, ipaddr, NULL);
#else
    return 1;
#endif
}
