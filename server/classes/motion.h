/* motion.h                                                 -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 10 May 2014, 18:22:48 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2014  Trinity Annabelle Quirk
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
 * This file contains the declaration of the motion object.  Keeping
 * the motion-related stuff separate from the objects themselves may
 * present a bit cleaner interface, than having to modify *each*
 * object every time there is a position change.
 *
 * Changes
 *   10 May 2014 TAQ - Created the file, moving the motion-related parameters
 *                     out of game_obj.h.
 *
 * Things to do
 *
 * $Id$
 */

#ifndef __INC_MOTION_H__
#define __INC_MOTION_H__

#include <sys/time.h>

class Motion;

#include "defs.h"
#include "control.h"

class Motion
{
  private:
    Control *default_master;

  public:
    Control *master;
    struct timeval last_updated;
    /* These vectors are in meters/degrees per second */
    Eigen::Vector3d position, movement, rotation, look;
    Eigen::Quaterniond orient;
    GameObject *object;

  public:
    Motion(GameObject *, Control *);

    bool connect(Control *);
    void disconnect(Control *);
};

#endif /* __INC_MOTION_H__ */
