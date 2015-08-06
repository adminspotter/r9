/* move.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 28 Jun 2014, 09:43:28 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2014  Trinity Annabelle Quirk
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

#include <Eigen/Core>

#include "../motion.h"

void action_move(Motion *source,
                 int intensity,
                 Motion *target,
                 Eigen::Vector3d &direction)
{
}

void action_rotate(Motion *source,
                   int intensity,
                   Motion *target,
                   Eigen::Vector3d &direction)
{
}
