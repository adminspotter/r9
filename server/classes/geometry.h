/* geometry.h                                              -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 10 Jul 2016, 09:48:17 tquirk
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
 * This file contains the geometry representation of a game object (or any
 * other kind of object) for the game system.
 *
 * Things to do
 *   - Consider how we want to represent our sequences.
 *   - Consider how we want to represent our bounding volumes.  From the
 *   looks of things, we're going to attempt a two-level bounding operation:
 *   first level is a bounding sphere, since it's easy and small, and the
 *   second level will be a bounding box, or set of boxes, which is much
 *   tighter than the initial sphere.
 *
 */

#ifndef __INC_GEOMETRY_H__
#define __INC_GEOMETRY_H__

#include <glm/vec3.hpp>

class Geometry
{
  public:
    //std::vector< std::vector<sequence_element> > sequences;
    glm::dvec3 center;
    double radius;

  public:
    Geometry();
    Geometry(const Geometry &);
    ~Geometry();
};

#endif /* __INC_GEOMETRY_H__ */
