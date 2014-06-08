/* dgram.h                                                 -*- C++ -*-
 *   by Trinity Quirk <trinity@ymb.net>
 *   last updated 02 Dec 2007, 10:12:55 trinity
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
 * This file contains the datagram socket object.
 *
 * Changes
 *   08 Sep 2007 TAQ - Created the file from the ashes of udpserver.c.
 *   09 Sep 2007 TAQ - Renamed dgram_user_list to dgram_user.  Added
 *                     operator methods to the dgram_user.
 *   13 Sep 2007 TAQ - Made port and sock public for the send_pool_workers.
 *   16 Sep 2007 TAQ - Added timestamp member to user class.
 *   22 Oct 2007 TAQ - Added a private reaper thread member.
 *   02 Dec 2007 TAQ - Added pending_logout member to user class.
 *
 * Things to do
 *
 * $Id: dgram.h 10 2013-01-25 22:13:00Z trinity $
 */

#ifndef __INC_DGRAM_H__
#define __INC_DGRAM_H__

#include <sys/types.h>
#include <sys/select.h>
#include <pthread.h>
#include <map>

#include "control.h"
#include "thread_pool.h"
#include "basesock.h"
#include "sockaddr.h"

class dgram_user
{
  public:
    u_int64_t userid;
    Control *control;
    time_t timestamp;
    Sockaddr sin;
    bool pending_logout;

    bool operator<(const dgram_user&) const;
    bool operator==(const dgram_user&) const;

    const dgram_user& operator=(const dgram_user&);
};

class dgram_socket : public basesock
{
  public:
    std::map<u_int64_t, dgram_user> users;
    std::map<Sockaddr, dgram_user *> socks;
    ThreadPool<packet_list> *send;
    int sock, port;

  private:
    pthread_t reaper;
    fd_set readfs, master_readfs;

  public:
    dgram_socket(int);
    ~dgram_socket();

    void listen(void);
};

extern "C"
{
    void *start_dgram_socket(void *);
}

#endif /* __INC_DGRAM_H__ */
