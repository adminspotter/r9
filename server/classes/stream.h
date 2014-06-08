/* stream.h                                                -*- C++ -*-
 *   by Trinity Quirk <trinity@ymb.net>
 *   last updated 22 Nov 2009, 13:20:17 trinity
 *
 * Revision IX game server
 * Copyright (C) 2007  Trinity Annabelle Quirk
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
 *
 * Things to do
 *
 * $Id: stream.h 10 2013-01-25 22:13:00Z trinity $
 */

#ifndef __INC_STREAM_H__
#define __INC_STREAM_H__

#include <sys/types.h>
#include <sys/select.h>
#include <pthread.h>
#include <vector>
#include <map>

#include "control.h"
#include "thread_pool.h"
#include "basesock.h"

class stream_user
{
  public:
    u_int64_t userid;
    Control *control;
    time_t timestamp;
    int subsrv, fd;
    bool pending_logout;

    bool operator<(const stream_user&) const;
    bool operator==(const stream_user&) const;

    const stream_user& operator=(const stream_user&);
};

class subserver
{
  public:
    int sock, pid, connections;

    const subserver& operator=(const subserver&);
};

class stream_socket : public basesock
{
  public:
    std::map<u_int64_t, stream_user> users;
    ThreadPool<packet_list> *send;
    int port;

  private:
    int sock, max_fd;
    pthread_t reaper;
    std::vector<subserver> subservers;
    fd_set readfs, master_readfs;
    struct cmsghdr cmptr;

  public:
    stream_socket(int);
    ~stream_socket(void);

    void listen(void);

  private:
    int create_subserver(void);
    int choose_subserver(void);
    int pass_fd(int, int);

    friend void *stream_reaper_worker(void *);
};

extern "C"
{
    void *start_stream_socket(void *);
}

#endif /* __INC_STREAM_H__ */
