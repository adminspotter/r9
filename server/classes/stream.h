/* stream.h                                                -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 12 Aug 2017, 11:48:31 tquirk
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
 * This file contains the stream socket object.
 *
 * Things to do
 *
 */

#ifndef __INC_STREAM_H__
#define __INC_STREAM_H__

#include <sys/types.h>
#include <sys/select.h>

#include <cstdint>
#include <map>

#include "control.h"
#include "listensock.h"

class stream_user : public base_user
{
  public:
    int fd;

    stream_user(uint64_t, GameObject *, listen_socket *);

    const stream_user& operator=(const stream_user&);
};

class stream_socket : public listen_socket
{
  public:
    std::map<int, stream_user *> fds;

  private:
    int max_fd;

    fd_set readfs, master_readfs;

  public:
    stream_socket(struct addrinfo *);
    ~stream_socket();

    std::string port_type(void) override;

    void start(void) override;

    void do_login(uint64_t, Control *, access_list&) override;
    void do_logout(base_user *) override;

    static void *stream_listen_worker(void *);
    int select_fd_set(void);
    void accept_new_connection(void);
    void handle_users(void);

    static void *stream_send_worker(void *);
};

#endif /* __INC_STREAM_H__ */
