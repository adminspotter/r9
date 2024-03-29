/* octree.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 02 Oct 2021, 09:20:56 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2021  Trinity Annabelle Quirk
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

#include <string.h>
#include <errno.h>

#include <string>
#include <system_error>

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

void Octree::enter_read(void)
{
    pthread_rwlock_rdlock(&this->lock);
}

void Octree::enter(void)
{
    pthread_rwlock_wrlock(&this->lock);
}

void Octree::leave(void)
{
    pthread_rwlock_unlock(&this->lock);
}

bool Octree::in_octant(const glm::dvec3& p)
{
    return p[0] >= this->min_point[0] && p[0] <= this->max_point[0]
        && p[1] >= this->min_point[1] && p[1] <= this->max_point[1]
        && p[2] >= this->min_point[2] && p[2] <= this->max_point[2];
}

int Octree::which_octant(const glm::dvec3& p)
{
    return ((p[0] < this->center_point[0] ? 0 : 4)
            | (p[1] < this->center_point[1] ? 0 : 2)
            | (p[2] < this->center_point[2] ? 0 : 1));
}

Octree *Octree::neighbor_test(int neigh, int oct)
{
    if (this->parent->neighbor[neigh] == NULL)
        return NULL;
    if (this->parent->neighbor[neigh]->octants[oct] == NULL)
        return this->parent->neighbor[neigh];
    return this->parent->neighbor[neigh]->octants[oct];
}

glm::dvec3 Octree::octant_min(int oct)
{
    glm::dvec3 mn;

    mn[0] = oct & 4 ? this->center_point[0] : this->min_point[0];
    mn[1] = oct & 2 ? this->center_point[1] : this->min_point[1];
    mn[2] = oct & 1 ? this->center_point[2] : this->min_point[2];
    return mn;
}

glm::dvec3 Octree::octant_max(int oct)
{
    glm::dvec3 mx;

    mx[0] = oct & 4 ? this->max_point[0] : this->center_point[0];
    mx[1] = oct & 2 ? this->max_point[1] : this->center_point[1];
    mx[2] = oct & 1 ? this->max_point[2] : this->center_point[2];
    return mx;
}

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
               glm::dvec3& min,
               glm::dvec3& max,
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

    pthread_rwlockattr_t lock_init;
    pthread_rwlockattr_init(&lock_init);
#ifdef PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP
    pthread_rwlockattr_setkind_np(&lock_init,
                                  PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
#endif /* PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP */
    if (pthread_rwlock_init(&this->lock, &lock_init))
    {
        std::string s("couldn't init octree lock");

        throw std::system_error(errno, std::generic_category(), s);
    }
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
    this->objects.clear();
    pthread_rwlock_destroy(&this->lock);
}

bool Octree::empty(void)
{
    return this->objects.empty();
}

void Octree::build(std::list<GameObject *>& objs)
{
    std::list<GameObject *> obj_list[8];
    std::list<GameObject *>::iterator i;
    int j;

    this->objects.insert(objs.begin(), objs.end());

    if (this->depth < Octree::MAX_DEPTH
        && (this->depth < Octree::MIN_DEPTH
            || objs.size() > Octree::MAX_LEAF_OBJECTS))
    {
        for (i = objs.begin(); i != objs.end(); ++i)
            obj_list[this->which_octant((*i)->get_position())].push_back(*i);

        /* Recurse if required for each octant. */
        for (j = 0; j < 8; ++j)
        {
            if (!obj_list[j].empty() || this->depth < Octree::MIN_DEPTH)
            {
                glm::dvec3 mn = this->octant_min(j);
                glm::dvec3 mx = this->octant_max(j);
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

void Octree::insert(GameObject *gobj)
{
    this->enter();
    this->objects.insert(gobj);
    if (this->depth < Octree::MAX_DEPTH
        && this->objects.size() > Octree::MAX_LEAF_OBJECTS)
    {
        /* Classify it and insert it into the appropriate subtree */
        int octant = this->which_octant(gobj->get_position());

        if (this->octants[octant] == NULL)
        {
            glm::dvec3 mn = this->octant_min(octant);
            glm::dvec3 mx = this->octant_max(octant);

            this->octants[octant] = new Octree(this, mn, mx, octant);
            this->octants[octant]->compute_neighbors();
        }
        this->octants[octant]->insert(gobj);
    }
    this->leave();
}

void Octree::remove(GameObject *gobj)
{
    this->enter();
    if (this->objects.find(gobj) != this->objects.end())
    {
        int octant = this->which_octant(gobj->get_position());

        if (this->octants[octant] != NULL)
            this->octants[octant]->remove(gobj);
        this->objects.erase(gobj);
    }
    this->leave();
    /* If our subtree doesn't have enough objects to warrant a
     * subtree, delete ourselves.
     */
    if (this->objects.size() < Octree::MAX_LEAF_OBJECTS
        && this->depth > Octree::MIN_DEPTH)
        delete this;
}
