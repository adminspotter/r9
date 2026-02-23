/* motion_pool.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *
 * Revision IX game server
 * Copyright (C) 2015-2026  Trinity Annabelle Quirk
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
 * The implementation of the Motion pool.  Any objects that are in
 * motion get pushed onto our work queue, and we handle their motion.
 * If they're still in motion once we get done with them, we just push
 * them back on our own queue.  We also push moved objects onto the
 * update pool's work queue, so people can be notified of their
 * movements.
 *
 * Things to do
 *   - The physics library, once we create it, should be a part of this
 *     object, and not the zone as we had originally intended.
 *
 */

#include "motion_pool.h"
#include "../server.h"

MotionPool::MotionPool(const char *pool_name, unsigned int pool_size)
    : ThreadPool<GameObject *>(pool_name, pool_size)
{
}

MotionPool::~MotionPool()
{
}

void MotionPool::start(void)
{
    this->startup_arg = (void *)this;
    this->ThreadPool<GameObject *>::start(MotionPool::motion_pool_worker);
}

void MotionPool::motion_pool_worker(void *arg)
{
    MotionPool *mot = (MotionPool *)arg;
    GameObject *req;
    Octree *sector;

    for (;;)
    {
        if (!mot->pop(&req))
            break;

        if (req->still_moving())
        {
            sector = zone->sector_contains(req->get_position());
            if (sector == NULL)
                continue;
            sector->remove(req);
            req->move_and_rotate();
            sector = zone->sector_contains(req->get_position());
            if (sector != NULL)
                sector->insert(req);
            /* else figure out the neighbor that it needs to go to */
            /*mot->physics->collide(sector, req);*/
            update_pool->push(req);

            if (req->still_moving())
                mot->push(req);
        }
    }
}
