/* listensock.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 16 Aug 2017, 17:35:18 tquirk
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

base_user::base_user(uint64_t u, GameObject *g, listen_socket *l)
    : Control(u, g)
{
    this->parent = l;
    this->timestamp = time(NULL);
    this->pending_logout = false;
    /* Come up with some sort of random sequence number to start? */
    this->sequence = 0L;
}

base_user::~base_user()
{
    if (this->slave != NULL)
    {
        /* Clean up a user who has logged out */
        this->slave->natures.insert("invisible");
        this->slave->natures.insert("non-interactive");
    }
}

const base_user& base_user::operator=(const base_user& u)
{
    this->Control::operator=((const Control&)u);
    this->timestamp = u.timestamp;
    this->pending_logout = u.pending_logout;
    this->sequence = u.sequence;
    return *this;
}

void base_user::send_ping(void)
{
    packet_list pkt;

    pkt.buf.basic.type = TYPE_PNGPKT;
    pkt.buf.basic.version = 1;
    pkt.buf.basic.sequence = this->sequence++;
    pkt.who = this;
    this->parent->send_pool->push(pkt);
}

void base_user::send_ack(uint8_t req, uint8_t misc)
{
    packet_list pkt;

    pkt.buf.ack.type = TYPE_ACKPKT;
    pkt.buf.ack.version = 1;
    pkt.buf.ack.sequence = this->sequence++;
    pkt.buf.ack.request = req;
    pkt.buf.ack.misc = misc;
    pkt.who = this;
    this->parent->send_pool->push(pkt);
}

listen_socket::listen_socket(struct addrinfo *ai)
    : users(), sock(ai)
{
    this->send_pool = new ThreadPool<packet_list>("send", config.send_threads);
    this->access_pool = new ThreadPool<access_list>("access",
                                                    config.access_threads);
    this->access_pool->clean_on_pop = true;

    this->reaper_running = false;
}

listen_socket::~listen_socket()
{
    listen_socket::users_iterator i;

    try { this->stop(); }
    catch (std::exception& e) { /* Do nothing */ }

    delete this->send_pool;
    delete this->access_pool;

    this->users.erase(this->users.begin(), this->users.end());
}

void listen_socket::start(void)
{
    int retval;

    std::clog << "starting connection loop for "
              << this->port_type() << " port "
              << this->sock.sa->port() << std::endl;

    /* Start up the reaping thread */
    if ((retval = pthread_create(&this->reaper, NULL,
                                 reaper_worker, (void *)this)) != 0)
    {
        std::ostringstream s;
        s << "couldn't create reaper thread for "
          << this->port_type() << " port "
          << this->sock.sa->port() << ": "
          << strerror(retval) << " (" << retval << ")";
        throw std::runtime_error(s.str());
    }
    this->reaper_running = true;

    sleep(0);
    this->access_pool->startup_arg = (void *)this;
    this->access_pool->start(listen_socket::access_pool_worker);
}

void listen_socket::stop(void)
{
    int retval;

    if (this->reaper_running)
    {
        if ((retval = pthread_cancel(this->reaper)) != 0)
        {
            std::ostringstream s;
            s << "couldn't cancel reaper thread for " << this->port_type()
              << " port " << this->sock.sa->port() << ": "
              << strerror(retval) << " (" << retval << ")";
            throw std::runtime_error(s.str());
        }
        sleep(0);
        if ((retval = pthread_join(this->reaper, NULL)) != 0)
        {
            std::ostringstream s;
            s << "couldn't join reaper thread for " << this->port_type()
              << " port " << this->sock.sa->port() << ": "
              << strerror(retval) << " (" << retval << ")";
            throw std::runtime_error(s.str());
        }
        this->reaper_running = false;
    }

    this->send_pool->stop();
    this->access_pool->stop();
    this->sock.stop();
}

void *listen_socket::access_pool_worker(void *arg)
{
    listen_socket *ls = (listen_socket *)arg;
    access_list req;

    std::clog << "started access pool worker for " << ls->port_type()
              << " port " << ls->sock.sa->port() << std::endl;
    for (;;)
    {
        ls->access_pool->pop(&req);

        ntoh_packet(&(req.buf), sizeof(packet));

        if (req.buf.basic.type == TYPE_LOGREQ)
            ls->login_user(req);
        else if (req.buf.basic.type == TYPE_LGTREQ)
            ls->logout_user(req.what.logout.who);

        /* Otherwise, we don't recognize it, and will ignore it */
    }
    return NULL;
}

