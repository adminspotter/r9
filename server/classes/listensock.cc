/* listensock.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 01 Nov 2015, 10:56:20 tquirk
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
 * This file contains the listening thread handling and user tracking
 * for our sockets.  This is not directly instantiable, as there are
 * pure-virtual methods in this class.
 *
 * Things to do
 *
 */

#include <string.h>
#include <errno.h>

#include "listensock.h"

#include "../server.h"
#include "../config_data.h"
#include "../log.h"

listen_socket::listen_socket(struct addrinfo *ai, uint16_t port)
    : users(), sock(ai, port)
{
    this->init();
}

listen_socket::~listen_socket()
{
    int retval;
    std::map<uint64_t, base_user *>::iterator i;

    try { this->stop(); }
    catch (std::exception& e) { /* Do nothing */ }

    if ((retval = pthread_cancel(this->reaper)) != 0)
    {
        std::clog << syslogErr << "couldn't cancel reaper thread for port "
                  << this->sock.port_num << ": "
                  << strerror(retval) << " (" << retval << ")" << std::endl;
    }
    sleep(0);
    if ((retval = pthread_join(this->reaper, NULL)) != 0)
        std::clog << syslogErr << "error terminating reaper thread for port "
                  << this->sock.port_num << ": "
                  << strerror(retval) << " (" << retval << ")" << std::endl;

    /* Clear out the users map */
    for (i = this->users.begin(); i != this->users.end(); ++i)
        delete (*i).second;
    this->users.erase(this->users.begin(), this->users.end());

    if (this->send_pool != NULL)
        delete this->send_pool;

    if (this->access_pool != NULL)
        delete this->access_pool;
}

void listen_socket::init(void)
{
    this->send_pool = new ThreadPool<packet_list>("send", config.send_threads);
    this->access_pool = new ThreadPool<access_list>("access",
                                                    config.access_threads);
    this->access_pool->clean_on_pop = true;
}

void listen_socket::stop(void)
{
    this->send_pool->stop();
    this->access_pool->stop();
    this->sock.stop();
}

void *listen_socket::access_pool_worker(void *arg)
{
    listen_socket *ls = (listen_socket *)arg;
    access_list req;

    std::clog << "started access pool worker";
    for (;;)
    {
        ls->access_pool->pop(&req);

        ntoh_packet(&(req.buf), sizeof(packet));

        if (req.buf.basic.type == TYPE_LOGREQ)
            ls->login_user(req);
        else if (req.buf.basic.type == TYPE_LGTREQ)
            ls->logout_user(req);
        /* Otherwise, we don't recognize it, and will ignore it */
    }
    std::clog << "access pool worker ending";
    return NULL;
}

void listen_socket::login_user(access_list& p)
{
    uint64_t userid = 0LL;
    std::string username(p.buf.log.username, sizeof(p.buf.log.username));
    std::string password(p.buf.log.password, sizeof(p.buf.log.password));

    userid = database->check_authentication(username, password);

    /* Don't want to keep passwords around in core if we can help it */
    memset(p.buf.log.password, 0, sizeof(p.buf.log.password));
    password.clear();

    std::clog << "login request from "
              << p.buf.log.username << " (" << userid << ")" << std::endl;
    if (userid != 0LL)
    {
        if (this->users.find(userid) == this->users.end())
        {
            Control *newcontrol = new Control(userid, NULL);
            newcontrol->username = username;

            /* Add this user to the userlist */
            this->do_login(userid, newcontrol, p);
            newcontrol->parent = (void *)(this->send_pool);

            std::clog << "logged in user "
                      << newcontrol->username
                      << " (" << userid << ")" << std::endl;

            /* Send an ack packet, to let the user know they're in */
            newcontrol->send_ack(TYPE_LOGREQ);
        }
    }
    /* Otherwise, do nothing, and send nothing */
}

void listen_socket::logout_user(access_list& p)
{
    std::map<uint64_t, base_user *>::iterator found;

    /* Most of this function is now handled by the reaper threads */
    if ((found = this->users.find(p.what.logout.who)) != this->users.end())
    {
        base_user *bu = found->second;
        bu->pending_logout = true;
        std::clog << "logout request from "
                  << bu->control->username
                  << " (" << bu->control->userid << ")" << std::endl;
        bu->control->send_ack(TYPE_LGTREQ);
    }
}
