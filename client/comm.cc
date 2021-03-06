/* comm.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 17 Apr 2021, 08:09:34 tquirk
 *
 * Revision IX game client
 * Copyright (C) 2021  Trinity Annabelle Quirk
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

#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

#include <cstdint>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "client_core.h"
#include "comm.h"
#include "l10n.h"
#include "configdata.h"

#include "../proto/dh.h"
#include "../proto/ec.h"

uint64_t Comm::sequence = 0LL;

static const std::string access_to_string(int access)
{
    const std::string access_type[5] =
        {
            translate("Access type", "unknown"),
            translate("Access type", "no"),         /* ACCESS_NONE */
            translate("Access type", "view-only"),  /* ACCESS_VIEW */
            translate("Access type", "regular"),    /* ACCESS_MOVE */
            translate("Access type", "modify")      /* ACCESS_MDFY */
        };

    if (access < 1 || access > 4)
        access = 0;
    return access_type[access];
}

/* Jump table for protocol handling */
Comm::pkt_handler Comm::pkt_type[] =
{
    &Comm::handle_ackpkt,       /* Ack             */
    &Comm::handle_unsupported,  /* Login req       */
    &Comm::handle_unsupported,  /* Action req      */
    &Comm::handle_posupd,       /* Position update */
    &Comm::handle_srvnot,       /* Server notice   */
    &Comm::handle_pngpkt,       /* Ping            */
    &Comm::handle_unsupported,  /* Logout req      */
    &Comm::handle_srvkey        /* Server key      */
};

#define COMM_MEMBER(a, b) ((a).*(b))

