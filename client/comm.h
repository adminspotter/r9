/* comm.h                                                  -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 19 Apr 2018, 07:44:02 tquirk
 *
 * Revision IX game client
 * Copyright (C) 2018  Trinity Annabelle Quirk
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
 * This file contains the class declaration for server communication.
 *
 * Things to do
 *
 */

#ifndef __INC_R9CLIENT_COMM_H__
#define __INC_R9CLIENT_COMM_H__

#include <config.h>

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif /* HAVE_SYS_SOCKET_H */
#if HAVE_NETDB_H
#include <netdb.h>
#endif /* HAVE_NETDB_H */
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif /* HAVE_NETINET_IN_H */
#include <pthread.h>

#include <cstdint>
#include <string>
#include <queue>
#include <atomic>

#include <glm/vec3.hpp>

#include "../proto/proto.h"

class Comm
{
  protected:
    int sock;
    struct sockaddr_storage remote;
    size_t remote_size;

    pthread_t send_thread, recv_thread;
    pthread_mutex_t send_lock;
    pthread_cond_t send_queue_not_empty;
    std::queue<packet *> send_queue;

    static uint32_t sequence;
    uint64_t src_object_id;
    std::atomic<bool> thread_exit_flag;

    typedef void (Comm::*pkt_handler)(packet&);
    static pkt_handler pkt_type[7];

    void create_socket(struct addrinfo *);

    static void *send_worker(void *);
    static void *recv_worker(void *);

    void handle_pngpkt(packet&);
    void handle_ackpkt(packet&);
    void handle_posupd(packet&);
    void handle_srvnot(packet&);
    void handle_unsupported(packet&);

  public:
    Comm(struct addrinfo *);
    virtual ~Comm();

    virtual void start(void);
    virtual void stop(void);

    virtual void send(packet *, size_t);

    virtual void send_login(const std::string&,
                            const std::string&,
                            const std::string&);
    virtual void send_action_request(uint16_t, uint64_t, uint8_t);
    virtual void send_action_request(uint16_t, glm::vec3&, uint8_t);
    virtual void send_logout(void);
    virtual void send_ack(uint8_t);
};

#endif /* __INC_R9CLIENT_COMM_H__ */
