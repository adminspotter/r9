/* comm.c
 *   by Trinity Quirk <trinity@ymb.net>
 *   last updated 29 Nov 2009, 17:01:57 trinity
 *
 * Revision IX game client
 * Copyright (C) 2007  Trinity Annabelle Quirk
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
 * This file contains the communication callbacks and various dispatch
 * and data-handling routines.
 *
 * 30 Jul 2006
 * It doesn't seem like the XtInput stuff works for reading UDP sockets.
 * We are getting stuff sent to us, but the read routine is never apparently
 * being invoked.  It sounds like we probably have to select to see if there
 * is anything waiting to be read, which defeats the whole purpose of what
 * I was trying to do (not block the program from running the event loop,
 * while also not doing a busy-wait).
 *
 * After a little thinking, this might work better as two threads, one for
 * sending and one for receiving.  We'll need to work out how to not collide
 * on trying to use the socket(s).  One mutex for the send queue, a cond
 * for the queue, and a mutex for the sockets themselves?  That sounds like
 * it should work fine.
 *
 * Now that I consider it a bit, we may not actually need the socket
 * lock; we can just set blocking reads and writes.  Or at the very
 * least we'll have to come up with a good deadlock prevention
 * strategy.
 *
 * Changes
 *   30 Jan 1999 TAQ - Created the file.
 *   22 May 1999 TAQ - Instead of constantly allocating and freeing memory
 *                     to hold our send buffers, we have a ring buffer
 *                     with constant-sized elements.
 *   18 Jul 2006 TAQ - Made the send structure hold a packet instead of a
 *                     generic character buffer.  Added GPL notice.
 *   26 Jul 2006 TAQ - Renamed some of the routines.  Fleshed out the
 *                     receive routine, and actually make some calls out of
 *                     it now.  As it stands, this program should actually
 *                     work with the limited feature set I have written; now
 *                     I just need to get the server working enough.
 *   30 Jul 2006 TAQ - Got the server working enough to send things to us,
 *                     and we're not getting them.  We need to do separate
 *                     threads for comm handling.  Made the sending queue
 *                     dynamically resizeable, because statically sized queues
 *                     are like putting a gun to your head.
 *   04 Aug 2006 TAQ - Some of the prototypes for geometry and texture updates
 *                     changed.
 *   09 Aug 2006 TAQ - Removed references to geometry and texture packets,
 *                     since we're no longer handling those in-band.
 *   10 Aug 2006 TAQ - Added some who-is-it checking on receipt of a packet.
 *   13 Oct 2007 TAQ - Added some better debugging output.
 *   15 Dec 2007 TAQ - Worked on responding to ping packets.
 *
 * Things to do
 *
 * $Id: comm.c 10 2013-01-25 22:13:00Z trinity $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>

#include "client.h"
#include "proto.h"

static void send_data(unsigned char *, size_t);
static void *send_worker(void *);
static void *recv_worker(void *);

static pthread_t send_thread, recv_thread;
static pthread_mutex_t send_queue_lock;
static pthread_cond_t send_queue_not_empty;
static packet *send_queue;
static int sock = -1, size, head, tail, thread_exit_flag = 0;
static struct sockaddr_in remote;
static u_int64_t sequence = 0LL;

void send_login(char *user, char *pass,
		u_int64_t userid, u_int64_t charid)
{
    if (sock != -1)
    {
	packet req;

	memset((void *)&(req.log), 0, sizeof(login_request));
	req.log.type = TYPE_LOGREQ;
	req.log.version = 1;
	req.log.sequence = sequence++;
	memcpy(req.log.username, user, sizeof(req.log.username));
	memcpy(req.log.password, pass, sizeof(req.log.password));
	send_data((unsigned char *)&req, sizeof(login_request));
    }
}

void send_action_request(u_int16_t actionid, u_int8_t power)
{
    if (sock != -1)
    {
	packet req;

	req.act.type = TYPE_ACTREQ;
	req.act.version = 1;
	req.act.sequence = sequence++;
	req.act.action_id = actionid;
	req.act.power_level = power;
	send_data((unsigned char *)&req, sizeof(action_request));
    }
}

void send_logout(u_int64_t userid, u_int64_t charid)
{
    if (sock != -1)
    {
	packet req;

	req.lgt.type = TYPE_LGTREQ;
	req.lgt.version = 1;
	req.lgt.sequence = sequence++;
	send_data((unsigned char *)&req, sizeof(logout_request));
    }
}

void send_ack(u_int8_t type)
{
    if (sock != -1)
    {
	packet req;

	req.ack.type = TYPE_ACKPKT;
	req.ack.version = 1;
	req.ack.sequence = sequence++;
	req.ack.request = type;
	send_data((unsigned char *)&req, sizeof(ack_packet));
    }
}

void start_comm(void)
{
    char errstr[256];
    int ret;

    /* Set up the sending queue */
    size = TX_RING_ELEMENTS;
    head = tail = 0;
    if ((send_queue = (packet *)malloc(sizeof(packet) * size)) == NULL)
    {
	snprintf(errstr, sizeof(errstr),
		 "Couldn't allocate send queue: %s", strerror(errno));
	main_post_message(errstr);
	return;
    }

    /* Init the mutex and cond variables */
    if ((ret = pthread_mutex_init(&send_queue_lock, NULL)) != 0)
    {
	snprintf(errstr, sizeof(errstr),
		 "Couldn't init queue mutex: %s", strerror(ret));
	main_post_message(errstr);
	free(send_queue);
	return;
    }
    if ((ret = pthread_cond_init(&send_queue_not_empty, NULL)) != 0)
    {
	snprintf(errstr, sizeof(errstr),
		 "Couldn't init queue not-empty cond: %s", strerror(ret));
	main_post_message(errstr);
	pthread_mutex_destroy(&send_queue_lock);
	free(send_queue);
	return;
    }

    create_socket(&config.server_addr, config.server_port);

    /* Now start up the actual threads */
    if ((ret = pthread_create(&send_thread, NULL, send_worker, NULL)) != 0)
    {
	snprintf(errstr, sizeof(errstr),
		 "Couldn't start send thread: %s", strerror(ret));
	main_post_message(errstr);
	pthread_cond_destroy(&send_queue_not_empty);
	pthread_mutex_destroy(&send_queue_lock);
	free(send_queue);
	return;
    }
    if ((ret = pthread_create(&recv_thread, NULL, recv_worker, NULL)) != 0)
    {
	snprintf(errstr, sizeof(errstr),
		 "Couldn't start recv thread: %s", strerror(ret));
	main_post_message(errstr);
	pthread_cancel(send_thread);
	pthread_join(send_thread, NULL);
	pthread_cond_destroy(&send_queue_not_empty);
	pthread_mutex_destroy(&send_queue_lock);
	free(send_queue);
	return;
    }
}

