/* update_pool.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 13 Nov 2015, 12:30:29 tquirk
 *
 * Revision IX game server
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
 * The minimal implementation of the Update pool.
 */

#include "../../proto/proto.h"

#include "update_pool.h"
#include "listensock.h"

extern std::vector<listen_socket *> sockets;

UpdatePool::UpdatePool(const char *pool_name, unsigned int pool_size)
    : ThreadPool<GameObject *>(pool_name, pool_size)
{
}

UpdatePool::~UpdatePool()
{
}

void UpdatePool::start(void)
{
    this->startup_arg = (void *)this;
    this->start(UpdatePool::update_pool_worker);
}

void UpdatePool::start(void *(*func)(void *))
{
    ThreadPool<GameObject *>::start(func);
}

void *UpdatePool::update_pool_worker(void *arg)
{
    UpdatePool *pool = (UpdatePool *)arg;
    GameObject *req;
    position_update pkt;
    std::vector<listen_socket *>::iterator sock;
    std::map<uint64_t, base_user *>::iterator user;

    for (;;)
    {
        pool->pop(&req);

        pkt.type = TYPE_POSUPD;
        pkt.object_id = req->get_object_id();
        pkt.x_pos = (uint64_t)(req->position.x() * 100);
        pkt.y_pos = (uint64_t)(req->position.y() * 100);
        pkt.z_pos = (uint64_t)(req->position.z() * 100);
        pkt.x_orient = (uint32_t)(req->orient.x() * 100);
        pkt.y_orient = (uint32_t)(req->orient.y() * 100);
        pkt.z_orient = (uint32_t)(req->orient.z() * 100);
        pkt.w_orient = (uint32_t)(req->orient.w() * 100);
        pkt.x_look = (uint32_t)(req->look.x() * 100);
        pkt.y_look = (uint32_t)(req->look.y() * 100);
        pkt.z_look = (uint32_t)(req->look.z() * 100);

        /* Figure out who to send it to */
        /* Send to EVERYONE (for now) */
        for (sock = sockets.begin(); sock != sockets.end(); ++sock)
            for (user = (*sock)->users.begin();
                 user != (*sock)->users.end();
                 ++user)
                user->second->control->send((packet *)&pkt);
    }
    return NULL;
}
