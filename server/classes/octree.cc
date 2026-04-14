/* octree.cc
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
 * If we run into problems allocating new r/w locks in subtrees, the
 * tree will be truncated at a depth less than our max.  This is
 * likely to cause degraded performance, since octants will have more
 * objects than we like, and collision calculations will have to take
 * excess object pairs into account.  We'll at least get logs, so we
 * can know when the truncation is happening.
 *
 * Things to do
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
#include <mutex>
#include <system_error>

#include "octree.h"
#include "log.h"

const int Octree::MAX_LEAF_OBJECTS = 3;
const int Octree::MIN_DEPTH = 5;
const int Octree::MAX_DEPTH = 10;

/* Orientation of octants:

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
*/

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

Octree::Octree(Octree *parent,
               glm::dvec3& min,
               glm::dvec3& max,
               uint8_t index)
    : min_point(min), center_point((max - min) * 0.5 + min), max_point(max),
      objects(), lock()
{
    this->parent = parent;
    memset(this->octants, 0, sizeof(Octree *) * 8);
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

    if (this->parent != NULL
        && this->parent->octants[this->parent_index] == this)
        this->parent->octants[this->parent_index] = NULL;

    /* Allowing the map destructor to clear itself out will delete all
     * the things in the map... not what we want.
     */
    this->objects.clear();
}

bool Octree::empty(void)
{
    return this->objects.empty();
}

void Octree::build(const std::list<GameObject *>& objs)
{
    Octree::object_set_t go_set;

    go_set.insert(objs.begin(), objs.end());
    this->build(go_set);
}

void Octree::build(const Octree::object_set_t& objs)
{
    Octree::object_set_t obj_list[8];
    int j;

    this->objects.insert(objs.begin(), objs.end());

    if (this->depth < Octree::MAX_DEPTH
        && (this->depth < Octree::MIN_DEPTH
            || objs.size() > Octree::MAX_LEAF_OBJECTS))
    {
        for (auto i = objs.begin(); i != objs.end(); ++i)
            obj_list[this->which_octant((*i)->get_position())].insert(*i);

        for (j = 0; j < 8; ++j)
        {
            if (!obj_list[j].empty() || this->depth < Octree::MIN_DEPTH)
            {
                glm::dvec3 mn = this->octant_min(j);
                glm::dvec3 mx = this->octant_max(j);
                try
                {
                    this->octants[j] = new Octree(this, mn, mx, j);
                }
                catch (std::system_error& e)
                {
                    std::clog << syslogErr
                              << "couldn't create octree subtree at depth "
                              << this->depth + 1 << ": " << e.code().message()
                              << " (" << e.code().value() << ")" << std::endl;
                    continue;
                }
                this->octants[j]->build(obj_list[j]);
            }
        }
    }
}

void Octree::insert(GameObject *gobj)
{
    std::unique_lock write_lock(this->lock);
    this->objects.insert(gobj);
    if (this->depth < Octree::MAX_DEPTH
        && this->objects.size() > Octree::MAX_LEAF_OBJECTS)
    {
        int octant = this->which_octant(gobj->get_position());

        if (this->octants[octant] == NULL)
        {
            glm::dvec3 mn = this->octant_min(octant);
            glm::dvec3 mx = this->octant_max(octant);

            try
            {
                this->octants[octant] = new Octree(this, mn, mx, octant);
            }
            catch (std::system_error& e)
            {
                std::clog << syslogErr
                          << "couldn't create octree subtree at depth "
                          << this->depth + 1 << ": " << e.code().message()
                          << " (" << e.code().value() << ")" << std::endl;
                return;
            }
            this->octants[octant]->build(this->objects);
        }
        this->octants[octant]->insert(gobj);
    }
}

void Octree::remove(GameObject *gobj)
{
    std::unique_lock write_lock(this->lock);
    if (this->objects.find(gobj) != this->objects.end())
    {
        int octant = this->which_octant(gobj->get_position());

        if (this->octants[octant] != NULL)
            this->octants[octant]->remove(gobj);
        this->objects.erase(gobj);

        if (this->objects.size() < Octree::MAX_LEAF_OBJECTS
            && this->depth > Octree::MIN_DEPTH)
        {
            if (this->parent != NULL)
                this->parent->octants[this->parent_index] = NULL;
            write_lock.unlock();
            delete this;
            return;
        }
    }
}

Octree::object_set_t Octree::get_objects(void)
{
    std::shared_lock read_lock(this->lock);
    return Octree::object_set_t(this->objects.begin(), this->objects.end());
}

Octree *Octree::find(GameObject *go)
{
    std::shared_lock read_lock(this->lock);
    if (this->objects.find(go) == this->objects.end())
        return NULL;

    int octant = this->which_octant(go->get_position());
    if (this->octants[octant] != NULL)
    {
        Octree *result = this->octants[octant]->find(go);
        if (result != NULL)
            return result;
    }
    return this;
}
