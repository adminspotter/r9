/* game_obj.cc
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
 * This file contains the implementation of the GameObject class.
 *
 * Things to do
 *   - Decide on a method to make sure we don't repeat object ID
 *   values (not that there's likely to be a very large chance of
 *   that happening, but you never know).
 *
 */

#include <algorithm>
#include <sstream>
#include <system_error>

#include "game_obj.h"
#include "zone.h"

std::mutex GameObject::max_mutex;
uint64_t GameObject::max_id_value = 0LL;

glm::dvec3 GameObject::no_movement(0.0, 0.0, 0.0);
glm::dquat GameObject::no_rotation(1.0, 0.0, 0.0, 0.0);

uint64_t GameObject::reset_max_id(void)
{
    uint64_t val;

    std::scoped_lock lock(GameObject::max_mutex);
    val = GameObject::max_id_value;
    GameObject::max_id_value = (uint64_t)0;
    return val;
}

GameObject::GameObject(Geometry *g, Control *c, uint64_t newid)
    : position(), movement(), look(0.0, 1.0, 0.0),
      orient(1.0, 0.0, 0.0, 0.0), rotation(1.0, 0.0, 0.0, 0.0), movement_lock()
{
    if (g == NULL)
        g = new Geometry();
    this->default_master = this->master = c;
    this->default_geometry = this->geometry = g;
    this->active = true;

    {
        std::scoped_lock lock(GameObject::max_mutex);
        if (newid == 0LL)
            newid = GameObject::max_id_value++;
        else
            /* This clause is mostly for recreating an object from some
             * saved state.
             */
            GameObject::max_id_value = std::max(GameObject::max_id_value,
                                                newid + 1);
    }

    this->id_value = newid;
    gettimeofday(&this->last_updated, NULL);
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

void GameObject::activate(void)
{
    std::unique_lock lock(this->movement_lock);
    this->natures.erase(GameObject::nature::invisible);
    this->natures.erase(GameObject::nature::non_interactive);
    this->active = true;
}

void GameObject::deactivate(void)
{
    /* When a user logs out, and this is their primary slave object,
     * we'll make the object "go away" entirely.  We'll remove its
     * movement and rotation, and make it stop interacting with the
     * rest of the universe.
     */
    std::unique_lock lock(this->movement_lock);
    this->movement = GameObject::no_movement;
    this->rotation = GameObject::no_rotation;
    this->natures.insert(GameObject::nature::invisible);
    this->natures.insert(GameObject::nature::non_interactive);
    this->active = false;
}

double GameObject::distance_from(const glm::dvec3& pt)
{
    std::shared_lock lock(this->movement_lock);
    return glm::distance(pt, this->position);
}

glm::dvec3 GameObject::get_position(void)
{
    std::shared_lock lock(this->movement_lock);
    glm::dvec3 res = this->position;
    return res;
}

glm::dvec3 GameObject::set_position(const glm::dvec3& p)
{
    std::unique_lock lock(this->movement_lock);
    this->position = p;
    gettimeofday(&this->last_updated, NULL);
    return p;
}

glm::dvec3 GameObject::get_look(void)
{
    std::shared_lock lock(this->movement_lock);
    glm::dvec3 res = this->look;
    return res;
}

glm::dvec3 GameObject::set_look(const glm::dvec3& l)
{
    std::unique_lock lock(this->movement_lock);
    this->look = l;
    gettimeofday(&this->last_updated, NULL);
    return l;
}

glm::dvec3 GameObject::get_movement(void)
{
    std::shared_lock lock(this->movement_lock);
    glm::dvec3 res = this->movement;
    return res;
}

glm::dvec3 GameObject::set_movement(const glm::dvec3& m)
{
    std::unique_lock lock(this->movement_lock);
    this->movement = m;
    gettimeofday(&this->last_updated, NULL);
    return m;
}

glm::dquat GameObject::get_orientation(void)
{
    std::shared_lock lock(this->movement_lock);
    glm::dquat res = this->orient;
    return res;
}

glm::dquat GameObject::set_orientation(const glm::dquat& o)
{
    std::unique_lock lock(this->movement_lock);
    this->orient = o;
    gettimeofday(&this->last_updated, NULL);
    return o;
}

glm::dquat GameObject::get_rotation(void)
{
    std::shared_lock lock(this->movement_lock);
    glm::dquat res = this->rotation;
    return res;
}

glm::dquat GameObject::set_rotation(const glm::dquat& r)
{
    std::unique_lock lock(this->movement_lock);
    this->rotation = r;
    gettimeofday(&this->last_updated, NULL);
    return r;
}

void GameObject::move_and_rotate(void)
{
    if (this->still_moving())
    {
        struct timeval current;
        double interval;
        std::unique_lock lock(this->movement_lock);

        gettimeofday(&current, NULL);
        interval = (current.tv_sec + (current.tv_usec / 1000000.0))
            - (this->last_updated.tv_sec
               + (this->last_updated.tv_usec / 1000000.0));
        if (this->rotation != GameObject::no_rotation)
            this->orient = glm::dquat(glm::eulerAngles(this->rotation)
                                      * interval)
                * this->orient;
        if (this->movement != GameObject::no_movement)
            this->position += this->orient * this->movement * interval;
        memcpy(&this->last_updated, &current, sizeof(struct timeval));
    }
}

bool GameObject::still_moving(void)
{
    std::shared_lock lock(this->movement_lock);
    return (this->active == true
            && (this->movement != GameObject::no_movement
                || this->rotation != GameObject::no_rotation));
}

bool GameObject::collide(GameObject *target)
{
    if (target == this)
        return false;

    const glm::dvec3& target_pos = target->get_position();

    if (this->active == true
        && this->distance_from(target_pos)
        <= this->geometry->radius + target->geometry->radius)
    {
        std::unique_lock lock(this->movement_lock);

        this->movement = -this->movement;
        return true;
    }
    return false;
}

void GameObject::generate_update_packet(packet& pkt)
{
    std::shared_lock lock(this->movement_lock);
    if (this->active == false)
    {
        pkt.del.type = TYPE_OBJDEL;
        pkt.del.version = R9_PROTO_VER;
        pkt.del.object_id = this->id_value;
    }
    else
    {
        glm::dvec3 pos = this->position * POSUPD_POS_SCALE;
        glm::dquat orient = this->orient * POSUPD_ORIENT_SCALE;
        glm::dvec3 look = this->look * POSUPD_LOOK_SCALE;

        pkt.pos.type = TYPE_POSUPD;
        pkt.pos.version = R9_PROTO_VER;
        pkt.pos.object_id = this->id_value;
        pkt.pos.x_pos = (uint64_t)pos.x;
        pkt.pos.y_pos = (uint64_t)pos.y;
        pkt.pos.z_pos = (uint64_t)pos.z;
        pkt.pos.w_orient = (uint32_t)orient.w;
        pkt.pos.x_orient = (uint32_t)orient.x;
        pkt.pos.y_orient = (uint32_t)orient.y;
        pkt.pos.z_orient = (uint32_t)orient.z;
        pkt.pos.x_look = (uint32_t)look.x;
        pkt.pos.y_look = (uint32_t)look.y;
        pkt.pos.z_look = (uint32_t)look.z;
    }
}