void cleanup_comm(void)
{
    if (sock != -1)
	send_logout(1LL, 1LL);
    thread_exit_flag = 1;
    pthread_cond_broadcast(&send_queue_not_empty);
}

/* This routine takes the address and port args in NETWORK ORDER */
void create_socket(struct in_addr *addr, u_int16_t port)
{
    char errstr[256];
    struct protoent *proto_ent;

    if ((proto_ent = getprotobyname("udp")) == NULL)
    {
	snprintf(errstr, sizeof(errstr), "Couldn't find protocol 'udp'");
	main_post_message(errstr);
	return;
    }
    if ((sock = socket(PF_INET, SOCK_DGRAM, proto_ent->p_proto)) == -1)
    {
	snprintf(errstr, sizeof(errstr),
		 "Couldn't open socket: %s", strerror(errno));
	main_post_message(errstr);
	return;
    }
    remote.sin_family = AF_INET;
    remote.sin_port = port;
    remote.sin_addr.s_addr = addr->s_addr;
}

static void send_data(unsigned char *buffer, size_t length)
{
    char errstr[256];

    pthread_mutex_lock(&send_queue_lock);

    if ((head == 0 && tail == (size - 1)) || tail == (head - 1))
    {
	snprintf(errstr, sizeof(errstr), "Ran out of transmit buffers");
	main_post_message(errstr);
	return;
    }
    if (!hton_packet((packet *)buffer, length))
    {
	snprintf(errstr, sizeof(errstr), "Error hton'ing packet");
	main_post_message(errstr);
	return;
    }
    memcpy((void *)(&send_queue[tail]),
	   buffer,
	   packet_size((packet *)buffer));
    if (++tail == size)
	tail = 0;

    pthread_cond_signal(&send_queue_not_empty);
    pthread_mutex_unlock(&send_queue_lock);
}

