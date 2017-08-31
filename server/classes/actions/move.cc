/* move.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 31 Aug 2017, 18:05:55 tquirk
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
 * This file contains the action routine to move a given game object.
 *
 * Things to do
 *   - Implement these functions, obviously.
 *
 */

#include <glm/vec3.hpp>

#include <algorithm>

#include "../../server.h"
#include "../game_obj.h"

int action_move(GameObject *source,
                int intensity,
                GameObject *target,
                glm::dvec3& direction)
{
    glm::dvec3 move = glm::normalize(direction);

    intensity = std::min(intensity, 100);
    move *= intensity / 100.0;

    source->movement = move;
    motion_pool->push(source);
    return intensity;
}

int action_stop(GameObject *source,
                int intensity,
                GameObject *target,
                glm::dvec3& direction)
{
    glm::dvec3 stop(0.0, 0.0, 0.0);

    source->movement = stop;
    source->rotation = stop;
    return 1;
}

int action_rotate(GameObject *source,
                  int intensity,
                  GameObject *target,
                  glm::dvec3& direction)
{
    return -1;
}
