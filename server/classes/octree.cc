/* octree.cc
 *   by Trinity Quirk <trinity@ymb.net>
 *   last updated 20 May 2014, 17:48:49 tquirk
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
 *   - We should be able to delete things from and add things to this
 *   octree on the fly.  If only a part of the octree is effected,
 *   then only that part should be recomputed.  Make sure to recompute
 *   neighbors as well.
 *
 *   - We need to calculate the normal of each poly we put into our
 *   tree, so we can easily do the line-plane intersection
 *   calculation.
 *
 * $Id: octree.cc 10 2013-01-25 22:13:00Z trinity $
 */

#include <string.h>
#include <math.h>
#include <list>
#include <algorithm>
#include <iostream>
#include "octree.h"
#include "polygon.h"

#ifdef REVISION9SERVER
#include "../config.h"
#define MIN_OCTREE_DEPTH  config.min_octree_depth
#define MAX_OCTREE_DEPTH  config.max_octree_depth
#define MAX_OCTREE_POLYS  config.max_octree_polys
#else
#define MIN_OCTREE_DEPTH  1
#define MAX_OCTREE_DEPTH  10
#define MAX_OCTREE_POLYS  3
#endif /* REVISION9SERVER */

static int classify_polygon(Eigen::Vector3d &, polygon *);
static void split_polygon(polygon *, octree, std::list<polygon *> []);
void compute_neighbors(octree);

