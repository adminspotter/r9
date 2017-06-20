/* zone.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 20 Jun 2017, 18:52:35 tquirk
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
 * This is the implementation of the zone object.
 *
 * Things to do
 *
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <glob.h>
#include <errno.h>

#include <sstream>
#include <stdexcept>

#include "zone.h"
#include "thread_pool.h"
#include "../config_data.h"
#include "../log.h"

/* Private methods */
void Zone::init(Library *action_lib, DB *database)
{
    int i;
    std::vector<Octree *> z_row;
    std::vector<std::vector<Octree *> > y_row;

    this->create_thread_pools(action_lib, database);
    database->get_server_objects(this->game_objects);
    std::clog << syslogNotice << "creating " << this->x_steps << 'x'
              << this->y_steps << 'x' << this->z_steps << " elements"
              << std::endl;
    z_row.reserve(this->z_steps);
    for (i = 0; i < this->z_steps; ++i)
        z_row.push_back(NULL);
    y_row.reserve(this->y_steps);
    for (i = 0; i < this->y_steps; ++i)
        y_row.push_back(z_row);
    this->sectors.reserve(x_steps);
    for (i = 0; i < this->x_steps; ++i)
        this->sectors.push_back(y_row);
}

void Zone::create_thread_pools(Library *action_lib, DB *database)
{
    this->action_pool = new ActionPool(config.action_threads,
                                       this->game_objects,
                                       action_lib,
                                       database);
    this->motion_pool = new MotionPool("motion", config.motion_threads);
    this->update_pool = new UpdatePool("update", config.update_threads);
}

/* Public methods */
Zone::Zone(uint64_t dim, uint16_t steps, Library *action_lib, DB *database)
    : sectors(), game_objects()
{
    this->x_dim = this->y_dim = this->z_dim = dim;
    this->x_steps = this->y_steps = this->z_steps = steps;
    this->init(action_lib, database);
}

Zone::Zone(uint64_t xd, uint64_t yd, uint64_t zd,
           uint16_t xs, uint16_t ys, uint16_t zs,
           Library *action_lib, DB *database)
    : sectors(), game_objects()
{
    this->x_dim = xd;
    this->y_dim = yd;
    this->z_dim = zd;
    this->x_steps = xs;
    this->y_steps = ys;
    this->z_steps = zs;
    this->init(action_lib, database);
}

Zone::~Zone()
{
    int i, j, k;

    /* Stop the thread pools first; deleting them should run through the
     * destructor, which will terminate all the threads and clean up
     * the queues and stuff.
     */
    std::clog << "deleting thread pools" << std::endl;
    if (this->action_pool != NULL)
    {
        delete this->action_pool;
        this->action_pool = NULL;
    }
    if (this->motion_pool != NULL)
    {
        delete this->motion_pool;
        this->motion_pool = NULL;
    }
    if (this->update_pool != NULL)
    {
        delete this->update_pool;
        this->update_pool = NULL;
    }

    /* Clear out the octrees */
    for (i = 0; i < this->x_steps; ++i)
        for (j = 0; j < this->y_steps; ++j)
            for (k = 0; k < this->z_steps; ++k)
                if (this->sectors[i][j][k] != NULL)
                    delete this->sectors[i][j][k];

    /* Delete all the game objects. */
    std::clog << "deleting game objects" << std::endl;
    if (this->game_objects.size())
    {
        Zone::objects_iterator i;

        for (i = this->game_objects.begin();
             i != this->game_objects.end();
             ++i)
            delete i->second;
        /* Maybe save the game objects' locations before deleting them? */
        this->game_objects.erase(this->game_objects.begin(),
                                 this->game_objects.end());
    }
}

void Zone::start(void)
{
    /* Do we want to load up all the game objects here, before we start
     * up the thread pools?
     */

    this->action_pool->startup_arg = (void *)this->action_pool;
    this->action_pool->start(ActionPool::action_pool_worker);

    this->motion_pool->startup_arg = (void *)this;
    this->motion_pool->start(MotionPool::motion_pool_worker);

    this->update_pool->start();
}

void Zone::stop(void)
{
    this->action_pool->stop();
    this->motion_pool->stop();
    this->update_pool->stop();
}

void Zone::connect_game_object(Control *con, uint64_t objid)
{
    GameObject *go;
    Zone::objects_iterator gi = this->game_objects.find(objid);

    /* Hook up to our character object */
    if (gi == this->game_objects.end())
    {
        /* Create the object? */
        go = new GameObject(NULL, con, objid);
        /* Set a position somewhere - the spawn point, perhaps? */
    }
    else
        go = gi->second;
    go->connect(con);
    this->update_pool->push(go);

    /* Send updates on all objects within visual range */
    for (gi = this->game_objects.begin(); gi != this->game_objects.end(); ++gi)
    {
        /* Figure out how to send only to specific users */
        if (go->distance_from(gi->second->position) < 1000.0)
            this->update_pool->push(gi->second);
    }
}
