/* inet_console.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 08 Jun 2014, 14:30:39 tquirk
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
 * This file contains the inet port console.
 *
 * Changes
 *   31 May 2014 TAQ - Created the file.
 *
 * Things to do
 *   - Figure out a clean way to get the name information in here for
 *     the request_init call in wrap_request.  Not critical at the moment
 *     since my host doesn't have an easily-available libwrap, but should
 *     be handled at some point.
 *
 */

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <syslog.h>
#ifdef HAVE_LIBWRAP
#include <tcpd.h>
#endif

#include "console.h"

InetConsole::InetConsole(u_int16_t port, struct addrinfo *ai)
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
	syslog(LOG_ERR, "socket creation failed for console port %d: %s",
	       port_num, strerror(errno));
	throw errno;
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
	    syslog(LOG_ERR,
		   "can't open console port %d as non-root user",
		   this->port_num);
            close(this->console_sock);
	    throw EACCES;
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
	syslog(LOG_ERR,
               "bind failed for console port %d: %s (%d)",
	       this->port_num, strerror(errno), errno);
	close(this->console_sock);
	throw errno;
    }
    /* Restore the original euid and egid of the process, if necessary. */
    if (do_uid)
    {
	seteuid(uid);
	setegid(gid);
    }

    if (listen(this->console_sock, 5) < 0)
    {
	syslog(LOG_ERR,
               "listen failed for console port %d: %s",
	       this->port_num, strerror(errno));
	close(this->console_sock);
	throw errno;
    }

    syslog(LOG_DEBUG,
           "console socket %d created on port %d",
           this->console_sock, this->port_num);
}

void *InetConsole::listener(void *arg)
{
    InetConsole *con = (InetConsole *)arg;
    int newsock;
    struct sockaddr_storage ss;
    socklen_t ss_len = sizeof(ss);
    char address[INET6_ADDRSTRLEN];
    ConsoleSession *sess = NULL;

    while ((newsock = accept(con->console_sock,
                             reinterpret_cast<struct sockaddr *>(&ss),
                             &ss_len)) != -1)
    {
        void *addr;

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
        inet_ntop(ss.ss_family, addr, address, sizeof(address));

        if (con->wrap_request(newsock))
        {
            syslog(LOG_NOTICE,
                   "began console session on %d from %s (%d)",
                   con->port_num,
                   address,
                   con->port_num);
            try
            {
                sess = new ConsoleSession(newsock);
            }
            catch (int e)
            {
                close(newsock);
                delete sess;
                continue;
            }
            con->sessions.push_back(sess);
        }
        else
        {
            syslog(LOG_NOTICE,
                   "console session on %d from %s failed wrapper check",
                   con->port_num, address);
            close(newsock);
        }
        ss_len = sizeof(ss);
    }
    pthread_exit(NULL);
    return NULL;
}

int InetConsole::wrap_request(int sock)
{
#ifdef HAVE_LIBWRAP
    struct request_info req;

    request_init(&req,
		 RQ_FILE, sock,
		 RQ_DAEMON, config.log_prefix, NULL);
    fromhost(&req);
    return hosts_access(&req);
#else
    return 1;
#endif
}
