/* game_obj.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 01 Dec 2019, 14:26:33 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2019  Trinity Annabelle Quirk
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
 * This file contains the implementation of the GameObject class.
 *
 * Things to do
 *   - Implement the destructor if necessary.
 *   - Decide on a method to make sure we don't repeat object ID
 *   values (not that there's likely to be a very large chance of
 *   that happening, but you never know).
 *
 */

#include <algorithm>

#include "game_obj.h"
#include "zone.h"

pthread_mutex_t GameObject::max_mutex = PTHREAD_MUTEX_INITIALIZER;
uint64_t GameObject::max_id_value = 0LL;

uint64_t GameObject::reset_max_id(void)
{
    uint64_t val;

    pthread_mutex_lock(&GameObject::max_mutex);
    val = GameObject::max_id_value;
    GameObject::max_id_value = (uint64_t)0;
    pthread_mutex_unlock(&GameObject::max_mutex);
    return val;
}

GameObject::GameObject(Geometry *g, Control *c, uint64_t newid)
    : position(), movement(), look(0.0, 1.0, 0.0),
      orient(1.0, 0.0, 0.0, 0.0), rotation(1.0, 0.0, 0.0, 0.0)
{
    this->default_master = this->master = c;
    this->default_geometry = this->geometry = g;
    pthread_mutex_lock(&GameObject::max_mutex);
    if (newid == 0LL)
        newid = GameObject::max_id_value++;
    else
        /* This clause is mostly for recreating an object from some
         * saved state.
         */
        GameObject::max_id_value = std::max(GameObject::max_id_value,
                                            newid + 1);

    pthread_mutex_unlock(&GameObject::max_mutex);
    this->id_value = newid;
}

GameObject::~GameObject()
{
    if (this->geometry != this->default_geometry && this->geometry != NULL)
        delete this->geometry;
    if (this->default_geometry != NULL)
        delete this->default_geometry;
}

GameObject *GameObject::clone(void) const
{
    /* Each object completely owns its geometry, so we need to make a
     * new copy for the new object.
     */
    Geometry *new_geom = new Geometry(*this->default_geometry);
    return new GameObject(new_geom, this->default_master);
}

uint64_t GameObject::get_object_id(void) const
{
    return this->id_value;
}

bool GameObject::connect(Control *con)
{
    /* Only one thing can control a given thing */
    if (this->default_master == this->master)
    {
        /* Permissions will be checked in the control_object action */
        this->master = con;
        return true;
    }
    return false;
}

void GameObject::disconnect(Control *con)
{
    /* Should we also check whether default_master is the same?  What would
     * we want to do in that case?
     */
    if (this->master == con)
        master = default_master;
}

void GameObject::move_and_rotate(double interval)
{
    this->position += this->movement * interval;
    this->orient = glm::mix(this->orient, this->rotation, interval);
}
