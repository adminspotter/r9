/* comm.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 26 Jul 2014, 14:33:13 tquirk
 *
 * Revision IX game client
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
 *   23 Jul 2014 TAQ - This is now a C++ file, and an actual class.
 *   26 Jul 2014 TAQ - Cleanups to compile.  Switched all the main_post_message
 *                     over to use std::clog.
 *
 * Things to do
 *
 */

#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

#include <iostream>
#include <sstream>
#include <stdexcept>

#include "r9client.h"
#include "comm.h"
#include "../proto/proto.h"

void Comm::create_socket(struct addrinfo *ai, u_int16_t port)
{
    if ((this->sock = socket(ai->ai_family,
                             ai->ai_socktype,
                             ai->ai_protocol)) < 0)
    {
        std::ostringstream s;
        s << "Couldn't open socket: "
          << strerror(errno) << " (" << errno << ")";
        throw std::runtime_error(s.str());
    }
    this->remote.ss_family = ai->ai_family;
    switch (ai->ai_family)
    {
      case AF_INET:
        {
            struct sockaddr_in *sin
                = reinterpret_cast<struct sockaddr_in *>(&(this->remote));
            sin->sin_port = htons(port);
            sin->sin_addr.s_addr = htonl(reinterpret_cast<struct sockaddr_in *>(ai->ai_addr)->sin_addr.s_addr);
            break;
        }

      case AF_INET6:
        {
            struct sockaddr_in6 *sin6
                = reinterpret_cast<struct sockaddr_in6 *>(&(this->remote));
            sin6->sin6_port = htons(port);
            memcpy(sin6->sin6_addr.s6_addr,
                   reinterpret_cast<struct sockaddr_in6 *>(ai->ai_addr)->sin6_addr.s6_addr,
                   sizeof(struct in6_addr));
            break;
        }

      default:
        throw std::runtime_error("Invalid address family for socket");
    }
}

void *Comm::send_worker(void *arg)
{
    Comm *comm = (Comm *)arg;

    for (;;)
    {
        pthread_mutex_lock(&(comm->send_lock));

        while (comm->send_queue.empty() && !Comm::thread_exit_flag)
            pthread_cond_wait(&(comm->send_queue_not_empty),
                              &(comm->send_lock));

        if (Comm::thread_exit_flag)
        {
            pthread_mutex_unlock(&(comm->send_lock));
            pthread_exit(NULL);
        }

        sendto(comm->sock,
               (void *)(&(comm->send_queue.front())),
               packet_size(&(comm->send_queue.front())),
               0,
               (struct sockaddr *)&(comm->remote),
               sizeof(struct sockaddr_storage));
        pthread_mutex_unlock(&(comm->send_lock));
    }
    return NULL;
}

void *Comm::recv_worker(void *arg)
{
    Comm *comm = (Comm *)arg;
    packet buf;
    struct sockaddr_storage sin;
    socklen_t fromlen;
    int len;

    for (;;)
    {
        fromlen = sizeof(struct sockaddr_storage);
        len = recvfrom(comm->sock, (void *)&buf, sizeof(packet), 0,
                       (struct sockaddr *)&sin, &fromlen);
        /* Verify that the sender is who we think it should be */
        if (memcmp(&sin, &(comm->remote), fromlen))
        {
            char addrstr[INET6_ADDRSTRLEN];
            std::clog << "Got packet from unknown sender ";
            inet_ntop(sin.ss_family,
                      sin.ss_family == AF_INET
                      ? (void *)&(reinterpret_cast<struct sockaddr_in *>(&sin)->sin_addr)
                      : (void *)&(reinterpret_cast<struct sockaddr_in6 *>(&sin)->sin6_addr),
                      addrstr, INET6_ADDRSTRLEN);
            std::clog << addrstr << std::endl;
            continue;
        }

        if (!ntoh_packet(&buf, len))
        {
            std::clog << "Error while ntoh'ing packet" << std::endl;
            continue;
        }
        comm->dispatch(buf);
    }
    return NULL;
}

void Comm::dispatch(packet& buf)
{
    /* We got a packet, now figure out what type it is and process it */
    switch (buf.basic.type)
    {
      case TYPE_ACKPKT:
        /* Acknowledgement packet */
        switch (buf.ack.request)
        {
          case TYPE_LOGREQ:
            /* The response to our login request */
            std::clog << "Login response, type " << buf.ack.misc << " access"
                      << std::endl;
            break;

          case TYPE_LGTREQ:
            /* The response to our logout request */
            std::clog << "Logout response, type " << buf.ack.misc << " access"
                      << std::endl;
            break;

          default:
            std::clog << "Got an unknown ack packet: " << buf.ack.request
                      << std::endl;
            break;
        }
        break;

      case TYPE_POSUPD:
        /* Position update */
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
        break;

      case TYPE_PNGPKT:
        this->send_ack(TYPE_PNGPKT);
        break;

      case TYPE_LOGREQ:
      case TYPE_LGTREQ:
      case TYPE_ACTREQ:
      default:
        std::clog << "Got an unexpected packet type: " << buf.basic.type
                  << std::endl;
        break;
    }
}

