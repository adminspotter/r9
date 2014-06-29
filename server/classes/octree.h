/* octree.h                                                -*- C++ -*-
 *   by Trinity Quirk <trinity@ymb.net>
 *   last updated 28 Jun 2014, 17:49:10 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2004  Trinity Annabelle Quirk
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
 * This file contains the declarations for the octree class.
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
 * Changes
 *   16 Jun 2000 TAQ - Created the file.
 *   21 Jun 2000 TAQ - Added the parent pointer.
 *   26 Jun 2000 TAQ - Created the point.h include file, so we don't need
 *                     to think about that any more in here.
 *   29 Jun 2000 TAQ - Added parent_index, which is the index of our node
 *                     in our parent.
 *   10 Jul 2000 TAQ - Removed some of the function prototypes.  This will
 *                     be an opaque interface, I think.
 *   27 Oct 2000 TAQ - Reworked the octrees to use the Math3d library.
 *                     Moved typedefs to defs.h.
 *   30 Mar 2004 TAQ - We're now passing around polygon pointers instead
 *                     of references.
 *   03 Apr 2004 TAQ - Added the octree_delete prototype.
 *   10 Apr 2004 TAQ - Changed some comments.
 *   04 Apr 2006 TAQ - Added namespace specifiers for std:: and Math3d::
 *                     objects.
 *   06 Jul 2006 TAQ - Added C++ tag at the top to get emacs to use the
 *                     right mode.
 *   28 Jul 2014 TAQ - Started making this into a class.
 *
 * Things to do
 *
 */

#ifndef __INC_OCTREE_H__
#define __INC_OCTREE_H__

#include <list>
#include <set>
#include <Eigen/Core>

#include "motion.h"

class Octree
{
  public:
    static const int MAX_LEAF_OBJECTS;
    static const int MIN_DEPTH;
    static const int MAX_DEPTH;

    Eigen::Vector3d min_point, center_point, max_point;
    Octree *parent, *octants[8], *neighbor[6];
    u_int8_t parent_index;

    std::set<Motion *> objects;

  private:
    inline int which_octant(const Eigen::Vector3d& p)
        {
            return ((p[0] < this->center_point[0] ? 0 : 4)
                    | (p[1] < this->center_point[1] ? 0 : 2)
                    | (p[2] < this->center_point[2] ? 0 : 1));
        }
    void compute_neighbors(void);

  public:
    Octree(Octree *, Eigen::Vector3d&, Eigen::Vector3d&, u_int8_t);
    ~Octree();

    bool empty(void);

    void build(std::list<Motion *>&, int);
    void insert(Motion *);
    void remove(Motion *);
};

#endif /* INC_OCTREE_H__ */
