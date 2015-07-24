/* octree.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 24 Jul 2015, 13:10:15 tquirk
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
 * The implementation of octree functions.
 *
 * Since this is a multi-level data structure, and each subtree can
 * function as a distinct tree on its own, we will keep track of all
 * the objects contained in each subtree, in every node in the
 * subtree.  That way, any node in the tree can be queried as to what
 * it contains, without having to start at the head, or traverse to
 * each leaf.  This may cause some consternation when moving an object
 * around in the tree, so we may need to revisit this at some point.
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
 *                     is proving to be tricky.
 *   02 Apr 2004 TAQ - We are now going to precompute the neighbors of each
 *                     node, and store them in the node.  Check out the
 *                     ascii art below.  The indices on the ascii art
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
 *                     the struct.
 *   24 Apr 2004 TAQ - Found a couple errors which might lose the contents
 *                     of an octant which has fewer than MAX_OCTREE_POLYS
 *                     in it, by simply not creating it.
 *   04 Apr 2006 TAQ - Fixed up namespaces for both std:: and Math3d:: objects.
 *   10 May 2006 TAQ - Altered MAX_OCTREE_POLYS for collider testing.
 *   28 Jun 2014 TAQ - Made this into a proper class.  Got rid of individual
 *                     polygons in the tree, in favor of geometry objects.
 *                     We should probably only deal with bounding boxes here,
 *                     and do collisions in the zone somewhere.
 *   15 Aug 2014 TAQ - Moved the neighbor_test from a define to an
 *                     inline in the header.  Added depth handling in
 *                     the constructor.  Implemented the remove
 *                     method.
 *   16 Aug 2014 TAQ - Cleaned the heck out of the neighbor-computing
 *                     routine.  Fleshed out the insert and move methods.
 *   17 Aug 2014 TAQ - Removed the move method for the time being.  It's a
 *                     lot more complicated than I expected it to be.
 *   24 Jul 2015 TAQ - Converted to stdint types.
 *
 * Things to do
 *   - Make the neighbor-finder breadth-first, not depth-first as it
 *     is now.  Half the neighbors are going to end up NULL the way it
 *     stands.
 *   - Consider if the neighbors are really necessary.  If not, we can
 *     save a bunch of computation by getting rid of all of it.
 *   - Consider how we would do a move operation.  As things stand
 *     right now, we need to do an remove/insert pair, which may
 *     involve a bunch of possibly unnecessary memory deallocation and
 *     allocation.  There are a lot of specific cases to consider, and
 *     my first attempt was not good.
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

Neighbor 0: if (index & 4 == 0) (parent->neighbor->octant[index - 4])
            else (index + 4)
Neighbor 1: if (index & 4) (parent->neighbor->octant[index + 4])
            else (index - 4)
Neighbor 2: if (index & 2 == 0) (parent->neighbor->octant[index - 2])
            else (index + 2)
Neighbor 3: if (index & 2) (parent->neighbor->octant[index + 2])
            else (index - 2)
Neighbor 4: if (index & 1 == 0) (parent->neighbor->octant[index - 1])
            else (index + 1)
Neighbor 5: if (index & 1) (parent->neighbor->octant[index + 1])
            else (index - 1)

*/

