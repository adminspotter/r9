/* comm.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 09 Dec 2015, 06:05:23 tquirk
 *
 * Revision IX game client
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
 * Things to do
 *
 */

#include <config.h>

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */
#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif /* HAVE_ARPA_INET_H */
#include <errno.h>

#include <cstdint>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "client_core.h"
#include "comm.h"

uint64_t Comm::sequence = 0LL;
volatile bool Comm::thread_exit_flag = false;

/* Jump table for protocol handling */
Comm::pkt_handler Comm::pkt_type[] =
{
    &Comm::handle_ackpkt,       /* Ack             */
    &Comm::handle_unsupported,  /* Login req       */
    &Comm::handle_unsupported,  /* Logout req      */
    &Comm::handle_unsupported,  /* Action req      */
    &Comm::handle_posupd,       /* Position update */
    &Comm::handle_srvnot,       /* Server notice   */
    &Comm::handle_pngpkt        /* Ping            */
};

#define COMM_MEMBER(a, b) ((a).*(b))

void Comm::create_socket(struct addrinfo *ai)
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
    memcpy(&this->remote, ai->ai_addr, sizeof(sockaddr_storage));

    /* OSX requires a family-specific size, rather than just
     * sizeof(sockaddr_storage).
     */
    switch (ai->ai_family)
    {
      case AF_INET:
        this->remote_size = sizeof(sockaddr_in);
        break;

      case AF_INET6:
        this->remote_size = sizeof(sockaddr_in6);
        break;

      default:
        break;
    }
}

void *Comm::send_worker(void *arg)
{
    Comm *comm = (Comm *)arg;
    packet *pkt;

    /* Make sure we can be cancelled as we expect. */
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

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

        pkt = comm->send_queue.front();
        comm->send_queue.pop();

        if (sendto(comm->sock,
                   (void *)pkt, packet_size(pkt),
                   0,
                   (struct sockaddr *)&comm->remote,
                   comm->remote_size) == -1)
            std::clog << "got a send error: "
                      << strerror(errno) << " (" << errno << ')' << std::endl;
        pthread_mutex_unlock(&(comm->send_lock));
        memset(pkt, 0, sizeof(packet));
        delete pkt;
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

    /* Make sure we can be cancelled as we expect. */
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    for (;;)
    {
        fromlen = sizeof(struct sockaddr_storage);
        if ((len = recvfrom(comm->sock, (void *)&buf, sizeof(packet), 0,
                            (struct sockaddr *)&sin, &fromlen)) < 0)
        {
            std::clog << "Error receiving packet: "
                      << strerror(errno) << " (" << errno << ')' << std::endl;
            continue;
        }
        /* Verify that the sender is who we think it should be */
        if (memcmp(&sin, &(comm->remote), fromlen))
        {
            std::clog << "Got packet from unknown sender" << std::endl;
            continue;
        }
        /* Needs to be a real packet type */
        if (buf.basic.type >= sizeof(pkt_type))
        {
            std::clog << "Unknown packet type " << buf.basic.type << std::endl;
            continue;
        }
        /* We should be able to convert to host byte ordering */
        if (!ntoh_packet(&buf, len))
        {
            std::clog << "Error while ntoh'ing packet" << std::endl;
            continue;
        }

        COMM_MEMBER(*comm, pkt_type[buf.basic.type])(buf);
    }
    return NULL;
}

void Comm::handle_pngpkt(packet& p)
{
    this->send_ack(TYPE_PNGPKT);
}

void Comm::handle_ackpkt(packet& p)
{
    char *access_type[] =
        {
            NULL,
            "ACCESS_NONE",
            "ACCESS_VIEW",
            "ACCESS_MOVE",
            "ACCESS_MDFY"
        };
    ack_packet& a = (ack_packet&)p.ack;

    switch (a.request)
    {
      case TYPE_LOGREQ:
        /* The response to our login request */
        std::clog << "Login response, type ";
        if (a.misc < 1 || a.misc >= sizeof(access_type))
            std::clog << "unknown" << std::endl;
        else
            std::clog << access_type[a.misc] << " access" << std::endl;
        break;

      case TYPE_LGTREQ:
        /* The response to our logout request */
        std::clog << "Logout response, type ";
        if (a.misc < 1 || a.misc >= sizeof(access_type))
            std::clog << "unknown" << std::endl;
        else
            std::clog << access_type[a.misc] << " access" << std::endl;
        break;

      default:
        std::clog << "Got an unknown ack packet: " << a.request
                  << std::endl;
        break;
    }
}

void Comm::handle_posupd(packet& p)
{
    position_update& u = (position_update&)p.pos;

    move_object(u.object_id,
                u.frame_number,
                (double)u.x_pos / 100.0,
                (double)u.y_pos / 100.0,
                (double)u.z_pos / 100.0,
                (double)u.x_orient / 100.0,
                (double)u.y_orient / 100.0,
                (double)u.z_orient / 100.0);
}

void Comm::handle_srvnot(packet& p)
{
    std::clog << "Got a server notice" << std::endl;
}

void Comm::handle_unsupported(packet& p)
{
    std::clog << "Got an unexpected packet type: " << p.basic.type
              << std::endl;
}

Comm::Comm(struct addrinfo *ai)
    : send_queue()
{
    int ret;

    this->create_socket(ai);

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

void Comm::send(packet *p, size_t len)
{
    pthread_mutex_lock(&(this->send_lock));
    if (!hton_packet(p, len))
        std::clog << "Error hton'ing packet" << std::endl;
    else
    {
        this->send_queue.push(p);
        pthread_cond_signal(&(this->send_queue_not_empty));
    }
    pthread_mutex_unlock(&(this->send_lock));
}

void Comm::send_login(const std::string& user,
                      const std::string& pass,
                      const std::string& character)
{
    packet *req = new packet;

    memset((void *)req, 0, sizeof(login_request));
    req->log.type = TYPE_LOGREQ;
    req->log.version = 1;
    req->log.sequence = this->sequence++;
    strncpy(req->log.username, user.c_str(), sizeof(req->log.username));
    strncpy(req->log.password, pass.c_str(), sizeof(req->log.password));
    strncpy(req->log.charname, character.c_str(), sizeof(req->log.charname));
    this->send(req, sizeof(login_request));
}

void Comm::send_action_request(uint16_t actionid,
                               uint64_t target,
                               uint8_t power)
{
    packet *req = new packet;

    req->act.type = TYPE_ACTREQ;
    req->act.version = 1;
    req->act.sequence = sequence++;
    req->act.action_id = actionid;
    req->act.power_level = power;
    req->act.dest_object_id = target;
    this->send(req, sizeof(action_request));
}

void Comm::send_logout(void)
{
    packet *req = new packet;

    req->lgt.type = TYPE_LGTREQ;
    req->lgt.version = 1;
    req->lgt.sequence = sequence++;
    this->send(req, sizeof(logout_request));
}

void Comm::send_ack(uint8_t type)
{
    packet *req = new packet;

    req->ack.type = TYPE_ACKPKT;
    req->ack.version = 1;
    req->ack.sequence = sequence++;
    req->ack.request = type;
    this->send(req, sizeof(ack_packet));
}
