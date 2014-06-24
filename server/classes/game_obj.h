/* game_obj.h                                               -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 24 Jun 2014, 18:25:16 tquirk
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
 * This file contains the declaration of the basic Game Object.
 *
 * Changes
 *   02 May 2000 TAQ - Created the file.
 *   04 May 2000 TAQ - Minor tweaking; some of this is still pretty
 *                     black-box as yet.
 *   10 Jun 2000 TAQ - Added GO_id_t, the type that is used for the
 *                     game objects' system ID values; currently an
 *                     unsigned long long.  Also added the static member
 *                     max_id_value and member function ResetMaxID, and
 *                     the normal private member id_value.
 *   11 Jun 2000 TAQ - Moved some things around.
 *   12 Jun 2000 TAQ - Added GetObjectID.
 *   21 Jun 2000 TAQ - Changed a little bit of the return values.  We
 *                     don't really need return values for much of this.
 *   23 Jun 2000 TAQ - Moved static member initializers into the .cc file.
 *   20 Aug 2000 TAQ - Tweaked ExecuteAction method.
 *   28 Oct 2000 TAQ - Removed point.h include since we're using Math3d now.
 *                     Moved typedefs into defs.h.
 *   24 Jun 2002 TAQ - We no longer really care anything about our geometry;
 *                     it will all be handled by the geometry object.  We'll
 *                     just need to know our position, and what frame we're on.
 *   04 Apr 2006 TAQ - Added std:: namespace specifiers to STL objects.
 *   15 May 2006 TAQ - Added the GPL notice.
 *   01 Jul 2006 TAQ - Made a lot of the attributes public, since it'll just
 *                     be easier in the long run.  Actions are now going to be
 *                     indexed by number, since it's likely that we'll have a
 *                     simple lookup table (since it's fast) to do the
 *                     dispatching.  Removed sector pointer, since we can
 *                     be in more than one at a time.
 *   12 Jul 2006 TAQ - Added directional/angular motion vectors and last-
 *                     updated timestamp.  Removed the ExecuteAction routine,
 *                     since that is now handled in the action thread pool
 *                     worker routine.  Added look vector, so we can look
 *                     in a different direction than we are facing.  Probably
 *                     won't be used anytime soon, but it's there now.
 *   12 Aug 2006 TAQ - We're just using u_int64_t for the object id now.
 *   16 Aug 2006 TAQ - Added prototype for can_see, which will be used to
 *                     determine if this object can see another specified
 *                     object, for purposes of sending position updates.
 *   11 Oct 2007 TAQ - Updated action_list to use correct u_int16_t id size.
 *   13 Oct 2007 TAQ - Renamed the <foo>_list members to just <foo>s.
 *   29 Nov 2009 TAQ - Added set_object_id method.
 *   29 Jun 2010 TAQ - Added quaternion for orientation.
 *   10 May 2014 TAQ - Moved the motion- and control-related stuff into
 *                     motion.h/cc and control.h/cc respectively.  Added
 *                     clone.  Removed can_see, since it's not meaningful
 *                     in here any longer.
 *   24 Jun 2014 TAQ - Took another look at the const id member, and didn't
 *                     come up with anything.
 *
 * Things to do
 *   - Scale might be a useful thing to add here.
 *
 */

#ifndef __INC_GAME_OBJ_H__
#define __INC_GAME_OBJ_H__

#include <pthread.h>

/* The STL types */
#include <string>
#include <map>

class GameObject;

#include "defs.h"
#include "geometry.h"

class GameObject
{
  private:
    static pthread_mutex_t max_mutex;
    static u_int64_t max_id_value;

    /* const */ u_int64_t id_value;
    Geometry *default_geometry;
  public:
    std::map<std::string, attribute> attributes;
    std::map<std::string, nature> natures;
    Geometry *geometry;

  public:
    static u_int64_t reset_max_id(void);

    GameObject(Geometry *, u_int64_t = 0LL);
    ~GameObject();

    GameObject *clone(void) const;

    u_int64_t get_object_id(void) const;
};

#endif /* __INC_GAME_OBJ_H__ */