void Octree::compute_neighbors(void)
{
    int i;

    /* The root octant has no neighbors, and they have already been set
     * to NULL in the constructor, so that's a no-op.
     */
    if (this->parent != NULL)
    {
        if (this->parent_index & 4)
        {
            this->neighbor[0] = this->neighbor_test(0, this->parent_index - 4);
            this->neighbor[1] = this->parent->octants[this->parent_index - 4];
        }
        else
        {
            this->neighbor[0] = this->parent->octants[this->parent_index + 4];
            this->neighbor[1] = this->neighbor_test(1, this->parent_index + 4);
        }

        if (this->parent_index & 2)
        {
            this->neighbor[2] = this->neighbor_test(2, this->parent_index - 2);
            this->neighbor[3] = this->parent->octants[this->parent_index - 2];
        }
        else
        {
            this->neighbor[2] = this->parent->octants[this->parent_index + 2];
            this->neighbor[3] = this->neighbor_test(3, this->parent_index + 2);
        }

        if (this->parent_index & 1)
        {
            this->neighbor[4] = this->neighbor_test(4, this->parent_index - 1);
            this->neighbor[5] = this->parent->octants[this->parent_index - 1];
        }
        else
        {
            this->neighbor[4] = this->parent->octants[this->parent_index + 1];
            this->neighbor[5] = this->neighbor_test(5, this->parent_index + 1);
        }
    }
    for (i = 0; i < 8; ++i)
        /* Recurse for each subspace */
        if (this->octants[i] != NULL)
            this->octants[i]->compute_neighbors();
}

Octree::Octree(Octree *parent,
               Eigen::Vector3d& min,
               Eigen::Vector3d& max,
               uint8_t index)
    : min_point(min), center_point((max - min) * 0.5), max_point(max),
      objects()
{
    this->parent = parent;
    memset(this->octants, 0, sizeof(Octree *) * 8);
    memset(this->neighbor, 0, sizeof(Octree *) * 6);
    this->parent_index = index;
    if (this->parent != NULL)
        this->depth = this->parent->depth + 1;
    else
        this->depth = 0;
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
    }
    /* Allowing the map destructor to clear itself out will delete all
     * the things in the map... not what we want.
     */
    this->objects.erase(this->objects.begin(), this->objects.end());
}

bool Octree::empty(void)
{
    return this->objects.empty();
}

void Octree::build(std::list<Motion *>& objs)
{
    std::list<Motion *> obj_list[8];
    std::list<Motion *>::iterator i;
    int j;

    if (this->depth == Octree::MAX_DEPTH
        || (this->depth >= Octree::MIN_DEPTH
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
            obj_list[this->which_octant((*i)->position)].push_back(*i);

        /* Recurse if required for each octant. */
        for (j = 0; j < 8; ++j)
        {
            if (!obj_list[j].empty())
            {
                Eigen::Vector3d mn = this->octant_min(j);
                Eigen::Vector3d mx = this->octant_max(j);
                this->octants[j] = new Octree(this, mn, mx, j);
                this->octants[j]->build(obj_list[j]);
            }
        }
    }
    /* Once we're back out of the creation recursion, calculate
     * everybody's neighbor pointers.  I don't think we can do this
     * during creation because of the way we do the higher-numbered
     * octants' check, and the fact that the target octants will not
     * have been created yet, though I'll look into it.
     */
    if (this->depth == 0)
        this->compute_neighbors();
}

void Octree::insert(Motion *mot)
{
    this->objects.insert(mot);
    if (this->depth < Octree::MAX_DEPTH
        && this->objects.size() > Octree::MAX_LEAF_OBJECTS)
    {
        /* Classify it and insert it into the appropriate subtree */
        int octant = this->which_octant(mot->position);

        if (this->octants[octant] == NULL)
        {
            Eigen::Vector3d mn = this->octant_min(octant);
            Eigen::Vector3d mx = this->octant_max(octant);

            this->octants[octant] = new Octree(this, mn, mx, octant);
            this->octants[octant]->compute_neighbors();
        }
        this->octants[octant]->insert(mot);
    }
}

void Octree::remove(Motion *mot)
{
    if (this->objects.find(mot) != this->objects.end())
    {
        int octant = this->which_octant(mot->position);

        if (this->octants[octant] != NULL)
            this->octants[octant]->remove(mot);
        this->objects.erase(mot);
    }
    /* If we're an empty subtree, delete ourselves */
    if (this->objects.size() == 0 && this->depth > Octree::MIN_DEPTH)
        delete this;
}
