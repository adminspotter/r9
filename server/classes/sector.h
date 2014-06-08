/* sector.h                                                -*- C++ -*-
 *   by Trinity Quirk <trinity@ymb.net>
 *   last updated 04 Jul 2006, 11:17:19 trinity
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
 * This file contains the Sector object, which is the fundamental unit
 * of space in the world.
 *
 * Changes
 *   15 Apr 2000 TAQ - Created the file.
 *   17 Jun 2000 TAQ - Removed all the Geometry-related routines, since
 *                     each sector won't really *have* geometry, but will
 *                     have GameObjects which have the geometry in them.
 *                     Also got rid of the neighbors stuff, since it's
 *                     all in a grid now.  We will use octrees, at least
 *                     until we figure out if they work...
 *   29 Jun 2000 TAQ - Changed the name of the octree member to 'space'.
 *   12 Jul 2000 TAQ - The Nature stuff is now similar to that of the
 *                     GameObject.
 *   24 Jul 2000 TAQ - Considered (Add|Move|Del)Object some more.
 *   13 Aug 2000 TAQ - Added (Get|Set|Del)Space methods.
 *   28 Oct 2000 TAQ - Moved typedefs to defs.h.
 *   04 Apr 2006 TAQ - Added namespaces for std:: and Math3d:: objects.
 *   15 May 2006 TAQ - Added the GPL notice.
 *   04 Jul 2006 TAQ - Cleaned up changes to the nature type.  Made all the
 *                     members public and got rid of the getters and setters.
 *
 * Things to do
 *
 * $Id: sector.h 10 2013-01-25 22:13:00Z trinity $
 */

#ifndef __INC_SECTOR_H__
#define __INC_SECTOR_H__

/* The STL types */
#include <deque>
#include <string>
#include <map>
#include <math3d/m3d.h>

class Sector;

#include "defs.h"
#include "octree.h"
#include "game_obj.h"

class Sector
{
  public:
    octree space;
    std::deque<GameObject *> objects;
    std::map<std::string, nature> nature_list, temp_nature_list;

  public:
    Sector();
    ~Sector();

    void DoObjectCollisions(void) const;
};

#endif /* __INC_SECTOR_H__ */