void *listen_socket::reaper_worker(void *arg)
{
    listen_socket *ls = (listen_socket *)arg;
    listen_socket::users_iterator i;
    base_user *bu;
    time_t now;

    std::clog << "started reaper thread for " << ls->port_type()
              << " port " << ls->sock.sa->port() << std::endl;
    for (;;)
    {
        sleep(listen_socket::REAP_TIMEOUT);
        now = time(NULL);
        for (i = ls->users.begin(); i != ls->users.end(); ++i)
        {
            pthread_testcancel();
            bu = (*i).second;
            if (bu->pending_logout == true
                || bu->timestamp < now - listen_socket::LINK_DEAD_TIMEOUT)
            {
                /* We'll consider the user link-dead */
                std::clog << "removing user " << bu->username << " ("
                          << bu->userid << ") from " << ls->port_type()
                          << " port " << ls->sock.sa->port() << std::endl;
                --i;
                ls->disconnect_user(bu);
            }
            else if (bu->timestamp < now - listen_socket::PING_TIMEOUT
                     && bu->pending_logout == false)
                bu->send_ping();
        }
        pthread_testcancel();
    }
    return NULL;
}

void listen_socket::login_user(access_list& p)
{
    uint64_t userid = this->get_userid(p.buf.log);

    if (userid == 0LL)
        return;

    /* If they're already in our list, turn off any pending logout. */
    if (this->users.find(userid) != this->users.end())
    {
        /* This is going to be a race with the reaper thread. */
        this->users[userid]->pending_logout = false;
        return;
    }

    base_user *bu = this->check_access(userid, p.buf.log);

    if (bu == NULL)
        return;

    this->connect_user(bu, p);
}

uint64_t listen_socket::get_userid(login_request& log)
{
    std::string username(log.username, std::min(sizeof(log.username),
                                                strlen(log.username)));
    std::string password(log.password, std::min(sizeof(log.password),
                                                strlen(log.password)));
    uint64_t userid = database->check_authentication(username, password);

    /* Don't want to keep passwords around in core if we can help it. */
    memset(log.password, 0, sizeof(log.password));
    password.clear();

    return userid;
}

base_user *listen_socket::check_access(uint64_t userid, login_request& log)
{
    std::string charname(log.charname, std::min(sizeof(log.charname),
                                                strlen(log.charname)));
    uint64_t charid = database->get_character_objectid(userid, charname);
    int auth_level = database->check_authorization(userid, charid);
    GameObject *go = NULL;

    if (auth_level < ACCESS_VIEW)
        return NULL;

    if (auth_level >= ACCESS_MOVE)
        go = zone->find_game_object(charid);

    base_user *bu = new base_user(userid, go, this);
    bu->username = std::string(log.username, std::min(sizeof(log.username),
                                                      strlen(log.username)));
    bu->auth_level = auth_level;

    std::clog << "login for user " << bu->username << " (" << bu->userid
              << "), char " << charname << " (" << charid
              << "), auth " << auth_level << std::endl;

    return bu;
}

void listen_socket::logout_user(uint64_t userid)
{
    packet_list pkt;
    listen_socket::users_iterator found;

    /* The reaper threads take care of the actual removing of the user
     * and whatnot.  We just set the flag.
     */
    if ((found = this->users.find(userid)) != this->users.end())
    {
        base_user *bu = found->second;
        std::clog << "logout request from " << bu->username
                  << " (" << bu->userid << ")" << std::endl;

        bu->send_ack(TYPE_LGTREQ, 0);
        bu->pending_logout = true;
    }
}

void listen_socket::connect_user(base_user *bu, access_list& al)
{
    this->users[bu->userid] = bu;
    bu->send_ack(TYPE_LOGREQ, bu->auth_level);
}

void listen_socket::disconnect_user(base_user *bu)
{
    this->users.erase(bu->userid);
}
