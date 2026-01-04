/* zone.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *
 * Revision IX game server
 * Copyright (C) 2000-2020  Trinity Annabelle Quirk
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

#include "zone.h"
#include "thread_pool.h"
#include "config_data.h"
#include "log.h"
#include "../server.h"

/* Private methods */
void Zone::init(DB *database)
{
    int i, j, k;
    std::vector<Octree *> z_row;
    std::vector<std::vector<Octree *> > y_row;

    database->get_server_objects(this->game_objects);
    std::clog << syslogNotice << "loaded " << this->game_objects.size()
              << " objects" << std::endl;
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

    /* If we have any memory problems in creating the octree roots, it
     * will be best to happen upon creation, so the blowups can cause
     * the zone to not construct.
     */
    for (i = 0; i < this->x_steps; ++i)
        for (j = 0; j < this->y_steps; ++j)
            for (k = 0; k < this->z_steps; ++k)
            {
                glm::dvec3 mn, mx;

                mn.x = i * this->x_dim;
                mn.y = j * this->y_dim;
                mn.z = k * this->z_dim;
                mx.x = mn.x + this->x_dim;
                mx.y = mn.y + this->y_dim;
                mx.z = mn.z + this->z_dim;
                this->sectors[i][j][k] = new Octree(NULL, mn, mx, 0);
            }

    for (auto& go : this->game_objects)
        this->sector_contains(go.second->get_position())->insert(go.second);
}

/* Public methods */
Zone::Zone(uint64_t dim, uint16_t steps, DB *database)
    : sectors(), game_objects()
{
    this->x_dim = this->y_dim = this->z_dim = dim;
    this->x_steps = this->y_steps = this->z_steps = steps;
    this->init(database);
}

Zone::Zone(uint64_t xd, uint64_t yd, uint64_t zd,
           uint16_t xs, uint16_t ys, uint16_t zs, DB *database)
    : sectors(), game_objects()
{
    this->x_dim = xd;
    this->y_dim = yd;
    this->z_dim = zd;
    this->x_steps = xs;
    this->y_steps = ys;
    this->z_steps = zs;
    this->init(database);
}

Zone::~Zone()
{
    int i, j, k;

    /* Clear out the octrees */
    for (i = 0; i < this->x_steps; ++i)
        for (j = 0; j < this->y_steps; ++j)
            for (k = 0; k < this->z_steps; ++k)
                if (this->sectors[i][j][k] != NULL)
                    delete this->sectors[i][j][k];

    /* Delete all the game objects. */
    if (this->game_objects.size())
    {
        std::clog << "deleting " << this->game_objects.size()
                  << " game objects" << std::endl;
        for (auto& i : this->game_objects)
            delete i.second;
        /* Maybe save the game objects' locations before deleting them? */
        this->game_objects.erase(this->game_objects.begin(),
                                 this->game_objects.end());
    }
}

Octree *Zone::sector_contains(const glm::dvec3& pos)
{
    glm::ivec3 sec = this->which_sector(pos);

    if (sec[0] < 0 || sec[0] >= x_steps
        || sec[1] < 0 || sec[1] >= y_steps
        || sec[2] < 0 || sec[2] >= z_steps)
        return NULL;

    return this->sectors[sec[0]][sec[1]][sec[2]];
}

glm::ivec3 Zone::which_sector(const glm::dvec3& pos)
{
    glm::ivec3 sector;

    sector.x = pos.x / this->x_dim;
    sector.y = pos.y / this->y_dim;
    sector.z = pos.z / this->z_dim;
    return sector;
}

GameObject *Zone::find_game_object(uint64_t objid)
{
    GameObject *go;
    GameObject::objects_iterator gi = this->game_objects.find(objid);

    /* Hook up to our character object */
    if (gi == this->game_objects.end())
    {
        /* Object doesn't exist, so we'll make it. */
        go = new GameObject(NULL, NULL, objid);
        this->game_objects[objid] = go;
        go->set_position(glm::dvec3(0.0, 0.0, 0.0));
        this->sector_contains(go->get_position())->insert(go);
    }
    else
        go = gi->second;

    return go;
}

void Zone::send_nearby_objects(uint64_t objid)
{
    GameObject *go = this->find_game_object(objid);

    update_pool->push(go);

    /* Send updates on all objects within visual range */
    for (auto& gi : this->game_objects)
    {
        /* We've already sent go, so no need to send it again. */
        /* Figure out how to send only to specific users */
        if (gi.second != go
            && go->distance_from(gi.second->get_position()) < 1000.0)
            update_pool->push(gi.second);
    }
}
