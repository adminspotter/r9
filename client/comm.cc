/* comm.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 30 Nov 2015, 20:33:28 tquirk
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

std::ostream& operator<<(std::ostream& os, const struct sockaddr *sa)
{
    char addrstr[INET6_ADDRSTRLEN];

    os << "Family:   ";
    switch (sa->sa_family)
    {
      case AF_INET:
      {
          struct sockaddr_in *sin = (struct sockaddr_in *)sa;

          os << "AF_INET" << std::endl;
          os << "Port:     " << ntohs(sin->sin_port) << std::endl;
          os << "Address:  ";
          inet_ntop(AF_INET, (void *)&sin->sin_addr, addrstr, INET6_ADDRSTRLEN);
          os << addrstr << std::endl;
          break;
      }

      case AF_INET6:
      {
          struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;

          os << "AF_INET6" << std::endl;
          os << "Port:     " << ntohs(sin6->sin6_port) << std::endl;
          os << "Flowinfo: " << ntohl(sin6->sin6_flowinfo) << std::endl;
          os << "Address:  ";
          inet_ntop(AF_INET6, (void *)&sin6->sin6_addr,
                    addrstr, INET6_ADDRSTRLEN);
          os << addrstr << std::endl;
          os << "Scope ID: " << ntohl(sin6->sin6_scope_id) << std::endl;
          break;
      }

      default:
        os << "unknown" << std::endl;
        break;
    }
    return os;
}

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
}

void *Comm::send_worker(void *arg)
{
    Comm *comm = (Comm *)arg;
    packet *pkt;

    for (;;)
    {
        pthread_mutex_lock(&(comm->send_lock));
        while (comm->send_queue.empty() && !Comm::thread_exit_flag)
            pthread_cond_wait(&(comm->send_queue_not_empty),
                              &(comm->send_lock));
        pkt = comm->send_queue.front();
        comm->send_queue.pop();

        if (Comm::thread_exit_flag)
        {
            pthread_mutex_unlock(&(comm->send_lock));
            pthread_exit(NULL);
        }

        std::cout << (struct sockaddr *)&comm->remote << std::endl;
        if (sendto(comm->sock,
                   (void *)pkt, packet_size(pkt),
                   0,
                   (struct sockaddr *)&comm->remote,
                   sizeof(struct sockaddr_storage)) == -1)
            std::cout << "got a send error: " << strerror(errno) << " (" << errno << ')' << std::endl;
        else
            std::cout << "sent packet" << std::endl;
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

    for (;;)
    {
        fromlen = sizeof(struct sockaddr_storage);
        len = recvfrom(comm->sock, (void *)&buf, sizeof(packet), 0,
                       (struct sockaddr *)&sin, &fromlen);
        /* Verify that the sender is who we think it should be */
        if (memcmp(&sin, &(comm->remote), fromlen))
        {
            std::clog << "Got packet from unknown sender " << std::endl;
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

void Comm::send_login(const std::string& user, const std::string& pass)
{
    packet *req = new packet;

    memset((void *)req, 0, sizeof(login_request));
    req->log.type = TYPE_LOGREQ;
    req->log.version = 1;
    req->log.sequence = this->sequence++;
    strncpy(req->log.username, user.c_str(), sizeof(req->log.username));
    strncpy(req->log.password, pass.c_str(), sizeof(req->log.password));
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
