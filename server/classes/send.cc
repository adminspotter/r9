/* send.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 20 May 2014, 18:31:05 tquirk
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
 * This file contains the packet-sending thread pool worker routine, and all
 * other related support routines.
 *
 * Changes
 *   11 Aug 2006 TAQ - Created the file.
 *   23 Aug 2007 TAQ - We expect to get the socket passed into us now; once
 *                     we move this into the subservers, it'll make a whole
 *                     lot more sense.
 *   05 Sep 2007 TAQ - Made this a bit more general, since each socket will
 *                     have its own sending pool.  Added create_send_pool,
 *                     destroy_send_pool, and add_send_request funcs, for
 *                     external C binding.
 *   08 Sep 2007 TAQ - Generalized things so that we have datagram- and
 *                     stream-specific thread workers, and will start one
 *                     or the other based on an arg to create_send_pool.
 *   13 Sep 2007 TAQ - Removed static-ness from pool_worker routines.
 *   17 Sep 2007 TAQ - Reformatted debugging output.
 *   18 Sep 2007 TAQ - Added more debugging.
 *   23 Sep 2007 TAQ - Constructor and push() methods of Thread Pool changed.
 *   07 Oct 2007 TAQ - Cleaned up excessive debugging output.
 *   20 May 2014 TAQ - Packets are actually sent now.
 *
 * Things to do
 *
 * $Id$
 */

#include <arpa/inet.h>
#include <errno.h>

#include "zone.h"
#include "zone_interface.h"
#include "dgram.h"
#include "stream.h"

#include "../config.h"

void *dgram_send_pool_worker(void *);
void *stream_send_pool_worker(void *);

void *create_send_pool(void *parent, int type)
{
    ThreadPool<packet_list> *send_pool = NULL;

    /* Weed out any idiot callers before we even get going */
    if (type != SOCK_DGRAM && type != SOCK_STREAM)
    {
	syslog(LOG_ERR,
	       "couldn't start send worker, unknown socket type %d",
	       type);
	errno = EINVAL;
	return NULL;
    }

    try
    {
	send_pool = new ThreadPool<packet_list>("send", config.send_threads);
    }
    catch (int error_val)
    {
	errno = error_val;
	return NULL;
    }

    sleep(0);
    send_pool->startup_arg = parent;
    try
    {
	if (type == SOCK_DGRAM)
	    send_pool->start(&dgram_send_pool_worker);
	else if (type == SOCK_STREAM)
	    send_pool->start(&stream_send_pool_worker);
    }
    catch (int error_val)
    {
	delete send_pool;
	errno = error_val;
	return NULL;
    }
    return (void *)send_pool;
}

void destroy_send_pool(void *pool)
{
    delete (ThreadPool<packet_list> *)pool;
}

void add_send_request(void *pool, u_int64_t who, packet *pkt, size_t size)
{
    packet_list pl;

    memcpy(&pl.buf, pkt, size);
    pl.who = who;
    ((ThreadPool<packet_list> *)pool)->push(pl);
}

void *dgram_send_pool_worker(void *arg)
{
    packet_list req;
    size_t realsize;
    dgram_socket *parent = (dgram_socket *)arg;

    syslog(LOG_DEBUG,
	   "started send pool worker for datagram port %d",
	   parent->port);
    for (;;)
    {
	/* Grab the packet to send off the queue */
	parent->send->pop(&req);

	/* Process the request */
	realsize = packet_size(&req.buf);
	if (hton_packet(&req.buf, realsize))
	{
	    /* Fetch the user entry from the userlist */
	    dgram_user &user = parent->users[req.who];
	    /* Encryption will happen right here */
	    /* Send it on out */
	    if (sendto(parent->sock,
		       (void *)&req.buf, realsize, 0,
		       (struct sockaddr *)&user.sin.sin,
		       sizeof(struct sockaddr_in)) == -1)
		syslog(LOG_ERR,
		       "error sending packet out datagram port %d: %s",
		       parent->port, strerror(errno));
	    else
		syslog(LOG_DEBUG, "sent a packet of type %d to %s:%d",
		       req.buf.basic.type,
		       inet_ntoa(user.sin.sin.sin_addr),
		       ntohs(user.sin.sin.sin_port));
	}
    }
    syslog(LOG_DEBUG,
	   "exiting send pool worker for datagram port %d",
	   parent->port);
    return NULL;
}

void *stream_send_pool_worker(void *arg)
{
    packet_list req;
    size_t realsize;
    stream_socket *parent = (stream_socket *)arg;

    syslog(LOG_DEBUG,
	   "started send pool worker for stream port %d",
	   parent->port);
    for (;;)
    {
	/* Grab the packet to send off the queue */
	parent->send->pop(&req);

	/* Process the request */
	realsize = packet_size(&req.buf);
	if (hton_packet(&req.buf, realsize))
	{
	    /* Fetch the user entry from the userlist */
	    stream_user &user = parent->users[req.who];
	    /* Encryption will happen right here */
	    /* Send it on out */
            user.control->send(&(req.buf));
	}
    }
    syslog(LOG_DEBUG,
	   "exiting send pool worker for stream port %d",
	   parent->port);
    return NULL;
}
