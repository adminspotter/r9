/* object.h                                            -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 27 Dec 2015, 09:16:52 tquirk
 *
 * Revision IX game client
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
 * This file contains the object cache class declaration.  An "object"
 * is a geometry, combined with position, rotation, and frame number.
 * For the time being, however, we're eliminating the geometry and
 * frame number, and focusing on the position and orientation ONLY.
 *
 * Things to do
 *
 */

#ifndef __INC_R9CLIENT_OBJECT_H__
#define __INC_R9CLIENT_OBJECT_H__

#include <config.h>

#include <stdint.h>

#include <glm/vec3.hpp>
#include <glm/gtx/quaternion.hpp>

#include "cache.h"

struct object
{
    glm::vec3 position;
    glm::fquat orientation;
};

typedef BasicCache<object> ObjectCache;

#endif /* __INC_R9CLIENT_OBJECT_H__ */
