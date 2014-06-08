/* dgram.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 12 May 2014, 21:25:51 tquirk
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

#include "dgram.h"
#include "zone_interface.h"
#include "../config.h"

extern volatile int main_loop_exit_flag;

extern void *dgram_send_pool_worker(void *);
static void *dgram_reaper_worker(void *);

bool dgram_user::operator<(const dgram_user& du) const
{
    return (this->userid < du.userid);
}

bool dgram_user::operator==(const dgram_user& du) const
{
    return (this->userid == du.userid);
}

const dgram_user& dgram_user::operator=(const dgram_user& du)
{
    this->userid = du.userid;
    this->control = du.control;
    this->timestamp = du.timestamp;
    this->sin = du.sin;
    this->pending_logout = du.pending_logout;
    return *this;
}

dgram_socket::dgram_socket(int p)
    : users()
{
    try
    {
	this->send = new ThreadPool<packet_list>("send", config.send_threads);
    }
    catch (int error_val)
    {
	errno = error_val;
	throw;
    }

    this->port = p;
    if ((this->sock = basesock::create_socket(SOCK_DGRAM, this->port)) == -1)
    {
	/* Maybe do something here?  Report the error somehow? */
	return;
    }
    FD_ZERO(&this->master_readfs);
    FD_SET(this->sock, &this->master_readfs);
}

dgram_socket::~dgram_socket()
{
    int retval;

    /* Should we send logout messages to everybody? */

    /* Terminate the reaping thread */
    pthread_cancel(this->reaper);
    sleep(0);
    if ((retval = pthread_join(this->reaper, NULL)) != 0)
	syslog(LOG_ERR,
	       "error terminating reaper thread in datagram port %d: %s",
	       this->port, strerror(retval));

    /* Close out the main socket */
    if (this->sock >= 0)
    {
	FD_CLR(this->sock, &this->master_readfs);
	close(this->sock);
    }

    /* Clear out the users map */
    this->users.erase(this->users.begin(), this->users.end());

    /* Stop the sending thread pool */
    if (this->send != NULL)
	delete this->send;
}