void Comm::create_socket(struct addrinfo *ai)
{
    if ((this->sock = socket(ai->ai_family,
                             ai->ai_socktype,
                             ai->ai_protocol)) < 0)
    {
        std::ostringstream s;
        char err[128];

        strerror_r(errno, err, sizeof(err));
        s << format(translate("Error opening socket: {1} ({2})")) % err % errno;
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

int Comm::encrypt_packet(packet& p)
{
    size_t pkt_sz;

    if (p.basic.type == TYPE_LOGREQ)
        return 1;
    if ((pkt_sz = packet_size(&p) - sizeof(basic_packet)) > 0)
    {
        uint8_t *pkt = (uint8_t *)&p + sizeof(basic_packet);

        return r9_encrypt(pkt, pkt_sz,
                          this->key,
                          this->iv, p.basic.sequence,
                          pkt);
    }
    return 1;
}

int Comm::decrypt_packet(packet& p)
{
    size_t pkt_sz;

    if (p.basic.type == TYPE_SRVKEY)
        return 1;
    if ((pkt_sz = packet_size(&p) - sizeof(basic_packet)) > 0)
    {
        uint8_t *pkt = (uint8_t *)&p + sizeof(basic_packet);

        return r9_decrypt(pkt, pkt_sz,
                          this->key,
                          this->iv, p.basic.sequence,
                          pkt);
    }
    return 1;
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
        while (comm->send_queue.empty() && !comm->thread_exit_flag)
            pthread_cond_wait(&(comm->send_queue_not_empty),
                              &(comm->send_lock));

        if (comm->thread_exit_flag)
        {
            pthread_mutex_unlock(&(comm->send_lock));
            pthread_exit(NULL);
        }

        pkt = comm->send_queue.front();

        if (sendto(comm->sock,
                   (void *)pkt, packet_size(pkt),
                   0,
                   (struct sockaddr *)&comm->remote,
                   comm->remote_size) == -1)
        {
            char err[128];

            strerror_r(errno, err, sizeof(err));
            std::clog << format(translate("Error sending packet: {1} ({2})"))
                % err % errno
                      << std::endl;
        }
        comm->send_queue.pop();
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
            char err[128];

            strerror_r(errno, err, sizeof(err));
            std::clog << format(translate("Error receiving packet: {1} ({2})"))
                % err % errno
                      << std::endl;
            continue;
        }
        /* Verify that the sender is who we think it should be */
        if (memcmp(&sin, &(comm->remote), fromlen))
        {
            std::clog << translate("Packet from unknown sender")
                      << std::endl;
            continue;
        }
        /* Needs to be a real packet type */
        if (buf.basic.type >= sizeof(pkt_type) / sizeof(pkt_handler))
        {
            std::clog << format(translate("Unknown packet type {1}"))
                % (int)buf.basic.type
                      << std::endl;
            continue;
        }
        if (!comm->decrypt_packet(buf))
        {
            std::clog << translate("Error decrypting packet")
                      << std::endl;
            continue;
        }
        /* We should be able to convert to host byte ordering */
        if (!ntoh_packet(&buf, len))
        {
            std::clog << translate("Error ntoh'ing packet") << std::endl;
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
    ack_packet& a = (ack_packet&)p.ack;

    switch (a.request)
    {
      case TYPE_LOGREQ:
        /* The response to our login request */
        std::clog << format(translate("Login response, {1} access"))
            % access_to_string(a.misc[0])
                  << std::endl;
        this->src_object_id = a.misc[1];
        self_obj = &((*obj)[this->src_object_id]);
        break;

      case TYPE_LGTREQ:
        /* The response to our logout request */
        std::clog << format(translate("Logout response, {1} access"))
            % access_to_string(a.misc[0])
                  << std::endl;
        break;

      default:
        break;
    }
}

void Comm::handle_posupd(packet& p)
{
    position_update& u = (position_update&)p.pos;

    move_object(u.object_id,
                u.frame_number,
                (float)u.x_pos / POSUPD_POS_SCALE,
                (float)u.y_pos / POSUPD_POS_SCALE,
                (float)u.z_pos / POSUPD_POS_SCALE,
                (float)u.w_orient / POSUPD_ORIENT_SCALE,
                (float)u.x_orient / POSUPD_ORIENT_SCALE,
                (float)u.y_orient / POSUPD_ORIENT_SCALE,
                (float)u.z_orient / POSUPD_ORIENT_SCALE,
                (float)u.x_look / POSUPD_LOOK_SCALE,
                (float)u.y_look / POSUPD_LOOK_SCALE,
                (float)u.z_look / POSUPD_LOOK_SCALE);
}

void Comm::handle_srvnot(packet& p)
{
    std::clog << "Got a server notice" << std::endl;
}

void Comm::handle_srvkey(packet& p)
{
    EVP_PKEY *pub = NULL;
    struct dh_message *shared = NULL;

    /* TODO: there should be some authentication done here.  Perhaps a
     * known_hosts mechanism like openssh?
     */
    if ((pub = public_key_to_pkey(p.key.pubkey, R9_PUBKEY_SZ)) == NULL)
    {
        std::clog << translate("Bad public key from server") << std::endl;
        return;
    }
    if ((shared = dh_shared_secret(config.priv_key, pub)) == NULL)
    {
        OPENSSL_free(pub);
        std::clog << translate("Could not derive shared secret with server")
                  << std::endl;
        return;
    }
    memcpy(this->key, shared->message, R9_SYMMETRIC_KEY_BUF_SZ);
    free_dh_message(shared);
    OPENSSL_free(pub);
    memcpy(this->iv, p.key.iv, R9_SYMMETRIC_IV_BUF_SZ);
}

void Comm::handle_unsupported(packet& p)
{
    std::clog << format(translate("Unknown packet type {1}"))
        % (int)p.basic.type
              << std::endl;
}

Comm::Comm(void)
    : send_queue(), thread_exit_flag(false)
{
    this->init();
}

void Comm::init(void)
{
    int ret;

    /* Init the mutex and cond variables */
    if ((ret = pthread_mutex_init(&(this->send_lock), NULL)) != 0)
    {
        char err[128];

        strerror_r(errno, err, sizeof(err));
        std::ostringstream s;
        s << format(translate("Error initializing queue mutex: {1} ({2})"))
            % err % ret;
        throw std::runtime_error(s.str());
    }
    if ((ret = pthread_cond_init(&send_queue_not_empty, NULL)) != 0)
    {
        char err[128];

        strerror_r(errno, err, sizeof(err));
        std::ostringstream s;
        s << format(translate("Error initializing queue-not-empty cond: "
                              "{1} ({2})"))
            % err % ret;
        pthread_mutex_destroy(&(this->send_lock));
        throw std::runtime_error(s.str());
    }

    this->src_object_id = 0LL;
    this->threads_started = false;
}

Comm::Comm(struct addrinfo *ai)
    : send_queue(), thread_exit_flag(false)
{
    this->create_socket(ai);
    this->init();
}

Comm::~Comm()
{
    try { this->stop(); }
    catch (std::exception e) { std::clog << e.what() << std::endl; }
    if (this->sock)
        close(this->sock);
    pthread_cond_destroy(&(this->send_queue_not_empty));
    pthread_mutex_destroy(&(this->send_lock));
}

void Comm::start(void)
{
    int ret;

    if (this->threads_started)
        return;

    /* Now start up the actual threads */
    this->thread_exit_flag = false;
    if ((ret = pthread_create(&(this->send_thread),
                              NULL,
                              Comm::send_worker,
                              (void *)this)) != 0)
    {
        std::ostringstream s;
        char err[128];

        strerror_r(errno, err, sizeof(err));
        s << format(translate("Error starting send thread: {1} ({2})"))
            % err % ret;
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
        char err[128];

        strerror_r(errno, err, sizeof(err));
        s << format(translate("Error starting receive thread: {1} ({2})"))
            % err % ret;
        pthread_cancel(this->send_thread);
        sleep(0);
        pthread_join(this->send_thread, NULL);
        pthread_cond_destroy(&(this->send_queue_not_empty));
        pthread_mutex_destroy(&(this->send_lock));
        throw std::runtime_error(s.str());
    }
    this->threads_started = true;
}

void Comm::stop(void)
{
    int ret;

    if (this->threads_started)
    {
        if (this->sock)
            this->send_logout();
        sleep(0);
        this->thread_exit_flag = true;
        if ((ret = pthread_cond_broadcast(&(this->send_queue_not_empty))) != 0)
        {
            std::ostringstream s;
            char err[128];

            strerror_r(ret, err, sizeof(err));
            s << format(translate("Error waking send thread: {1} ({2})"))
                % err % ret;
            throw std::runtime_error(s.str());
        }
        sleep(0);
        if ((ret = pthread_join(this->send_thread, NULL)) != 0)
        {
            std::ostringstream s;
            char err[128];

            strerror_r(ret, err, sizeof(err));
            s << format(translate("Error joining send thread: {1} ({2})"))
                % err % ret;
            throw std::runtime_error(s.str());
        }
        if ((ret = pthread_cancel(this->recv_thread)) != 0)
        {
            std::ostringstream s;
            char err[128];

            strerror_r(ret, err, sizeof(err));
            s << format(translate("Error cancelling receive thread: {1} ({2})"))
                % err % ret;
            throw std::runtime_error(s.str());
        }
        sleep(0);
        if ((ret = pthread_join(this->recv_thread, NULL)) != 0)
        {
            std::ostringstream s;
            char err[128];

            strerror_r(ret, err, sizeof(err));
            s << format(translate("Error joining receive thread: {1} ({2})"))
                % err % ret;
            throw std::runtime_error(s.str());
        }
        this->threads_started = false;
    }
}

void Comm::send(packet *p, size_t len)
{
    pthread_mutex_lock(&(this->send_lock));
    if (!hton_packet(p, len))
    {
        std::clog << translate("Error hton'ing packet") << std::endl;
        delete p;
    }
    else if (!this->encrypt_packet(*p))
    {
        std::clog << translate("Error encrypting packet") << std::endl;
        delete p;
    }
    else
    {
        this->send_queue.push(p);
        pthread_cond_signal(&(this->send_queue_not_empty));
    }
    pthread_mutex_unlock(&(this->send_lock));
}

void Comm::send_login(const std::string& user,
                      const std::string& character)
{
    packet *req = new packet;

    memset(req, 0, sizeof(packet));
    req->log.type = TYPE_LOGREQ;
    req->log.version = R9_PROTO_VER;
    req->log.sequence = this->sequence++;
    strncpy(req->log.username, user.c_str(), sizeof(req->log.username));
    strncpy(req->log.charname, character.c_str(), sizeof(req->log.charname));
    memcpy(req->log.pubkey, config.pub_key, R9_PUBKEY_SZ);
    this->send(req, sizeof(login_request));
}

void Comm::send_action_request(uint16_t actionid,
                               uint64_t target,
                               uint8_t power)
{
    packet *req = new packet;

    memset(req, 0, sizeof(packet));
    req->act.type = TYPE_ACTREQ;
    req->act.version = R9_PROTO_VER;
    req->act.sequence = sequence++;
    req->act.object_id = this->src_object_id;
    req->act.action_id = actionid;
    req->act.power_level = power;
    req->act.dest_object_id = target;
    this->send(req, sizeof(action_request));
}

void Comm::send_action_request(uint16_t actionid,
                               glm::vec3& direction,
                               uint8_t power)
{
    packet *req = new packet;

    memset(req, 0, sizeof(packet));
    req->act.type = TYPE_ACTREQ;
    req->act.version = R9_PROTO_VER;
    req->act.sequence = sequence++;
    req->act.object_id = this->src_object_id;
    req->act.action_id = actionid;
    req->act.x_pos_dest = (uint64_t)(direction.x * ACTREQ_POS_SCALE);
    req->act.y_pos_dest = (uint64_t)(direction.y * ACTREQ_POS_SCALE);
    req->act.z_pos_dest = (uint64_t)(direction.z * ACTREQ_POS_SCALE);
    req->act.power_level = power;
    this->send(req, sizeof(action_request));
}

void Comm::send_logout(void)
{
    packet *req = new packet;

    memset(req, 0, sizeof(packet));
    req->basic.type = TYPE_LGTREQ;
    req->basic.version = R9_PROTO_VER;
    req->basic.sequence = sequence++;
    this->send(req, sizeof(basic_packet));
}

void Comm::send_ack(uint8_t type)
{
    packet *req = new packet;

    memset(req, 0, sizeof(packet));
    req->ack.type = TYPE_ACKPKT;
    req->ack.version = R9_PROTO_VER;
    req->ack.sequence = sequence++;
    req->ack.request = type;
    this->send(req, sizeof(ack_packet));
}
