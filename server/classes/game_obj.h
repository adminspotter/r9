/* game_obj.h                                              -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 10 Feb 2020, 23:10:34 tquirk
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
 * This file contains the declaration of the basic Game Object.
 *
 * Things to do
 *   - Scale might be a useful thing to add here.
 *
 */

#ifndef __INC_GAME_OBJ_H__
#define __INC_GAME_OBJ_H__

#include <sys/time.h>
#include <pthread.h>

#include <cstdint>
#include <string>
#include <unordered_map>
#if STD_UNORDERED_SET_WORKS
#include <unordered_set>
#else
#include <set>
#endif /* STD_UNORDERED_SET_WORKS */

#include <glm/vec3.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>

class GameObject;

#include "control.h"
#include "geometry.h"

class GameObject
{
  public:
    typedef enum
    {
        invisible, non_interactive
    }
    nature;

    typedef std::unordered_map<uint64_t, GameObject *> objects_map;
    typedef objects_map::iterator objects_iterator;

    typedef int attribute;

  private:
    static pthread_mutex_t max_mutex;
    static uint64_t max_id_value;

    static glm::dvec3 no_movement;
    static glm::dquat no_rotation;

    /* const */ uint64_t id_value;
    Geometry *default_geometry;
    Control *default_master;

    struct timeval last_updated;
    glm::dvec3 position, movement, look;
    glm::dquat orient, rotation;

  public:
    std::unordered_map<std::string, attribute> attributes;
#if STD_UNORDERED_SET_WORKS
    std::unordered_set<nature> natures;
#else
    std::set<nature> natures;
#endif /* STD_UNORDERED_SET_WORKS */
    Geometry *geometry;
    Control *master;

  public:
    static uint64_t reset_max_id(void);

    GameObject(Geometry *, Control *, uint64_t = 0LL);
    ~GameObject();

    GameObject *clone(void) const;

    uint64_t get_object_id(void) const;

    bool connect(Control *);
    void disconnect(Control *);

    void activate(void);
    void deactivate(void);

    double distance_from(const glm::dvec3& pt);

    glm::dvec3 get_position(void);
    glm::dvec3 set_position(const glm::dvec3&);
    glm::dvec3 get_movement(void);
    glm::dvec3 set_movement(const glm::dvec3&);
    glm::dvec3 get_look(void);
    glm::dvec3 set_look(const glm::dvec3&);
    glm::dquat get_orientation(void);
    glm::dquat set_orientation(const glm::dquat&);
    glm::dquat get_rotation(void);
    glm::dquat set_rotation(const glm::dquat&);

    void move_and_rotate(void);
    bool still_moving(void);
};

#endif /* __INC_GAME_OBJ_H__ */
