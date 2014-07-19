/* octree.cc
 *   by Trinity Quirk <trinity@ymb.net>
 *   last updated 28 Jun 2014, 22:44:05 tquirk
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
 * The implementation of octree functions.
 *
 * When doing the actual plane-comparisons, it is necessary to figure out
 * which side of the clipping plane the point is on, not only the octant
 * number where the point is.  So we figure out the octants and bitwise-
 * and them to filter out all the other sides.  Only then do we properly
 * clip the line segments.
 *
 * Changes
 *   27 Jun 2000 TAQ - Created the file.
 *   29 Jun 2000 TAQ - Finished up build_octree.  Now called octree.cc.
 *                     Completed classify_polygon.
 *   01 Jul 2000 TAQ - Reworked classify_polygon so that it's much faster
 *                     and lighter on memory; it bails out as soon as it
 *                     sees a point that isn't in the previously-seen octant.
 *   02 Jul 2000 TAQ - No longer check against depth at the beginning of
 *                     build_octree; found a more general solution.
 *   11 Jul 2000 TAQ - Finished up all the splitting.  Now do testing.
 *                     It compiles, but who knows if it works.
 *   12 Jul 2000 TAQ - The splitter didn't work properly.  Lots of changes
 *                     so that it seems to work correctly now.
 *   13 Jul 2000 TAQ - All the debugging is commented out, as is the
 *                     iostream.h include.  Everything looks like it's
 *                     working as expected.
 *   21 Aug 2000 TAQ - Needed some help with the double-scaled vector.
 *   27 Oct 2000 TAQ - Reworked this to use floating-point numbers and
 *                     the Math3d library.  Generalized the
 *                     clip_along_<foo>_plane routines to take a couple
 *                     more arguments and be a single clip routine.
 *   28 Oct 2000 TAQ - clip is no longer static, since we use it in
 *                     the zone object.
 *   30 Mar 2004 TAQ - I've been incredibly stupid, storing the *clipped*
 *                     polys in the tree, when I should instead be storing a
 *                     reference of some sort to the *whole* poly.  It'll
 *                     save on memory, and be non-stupid.  Testing, removal,
 *                     and a whole bunch of other operations then becomes
 *                     much simpler.  We're having to use a pointer, which
 *                     is proving to be tricky.  Freakin' STL.
 *   02 Apr 2004 TAQ - We are now going to precompute the neighbors of each
 *                     node, and store them in the node.  Check out the
 *                     bitchen ascii art below.  The indices on the ascii art
 *                     started out wrong, but they're fixed now.
 *   03 Apr 2004 TAQ - Finished up the neighbor-finding routine.  There is
 *                     surely a much more general way to do it, but I had
 *                     to get the numbers down first, to see if there are
 *                     any patterns I can see.  After a quick segfault problem,
 *                     it seems to be working just fine.  Also added the
 *                     octree_delete routine, an easy tail-recursion.
 *   10 Apr 2004 TAQ - Changed some of the commented regions to be ifdef'ed
 *                     to DEBUG, and some others to change based on
 *                     REVISION9SERVER.  We now set a min and max point in
 *                     each octant, so we can more easily tell just how much
 *                     area we cover (should be needed for an inorder tree
 *                     walk).
 *   21 Apr 2004 TAQ - Fixed some comments.  The polygon needs to have a
 *                     normal vector, so we had to make it into a struct.
 *                     The actual list of points is now called 'points' inside
 *                     the struct; how droll.
 *   24 Apr 2004 TAQ - Found a couple errors which might lose the contents
 *                     of an octant which has fewer than MAX_OCTREE_POLYS
 *                     in it, by simply not creating it.
 *   04 Apr 2006 TAQ - Fixed up namespaces for both std:: and Math3d:: objects.
 *   10 May 2006 TAQ - Altered MAX_OCTREE_POLYS for collider testing.
 *   28 Jun 2014 TAQ - Made this into a proper class.  Got rid of individual
 *                     polygons in the tree, in favor of geometry objects.
 *                     We should probably only deal with bounding boxes here,
 *                     and do collisions in the zone somewhere.
 *
 * Things to do
 *   - Make the neighbor-finder BFS, not DFS as it is now.  Half the
 *   neighbors are going to end up NULL the way it stands.
 *
 *   - Work on exploiting the pattern that appeared in the neighbor-
 *   finding routine.  If it turns out to be almost as simple as I
 *   think it might be, we may not even have to keep the pointers
 *   around, because doing it on-the-fly will be fast enough.  The way
 *   the pattern looks, it just calls out for a lookup table, which
 *   will be super-fast.  As it stands right now, it's just three
 *   compares and three pointer derefs, not a tall order cycle-wise.
 *   The optimizer should be able to simplify the repetition of terms.
 *
 */

