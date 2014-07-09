/* basesock.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 09 Jul 2014, 13:24:27 trinityquirk
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
 *   21 Jun 2014 TAQ - Converted syslog to new style stream log.
 *   01 Jul 2014 TAQ - Moved the access thread pool into the listen_socket.
 *                     Added a stop method as well.
 *   05 Jul 2014 TAQ - The zone_interface has gone away, moved to server.h.
 *   09 Jul 2014 TAQ - We're now throwing std::runtime_error instead of a
 *                     bunch of random stuff (int, std::string, etc.).  There
 *                     are a couple of instances of exception-worthy errors
 *                     within the listen_socket destructor - is it valid to
 *                     throw exceptions out of a destructor?
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

#include "../server.h"
#include "../config.h"
#include "../log.h"

basesock::basesock(struct addrinfo *ai, u_int16_t port)
{
    this->port_num = port;
    this->listen_arg = NULL;
    this->create_socket(ai);
}

basesock::~basesock()
{
    try { this->stop(); }
    catch (...) { /* Nothing to do here */ }
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

    this->stop();

    if ((retval = pthread_cancel(this->reaper)) != 0)
    {
        std::clog << syslogErr << "couldn't cancel reaper thread for port "
                  << this->sock.port_num << ": "
                  << strerror(retval) << " (" << retval << ")" << std::endl;
    }
    sleep(0);
    if ((retval = pthread_join(this->reaper, NULL)) != 0)
	std::clog << syslogErr << "error terminating reaper thread for port "
                  << this->sock.port_num << ": "
                  << strerror(retval) << " (" << retval << ")" << std::endl;

    /* Clear out the users map */
    for (i = this->users.begin(); i != this->users.end(); ++i)
        delete (*i).second;
    this->users.erase(this->users.begin(), this->users.end());

    if (this->send_pool != NULL)
        delete this->send_pool;

    if (this->access_pool != NULL)
        delete this->access_pool;
}

void listen_socket::init(void)
{
    this->send_pool = new ThreadPool<packet_list>("send", config.send_threads);
    this->access_pool = new ThreadPool<access_list>("access",
                                                    config.access_threads);
    this->access_pool->clean_on_pop = true;
}

void listen_socket::stop(void)
{
    this->send_pool->stop();
    this->access_pool->stop();
    this->sock.stop();
}

void *listen_socket::access_pool_worker(void *arg)
{
    listen_socket *ls = (listen_socket *)arg;
    access_list req;

    std::clog << "started access pool worker";
    for (;;)
    {
        ls->access_pool->pop(&req);

	ntoh_packet(&(req.buf), sizeof(packet));

	if (req.buf.basic.type == TYPE_LOGREQ)
	    ls->login_user(req);
	else if (req.buf.basic.type == TYPE_LGTREQ)
	    ls->logout_user(req);
	/* Otherwise, we don't recognize it, and will ignore it */
    }
    std::clog << "access pool worker ending";
    return NULL;
}

void listen_socket::login_user(access_list& p)
{
    u_int64_t userid = 0LL;
    std::string username(p.buf.log.username, sizeof(p.buf.log.username));
    std::string password(p.buf.log.password, sizeof(p.buf.log.password));

    userid = database->check_authentication(username, password);

    /* Don't want to keep passwords around in core if we can help it */
    memset(p.buf.log.password, 0, sizeof(p.buf.log.password));
    password.clear();

    std::clog << "login request from "
              << p.buf.log.username << " (" << userid << ")" << std::endl;
    if (userid != 0LL)
    {
	if (this->users.find(userid) == this->users.end())
	{
	    Control *newcontrol = new Control(userid, NULL);
	    newcontrol->username = username;

	    /* Add this user to the userlist */
            this->do_login(userid, newcontrol, p);
            newcontrol->parent = (void *)(this->send_pool);

            std::clog << "logged in user "
                      << newcontrol->username
                      << " (" << userid << ")" << std::endl;

            /* Send an ack packet, to let the user know they're in */
            newcontrol->send_ack(TYPE_LOGREQ);
	}
    }
    /* Otherwise, do nothing, and send nothing */
}

void listen_socket::logout_user(access_list& p)
{
    std::map<u_int64_t, base_user *>::iterator found;

    /* Most of this function is now handled by the reaper threads */
    if ((found = this->users.find(p.what.logout.who)) != this->users.end())
    {
        base_user *bu = found->second;
        bu->pending_logout = true;
	std::clog << "logout request from "
                  << bu->control->username
                  << " (" << bu->control->userid << ")" << std::endl;
	bu->control->send_ack(TYPE_LGTREQ);
    }
}