void dgram_socket::listen(void)
{
    int max_fd = this->sock + 1, retval, len;
    packet buf;
    Sockaddr from;
    socklen_t fromlen;
    u_int64_t userid;
    std::map<u_int64_t, dgram_user>::iterator i;
    access_list a;
    packet_list p;

    syslog(LOG_DEBUG,
	   "starting connection loop for datagram port %d",
	   this->port);

    /* Start up the sending thread pool */
    sleep(0);
    this->send->startup_arg = (void *)this;
    try
    {
	this->send->start(dgram_send_pool_worker);
    }
    catch (int error_val)
    {
	syslog(LOG_ERR,
	       "couldn't start send pool for datagram port %d: %s",
	       this->port, strerror(error_val));
	errno = error_val;
	return;
    }

    /* Start up the reaping thread */
    if ((retval = pthread_create(&this->reaper, NULL,
				 dgram_reaper_worker, (void *)this)) != 0)
    {
	syslog(LOG_ERR,
	       "couldn't create reaper thread for datagram port %d: %s",
	       this->port, strerror(retval));
	errno = retval;
	return;
    }

    /* Do the receiving part */
    for (;;)
    {
	if (main_loop_exit_flag == 1)
	    break;

	memcpy(&(this->readfs), &(this->master_readfs), sizeof(fd_set));

	if ((retval = select(max_fd, &(this->readfs), NULL, NULL, NULL)) == 0)
	    continue;
	else if (retval == -1)
	{
	    if (errno == EINTR)
	    {
		syslog(LOG_NOTICE,
		       "select interrupted by signal in datagram port %d",
		       this->port);
		continue;
	    }
	    else
	    {
		syslog(LOG_ERR,
		       "select error in datagram port %d: %s",
		       this->port, strerror(errno));
		/* Should we blow up or something here? */
	    }
	}

	/* We must have gotten something, so grab it */
	if (FD_ISSET(this->sock, &this->readfs))
	{
	    /* We HAVE to pass in a proper size value in fromlen */
	    fromlen = sizeof(struct sockaddr_in);
	    len = recvfrom(this->sock,
			   (void *)&buf,
			   sizeof(packet),
			   0,
			   (struct sockaddr *)&from.sin, &fromlen);

	    /* Figure out who sent this packet */
	    if (this->socks.find(from) != this->socks.end())
		userid = this->socks[from]->userid;

	    /* Do something with whatever we got */
	    switch (buf.basic.type)
	    {
	      case TYPE_ACKPKT:
		/* Acknowledgement packet */
		syslog(LOG_DEBUG, "got an ack packet");
		users[userid].timestamp = time(NULL);
		break;

	      case TYPE_LOGREQ:
		/* Login request */
		syslog(LOG_DEBUG, "got a login packet");
		memcpy((unsigned char *)&a.buf, (unsigned char *)&buf, len);
		a.parent = this;
		memcpy((unsigned char *)&a.what.login.who.dgram,
		       (unsigned char *)&from,
		       sizeof(struct sockaddr_in));
		access_pool->push(a);
		break;

	      case TYPE_LGTREQ:
		/* Logout request */
		syslog(LOG_DEBUG, "got a logout packet");
		users[userid].timestamp = time(NULL);
		memcpy((unsigned char *)&a.buf, (unsigned char *)&buf, len);
		a.parent = this;
		a.what.logout.who = userid;
		access_pool->push(a);
		break;

	      case TYPE_ACTREQ:
		/* Action request */
		syslog(LOG_DEBUG, "got an action request packet");
		users[userid].timestamp = time(NULL);
		memcpy((unsigned char *)&p.buf, (unsigned char *)&buf, len);
		p.who = userid;
		zone->action_pool->push(p);
		break;

	      default:
		break;
	    }
	}
    }
    syslog(LOG_DEBUG,
	   "exiting connection loop for datagram port %d",
	   this->port);
}

/* The reaper thread worker routine */
void *dgram_reaper_worker(void *arg)
{
    dgram_socket *sock = (dgram_socket *)arg;
    std::map<u_int64_t, dgram_user>::iterator i;
    time_t now;

    syslog(LOG_DEBUG,
	   "started reaper thread for datagram port %d",
	   sock->port);
    for (;;)
    {
	sleep(15);
	now = time(NULL);
	for (i = sock->users.begin(); i != sock->users.end(); ++i)
	{
	    pthread_testcancel();
	    if ((*i).second.timestamp < now - 75)
	    {
		/* After 75 seconds, we'll consider the user link-dead */
		syslog(LOG_DEBUG,
		       "removing user %s (%llu) from datagram port %d",
		       (*i).second.control->username.c_str(),
                       (*i).second.userid,
		       sock->port);
		if ((*i).second.control->slave != NULL)
		{
		    /* Clean up a user who has logged out */
		    (*i).second.control->slave->object->natures["invisible"] = 1;
		    (*i).second.control->slave->object->natures["non-interactive"] = 1;
		}
		pthread_mutex_lock(&active_users_mutex);
		active_users->erase((*i).second.userid);
		pthread_mutex_unlock(&active_users_mutex);
		delete (*i).second.control;
		sock->socks.erase((*i).second.sin);
		sock->users.erase((*(i--)).second.userid);
	    }
	    else if ((*i).second.timestamp < now - 30
		     && (*i).second.pending_logout == false)
		/* After 30 seconds, see if the user is still there */
		(*i).second.control->send_ping();
	}
	pthread_testcancel();
    }
    return NULL;
}

/* The C-linked thread startup routine.  arg points at the port number. */
void *start_dgram_socket(void *arg)
{
    dgram_socket *dgs = new dgram_socket(*(int *)arg);
    if (dgs != NULL)
    {
	dgs->listen();
	delete dgs;
    }
    return NULL;
}