void build_octree(octree tree,
		  const Eigen::Vector3d &max_pos,
		  std::list<polygon *> &polys,
		  int depth)
{
    std::list<polygon *> poly_list[8];
    std::list<polygon *>::iterator i;
    double factor = pow(0.5, depth + 1);
    int j;

    /* Set the center and min/max points. */
    if (tree->parent == NULL)
    {
	tree->min_point << 0, 0, 0;
	tree->center_point << max_pos[0] * factor,
            max_pos[1] * factor,
            max_pos[2] * factor;
	tree->max_point = max_pos;
    }
    else
    {
	tree->center_point << tree->parent->center_point[0]
            + (tree->parent_index & 4
               ? max_pos[0] * factor : -(max_pos[0] * factor)),
            tree->parent->center_point[1]
            + (tree->parent_index & 2
               ? max_pos[1] * factor : -(max_pos[1] * factor)),
            tree->parent->center_point[2]
            + (tree->parent_index & 1
               ? max_pos[2] * factor : -(max_pos[2] * factor));
	tree->min_point << tree->center_point[0] - (max_pos[0] * factor),
            tree->center_point[1] - (max_pos[1] * factor),
            tree->center_point[2] - (max_pos[2] * factor);
	tree->max_point << tree->center_point[0] + (max_pos[0] * factor),
            tree->center_point[1] + (max_pos[1] * factor),
            tree->center_point[2] + (max_pos[2] * factor);
    }
#ifdef DEBUG
    for (j = 0; j < depth; ++j)
	std::cout << " ";
    std::cout << "(" << depth << " " << (int)tree->parent_index
	      << ") <" << tree->center_point[0]
	      << ", " << tree->center_point[1]
	      << ", " << tree->center_point[2] << "> "
	      << polys.size() << " polys ";
#endif /* DEBUG */

    if (depth == MAX_OCTREE_DEPTH
	|| (depth >= MIN_OCTREE_DEPTH && polys.size() <= MAX_OCTREE_POLYS))
    {
	/* Copy the list's elements into the node and stop recursing
	 * along this branch of the tree.
	 */
#ifdef DEBUG
	std::cout << "not recursing, " << polys.size() << " polys\n";
#endif /* DEBUG */
	for (i = polys.begin(); i != polys.end(); ++i)
	    tree->contents.push_back(*i);
    }
    else
    {
	for (i = polys.begin(); i != polys.end(); ++i)
	{
	    if ((j = classify_polygon(tree->center_point, *i)) == -1)
		split_polygon(*i, tree, poly_list);
	    else
	    {
#ifdef DEBUG
		std::cout << "pushing poly into list " << j << std::endl;
#endif /* DEBUG */
		poly_list[j].push_back(*i);
	    }
	}

	/* Recurse if required for each octant. */
	for (j = 0; j < 8; ++j)
	{
	    if (depth < MIN_OCTREE_DEPTH
		|| (poly_list[j].size() > 0 && depth < MAX_OCTREE_DEPTH))
	    {
#ifdef DEBUG
		std::cout << "recursing->(" << depth + 1 << " " << j
			  << ") " << poly_list[j].size() << " polys"
			  << std::endl;
#endif /* DEBUG */
		tree->octants[j] = new octant;
		tree->octants[j]->parent = tree;
		tree->octants[j]->parent_index = j;
		memset(tree->octants[j]->octants, 0, sizeof(octree) * 8);
		build_octree(tree->octants[j],
			     max_pos,
			     poly_list[j],
			     depth + 1);
#ifdef DEBUG
		std::cout << std::endl;
		for (int m = 0; m < depth; ++m)
		    std::cout << " ";
#endif /* DEBUG */
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
	compute_neighbors(tree);
}

void octree_delete(octree tree)
{
    int i;

    for (i = 0; i < 8; ++i)
	if (tree->octants[i] != NULL)
	    octree_delete(tree->octants[i]);
    delete tree;
}

/* This routine figures out which octant the poly is in, or if it spans
 * multiple octants.  Returns either the octant number (0-7) or -1 if
 * it spans and needs splitting.
 */
int classify_polygon(Eigen::Vector3d &center, polygon *poly)
{
    std::vector<Eigen::Vector3d>::iterator i = poly->points.begin();
    int octant, retval = -1;

    retval = which_octant(center, *i);
#ifdef DEBUG
    std::cout << poly->points.size() << " points, octants " << retval;
#endif /* DEBUG */
    for (++i; i != poly->points.end(); ++i)
    {
	if ((octant = which_octant(center, *i)) != retval)
	    retval = -1;
#ifdef DEBUG
	std::cout << " " << octant;
#endif /* DEBUG */
    }
#ifdef DEBUG
    std::cout << std::endl;
#endif /* DEBUG */
    return retval;
}

/* Any polygon we get in this function will have to be split; we've
 * filtered out all other polygons before we call split_polygon.
 */
void split_polygon(polygon *poly,
		   octree tree,
		   std::list<polygon *> poly_list[8])
{
    polygon new_poly;
    int i;

    /* Do the clipping for *each* octant. */
    for (i = 0; i < 8; ++i)
    {
	/* Bail out whenever we end up with nothing. */
	new_poly = clip(poly, tree->center_point, i, 4, 0);
#ifdef DEBUG
	std::cout << "points " << new_poly.points.size();
#endif /* DEBUG */
	if (new_poly.points.size())
	{
	    new_poly = clip(&new_poly, tree->center_point, i, 2, 1);
#ifdef DEBUG
	    std::cout << " " << new_poly.points.size();
#endif /* DEBUG */
	    if (new_poly.points.size())
	    {
		new_poly = clip(&new_poly, tree->center_point, i, 1, 2);
#ifdef DEBUG
		std::cout << " " << new_poly.points.size();
#endif /* DEBUG */
		if (new_poly.points.size())
		    poly_list[i].push_back(poly);
	    }
	}
#ifdef DEBUG
	std::cout << std::endl;
#endif /* DEBUG */
    }
}

polygon clip(polygon *poly, Eigen::Vector3d &clip, int octant, int mask, int e)
{
    std::vector<Eigen::Vector3d>::iterator pt_a, pt_b;
    polygon new_poly;
    Eigen::Vector3d pt_i;
    int octant_a, octant_b;
    double scale;

    octant &= mask;
    for (pt_a = poly->points.end() - 1, pt_b = poly->points.begin();
	 pt_b != poly->points.end();
	 ++pt_a, ++pt_b)
    {
	/* Handle starting point wraparound properly. */
	if (pt_a == poly->points.end())
	    pt_a = poly->points.begin();
	octant_a = which_octant(clip, *pt_a) & mask;
	octant_b = which_octant(clip, *pt_b) & mask;
	if (octant_b == octant)
	{
	    if (octant_a != octant)
	    {
		/* The starting point is in another octant; clip. */
		scale = (double)(clip[e] - (*pt_a)[e])
		    / (double)((*pt_b)[e] - (*pt_a)[e]);
		pt_i = *pt_a + ((*pt_b - *pt_a) * scale);
		new_poly.points.push_back(pt_i);
	    }
	    new_poly.points.push_back(*pt_b);
	}
	else if (octant_a == octant)
	{
	    /* The ending point is in another octant; clip. */
	    scale = (double)(clip[e] - (*pt_a)[e])
		/ (double)((*pt_b)[e] - (*pt_a)[e]);
	    pt_i = *pt_a + ((*pt_b - *pt_a) * scale);
	    new_poly.points.push_back(pt_i);
	}
    }
    return new_poly;
}

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

void compute_neighbors(octree tree)
{
    int i;

    if (tree->parent == NULL)
	/* We are the root, and have no neighbors.  This may not be completely
	 * true, since we'll be in a 3D grid of octrees.
	 */
	for (i = 0; i < 6; ++i)
	    tree->neighbor[i] = NULL;
    else
    {
	/* There is likely a much more general way to do this, but I'm just
	 * going to get it working right now.  A lookup table is possible,
	 * based on the pattern that has emerged.
	 */
	switch (tree->parent_index)
	{
#define neighbor_test(x,y) (tree->parent->neighbor[(x)] == NULL ? NULL \
    : (tree->parent->neighbor[(x)]->octants[(y)] == NULL \
       ? tree->parent->neighbor[(x)] \
       : tree->parent->neighbor[(x)]->octants[(y)]))

	  case 0:
	    /* An interesting pattern has emerged:  the + and - direction
	     * of any axis relates to the same-numbered octant, except for
	     * the fact that we may be looking to an indirect sibling.
	     * Interesting.  This is screaming lookup-table to me.
	     */
	    tree->neighbor[0] = tree->parent->octants[4];
	    tree->neighbor[1] = neighbor_test(1, 4);
	    tree->neighbor[2] = tree->parent->octants[2];
	    tree->neighbor[3] = neighbor_test(3, 2);
	    tree->neighbor[4] = tree->parent->octants[1];
	    tree->neighbor[5] = neighbor_test(5, 1);
	    break;

	  case 1:
	    tree->neighbor[0] = tree->parent->octants[5];
	    tree->neighbor[1] = neighbor_test(1, 5);
	    tree->neighbor[2] = tree->parent->octants[3];
	    tree->neighbor[3] = neighbor_test(3, 3);
	    tree->neighbor[4] = neighbor_test(4, 0);
	    tree->neighbor[5] = tree->parent->octants[0];
	    break;

	  case 2:
	    tree->neighbor[0] = tree->parent->octants[6];
	    tree->neighbor[1] = neighbor_test(1, 6);
	    tree->neighbor[2] = neighbor_test(2, 0);
	    tree->neighbor[3] = tree->parent->octants[0];
	    tree->neighbor[4] = tree->parent->octants[3];
	    tree->neighbor[5] = neighbor_test(5, 3);
	    break;

	  case 3:
	    tree->neighbor[0] = tree->parent->octants[7];
	    tree->neighbor[1] = neighbor_test(1, 7);
	    tree->neighbor[2] = neighbor_test(2, 1);
	    tree->neighbor[3] = tree->parent->octants[1];
	    tree->neighbor[4] = neighbor_test(4, 2);
	    tree->neighbor[5] = tree->parent->octants[2];
	    break;

	  case 4:
	    tree->neighbor[0] = neighbor_test(0, 0);
	    tree->neighbor[1] = tree->parent->octants[0];
	    tree->neighbor[2] = tree->parent->octants[6];
	    tree->neighbor[3] = neighbor_test(3, 6);
	    tree->neighbor[4] = tree->parent->octants[5];
	    tree->neighbor[5] = neighbor_test(5, 5);
	    break;

	  case 5:
	    tree->neighbor[0] = neighbor_test(0, 1);
	    tree->neighbor[1] = tree->parent->octants[1];
	    tree->neighbor[2] = tree->parent->octants[7];
	    tree->neighbor[3] = neighbor_test(3, 7);
	    tree->neighbor[4] = neighbor_test(4, 4);
	    tree->neighbor[5] = tree->parent->octants[4];
	    break;

	  case 6:
	    tree->neighbor[0] = neighbor_test(0, 2);
	    tree->neighbor[1] = tree->parent->octants[2];
	    tree->neighbor[2] = neighbor_test(2, 4);
	    tree->neighbor[3] = tree->parent->octants[4];
	    tree->neighbor[4] = tree->parent->octants[7];
	    tree->neighbor[5] = neighbor_test(5, 7);
	    break;

	  case 7:
	    tree->neighbor[0] = neighbor_test(0, 3);
	    tree->neighbor[1] = tree->parent->octants[3];
	    tree->neighbor[2] = neighbor_test(2, 5);
	    tree->neighbor[3] = tree->parent->octants[5];
	    tree->neighbor[4] = neighbor_test(4, 6);
	    tree->neighbor[5] = tree->parent->octants[6];
	    break;
	}
#undef neighbor_test
    }
    for (i = 0; i < 8; ++i)
	/* Recurse for each subspace */
	if (tree->octants[i] != NULL)
	    compute_neighbors(tree->octants[i]);
}
