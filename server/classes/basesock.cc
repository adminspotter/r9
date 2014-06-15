/* basesock.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 15 Jun 2014, 08:53:38 tquirk
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
 * This file contains the socket creation routine and the listening
 * thread handling.  This is directly instantiable, and only needs a
 * requirement-specific listening routine to be fully usable.
 *
 * Changes
 *   13 Sep 2007 TAQ - Created the file from ../sockets.c.  Moved blank
 *                     constructor and destructor in here.
 *   13 Oct 2007 TAQ - Cleaned up some debugging info.
 *   22 Nov 2009 TAQ - Fixed const char warnings in create_socket.
 *   14 Jun 2014 TAQ - Moved the guts of the new socket into here, so this
 *                     can be used wherever we need a listening socket.
 *                     Added the base_user and listen_socket base classes.
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
#include <syslog.h>

#include "basesock.h"

#include "../config.h"

basesock::basesock(struct addrinfo *ai, u_int16_t port)
{
    this->port_num = port;
    this->listen_arg = NULL;
    try
    {
        this->create_socket(ai);
    }
    catch (int e)
    {
        throw;
    }
}

basesock::~basesock()
{
    try
    {
        this->stop();
    }
    catch (int e)
    {
        /* Is there anything to do here? */
    }
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
    const char *typestr = (ai->ai_socktype == SOCK_DGRAM ? "dgram" : "stream");

    if ((this->sock = socket(ai->ai_family,
                             ai->ai_socktype,
                             ai->ai_protocol)) < 0)
    {
	syslog(LOG_ERR,
	       "socket creation failed for %s port %d: %s (%d)",
	       typestr, this->port_num, strerror(errno), errno);
        this->sock = 0;
	throw errno;
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
        throw EINVAL;
    }

    /* If root's gotta open the port, become root, if possible. */
    if (do_uid)
    {
	if (getuid() != 0)
	{
	    syslog(LOG_ERR,
		   "can't open %s port %d as non-root user",
		   typestr, this->port_num);
	    close(this->sock);
            this->sock = 0;
	    throw EACCES;
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
	syslog(LOG_ERR,
	       "bind failed for %s port %d: %s (%d)",
	       typestr, this->port_num, strerror(errno), errno);
	close(this->sock);
        this->sock = 0;
        throw errno;
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
	    syslog(LOG_ERR,
		   "listen failed for %s port %d: %s (%d)",
		   typestr, this->port_num, strerror(errno), errno);
	    close(this->sock);
            this->sock = 0;
            throw errno;
	}
    }

    syslog(LOG_DEBUG,
	   "created %s socket %d on port %d",
	   typestr, this->sock, this->port_num);
}

void basesock::start(void *(*func)(void *))
{
    int ret;

    if (this->sock == 0)
    {
        syslog(LOG_ERR,
               "no socket available to listen for port %d",
               this->port_num);
        throw ENOENT;
    }
    if ((ret = pthread_create(&(this->listen_thread),
                              NULL,
                              func,
                              this->listen_arg)) != 0)
    {
        syslog(LOG_ERR,
               "couldn't start listen thread for port %d: %s (%d)",
               this->port_num, strerror(ret), ret);
        throw ret;
    }
}

void basesock::stop(void)
{
    int ret;

    if ((ret = pthread_cancel(this->listen_thread)) != 0)
    {
        syslog(LOG_ERR,
               "couldn't cancel listen thread for port %d: %s (%d)",
               this->port_num, strerror(ret), ret);
        throw ret;
    }
    sleep(0);
    if ((ret = pthread_join(this->listen_thread, NULL)) != 0)
    {
        syslog(LOG_ERR,
               "couldn't join listen thread for port %d: %s (%d)",
               this->port_num, strerror(ret), ret);
        throw ret;
    }
}

base_user::base_user(u_int64_t u, Control *c)
{
    this->init(u, c);
}

base_user::~base_user()
{
}

void base_user::init(u_int64_t u, Control *c)
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

listen_socket::listen_socket(struct addrinfo *ai, u_int16_t port)
    : users(), sock(ai, port)
{
    this->init();
}

listen_socket::~listen_socket()
{
    int retval;
    std::map<u_int64_t, base_user *>::iterator i;

    if ((retval = pthread_cancel(this->reaper)) != 0)
    {
        syslog(LOG_ERR,
               "couldn't cancel reaper thread for port %d: %s (%d)",
               this->sock.port_num, strerror(retval), retval);
    }
    sleep(0);
    if ((retval = pthread_join(this->reaper, NULL)) != 0)
	syslog(LOG_ERR,
	       "error terminating reaper thread for port %d: %s (%d)",
	       this->sock.port_num, strerror(retval), retval);

    /* Clear out the users map */
    for (i = this->users.begin(); i != this->users.end(); ++i)
        delete (*i).second;
    this->users.erase(this->users.begin(), this->users.end());
}

void listen_socket::init(void)
{
    this->send = new ThreadPool<packet_list>("send", config.send_threads);
}

base_user *listen_socket::logout_user(u_int64_t userid)
{
    if (users.find(userid) != users.end())
    {
        users[userid]->pending_logout = true;
        return users[userid];
    }
    return NULL;
}
