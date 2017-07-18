/* dgram.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 18 Jul 2017, 08:23:29 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2017  Trinity Annabelle Quirk
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

#include "../server.h"
#include "../log.h"

extern volatile int main_loop_exit_flag;

dgram_user::dgram_user(uint64_t u, Control *c)
    : base_user(u, c)
{
}

const dgram_user& dgram_user::operator=(const dgram_user& du)
{
    this->sa = du.sa;
    this->base_user::operator=(du);
    return *this;
}

dgram_socket::dgram_socket(struct addrinfo *ai)
    : listen_socket(ai)
{
}

dgram_socket::~dgram_socket()
{
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

void dgram_socket::do_login(uint64_t userid,
                            Control *con,
                            access_list& al)
{
    dgram_user *dgu = new dgram_user(userid, con);
    dgu->sa = build_sockaddr((struct sockaddr&)(al.what.login.who.dgram));
    this->users[userid] = dgu;
    this->socks[dgu->sa] = dgu;

    this->connect_user((base_user *)dgu, al);
}

/* The do_logout method performs the only dgram_socket-specific work
 * for removing a datagram user from the object.  Everything else is
 * handled in listen_socket::reaper_worker.
 */
void dgram_socket::do_logout(base_user *bu)
{
    dgram_user *dgu = dynamic_cast<dgram_user *>(bu);

    if (dgu != NULL)
        dgs->socks.erase(dgu->sa);
}

void *dgram_socket::dgram_listen_worker(void *arg)
{
    dgram_socket *dgs = (dgram_socket *)arg;
    int len;
    packet buf;
    Sockaddr *sa;
    struct sockaddr_storage from;
    socklen_t fromlen;
    dgram_socket::socks_iterator found;
    access_list a;
    packet_list p;

    /* Make sure we can be cancelled as we expect. */
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    /* Do the receiving part */
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

        /* Did we actually even get anything? */
        if (len <= 0 || fromlen == 0)
            continue;

        /* Figure out who sent this packet */
        try { sa = build_sockaddr((struct sockaddr&)from); }
        catch (...) { continue; }

        /* If we got a packet which wasn't the right size for its
         * type, just drop it on the floor.
         */
        if (!ntoh_packet(&buf, len))
            continue;

        /* At this point, we know that the sender is a real host, and
         * the packet is a legitimate packet.
         */
        found = dgs->socks.find(sa);
        if (found == dgs->socks.end() && buf.basic.type == TYPE_LOGREQ)
        {
            std::clog << "got a login packet" << std::endl;
            memcpy(&a.buf, &buf, len);
            a.parent = dgs;
            memcpy(&a.what.login.who.dgram, &from,
                   sizeof(struct sockaddr_storage));
            dgs->access_pool->push(a);
        }

        /* Do something with whatever we got */
        switch (buf.basic.type)
        {
          case TYPE_ACKPKT:
            /* Acknowledgement packet */
            std::clog << "got an ack packet" << std::endl;
            found->second->timestamp = time(NULL);
            break;

          case TYPE_LGTREQ:
            /* Logout request */
            std::clog << "got a logout packet" << std::endl;
            found->second->timestamp = time(NULL);
            memcpy(&a.buf, &buf, len);
            a.parent = dgs;
            a.what.logout.who = found->second->userid;
            dgs->access_pool->push(a);
            break;

          case TYPE_ACTREQ:
            /* Action request */
            std::clog << "got an action request packet" << std::endl;
            found->second->timestamp = time(NULL);
            memcpy(&p.buf, &buf, len);
            p.who = found->second->control;
            p.parent = dgs;
            action_pool->push(p);
            break;

          default:
            break;
        }
        delete sa;
    }
    std::clog << "exiting connection loop for datagram port "
              << dgs->sock.sa->port() << std::endl;
    return NULL;
}

void *dgram_socket::dgram_send_worker(void *arg)
{
    dgram_socket *dgs = (dgram_socket *)arg;
    dgram_user *dgu;
    packet_list req;
    size_t realsize;

    std::clog << "started send pool worker for datagram port "
              << dgs->sock.sa->port() << std::endl;
    for (;;)
    {
        dgs->send_pool->pop(&req);

        realsize = packet_size(&req.buf);
        if (hton_packet(&req.buf, realsize))
        {
            /* The users member comes from the listen_socket, and is a
             * map of userid to base_user; we're 99.99% sure that all
             * the elements in our users map will be dgram_users, but
             * we'll dynamic cast and check the return just to be 100%
             * sure.  If it becomes a problem, we can cast explicitly.
             */
            dgu = dynamic_cast<dgram_user *>(dgs->users[req.who->userid]);
            if (dgu == NULL)
                continue;

            /* TODO: Encryption */
            if (sendto(dgs->sock.sock,
                       (void *)&req.buf, realsize, 0,
                       (struct sockaddr *)&(dgu->sa->ss),
                       sizeof(struct sockaddr_storage)) == -1)
                std::clog << syslogErr
                          << "error sending packet out datagram port "
                          << dgs->sock.sa->port() << ": "
                          << strerror(errno) << " (" << errno << ")"
                          << std::endl;
        }
    }
    std::clog << "exiting send pool worker for datagram port "
              << dgs->sock.sa->port() << std::endl;
    return NULL;
}
