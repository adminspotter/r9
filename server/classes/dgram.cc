/* dgram.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 15 Jul 2019, 08:15:37 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2019  Trinity Annabelle Quirk
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

#include "../server.h"

static std::map<int, listen_socket::packet_handler> packet_handlers =
{
    { TYPE_LOGREQ, dgram_socket::handle_login },
    { TYPE_ACKPKT, listen_socket::handle_ack },
    { TYPE_ACTREQ, listen_socket::handle_action },
    { TYPE_LGTREQ, listen_socket::handle_logout }
};

dgram_socket::dgram_socket(struct addrinfo *ai)
    : listen_socket(ai)
{
}

dgram_socket::~dgram_socket()
{
    /* In order to prevent any races between our various user maps and
     * the worker loops, we'll stop ourselves first.
     */
    try { this->stop(); }
    catch (std::exception& e) { /* Do nothing */ }

    /* Should we send logout messages to everybody? */

    /* Thread pools are handled by the listen_socket destructor */
}

std::string dgram_socket::port_type(void)
{
    return "datagram";
}

void dgram_socket::start(void)
{
    this->listen_socket::start();

    /* Start up the listen thread and thread pools */
    this->send_pool->startup_arg = (void *)this;
    this->send_pool->start(dgram_socket::dgram_send_worker);
    this->sock.listen_arg = (void *)this;
    this->sock.start(dgram_socket::dgram_listen_worker);
}

void dgram_socket::connect_user(base_user *bu, access_list& al)
{
    this->socks[al.what.login.who.dgram] = bu;
    this->user_socks[bu->userid] = al.what.login.who.dgram;

    this->listen_socket::connect_user(bu, al);
}

void dgram_socket::disconnect_user(base_user *bu)
{
    Sockaddr *sa = this->user_socks[bu->userid];
    this->socks.erase(sa);
    this->user_socks.erase(bu->userid);

    this->listen_socket::disconnect_user(bu);
}

void *dgram_socket::dgram_listen_worker(void *arg)
{
    dgram_socket *dgs = (dgram_socket *)arg;
    int len;
    packet buf;
    Sockaddr *sa;
    struct sockaddr_storage from;
    socklen_t fromlen;

    /* Make sure we can be cancelled as we expect. */
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    for (;;)
    {
        if (main_loop_exit_flag == 1)
            break;

        pthread_testcancel();

        memset((char *)&buf, 0, sizeof(packet));
        fromlen = sizeof(struct sockaddr_storage);

        /* recvfrom should be interruptible by pthread_cancel */
        len = recvfrom(dgs->sock.sock,
                       (void *)&buf,
                       sizeof(packet),
                       0,
                       (struct sockaddr *)&from, &fromlen);
        pthread_testcancel();

        /* If anything is wrong, just ignore what we got */
        if (len <= 0 || fromlen == 0)
            continue;

        try { sa = build_sockaddr((struct sockaddr&)from); }
        catch (std::runtime_error& e) {
            std::clog << syslogWarn << e.what() << std::endl;
            continue;
        }

        dgs->handle_packet(buf, len, sa);
    }
    std::clog << "exiting connection loop for datagram port "
              << dgs->sock.sa->port() << std::endl;
    return NULL;
}

void dgram_socket::handle_packet(packet& p, int len, Sockaddr *sa)
{
    auto handler = packet_handlers.find(p.basic.type);
    auto found = this->socks.find(sa);

    if (found != this->socks.end() && handler != packet_handlers.end())
        if (found->second->decrypt_packet(p) && ntoh_packet(&p, len))
            (handler->second)(this, p, found->second, sa);
    delete sa;
}

void dgram_socket::handle_login(listen_socket *s, packet& p,
                                base_user *u, void *sa)
{
    access_list al;

    memcpy(&al.buf, &p, sizeof(login_request));
    al.what.login.who.dgram = build_sockaddr(*((Sockaddr *)sa)->sockaddr());
    s->access_pool->push(al);
}

void *dgram_socket::dgram_send_worker(void *arg)
{
    dgram_socket *dgs = (dgram_socket *)arg;
    packet_list req;
    size_t realsize;

    std::clog << "started send pool worker for datagram port "
              << dgs->sock.sa->port() << std::endl;
    for (;;)
    {
        dgs->send_pool->pop(&req);

        realsize = packet_size(&req.buf);
        if (hton_packet(&req.buf, realsize) && req.who->encrypt_packet(req.buf))
        {
            if (sendto(dgs->sock.sock,
                       (void *)&req.buf, realsize, 0,
                       dgs->user_socks[req.who->userid]->sockaddr(),
                       sizeof(struct sockaddr_storage)) == -1)
            {
                char err[128];

                strerror_r(errno, err, sizeof(err));
                std::clog << syslogErr
                          << "error sending packet out datagram port "
                          << dgs->sock.sa->port() << ": "
                          << err << " (" << errno << ")"
                          << std::endl;
            }
        }
    }
    std::clog << "exiting send pool worker for datagram port "
              << dgs->sock.sa->port() << std::endl;
    return NULL;
}
