/* dgram.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *
 * Revision IX game server
 * Copyright (C) 2007-2026  Trinity Annabelle Quirk
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
 * Things to do
 *   - We might need to have a mutex on the socket, since we'll probably
 *     be trying to read from and write to it at the same time.
 *   - We should probably have a mutex around the users and socks members
 *     since we use those in a few different places.
 *   - We do an awful lot of memcpying in the receive worker.
 *
 */

#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <errno.h>

#include <sstream>
#include <stdexcept>

#include "dgram.h"
#include "log.h"

static std::map<int, listen_socket::packet_handler> packet_handlers =
{
    { TYPE_LOGREQ, dgram_socket::handle_login },
    { TYPE_ACKPKT, listen_socket::handle_ack },
    { TYPE_ACTREQ, listen_socket::handle_action },
    { TYPE_LGTREQ, listen_socket::handle_logout }
};

dgram_socket::dgram_socket(Addrinfo *ai)
    : listen_socket(ai)
{
}

dgram_socket::~dgram_socket()
{
    /* Should we send logout messages to everybody? */

    /* Thread pools and listen sockets are handled by the
     * listen_socket destructor.
     */
}

void dgram_socket::start(void)
{
    this->listen_socket::start();

    this->send_pool->start(dgram_socket::dgram_send_worker, (void *)this);
    this->basesock::start(dgram_socket::dgram_listen_worker, (void *)this);
}

void dgram_socket::connect_user(base_user *bu, access_list& al)
{
    this->socks[al.what.login.who.dgram] = bu;
    this->user_socks[bu->userid] = al.what.login.who.dgram;

    this->listen_socket::connect_user(bu, al);
}

void dgram_socket::disconnect_user(base_user *bu)
{
    {
        std::unique_lock lock(this->user_mutex);
        if (this->user_socks.find(bu->userid) != this->user_socks.end())
        {
            this->socks.erase(this->user_socks[bu->userid]);
            this->user_socks.erase(bu->userid);
        }
    }

    this->listen_socket::disconnect_user(bu);
}

void dgram_socket::dgram_listen_worker(void *arg)
{
    dgram_socket *dgs = (dgram_socket *)arg;
    int len;
    packet buf;
    Sockaddr *sa;
    struct sockaddr_storage from;
    socklen_t fromlen;

    for (;;)
    {
        if (dgs->exit_flag)
            break;

        memset((char *)&buf, 0, sizeof(packet));
        fromlen = sizeof(struct sockaddr_storage);

        if ((len = recvfrom(dgs->sock,
                            (void *)&buf,
                            sizeof(packet),
                            0,
                            (struct sockaddr *)&from, &fromlen)) <= 0
            || fromlen == 0)
            continue;

        try { sa = build_sockaddr((struct sockaddr&)from); }
        catch (std::runtime_error& e) {
            std::clog << syslogWarn << e.what() << std::endl;
            continue;
        }

        dgs->handle_packet(buf, len, sa);
        delete sa;
    }
    std::clog << "exiting connection loop for datagram port "
              << dgs->sa->port() << std::endl;
}

void dgram_socket::handle_packet(packet& p, int len, Sockaddr *sa)
{
    base_user *user = NULL;

    if (packet_handlers.find(p.basic.type) != packet_handlers.end())
    {
        std::shared_lock lock(this->user_mutex);
        if (this->socks.find(sa) != this->socks.end())
        {
            user = this->socks[sa];
            if (!user->decrypt_packet(p))
                return;
        }
        if (!ntoh_packet(&p, len))
            return;
        packet_handlers[p.basic.type](this, p, user, sa);
    }
}

void dgram_socket::handle_login(listen_socket *s, packet& p,
                                base_user *u, void *sa)
{
    access_list al;

    memcpy(&al.buf, &p, sizeof(login_request));
    /* sa gets cleaned up at the end of packet handling, but we need
     * to keep the object around for our user maps.  This is not an
     * awesome memory allocation strategy, but we're kind of stuck
     * because we can't return objects out of an abstract factory
     * function by value.  They must be by pointer.
     */
    al.what.login.who.dgram = ((Sockaddr *)sa)->clone();
    s->access_pool->push(al);
}

void dgram_socket::dgram_send_worker(void *arg)
{
    dgram_socket *dgs = (dgram_socket *)arg;
    packet_list req;
    size_t realsize;

    std::clog << "started send pool worker for datagram port "
              << dgs->sa->port() << std::endl;
    for (;;)
    {
        if (!dgs->send_pool->pop(&req))
            break;

        realsize = packet_size(&req.buf);

        std::shared_lock lock(dgs->user_mutex);
        if (dgs->user_socks.find(req.who->userid) != dgs->user_socks.end()
            && hton_packet(&req.buf, realsize)
            && req.who->encrypt_packet(req.buf))
        {
            if (sendto(dgs->sock,
                       (void *)&req.buf, realsize, 0,
                       dgs->user_socks[req.who->userid]->sockaddr(),
                       sizeof(struct sockaddr_storage)) == -1)
            {
                char err[128];

                std::clog << syslogErr
                          << "error sending packet out datagram port "
                          << dgs->sa->port() << ": "
                          << strerror_r(errno, err, sizeof(err))
                          << " (" << errno << ")"
                          << std::endl;
            }
        }
    }
    std::clog << "exiting send pool worker for datagram port "
              << dgs->sa->port() << std::endl;
}
