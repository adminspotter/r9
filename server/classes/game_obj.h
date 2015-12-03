/* game_obj.h                                               -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 02 Dec 2015, 17:48:00 tquirk
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
#include <map>
#include <set>

#include <Eigen/Dense>

class GameObject;

#include "defs.h"
#include "control.h"
#include "geometry.h"

class GameObject
{
  private:
    static pthread_mutex_t max_mutex;
    static uint64_t max_id_value;

    /* const */ uint64_t id_value;
    Geometry *default_geometry;
    Control *default_master;

  public:
    std::map<std::string, attribute> attributes;
    std::set<std::string> natures;
    Geometry *geometry;
    Control *master;
    struct timeval last_updated;
    /* These vectors are in meters/degrees per second */
    Eigen::Vector3d position, movement, rotation, look;
    Eigen::Quaterniond orient;

  public:
    static uint64_t reset_max_id(void);

    GameObject(Geometry *, Control *, uint64_t = 0LL);
    ~GameObject();

    GameObject *clone(void) const;

    uint64_t get_object_id(void) const;

    bool connect(Control *);
    void disconnect(Control *);
};

#endif /* __INC_GAME_OBJ_H__ */
