/* dgram.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 13 Nov 2015, 12:31:34 tquirk
 *
 * Revision IX game server
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
 * This file contains the implementation of the datagram socket object.
 *
 * Things to do
 *   - We might need to have a mutex on the socket, since we'll probably
 *     be trying to read from and write to it at the same time.
 *   - We should probably have a mutex around the users and socks members
 *     since we use those in a few different places.
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

void dgram_socket::start(void)
{
    int retval;

    std::clog << "starting connection loop for datagram port "
              << this->sock.sa->port() << std::endl;

    /* Start up the listen thread and thread pools */
    sleep(0);
    this->send_pool->startup_arg = (void *)this;
    this->send_pool->start(dgram_socket::dgram_send_worker);
    this->access_pool->startup_arg = (void *)this;
    this->access_pool->start(listen_socket::access_pool_worker);
    this->sock.listen_arg = (void *)this;
    this->sock.start(dgram_socket::dgram_listen_worker);

    /* Start up the reaping thread */
    if ((retval = pthread_create(&this->reaper, NULL,
                                 dgram_reaper_worker, (void *)this)) != 0)
    {
        std::ostringstream s;
        s << "couldn't create reaper thread for datagram port "
          << this->sock.sa->port() << ": "
          << strerror(retval) << " (" << retval << ")";
        throw std::runtime_error(s.str());
    }
}

void dgram_socket::do_login(uint64_t userid, Control *con, access_list& al)
{
    dgram_user *dgu = new dgram_user(userid, con);
    dgu->sa = build_sockaddr((struct sockaddr&)(al.what.login.who.dgram));
    this->users[userid] = dgu;
    this->socks[dgu->sa] = dgu;
}

void *dgram_socket::dgram_listen_worker(void *arg)
{
    dgram_socket *dgs = (dgram_socket *)arg;
    int len;
    packet buf;
    struct sockaddr_storage from;
    socklen_t fromlen;
    uint64_t userid;
    std::map<uint64_t, dgram_user *>::iterator i;
    std::map<Sockaddr *, dgram_user *, less_sockaddr>::iterator found;
    access_list a;
    packet_list p;

    /* Do the receiving part */
    for (;;)
    {
        if (main_loop_exit_flag == 1)
            break;

        pthread_testcancel();
        /* We HAVE to pass in a proper size value in fromlen */
        fromlen = sizeof(Sockaddr);
        userid = 0LL;
        /* Will this be interrupted by a pthread_cancel? */
        len = recvfrom(dgs->sock.sock,
                       (void *)&buf,
                       sizeof(packet),
                       0,
                       (struct sockaddr *)&from, &fromlen);
        pthread_testcancel();

        /* Figure out who sent this packet */
        Sockaddr *sa = build_sockaddr((struct sockaddr&)from);
        if (dgs->socks.find(sa) != dgs->socks.end())
            userid = dgs->socks[sa]->userid;

        /* Do something with whatever we got */
        switch (buf.basic.type)
        {
          case TYPE_ACKPKT:
            /* Acknowledgement packet */
            std::clog << "got an ack packet" << std::endl;
            if (!userid)
                break;
            dgs->users[userid]->timestamp = time(NULL);
            break;

          case TYPE_LOGREQ:
            /* Login request */
            std::clog << "got a login packet" << std::endl;
            memcpy(&a.buf, &buf, len);
            a.parent = dgs;
            memcpy(&a.what.login.who.dgram, &from,
                   sizeof(struct sockaddr_storage));
            dgs->access_pool->push(a);
            break;

          case TYPE_LGTREQ:
            /* Logout request */
            std::clog << "got a logout packet" << std::endl;
            if (!userid)
                break;
            dgs->users[userid]->timestamp = time(NULL);
            memcpy(&a.buf, &buf, len);
            a.parent = dgs;
            a.what.logout.who = userid;
            dgs->access_pool->push(a);
            break;

          case TYPE_ACTREQ:
            /* Action request */
            std::clog << "got an action request packet" << std::endl;
            if (!userid)
                break;
            dgs->users[userid]->timestamp = time(NULL);
            memcpy(&p.buf, &buf, len);
            p.who = userid;
            zone->action_pool->push(p);
            break;

          default:
            break;
        }
        pthread_testcancel();
    }
    std::clog << "exiting connection loop for datagram port "
              << dgs->sock.sa->port() << std::endl;
    return NULL;
}

void *dgram_socket::dgram_reaper_worker(void *arg)
{
    dgram_socket *dgs = (dgram_socket *)arg;
    std::map<uint64_t, base_user *>::iterator i;
    dgram_user *dgu;
    time_t now;

    std::clog << "started reaper thread for datagram port "
              << dgs->sock.sa->port() << std::endl;
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
                std::clog << "removing user "
                          << dgu->control->username << " ("
                          << dgu->userid << ") from datagram port "
                          << dgs->sock.sa->port() << std::endl;
                if (dgu->control->slave != NULL)
                {
                    /* Clean up a user who has logged out */
                    dgu->control->slave->natures["invisible"] = 1;
                    dgu->control->slave->natures["non-interactive"] = 1;
                }
                delete dgu->control;
                dgs->socks.erase(dgu->sa);
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

    std::clog << "started send pool worker for datagram port "
              << dgs->sock.sa->port() << std::endl;
    for (;;)
    {
        dgs->send_pool->pop(&req);

        realsize = packet_size(&req.buf);
        if (hton_packet(&req.buf, realsize))
        {
            dgu = dynamic_cast<dgram_user *>(dgs->users[req.who]);
            if (dgu == NULL)
                continue;
            /* TODO: Encryption */
            if (sendto(dgs->sock.sock,
                       (void *)&req.buf, realsize, 0,
                       (struct sockaddr *)&(dgu->sa),
                       sizeof(struct sockaddr_storage)) == -1)
                std::clog << syslogErr
                          << "error sending packet out datagram port "
                          << dgs->sock.sa->port() << ": "
                          << strerror(errno) << " (" << errno << ")"
                          << std::endl;
            else
                std::clog << "sent a packet of type "
                          << req.buf.basic.type << " to "
                          << dgu->sa->ntop() << std::endl;
        }
    }
    std::clog << "exiting send pool worker for datagram port "
              << dgs->sock.sa->port() << std::endl;
    return NULL;
}
