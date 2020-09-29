/* motion_pool.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 10 Feb 2020, 08:59:45 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2020  Trinity Annabelle Quirk
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
 *   - Consider whether passing a struct into the worker, composed of
 *     pointers to the motion pool, update pool, and zone, might be
 *     more appropriate.
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

/* We will only use this thread pool in a very specific way, so making
 * the caller handle stuff that it doesn't need to know about is
 * inappropriate.  We'll set the required arg and start things up with
 * the expected function.
 */
void MotionPool::start(void)
{
    this->startup_arg = (void *)this;
    this->ThreadPool<GameObject *>::start(MotionPool::motion_pool_worker);
}

/* We take ourselves as the arg, but we also use the zone and update
 * pool from the global scope.  It's probably not the ideal way to go,
 * but it'll get us going for now.
 */
void *MotionPool::motion_pool_worker(void *arg)
{
    MotionPool *mot = (MotionPool *)arg;
    GameObject *req;
    Octree *sector;

    for (;;)
    {
        mot->pop(&req);

        zone->sector_contains(req->get_position())->remove(req);
        req->move_and_rotate();
        sector = zone->sector_contains(req->get_position());
        sector->insert(req);
        /*mot->physics->collide(sector, req);*/
        update_pool->push(req);

        if (req->still_moving())
            mot->push(req);
    }
    return NULL;
}
