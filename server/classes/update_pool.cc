/* update_pool.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 07 Dec 2019, 14:34:23 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2019  Trinity Annabelle Quirk
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
    packet_list pkt;
    std::vector<listen_socket *>::iterator sock;
    listen_socket::users_iterator user;

    for (;;)
    {
        pool->pop(&req);

        pkt.buf.pos.type = TYPE_POSUPD;
        pkt.buf.pos.version = 1;
        pkt.buf.pos.object_id = req->get_object_id();
        pkt.buf.pos.x_pos = (uint64_t)(req->position.x * POSUPD_POS_SCALE);
        pkt.buf.pos.y_pos = (uint64_t)(req->position.y * POSUPD_POS_SCALE);
        pkt.buf.pos.z_pos = (uint64_t)(req->position.z * POSUPD_POS_SCALE);
        pkt.buf.pos.w_orient = (uint32_t)(req->orient.w * POSUPD_ORIENT_SCALE);
        pkt.buf.pos.x_orient = (uint32_t)(req->orient.x * POSUPD_ORIENT_SCALE);
        pkt.buf.pos.y_orient = (uint32_t)(req->orient.y * POSUPD_ORIENT_SCALE);
        pkt.buf.pos.z_orient = (uint32_t)(req->orient.z * POSUPD_ORIENT_SCALE);
        pkt.buf.pos.x_look = (uint32_t)(req->look.x * POSUPD_LOOK_SCALE);
        pkt.buf.pos.y_look = (uint32_t)(req->look.y * POSUPD_LOOK_SCALE);
        pkt.buf.pos.z_look = (uint32_t)(req->look.z * POSUPD_LOOK_SCALE);

        /* Figure out who to send it to */
        /* Send to EVERYONE (for now) */
        for (auto sock : sockets)
            for (auto& user : sock->users)
            {
                pkt.who = user.second;
                pkt.buf.pos.sequence = user.second->sequence++;
                sock->send_pool->push(pkt);
            }
    }
    return NULL;
}