#include "octree.h"

const int Octree::MAX_LEAF_OBJECTS = 3;
const int Octree::MIN_DEPTH = 5;
const int Octree::MAX_DEPTH = 10;

/* Orientation of octants and neighbors:

     +---------+--------+
     |\         \        \
     | \         \    7   \
     |  \         \        \
     |   +---------+--------+
     + 3 |\         \        \
     |\  | \         \        \
     | \ |  \         \        \
     |  \|   +---------+--------+               4    2
     |   + 2 |         |        |                ^   ^
     + 1 |\  |         |        |                 \  |
      \  | \ |         |    6   |                  \ |
       \ |  \|         |        |                   \|
        \|   +---------+--------+         1 <--------+-------> 0
         + 0 |         |        |                    |\
          \  |         |        |                    | \
           \ |         |    4   |                    |  \
            \|         |        |                    ~   ~
             +---------+--------+                    3    5

The pseudo for neighbor finding:

if root
  then return NULL
if direction indicates a sibling
  then return the sibling
define temp_neighbor = neighbor(parent)
if temp_neighbor is NULL
  then return NULL
if temp_neighbor is larger than us
  then return the matching sibling
else
  return temp_neighbor

*/

void Octree::compute_neighbors(void)
{
    int i;

    /* The root octant has no neighbors, and they have already been set
     * to NULL in the constructor, so that's a no-op.
     */
    if (this->parent != NULL)
    {
        /* There is likely a much more general way to do this, but I'm just
         * going to get it working right now.  A lookup table is possible,
         * based on the pattern that has emerged.
         */
        switch (this->parent_index)
        {
#define neighbor_test(x,y) (this->parent->neighbor[(x)] == NULL ? NULL \
    : (this->parent->neighbor[(x)]->octants[(y)] == NULL \
       ? this->parent->neighbor[(x)] \
       : this->parent->neighbor[(x)]->octants[(y)]))

          case 0:
            /* An interesting pattern has emerged:  the + and - direction
             * of any axis relates to the same-numbered octant, except for
             * the fact that we may be looking to an indirect sibling.
             * Interesting.  This is screaming lookup-table to me.
             */
            this->neighbor[0] = this->parent->octants[4];
            this->neighbor[1] = neighbor_test(1, 4);
            this->neighbor[2] = this->parent->octants[2];
            this->neighbor[3] = neighbor_test(3, 2);
            this->neighbor[4] = this->parent->octants[1];
            this->neighbor[5] = neighbor_test(5, 1);
            break;

          case 1:
            this->neighbor[0] = this->parent->octants[5];
            this->neighbor[1] = neighbor_test(1, 5);
            this->neighbor[2] = this->parent->octants[3];
            this->neighbor[3] = neighbor_test(3, 3);
            this->neighbor[4] = neighbor_test(4, 0);
            this->neighbor[5] = this->parent->octants[0];
            break;

          case 2:
            this->neighbor[0] = this->parent->octants[6];
            this->neighbor[1] = neighbor_test(1, 6);
            this->neighbor[2] = neighbor_test(2, 0);
            this->neighbor[3] = this->parent->octants[0];
            this->neighbor[4] = this->parent->octants[3];
            this->neighbor[5] = neighbor_test(5, 3);
            break;

          case 3:
            this->neighbor[0] = this->parent->octants[7];
            this->neighbor[1] = neighbor_test(1, 7);
            this->neighbor[2] = neighbor_test(2, 1);
            this->neighbor[3] = this->parent->octants[1];
            this->neighbor[4] = neighbor_test(4, 2);
            this->neighbor[5] = this->parent->octants[2];
            break;

          case 4:
            this->neighbor[0] = neighbor_test(0, 0);
            this->neighbor[1] = this->parent->octants[0];
            this->neighbor[2] = this->parent->octants[6];
            this->neighbor[3] = neighbor_test(3, 6);
            this->neighbor[4] = this->parent->octants[5];
            this->neighbor[5] = neighbor_test(5, 5);
            break;

          case 5:
            this->neighbor[0] = neighbor_test(0, 1);
            this->neighbor[1] = this->parent->octants[1];
            this->neighbor[2] = this->parent->octants[7];
            this->neighbor[3] = neighbor_test(3, 7);
            this->neighbor[4] = neighbor_test(4, 4);
            this->neighbor[5] = this->parent->octants[4];
            break;

          case 6:
            this->neighbor[0] = neighbor_test(0, 2);
            this->neighbor[1] = this->parent->octants[2];
            this->neighbor[2] = neighbor_test(2, 4);
            this->neighbor[3] = this->parent->octants[4];
            this->neighbor[4] = this->parent->octants[7];
            this->neighbor[5] = neighbor_test(5, 7);
            break;

          case 7:
            this->neighbor[0] = neighbor_test(0, 3);
            this->neighbor[1] = this->parent->octants[3];
            this->neighbor[2] = neighbor_test(2, 5);
            this->neighbor[3] = this->parent->octants[5];
            this->neighbor[4] = neighbor_test(4, 6);
            this->neighbor[5] = this->parent->octants[6];
            break;
        }
#undef neighbor_test
    }
    for (i = 0; i < 8; ++i)
        /* Recurse for each subspace */
        if (this->octants[i] != NULL)
            this->octants[i]->compute_neighbors();
}

