/* dgram.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 15 Jun 2014, 09:38:00 tquirk
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
 * This file contains the implementation of the datagram socket object.
 *
 * Changes
 *   08 Sep 2007 TAQ - Created the file from the ashes of udpserver.c.
 *   09 Sep 2007 TAQ - A few minor cleanups, moved some initialization out
 *                     of listen() to the constructor.  Added operator
 *                     members to the dgram_user.
 *   13 Sep 2007 TAQ - Removed server.h include.  Used basesock's static
 *                     create_socket instead of C version.
 *   16 Sep 2007 TAQ - Added some processing of input packets.
 *   17 Sep 2007 TAQ - Added another index to convert sockaddr_in to
 *                     an entry in the user list.
 *   23 Sep 2007 TAQ - Constructor and push() methods of ThreadPool changed.
 *   02 Dec 2007 TAQ - Added reaping of logged out users, via the new
 *                     pending_logout member of the user structure.
 *   15 Dec 2007 TAQ - Trying to debug why pinging doesn't seem to work
 *                     correctly.  Updated copy constructor of dgram_user
 *                     to include all members.
 *   16 Dec 2007 TAQ - Renamed reaper to dgram_reaper_worker.  Minor syntax
 *                     cleanups.
 *   19 Sep 2013 TAQ - Return NULL at the end of the worker routine to quiet
 *                     gcc.
 *   11 May 2014 TAQ - We've moved the motion- and position-related parameters
 *                     out of the GameObject and into the Motion object, so
 *                     some pointers point at different things.
 *   14 Jun 2014 TAQ - Lots of restructuring of base classes.
 *   15 Jun 2014 TAQ - Moved the send worker in here.
 *
 * Things to do
 *   - We might need to have a mutex on the socket, since we'll probably
 *     be trying to read from and write to it at the same time.
 *   - We should probably have a mutex around the users and socks members
 *     since we use those in a few different places.
 *
 * $Id$
 */

#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <errno.h>

#include "dgram.h"

#include "zone.h"
#include "zone_interface.h"

extern volatile int main_loop_exit_flag;

dgram_user::dgram_user(u_int64_t u, Control *c)
    : base_user(u, c), sin()
{
}

const dgram_user& dgram_user::operator=(const dgram_user& du)
{
    this->sin = du.sin;
    this->base_user::operator=(du);
    return *this;
}

dgram_socket::dgram_socket(struct addrinfo *ai, u_int16_t port)
    : listen_socket(ai, port)
{
}

dgram_socket::~dgram_socket()
{
    /* Should we send logout messages to everybody? */

    /* Stop the sending thread pool */
    if (this->send != NULL)
	delete this->send;
}

void dgram_socket::start(void)
{
    int retval;

    syslog(LOG_DEBUG,
	   "starting connection loop for datagram port %d",
	   this->sock.port_num);

    /* Start up the sending thread pool */
    sleep(0);
    this->send->startup_arg = (void *)this;
    try
    {
	this->send->start(dgram_send_worker);
    }
    catch (int e)
    {
	syslog(LOG_ERR,
	       "couldn't start send pool for datagram port %d: %s (%d)",
	       this->sock.port_num, strerror(e), e);
        throw;
    }

    /* Start up the reaping thread */
    if ((retval = pthread_create(&this->reaper, NULL,
				 dgram_reaper_worker, (void *)this)) != 0)
    {
	syslog(LOG_ERR,
	       "couldn't create reaper thread for datagram port %d: %s (%d)",
	       this->sock.port_num, strerror(retval), retval);
        throw retval;
    }

    this->sock.listen_arg = (void *)this;
    this->sock.start(dgram_socket::dgram_listen_worker);
}

base_user *dgram_socket::login_user(u_int64_t userid,
                                    Control *con,
                                    access_list& al)
{
    dgram_user *dgu = new dgram_user(userid, con);
    dgu->sin = al.what.login.who.dgram;
    users[userid] = dgu;
    socks[al.what.login.who.dgram] = dgu;
    return users[userid];
}

