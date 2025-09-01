/* listensock.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *
 * Revision IX game server
 * Copyright (C) 2015-2021  Trinity Annabelle Quirk
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
 * for our sockets.  Most of the login and logout mechanisms are also
 * here; each protocol-specific class extends connect_user() and
 * disconnect_user() slightly for its own needs, but the bulk of the
 * logic is in this class.
 *
 * Things to do
 *
 */

#include <openssl/crypto.h>
#include <proto/dh.h>
#include <proto/ec.h>

#include <sstream>
#include <system_error>

#include "listensock.h"
#include "zone.h"
#include "config_data.h"

#include "../server.h"

static size_t rand_bytes(uint8_t *buf, size_t buf_sz)
{
    size_t i;

    srand(time(NULL));
    for (i = 0; i < buf_sz; ++i)
        buf[i] = rand() % 256;
    return i;
}

base_user::base_user(uint64_t userid)
    : username(), charactername(), Control(userid, NULL)
{
    this->parent = NULL;
    this->timestamp = time(NULL);
    this->pending_logout = false;
    this->sequence = 0LL;
    this->auth_level = ACCESS_NONE;
    this->slave = this->default_slave = NULL;
    this->characterid = 0LL;
}

base_user::base_user(uint64_t userid,
                     const std::string& uname,
                     const std::string& cname,
                     listen_socket *l)
    : username(uname), charactername(cname), Control(userid, NULL)
{
    this->parent = l;
    this->timestamp = time(NULL);
    this->pending_logout = false;
    /* Come up with some sort of random sequence number to start? */
    this->sequence = 0LL;

    this->auth_level = database->check_authorization(userid, cname);
    if (this->auth_level < ACCESS_VIEW)
        throw std::runtime_error("unauthorized user");
    if (this->auth_level >= ACCESS_MOVE)
    {
        uint64_t objid = database->get_character_objectid(userid, cname);
        this->default_slave = this->slave = zone->find_game_object(objid);
        this->slave->connect(this);
    }
    this->characterid = database->get_characterid(userid, cname);
    database->get_player_server_skills(this->userid,
                                       this->characterid,
                                       this->actions);

    rand_bytes(this->iv, R9_SYMMETRIC_IV_BUF_SZ);
}

base_user::~base_user()
{
    if (this->slave != NULL)
        /* Clean up a user who has logged out */
        this->slave->deactivate();
}

const base_user& base_user::operator=(const base_user& u)
{
    this->Control::operator=((const Control&)u);
    this->username = u.username;
    this->timestamp = u.timestamp;
    this->pending_logout = u.pending_logout;
    this->sequence = u.sequence;
    return *this;
}

std::string base_user::to_string(void)
{
    std::ostringstream s;
    uint64_t obj_id = (this->default_slave != NULL
                       ? this->default_slave->get_object_id()
                       : 0LL);

    s << this->username
      << " (" << this->userid
      << "), char " << this->charactername
      << " (" << this->characterid
      << ") obj " << obj_id
      << ", " << this->actions.size()
      << " actions, auth " << (int)this->auth_level;
    return s.str();
}

void base_user::set_shared_key(EVP_PKEY *priv, uint8_t *pubkey, size_t key_sz)
{
    EVP_PKEY *pub = NULL;
    struct dh_message *shared = NULL;

    if ((pub = public_key_to_pkey(pubkey, key_sz)) == NULL)
        throw std::runtime_error("couldn't convert string to EVP_PKEY");
    if ((shared = dh_shared_secret(priv, pub)) == NULL)
    {
        OPENSSL_free(pub);
        throw std::runtime_error("couldn't calculate shared secret");
    }
    memcpy(this->key, shared->message, R9_SYMMETRIC_KEY_BUF_SZ);
    free_dh_message(shared);
    OPENSSL_free(pub);
}