Octree::Octree(Octree *parent,
               Eigen::Vector3d& min,
               Eigen::Vector3d& max,
               u_int8_t index)
    : min_point(min), center_point((max - min) * 0.5), max_point(max),
      objects()
{
    this->parent = parent;
    memset(this->octants, 0, sizeof(Octree *) * 8);
    memset(this->neighbor, 0, sizeof(Octree *) * 6);
    this->parent_index = index;
}

Octree::~Octree()
{
    int i;

    for (i = 0; i < 8; ++i)
        if (this->octants[i] != NULL)
            delete this->octants[i];

    /* It is possible that we may only remove part of a tree, so make
     * any neighbors point to our parent, and our parent point to
     * nothing, for consistency.  If we're the root of the tree, it
     * doesn't matter, of course.
     */
    if (this->parent != NULL)
    {
        /* Detach from our parent so the neighbor recalc will ignore us */
        if (this->parent->octants[this->parent_index] == this)
            this->parent->octants[this->parent_index] = NULL;
        /* Reset our neighbors' neighbor pointers */
        for (i = 0; i < 6; ++i)
            if (this->neighbor[i] != NULL)
                this->neighbor[i]->compute_neighbors();

        /* Move all our objects into our parent */
        this->parent->objects.insert(this->objects.begin(),
                                     this->objects.end());
    }
    /* Allowing the map destructor to clear itself out will delete all
     * the things in the map... not what we want.
     */
    this->objects.erase(this->objects.begin(), this->objects.end());
}

bool Octree::empty(void)
{
    return objects.empty();
}

void Octree::build(std::list<Motion *>& objs, int depth)
{
    std::list<Motion *> obj_list[8];
    std::list<Motion *>::iterator i;
    int j;

    if (depth == Octree::MAX_DEPTH
        || (depth >= Octree::MIN_DEPTH
            && objs.size() <= Octree::MAX_LEAF_OBJECTS))
    {
        /* Copy the list's elements into the node and stop recursing
         * along this branch of the tree.
         */
        this->objects.insert(objs.begin(), objs.end());
    }
    else
    {
        for (i = objs.begin(); i != objs.end(); ++i)
        {
            obj_list[this->which_octant((*i)->position)].push_back(*i);
        }

        /* Recurse if required for each octant. */
        for (j = 0; j < 8; ++j)
        {
            if (!obj_list[j].empty())
            {
                Eigen::Vector3d min, max;

                if (j & 4)
                {
                    min[0] = this->center_point[0];
                    max[0] = this->max_point[0];
                }
                else
                {
                    min[0] = this->min_point[0];
                    max[0] = this->center_point[0];
                }
                if (j & 2)
                {
                    min[1] = this->center_point[1];
                    max[1] = this->max_point[1];
                }
                else
                {
                    min[1] = this->min_point[1];
                    max[1] = this->center_point[1];
                }
                if (j & 1)
                {
                    min[2] = this->center_point[2];
                    max[2] = this->max_point[2];
                }
                else
                {
                    min[2] = this->min_point[2];
                    max[2] = this->center_point[2];
                }
                this->octants[j] = new Octree(this, min, max, j);
                this->octants[j]->build(obj_list[j], depth + 1);
            }
        }
    }
    /* Once we're back out of the creation recursion, calculate
     * everybody's neighbor pointers.  I don't think we can do this
     * during creation because of the way we do the higher-numbered
     * octants' check, and the fact that the target octants will not
     * have been created yet, though I'll look into it.
     */
    if (depth == 0)
        this->compute_neighbors();
}

void Octree::insert(Motion *mot)
{
}

void Octree::remove(Motion *mot)
{
}
