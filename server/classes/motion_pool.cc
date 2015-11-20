/* motion_pool.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 19 Nov 2015, 17:54:15 tquirk
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
 * The minimal implementation of the Motion pool.
 */

#include "motion_pool.h"
#include "zone.h"

MotionPool::MotionPool(const char *pool_name, unsigned int pool_size)
    : ThreadPool<GameObject *>(pool_name, pool_size)
{
}

MotionPool::~MotionPool()
{
}

void MotionPool::start(void *(*func)(void *))
{
    ThreadPool<GameObject *>::start(func);
}

/* Unfortunately since we depend on some other stuff in the zone, we
 * have to take it, rather than ourselves, as an argument.  For
 * testing purposes, we should be able to mock the zone out enough to
 * verify that we're operating correctly.
 */
void *MotionPool::motion_pool_worker(void *arg)
{
    Zone *zone = (Zone *)arg;
    GameObject *req;
    struct timeval current;
    double interval;
    Octree *sector;

    for (;;)
    {
        zone->motion_pool->pop(&req);

        gettimeofday(&current, NULL);
        interval = (current.tv_sec + (current.tv_usec / 1000000.0))
            - (req->last_updated.tv_sec
               + (req->last_updated.tv_usec / 1000000.0));
        memcpy(&req->last_updated, &current, sizeof(struct timeval));
        zone->sector_contains(req->position)->remove(req);
        req->position += req->movement * interval;
        /*req->orient += req->rotation * interval;*/
        sector = zone->sector_contains(req->position);
        sector->insert(req);
        /*zone->physics->collide(sector, req);*/
        zone->update_pool->push(req);

        if ((req->movement[0] != 0.0
             || req->movement[1] != 0.0
             || req->movement[2] != 0.0)
            || (req->rotation[0] != 0.0
                || req->rotation[1] != 0.0
                || req->rotation[2] != 0.0))
            zone->motion_pool->push(req);
    }
    return NULL;
}