int base_user::encrypt_packet(packet& pkt)
{
    size_t buf_sz = packet_size(&pkt) - sizeof(basic_packet);

    if (pkt.basic.type == TYPE_SRVKEY)
        return 1;
    if (buf_sz > 0)
    {
        uint8_t *buf = (uint8_t *)&pkt + sizeof(basic_packet);

        return r9_encrypt(buf, buf_sz,
                          this->key,
                          this->iv, pkt.basic.sequence,
                          buf);
    }
    return 1;
}

int base_user::decrypt_packet(packet& pkt)
{
    size_t buf_sz = packet_size(&pkt) - sizeof(basic_packet);

    if (pkt.basic.type == TYPE_LOGREQ)
        return 1;
    if (buf_sz > 0)
    {
        uint8_t *buf = (uint8_t *)&pkt + sizeof(basic_packet);

        return r9_decrypt(buf, buf_sz,
                          this->key,
                          this->iv, pkt.basic.sequence,
                          buf);
    }
    return 1;
}

void base_user::send_server_key(uint8_t *pubkey, size_t key_sz)
{
    packet_list pkt;

    pkt.buf.key.type = TYPE_SRVKEY;
    pkt.buf.key.version = R9_PROTO_VER;
    pkt.buf.key.sequence = this->sequence++;
    memset(pkt.buf.key.pubkey, 0, sizeof(pkt.buf.key.pubkey));
    memcpy(pkt.buf.key.pubkey, pubkey, std::min(key_sz,
                                                sizeof(pkt.buf.key.pubkey)));
    memcpy(pkt.buf.key.iv, this->iv, R9_SYMMETRIC_IV_BUF_SZ);
    pkt.who = this;
    this->parent->send_pool->push(pkt);
}

void base_user::send_ping(void)
{
    packet_list pkt;

    pkt.buf.basic.type = TYPE_PNGPKT;
    pkt.buf.basic.version = R9_PROTO_VER;
    pkt.buf.basic.sequence = this->sequence++;
    pkt.who = this;
    this->parent->send_pool->push(pkt);
}

void base_user::send_ack(uint8_t req,
                         uint64_t misc0, uint64_t misc1,
                         uint64_t misc2, uint64_t misc3)
{
    packet_list pkt;

    pkt.buf.ack.type = TYPE_ACKPKT;
    pkt.buf.ack.version = R9_PROTO_VER;
    pkt.buf.ack.sequence = this->sequence++;
    pkt.buf.ack.request = req;
    pkt.buf.ack.misc[0] = misc0;
    pkt.buf.ack.misc[1] = misc1;
    pkt.buf.ack.misc[2] = misc2;
    pkt.buf.ack.misc[3] = misc3;
    pkt.who = this;
    this->parent->send_pool->push(pkt);
}

listen_socket::listen_socket()
    : users(), sock()
{
    this->init();
}

void listen_socket::init(void)
{
    this->send_pool = new ThreadPool<packet_list>("send", config.send_threads);
    this->access_pool = new ThreadPool<access_list>("access",
                                                    config.access_threads);
    this->access_pool->clean_on_pop = true;

    this->reaper_running = false;
}

listen_socket::listen_socket(struct addrinfo *ai)
    : users(), sock(ai)
{
    this->init();
}

listen_socket::~listen_socket()
{
    try { this->stop(); }
    catch (std::exception& e) { /* Do nothing */ }

    delete this->send_pool;
    delete this->access_pool;

    this->users.erase(this->users.begin(), this->users.end());
}

