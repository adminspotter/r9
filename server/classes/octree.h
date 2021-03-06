/* octree.h                                                -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 05 Nov 2020, 07:26:05 tquirk
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
 * This file contains the declaration of the octree class.
 *
 * We are not going to subdivide anything - it's more work than we may
 * ever need to do.  For the time being, we'll just classify based on
 * the center of the object.  We may eventually need to test our
 * neighbors' contents for a full collision test, but this'll be
 * lightweight for now.
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

#include <pthread.h>

#include <cstdint>
#include <list>
#include <set>

#include <glm/vec3.hpp>

#include "game_obj.h"

class Octree
{
  public:
    static const int MAX_LEAF_OBJECTS;
    static const int MIN_DEPTH;
    static const int MAX_DEPTH;

  private:
    pthread_rwlock_t lock;

  public:
    glm::dvec3 min_point, center_point, max_point;
    Octree *parent, *octants[8], *neighbor[6];
    uint8_t parent_index;
    int depth;

    std::set<GameObject *> objects;

  private:
    inline void enter_read(void);
    inline void enter(void);
    inline void leave(void);
    inline bool in_octant(const glm::dvec3&);
    inline int which_octant(const glm::dvec3&);
    inline Octree *neighbor_test(int neigh, int oct);
    inline glm::dvec3 octant_min(int oct);
    inline glm::dvec3 octant_max(int oct);
    void compute_neighbors(void);

  public:
    Octree(Octree *, glm::dvec3&, glm::dvec3&, uint8_t);
    ~Octree();

    bool empty(void);

    void build(std::list<GameObject *>&);
    void insert(GameObject *);
    void remove(GameObject *);
};

#endif /* INC_OCTREE_H__ */