Comm::Comm(struct addrinfo *ai, u_int16_t port)
    : send_queue()
{
    int ret;

    this->create_socket(ai, port);

    /* Init the mutex and cond variables */
    if ((ret = pthread_mutex_init(&(this->send_lock), NULL)) != 0)
    {
        std::ostringstream s;
        s << "Couldn't init queue mutex: "
          << strerror(ret) << " (" << ret << ")";
        throw std::runtime_error(s.str());
    }
    if ((ret = pthread_cond_init(&send_queue_not_empty, NULL)) != 0)
    {
        std::ostringstream s;
        s << "Couldn't init queue-not-empty cond: "
          << strerror(ret) << " (" << ret << ")";
        pthread_mutex_destroy(&(this->send_lock));
        throw std::runtime_error(s.str());
    }

    /* Now start up the actual threads */
    if ((ret = pthread_create(&(this->send_thread),
                              NULL,
                              Comm::send_worker,
                              (void *)this)) != 0)
    {
        std::ostringstream s;
        s << "Couldn't start send thread: "
          << strerror(ret) << " (" << ret << ")";
        pthread_cond_destroy(&(this->send_queue_not_empty));
        pthread_mutex_destroy(&(this->send_lock));
        throw std::runtime_error(s.str());
    }
    if ((ret = pthread_create(&(this->recv_thread),
                              NULL,
                              Comm::recv_worker,
                              (void *)this)) != 0)
    {
        std::ostringstream s;
        s << "Couldn't start recv thread: "
          << strerror(ret) << " (" << ret << ")";
        pthread_cancel(this->send_thread);
        sleep(0);
        pthread_join(this->send_thread, NULL);
        pthread_cond_destroy(&(this->send_queue_not_empty));
        pthread_mutex_destroy(&(this->send_lock));
        throw std::runtime_error(s.str());
    }
}

Comm::~Comm()
{
    if (this->sock)
        this->send_logout();
    this->thread_exit_flag = 1;
    pthread_cond_broadcast(&(this->send_queue_not_empty));
    sleep(0);
    pthread_join(this->send_thread, NULL);
    pthread_cancel(this->recv_thread);
    sleep(0);
    pthread_join(this->recv_thread, NULL);
}

void Comm::send(packet& p, size_t len)
{
    pthread_mutex_lock(&(this->send_lock));
    if (!hton_packet(&p, len))
        std::clog << "Error hton'ing packet" << std::endl;
    else
    {
        this->send_queue.push(p);
        pthread_cond_signal(&(this->send_queue_not_empty));
    }
    pthread_mutex_unlock(&(this->send_lock));
}

void Comm::send_login(const std::string& user, const std::string& pass)
{
    packet req;

    memset((void *)&(req.log), 0, sizeof(login_request));
    req.log.type = TYPE_LOGREQ;
    req.log.version = 1;
    req.log.sequence = sequence++;
    strncpy(req.log.username, user.c_str(), sizeof(req.log.username));
    strncpy(req.log.password, pass.c_str(), sizeof(req.log.password));
    this->send(req, sizeof(login_request));
    /* Get rid of passwords from memory where possible */
    memset((void *)&req, 0, sizeof(login_request));
}

void Comm::send_action_request(u_int16_t actionid,
                               u_int64_t target,
                               u_int8_t power)
{
    packet req;

    req.act.type = TYPE_ACTREQ;
    req.act.version = 1;
    req.act.sequence = sequence++;
    req.act.action_id = actionid;
    req.act.power_level = power;
    req.act.dest_object_id = target;
    this->send(req, sizeof(action_request));
}

void Comm::send_logout(void)
{
    packet req;

    req.lgt.type = TYPE_LGTREQ;
    req.lgt.version = 1;
    req.lgt.sequence = sequence++;
    this->send(req, sizeof(logout_request));
}

void Comm::send_ack(u_int8_t type)
{
    packet req;

    req.ack.type = TYPE_ACKPKT;
    req.ack.version = 1;
    req.ack.sequence = sequence++;
    req.ack.request = type;
    this->send(req, sizeof(ack_packet));
}