void *dgram_socket::dgram_listen_worker(void *arg)
{
    dgram_socket *dgs = (dgram_socket *)arg;
    int len;
    packet buf;
    Sockaddr from;
    socklen_t fromlen;
    u_int64_t userid;
    std::map<u_int64_t, dgram_user *>::iterator i;
    access_list a;
    packet_list p;

    /* Do the receiving part */
    for (;;)
    {
	if (main_loop_exit_flag == 1)
	    break;

        pthread_testcancel();
        /* We HAVE to pass in a proper size value in fromlen */
        fromlen = sizeof(struct sockaddr_in6);
        /* Will this be interrupted by a pthread_cancel? */
        len = recvfrom(dgs->sock.sock,
                       (void *)&buf,
                       sizeof(packet),
                       0,
                       (struct sockaddr *)&from.sin, &fromlen);
        pthread_testcancel();

        /* Figure out who sent this packet */
        if (dgs->socks.find(from) != dgs->socks.end())
            userid = dgs->socks[from]->userid;

        /* Do something with whatever we got */
        switch (buf.basic.type)
        {
          case TYPE_ACKPKT:
            /* Acknowledgement packet */
            syslog(LOG_DEBUG, "got an ack packet");
            dgs->users[userid]->timestamp = time(NULL);
            break;

          case TYPE_LOGREQ:
            /* Login request */
            syslog(LOG_DEBUG, "got a login packet");
            memcpy((unsigned char *)&a.buf, (unsigned char *)&buf, len);
            a.parent = dgs;
            memcpy((unsigned char *)&a.what.login.who.dgram,
                   (unsigned char *)&from,
                   sizeof(struct sockaddr_in));
            access_pool->push(a);
            break;

          case TYPE_LGTREQ:
            /* Logout request */
            syslog(LOG_DEBUG, "got a logout packet");
            dgs->users[userid]->timestamp = time(NULL);
            memcpy((unsigned char *)&a.buf, (unsigned char *)&buf, len);
            a.parent = dgs;
            a.what.logout.who = userid;
            access_pool->push(a);
            break;

          case TYPE_ACTREQ:
            /* Action request */
            syslog(LOG_DEBUG, "got an action request packet");
            dgs->users[userid]->timestamp = time(NULL);
            memcpy((unsigned char *)&p.buf, (unsigned char *)&buf, len);
            p.who = userid;
            zone->action_pool->push(p);
            break;

          default:
            break;
        }
        pthread_testcancel();
    }
    syslog(LOG_DEBUG,
	   "exiting connection loop for datagram port %d",
	   dgs->sock.port_num);
    return NULL;
}

void *dgram_socket::dgram_reaper_worker(void *arg)
{
    dgram_socket *dgs = (dgram_socket *)arg;
    std::map<u_int64_t, base_user *>::iterator i;
    dgram_user *dgu;
    time_t now;

    syslog(LOG_DEBUG,
	   "started reaper thread for datagram port %d",
	   dgs->sock.port_num);
    for (;;)
    {
	sleep(listen_socket::REAP_TIMEOUT);
	now = time(NULL);
	for (i = dgs->users.begin(); i != dgs->users.end(); ++i)
	{
	    pthread_testcancel();
            dgu = dynamic_cast<dgram_user *>((*i).second);
	    if (dgu->timestamp < now - listen_socket::LINK_DEAD_TIMEOUT)
	    {
		/* We'll consider the user link-dead */
		syslog(LOG_DEBUG,
		       "removing user %s (%llu) from datagram port %d",
		       dgu->control->username.c_str(),
                       dgu->userid,
		       dgs->sock.port_num);
		if (dgu->control->slave != NULL)
		{
		    /* Clean up a user who has logged out */
		    dgu->control->slave->object->natures["invisible"] = 1;
		    dgu->control->slave->object->natures["non-interactive"] = 1;
		}
		pthread_mutex_lock(&active_users_mutex);
		active_users->erase(dgu->userid);
		pthread_mutex_unlock(&active_users_mutex);
		delete dgu->control;
		dgs->socks.erase(dgu->sin);
		dgs->users.erase((*(i--)).second->userid);
	    }
	    else if (dgu->timestamp < now - listen_socket::PING_TIMEOUT
		     && dgu->pending_logout == false)
		dgu->control->send_ping();
	}
	pthread_testcancel();
    }
    return NULL;
}

void *dgram_socket::dgram_send_worker(void *arg)
{
    dgram_socket *dgs = (dgram_socket *)arg;
    dgram_user *dgu;
    packet_list req;
    size_t realsize;

    syslog(LOG_DEBUG,
	   "started send pool worker for datagram port %d",
	   dgs->sock.port_num);
    for (;;)
    {
	dgs->send->pop(&req);

	realsize = packet_size(&req.buf);
	if (hton_packet(&req.buf, realsize))
	{
	    dgu = dynamic_cast<dgram_user *>(dgs->users[req.who]);
            if (dgu == NULL)
                continue;
	    /* TODO: Encryption */
	    if (sendto(dgs->sock.sock,
		       (void *)&req.buf, realsize, 0,
		       (struct sockaddr *)&(dgu->sin.sin),
		       sizeof(struct sockaddr_in)) == -1)
		syslog(LOG_ERR,
		       "error sending packet out datagram port %d: %s",
		       dgs->sock.port_num, strerror(errno));
	    else
		syslog(LOG_DEBUG, "sent a packet of type %d to %s:%d",
		       req.buf.basic.type,
		       inet_ntoa(dgu->sin.sin.sin_addr),
		       ntohs(dgu->sin.sin.sin_port));
	}
    }
    syslog(LOG_DEBUG,
	   "exiting send pool worker for datagram port %d",
	   dgs->sock.port_num);
    return NULL;
}
