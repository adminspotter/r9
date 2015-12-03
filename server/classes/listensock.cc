/* listensock.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 03 Dec 2015, 16:51:23 tquirk
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

base_user::base_user(uint64_t u, Control *c)
{
    this->init(u, c);
}

base_user::~base_user()
{
}

void base_user::init(uint64_t u, Control *c)
{
    this->userid = u;
    this->control = c;
    this->timestamp = time(NULL);
    this->pending_logout = false;
    /* Come up with some sort of random sequence number to start? */
    this->sequence = 0L;
}

bool base_user::operator<(const base_user& u) const
{
    return (this->userid < u.userid);
}

bool base_user::operator==(const base_user& u) const
{
    return (this->userid == u.userid);
}

const base_user& base_user::operator=(const base_user& u)
{
    this->userid = u.userid;
    this->control = u.control;
    this->timestamp = u.timestamp;
    this->pending_logout = u.pending_logout;
    return *this;
}

listen_socket::listen_socket(struct addrinfo *ai)
    : users(), sock(ai)
{
    this->init();
}

listen_socket::~listen_socket()
{
    int retval;
    listen_socket::users_iterator i;

    try { this->stop(); }
    catch (std::exception& e) { /* Do nothing */ }

    if (this->reaper_running)
    {
        if ((retval = pthread_cancel(this->reaper)) != 0)
        {
            std::clog << syslogErr << "couldn't cancel reaper thread for port "
                      << this->sock.sa->port() << ": "
                      << strerror(retval) << " (" << retval << ")" << std::endl;
        }
        sleep(0);
        if ((retval = pthread_join(this->reaper, NULL)) != 0)
            std::clog << syslogErr
                      << "error terminating reaper thread for port "
                      << this->sock.sa->port() << ": "
                      << strerror(retval) << " (" << retval << ")" << std::endl;
    }

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

    this->reaper_running = false;
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
    uint64_t userid = 0LL, char_objid = 0LL;
    std::string username(p.buf.log.username, sizeof(p.buf.log.username));
    std::string password(p.buf.log.password, sizeof(p.buf.log.password));
    std::string charname(p.buf.log.charname, sizeof(p.buf.log.charname));

    userid = database->check_authentication(username, password);

    /* Don't want to keep passwords around in core if we can help it */
    memset(p.buf.log.password, 0, sizeof(p.buf.log.password));
    password.clear();

    char_objid = database->get_character_objectid(charname);

    if (userid != 0LL && char_objid != 0LL)
    {
        if (this->users.find(userid) == this->users.end())
        {
            Control *newcontrol = new Control(userid, NULL);
            newcontrol->username = username;

            /* Add this user to the userlist */
            this->do_login(userid, newcontrol, p);

            std::clog << "logged in user "
                      << newcontrol->username
                      << " (" << userid << ")" << std::endl;

            /* Hook the new control up to the appropriate object */
            zone->connect_game_object(newcontrol, char_objid);
        }
    }
    /* Otherwise, do nothing, and send nothing */
}

void listen_socket::logout_user(access_list& p)
{
    packet_list pkt;
    listen_socket::users_iterator found;

    /* Most of this function is now handled by the reaper threads */
    if ((found = this->users.find(p.what.logout.who)) != this->users.end())
    {
        base_user *bu = found->second;
        bu->pending_logout = true;
        std::clog << "logout request from "
                  << bu->control->username
                  << " (" << bu->control->userid << ")" << std::endl;

        pkt.buf.ack.type = TYPE_ACKPKT;
        pkt.buf.ack.version = 1;
        pkt.buf.ack.sequence = bu->sequence++;
        pkt.buf.ack.request = TYPE_LGTREQ;
        pkt.buf.ack.misc = 0;
        pkt.who = bu->control;
        pkt.parent = this;
        this->send_pool->push(pkt);
    }
}
