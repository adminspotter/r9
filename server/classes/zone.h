/* zone.h                                                  -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 29 Dec 2019, 14:02:51 tquirk
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

#include "octree.h"

#include "modules/db.h"

class Zone
{
  private:
    uint16_t x_steps, y_steps, z_steps;
    uint64_t x_dim, y_dim, z_dim;

    std::vector< std::vector< std::vector<Octree *> > > sectors;

  public:
    GameObject::objects_map game_objects;

  protected:
    virtual void init(DB *);

  public:
    Zone(uint64_t, uint16_t, DB *);
    Zone(uint64_t, uint64_t, uint64_t, uint16_t, uint16_t, uint16_t, DB *);
    ~Zone();

    Octree *sector_contains(glm::dvec3&);
    glm::ivec3 which_sector(glm::dvec3&);

    GameObject *find_game_object(uint64_t);
    virtual void send_nearby_objects(uint64_t);
};

#endif /* __INC_ZONE_H__ */