/* ARGSUSED */
static void *send_worker(void *notused)
{
    for (;;)
    {
	pthread_mutex_lock(&send_queue_lock);

	while (head == tail && !thread_exit_flag)
	    pthread_cond_wait(&send_queue_not_empty, &send_queue_lock);

	if (thread_exit_flag)
	{
	    pthread_mutex_unlock(&send_queue_lock);
	    pthread_exit(NULL);
	}

	/* Should we try to send the entire queue contents here, if there's
	 * more than just one packet?
	 */
	sendto(sock,
	       (void *)(&send_queue[head]), packet_size(&send_queue[head]),
	       0,
	       (struct sockaddr *)&remote, sizeof(struct sockaddr_in));
	fprintf(stderr,
		"sent a packet of type %d to %s:%d\n",
		send_queue[head].basic.type, inet_ntoa(remote.sin_addr),
		ntohs(remote.sin_port));
	if (++head == size)
	    head = 0;
	pthread_mutex_unlock(&send_queue_lock);
    }
}

/* ARGSUSED */
static void *recv_worker(void *notused)
{
    packet buf;
    struct sockaddr_in sin;
    socklen_t fromlen;
    char errstr[256];
    int len;

    for (;;)
    {
	fromlen = sizeof(struct sockaddr_in);
	len = recvfrom(sock, (void *)&buf, sizeof(packet), 0,
		       (struct sockaddr *)&sin, &fromlen);
	/* Verify that the sender is who we think it should be */
	if (memcmp(&sin, &remote, sizeof(struct sockaddr_in)))
	{
	    snprintf(errstr, sizeof(errstr),
		     "Got packet from unknown sender %s",
		     inet_ntoa(sin.sin_addr));
	    main_post_message(errstr);
	    continue;
	}

	fprintf(stderr,
		"got a packet of type %d from %s:%d\n",
		buf.basic.type, inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
	if (!ntoh_packet(&buf, len))
	{
	    snprintf(errstr, sizeof(errstr), "Error while ntoh'ing packet\n");
	    main_post_message(errstr);
	    continue;
	}
	/* We got a packet, now figure out what type it is and process it */
	switch (buf.basic.type)
	{
	  case TYPE_ACKPKT:
	    /* Acknowledgement packet */
	    switch (buf.ack.request)
	    {
	      case TYPE_LOGREQ:
		/* The response to our login request */
		snprintf(errstr, sizeof(errstr),
			 "Login response, type %d access", buf.ack.misc);
		main_post_message(errstr);
		break;

	      case TYPE_LGTREQ:
		/* The response to our logout request */
		snprintf(errstr, sizeof(errstr),
			 "Logout response, type %d access", buf.ack.misc);
		main_post_message(errstr);
		break;

	      default:
		snprintf(errstr, sizeof(errstr),
			 "Got an unknown ack packet: %d", buf.ack.request);
		main_post_message(errstr);
		break;
	    }
	    break;

	  case TYPE_POSUPD:
	    /* Position update */
	    fprintf(stderr, "Got a position update for object %lld\n",
		    buf.pos.object_id);
	    move_object(buf.pos.object_id,
			buf.pos.frame_number,
			(double)buf.pos.x_pos / 100.0,
			(double)buf.pos.y_pos / 100.0,
			(double)buf.pos.z_pos / 100.0,
			(double)buf.pos.x_orient / 100.0,
			(double)buf.pos.y_orient / 100.0,
			(double)buf.pos.z_orient / 100.0);
	    break;

	  case TYPE_SRVNOT:
	    /* Server notification */
	    fprintf(stderr, "Got a server notification packet, ignoring\n");
	    break;

	  case TYPE_PNGPKT:
	    fprintf(stderr, "Got a ping packet, responding\n");
	    send_ack(TYPE_PNGPKT);
	    break;

	  case TYPE_LOGREQ:
	  case TYPE_LGTREQ:
	  case TYPE_ACTREQ:
	  default:
	    snprintf(errstr, sizeof(errstr),
		     "Got an unexpected packet type: %d\n", buf.basic.type);
	    main_post_message(errstr);
	    break;
	}
    }
}
