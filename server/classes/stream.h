/* stream.h                                                -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 18 Jul 2017, 07:32:37 tquirk
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
#include <vector>

#include "control.h"
#include "listensock.h"

class stream_user : public base_user
{
  public:
    int subsrv, fd;

    stream_user(uint64_t, Control *);

    const stream_user& operator=(const stream_user&);
};

class stream_socket : public listen_socket
{
  private:
    class subserver
    {
      public:
        int sock, pid, connections;

        const subserver& operator=(const subserver&);
    };

  private:
    int max_fd;
    std::vector<stream_socket::subserver> subservers;

    typedef std::vector<stream_socket::subserver>::iterator subserver_iterator;

    fd_set readfs, master_readfs;

  private:
    int create_subserver(void);
    int choose_subserver(void);
    int pass_fd(int, int);

  public:
    stream_socket(struct addrinfo *);
    ~stream_socket();

    std::string port_type(void) override;

    void start(void) override;

    void do_login(uint64_t, Control *, access_list&) override;
    void do_logout(base_user *) override;

    static void *stream_listen_worker(void *);
    static void *stream_send_worker(void *);
};

#endif /* __INC_STREAM_H__ */
