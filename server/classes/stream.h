/* stream.h                                                -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 15 Jun 2014, 09:46:51 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2014  Trinity Annabelle Quirk
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
 * Changes
 *   09 Sep 2007 TAQ - Created the file from the ashes of tcpserver.c.
 *   12 Sep 2007 TAQ - Added the socket include and derived this class
 *                     publicly from socket.
 *   13 Sep 2007 TAQ - Made port member public for the send_pool_workers.
 *   02 Dec 2007 TAQ - Added pending_logout member to stream_user.
 *   16 Dec 2007 TAQ - Added timestamp field to stream_user.  Added reaper
 *                     member to stream_socket.
 *   22 Nov 2009 TAQ - Added the stream_reaper_worker to be a friend of the
 *                     stream_socket class.
 *   14 Jun 2014 TAQ - Totally reworked the basesock, and this needed a lot
 *                     of work as well.
 *   15 Jun 2014 TAQ - Moved the send worker here as well.
 *
 * Things to do
 *
 */

#ifndef __INC_STREAM_H__
#define __INC_STREAM_H__

#include <sys/types.h>
#include <sys/select.h>
#include <vector>

#include "control.h"
#include "basesock.h"

class stream_user : public base_user
{
  public:
    int subsrv, fd;

    stream_user(u_int64_t, Control *);

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
    fd_set readfs, master_readfs;
    struct cmsghdr cmptr;

  private:
    int create_subserver(void);
    int choose_subserver(void);
    int pass_fd(int, int);

  public:
    stream_socket(struct addrinfo *, u_int16_t);
    ~stream_socket();

    void start(void);

    base_user *login_user(u_int64_t, Control *, access_list&);

    static void *stream_listen_worker(void *);
    static void *stream_reaper_worker(void *);
    static void *stream_send_worker(void *);
};

#endif /* __INC_STREAM_H__ */
