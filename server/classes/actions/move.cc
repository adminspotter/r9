/* move.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 20 Oct 2020, 23:08:28 tquirk
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
 * This file contains the action routines to move a given game object.
 *
 * Things to do
 *
 */

#include <algorithm>

#include "../motion_pool.h"
#include "../game_obj.h"

extern MotionPool *motion;

#define AVERAGE_WALKING_SPEED   1.4  /* m/s */
#define AVERAGE_ROTATION_SPEED  1.0  /* radians/s */

int action_move(GameObject *source,
                int intensity,
                GameObject *target,
                glm::dvec3& direction)
{
    if (glm::length(direction) == 0)
    {
        source->set_movement(glm::dvec3(0.0, 0.0, 0.0));
        return 0;
    }

    glm::dvec3 move = glm::normalize(direction);

    intensity = std::min(intensity, 100);

    source->set_movement(source->get_movement()
                         + (move
                            * (double)intensity / 100.0
                            * AVERAGE_WALKING_SPEED));
    motion->push(source);
    return intensity;
}

int action_stop(GameObject *source,
                int intensity,
                GameObject *target,
                glm::dvec3& direction)
{
    source->set_movement(glm::dvec3(0.0, 0.0, 0.0));
    source->set_rotation(glm::dquat(1.0, 0.0, 0.0, 0.0));
    return 1;
}

int action_rotate(GameObject *source,
                  int intensity,
                  GameObject *target,
                  glm::dvec3& axis)
{
    if (glm::length(axis) == 0)
    {
        source->set_rotation(glm::dquat(1.0, 0.0, 0.0, 0.0));
        return 0;
    }

    glm::dvec3 rot = glm::normalize(axis);

    double inten = std::min(intensity, 100) / 100.0 * AVERAGE_ROTATION_SPEED;

    source->set_rotation(glm::mix(source->get_rotation(),
                                  glm::angleAxis(inten, rot),
                                  inten));
    motion->push(source);
    return (int)inten;
}
