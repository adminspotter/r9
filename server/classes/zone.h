/* zone.h                                                  -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 05 Jun 2017, 18:48:49 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2017  Trinity Annabelle Quirk
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
 * This file contains the overlying Zone object, which is basically
 * the "world" that resides in the server.
 *
 * The (x|y|z)_dim members are the sizes of *each* sector, in meters.
 * The (x|y|z)_steps members are the sizes of the sector grid along each
 * dimension.
 *
 * We want to keep the entire representation of the map in memory even
 * after we chop it up in the sectors' octrees.  It'll use a lot of
 * core, but it might be more effective for a constantly changing map.
 * Plus, if we have geometry updates (people blowing up dynamite and
 * such, heh), it'll be easier and faster to tell people what happened.
 *
 * Things to do
 *
 */

#ifndef __INC_ZONE_H__
#define __INC_ZONE_H__

#include <cstdint>
#include <vector>
#include <map>

#include "defs.h"
#include "control.h"
#include "action_pool.h"
#include "motion_pool.h"
#include "update_pool.h"
#include "octree.h"

class Zone
{
    friend class MotionPool;

  private:
    uint16_t x_steps, y_steps, z_steps;
    uint64_t x_dim, y_dim, z_dim;

    std::vector< std::vector< std::vector<Octree *> > > sectors;

  public:
    std::map<uint64_t, GameObject *> game_objects;

    typedef std::map<uint64_t, GameObject *>::iterator objects_iterator;

    ActionPool *action_pool;   /* Takes action requests      */
    MotionPool *motion_pool;   /* Processes motion/collision */
    UpdatePool *update_pool;   /* Sends motion updates       */

  protected:
    virtual void init(DB *);
    virtual void create_thread_pools(DB *);

    inline Octree *sector_contains(glm::dvec3& pos)
        {
            glm::ivec3 sec = this->which_sector(pos);
            Octree *oct = this->sectors[sec[0]][sec[1]][sec[2]];
            if (oct == NULL)
            {
                glm::ivec3 sec = this->which_sector(pos);
                glm::dvec3 mn, mx;

                mn.x = sec.x * this->x_dim;
                mn.y = sec.y * this->y_dim;
                mn.z = sec.z * this->z_dim;
                mx.x = mn.x + this->x_dim;
                mx.y = mn.y + this->y_dim;
                mx.z = mn.z + this->z_dim;
                oct = new Octree(NULL, mn, mx, 0);
                this->sectors[sec[0]][sec[1]][sec[2]] = oct;
            }
            return oct;
        };
    inline glm::ivec3 which_sector(glm::dvec3& pos)
        {
            glm::ivec3 sector;

            sector.x = pos.x / this->x_dim;
            sector.y = pos.y / this->y_dim;
            sector.z = pos.z / this->z_dim;
            return sector;
        };

  public:
    Zone(uint64_t, uint16_t, DB *);
    Zone(uint64_t, uint64_t, uint64_t, uint16_t, uint16_t, uint16_t, DB *);
    ~Zone();

    void start(void);
    void stop(void);

    virtual void connect_game_object(Control *, uint64_t);
};

#endif /* __INC_ZONE_H__ */
