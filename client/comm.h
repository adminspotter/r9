/* comm.h                                                  -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 24 Jul 2015, 12:19:11 tquirk
 *
 * Revision IX game client
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
 * This file contains the class declaration for server communication.
 *
 * Changes
 *   23 Jul 2014 TAQ - Created the file.
 *   24 Jul 2014 TAQ - Converted to stdint types.
 *
 * Things to do
 *
 */

#ifndef __INC_R9CLIENT_COMM_H__
#define __INC_R9CLIENT_COMM_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>

#include <cstdint>
#include <queue>

class Comm
{
  private:
    int sock;
    struct sockaddr_storage remote;

    pthread_t send_thread, recv_thread;
    pthread_mutex_t send_lock;
    pthread_cond_t send_queue_not_empty;
    std::queue<packet> send_queue;

    static uint64_t sequence;
    static volatile bool thread_exit_flag;

    void create_socket(struct addrinfo *, uint16_t);

    static void *send_worker(void *);
    static void *recv_worker(void *);

    void dispatch(packet&);

  public:
    Comm(struct addrinfo *, uint16_t);
    ~Comm();

    void send(packet&, size_t);

    void send_login(const std::string&, const std::string&);
    void send_action_request(uint16_t, uint64_t, uint8_t);
    void send_logout(void);
    void send_ack(uint8_t);
};

#endif /* __INC_R9CLIENT_COMM_H__ */
