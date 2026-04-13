/* octree.h                                                -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *
 * Revision IX game server
 * Copyright (C) 2000-2026  Trinity Annabelle Quirk
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
 * This file contains the declaration of the octree class.
 *
 * We are not going to subdivide anything - it's more work than we may
 * ever need to do.  For the time being, we'll just classify based on
 * the center of the object.
 *
 * We need to have a max tree height too, but I don't know what range
 * is realistic.  Perhaps 10 or so might be a good start?  We might
 * also want to have a minimum tree height as well, to get a
 * particular maximum sized voxel.  We may need to have config options
 * which determine these parameters, but for the time being, we'll
 * just have constants in the class.
 *
 * Some links that look like decent info:
 * http://hpcc.engin.umich.edu/CFD/users/charlton/Thesis/html/node29.html
 * http://www.csd.uwo.ca/faculty/irene/octree.html
 * http://www.cg.tuwien.ac.at/research/vr/lodestar/tech/octree/
 * http://www.altdev.co/2011/08/01/loose-octrees-for-frustum-culling-part-1/
 *
 * Things to do
 *
 */

#ifndef __INC_OCTREE_H__
#define __INC_OCTREE_H__

#include <cstdint>
#include <list>
#include <set>
#include <shared_mutex>

#include <glm/vec3.hpp>

#include "game_obj.h"

class Octree
{
  public:
    typedef std::set<GameObject *> object_set_t;

    static const int MAX_LEAF_OBJECTS;
    static const int MIN_DEPTH;
    static const int MAX_DEPTH;

  private:
    std::shared_mutex lock;

  public:
    glm::dvec3 min_point, center_point, max_point;
    Octree *parent, *octants[8];
    uint8_t parent_index;
    int depth;

    object_set_t objects;

  private:
    inline bool in_octant(const glm::dvec3&);
    inline int which_octant(const glm::dvec3&);
    inline glm::dvec3 octant_min(int oct);
    inline glm::dvec3 octant_max(int oct);

  public:
    Octree(Octree *, glm::dvec3&, glm::dvec3&, uint8_t);
    ~Octree();

    bool empty(void);

    void build(const std::list<GameObject *>&);
    void build(const object_set_t&);
    void insert(GameObject *);
    void remove(GameObject *);

    object_set_t get_objects(void);

    Octree *find(GameObject *);
};

#endif /* INC_OCTREE_H__ */
