/* octree.h                                                -*- C++ -*-
 *   by Trinity Quirk <trinity@ymb.net>
 *   last updated 20 May 2014, 17:36:20 tquirk
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
 * This file contains the declarations for an octree structure.
 *
 * For a start, we're going to try to subdivide the polys we get until
 * there is no more than a given number of polys inside each voxel.
 * Hopefully that won't make us too crazy.
 *
 * We need to have a max tree height too, but I don't know what range
 * is realistic.  Perhaps 10 or so might be a good start?  We might also
 * want to have a minimum tree height as well, to get a particular maximum
 * sized voxel.
 *
 * Some links that look like decent info:
 * http://hpcc.engin.umich.edu/CFD/users/charlton/Thesis/html/node29.html
 * http://www.csd.uwo.ca/faculty/irene/octree.html
 * http://www.cg.tuwien.ac.at/research/vr/lodestar/tech/octree/
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
 *
 * Things to do
 *   - Think about making this a class.  Though we want to be able to walk
 *   the tree without problems.
 *
 * $Id: octree.h 10 2013-01-25 22:13:00Z trinity $
 */

#ifndef __INC_OCTREE_H__
#define __INC_OCTREE_H__

#include <vector>
#include <deque>
#include <list>
#include "defs.h"

/* o is an octree center point and p is a point */
#define which_octant(o, p)  (((p)[0] < (o)[0] ? 0 : 4) \
	                     | ((p)[1] < (o)[1] ? 0 : 2) \
                             | ((p)[2] < (o)[2] ? 0 : 1))

void build_octree(octree,
		  const Eigen::Vector3d &,
		  std::list<polygon *> &,
		  int = 0);
polygon clip(polygon *, Eigen::Vector3d &, int, int, int);
void octree_delete(octree);

#endif /* INC_OCTREE_H__ */