std::string listen_socket::port_type(void)
{
    return "listen";
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
          << this->sock.sa->port();
        throw std::system_error(retval, std::generic_category(), s.str());
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
              << " port " << this->sock.sa->port();
            throw std::system_error(retval, std::generic_category(), s.str());
        }
        sleep(0);
        if ((retval = pthread_join(this->reaper, NULL)) != 0)
        {
            std::ostringstream s;

            s << "couldn't join reaper thread for " << this->port_type()
              << " port " << this->sock.sa->port();
            throw std::system_error(retval, std::generic_category(), s.str());
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
    time_t now, link_dead, sleepy;

    std::clog << "started reaper thread for " << ls->port_type()
              << " port " << ls->sock.sa->port() << std::endl;
    for (;;)
    {
        sleep(listen_socket::REAP_TIMEOUT);
        now = time(NULL);
        link_dead = now - listen_socket::LINK_DEAD_TIMEOUT;
        sleepy = now - listen_socket::PING_TIMEOUT;
        i = ls->users.begin();
        while (i != ls->users.end())
        {
            pthread_testcancel();
            bu = (*i).second;
            if (bu->pending_logout == true || bu->timestamp < link_dead)
            {
                /* We'll consider the user link-dead */
                std::clog << "removing user " << bu->username << " ("
                          << bu->userid << ") from " << ls->port_type()
                          << " port " << ls->sock.sa->port() << std::endl;
                ++i;
                ls->disconnect_user(bu);
                continue;
            }
            else if (bu->timestamp < sleepy && bu->pending_logout == false)
                bu->send_ping();
            ++i;
        }
        pthread_testcancel();
    }
    return NULL;
}

void listen_socket::handle_ack(listen_socket *s, packet& p,
                               base_user *u, void *unused)
{
    if (u != NULL)
        u->timestamp = time(NULL);
}

void listen_socket::handle_action(listen_socket *s, packet& p,
                                  base_user *u, void *unused)
{
    packet_list pl;

    if (u != NULL)
    {
        u->timestamp = time(NULL);
        memcpy(&pl.buf, &p, sizeof(action_request));
        pl.who = u;
        action_pool->push(pl);
    }
}

void listen_socket::handle_logout(listen_socket *s, packet& p,
                                  base_user *u, void *unused)
{
    access_list al;

    if (u != NULL)
    {
        u->timestamp = time(NULL);
        memcpy(&al.buf, &p, sizeof(basic_packet));
        al.what.logout.who = u->userid;
        s->access_pool->push(al);
    }
}

void listen_socket::login_user(access_list& p)
{
    std::string username(p.buf.log.username,
                         std::min(sizeof(p.buf.log.username),
                                  strlen(p.buf.log.username)));
    uint64_t userid = database->check_authentication(username,
                                                     p.buf.log.pubkey,
                                                     R9_PUBKEY_SZ);
    if (userid == 0LL)
        return;

    /* If they're already in our list, turn off any pending logout. */
    if (this->users.find(userid) != this->users.end())
    {
        /* This is going to be a race with the reaper thread. */
        this->users[userid]->pending_logout = false;
        return;
    }

    base_user *bu = NULL;
    try
    {
        std::string charname(p.buf.log.charname,
                             std::min(sizeof(p.buf.log.charname),
                                      strlen(p.buf.log.charname)));
        bu = new base_user(userid, username, charname, this);
        bu->set_shared_key(config.key.priv_key,
                           p.buf.log.pubkey, R9_PUBKEY_SZ);
    }
    catch (std::runtime_error e)
    {
        if (bu != NULL)
            delete bu;
        return;
    }

    std::clog << "login for user " << bu->to_string() << std::endl;
    this->connect_user(bu, p);
    zone->send_nearby_objects(bu->characterid);
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

        bu->send_ack(TYPE_LGTREQ);
        bu->pending_logout = true;
        if (bu->default_slave != NULL)
            bu->default_slave->deactivate();
    }
}

void listen_socket::connect_user(base_user *bu, access_list& al)
{
    uint64_t obj_id = 0LL;

    this->users[bu->userid] = bu;
    if (bu->default_slave != NULL)
    {
        bu->default_slave->activate();
        obj_id = bu->default_slave->get_object_id();
    }
    bu->send_server_key(config.key.pub_key, R9_PUBKEY_SZ);
    bu->send_ack(TYPE_LOGREQ, bu->auth_level, obj_id);
}

void listen_socket::disconnect_user(base_user *bu)
{
    this->users.erase(bu->userid);
}
