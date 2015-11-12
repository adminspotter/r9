/* zone.h                                                  -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 15 Nov 2015, 11:48:51 tquirk
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
#include "thread_pool.h"
#include "action_pool.h"
#include "update_pool.h"
#include "library.h"
#include "octree.h"
#include "../../proto/proto.h"

class Zone
{
  private:
    uint16_t x_steps, y_steps, z_steps;
    uint64_t x_dim, y_dim, z_dim;

    std::vector< std::vector< std::vector<Octree *> > > sectors;

    Library *action_lib;

  public:
    std::map<uint16_t, action_rec> actions;
    std::map<uint64_t, GameObject *> game_objects;
    ActionPool *action_pool;                /* Takes action requests      */
    ThreadPool<GameObject *> *motion_pool;  /* Processes motion/collision */
    UpdatePool *update_pool;                /* Sends motion updates       */

  private:
    void init(void);
    void load_actions(const std::string&);
    void create_thread_pools(void);

    inline Octree *sector_contains(Eigen::Vector3d& pos)
        {
            Eigen::Vector3i sec = this->which_sector(pos);
            return this->sectors[sec[0]][sec[1]][sec[2]];
        };
    inline Eigen::Vector3i which_sector(Eigen::Vector3d& pos)
        {
            Eigen::Vector3i sector;

            sector[0] = pos[0] / this->x_dim;
            sector[1] = pos[1] / this->y_dim;
            sector[2] = pos[2] / this->z_dim;
            return sector;
        };

    static void *motion_pool_worker(void *);

  public:
    Zone(uint64_t, uint16_t);
    Zone(uint64_t, uint64_t, uint64_t, uint16_t, uint16_t, uint16_t);
    ~Zone();

    void start(void);
    void stop(void);

    /* Interface to the action pool */
    void add_action_request(uint64_t, packet *, size_t);

    void execute_action(Control *, action_request&, size_t);
};

#endif /* __INC_ZONE_H__ */
