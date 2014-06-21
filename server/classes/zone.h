/* zone.h                                                  -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 21 Jun 2014, 08:35:52 tquirk
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
 * This file contains the overlying Zone object, which is basically
 * the "world" that resides in the server.
 *
 * The (x|y|z)_dim members are the sizes of *each* sector, in meters.
 * The (x|y|z)_steps members are the sizes of the sector grid along each
 * dimension.
 *
 * We want to keep the entire representation of the map in memory even
 * after we chop it up in the sectors' octrees.  It'll use a lot of
 * core, but it might be more effective for a constantly changing map.
 * Plus, if we have geometry updates (people blowing up dynamite and
 * such, heh), it'll be easier and faster to tell people what happened.
 *
 * Changes
 *   04 Apr 2000 TAQ - Created the file.
 *   10 Jun 2000 TAQ - Thought a lot more on how to actually properly
 *                     design this thing.
 *   11 Jun 2000 TAQ - Lots of changes, fleshing out.
 *   12 Jun 2000 TAQ - Solidified lots of stuff.  Getting close to a
 *                     finalish form.
 *   10 Jul 2000 TAQ - Adjusted sector_grid declaration.  AddObject no
 *                     longer returns a GO_id_t.
 *   12 Jul 2000 TAQ - The sector_grid is implemented as a vector of
 *                     vectors of vectors of Sector pointers.  This
 *                     should give us the indexing ability that we want
 *                     and handle memory allocation for us too.  It will
 *                     also handle dynamic resizing if necessary (do we
 *                     want to implement that?).
 *   24 Jul 2000 TAQ - Fixed sector_to_world, world_to_sector, and
 *                     which_sector prototypes.  The sector dimensions
 *                     are now doubles.
 *   27 Jul 2000 TAQ - Added an extern Zone pointer.
 *   13 Aug 2000 TAQ - Added the sectorize_polygons and (yz|xz|xy)_clip
 *                     private methods.
 *   20 Aug 2000 TAQ - Changed (yz|xz|xy)_clip for simply clip with an
 *                     extra argument.
 *   29 Sep 2000 TAQ - Added (setup|cleanup)_zone prototypes and C++
 *                     protection so that C files don't try to parse
 *                     the class definition.  Added get_library and
 *                     load_lib_path.
 *   02 Oct 2000 TAQ - Added char methods for (open|get|close)_library.
 *   28 Oct 2000 TAQ - Reworked world_to_sector, sector_to_world and
 *                     which_sector.  Removed clip, since we're now using
 *                     the same routine as in the octree files.  Moved
 *                     typedefs to defs.h.
 *   06 Apr 2004 TAQ - sectorize_polygons now takes lists of poly pointers.
 *   04 Apr 2006 TAQ - Added namespaces for std:: and Math3d:: objects.
 *   15 May 2006 TAQ - Added the GPL notice.
 *   01 Jul 2006 TAQ - Commented out all the library stuff, since we're going
 *                     to get things working without a lot of the frills,
 *                     most of which we won't need until later anyway.
 *                     Actions are now indexed by ints.
 *   04 Jul 2006 TAQ - Added thread pools.  Moved some of the members to
 *                     be public, for simplification.
 *   06 Jul 2006 TAQ - Commented zone controls for now.  Changed dimensions
 *                     to u_int64_t.
 *   12 Jul 2006 TAQ - Commented all the octree stuff out to slim everything
 *                     down to test spheres as game objects.  Added position
 *                     and orientation of the individual game objects.
 *   23 Jul 2006 TAQ - The proto.h include moved.
 *   26 Jul 2006 TAQ - Added access and geometry thread pools.  Added extern
 *                     C routines to add to access, action, and geometry
 *                     request queues.
 *   09 Aug 2006 TAQ - Removed geometry pool and add_geometry_request
 *                     prototype, since we'll no longer be handling those.
 *   11 Aug 2006 TAQ - Added map of control objects, keyed by a struct
 *                     sockaddr_in, so we can find them easily.  Hopefully
 *                     proxies won't mess this idea up.  Actually *deleted*
 *                     all the now-superfluous stuff, since it may well
 *                     change drastically by the time we need it.  Removed
 *                     character-string args from the constructors.
 *   12 Aug 2006 TAQ - We're using a u_int64_t for the game object id now.
 *   16 Aug 2006 TAQ - The login_list type is now called packet_list.  The
 *                     sending and action pools now take packet_lists.
 *   30 Jun 2007 TAQ - Updated Sockaddr to reflect new naming.
 *   24 Aug 2007 TAQ - Commented access and sending pools and players map,
 *                     since they're going to be moving out of here.
 *   05 Sep 2007 TAQ - Removed the access pool, since we've got that sorted
 *                     out elsewhere.  Removed players map, since we're
 *                     letting the Control objects handle all that.  Removed
 *                     the sending pool, since each socket thread will
 *                     handle that.  Removed sockaddr.h include.
 *   07 Oct 2007 TAQ - Added create_thread_pools and start methods.  Added
 *                     throw declarations to constructors.
 *   11 Oct 2007 TAQ - Changed action_routines map a bit to include a valid
 *                     flag among other things, and also an explicit index
 *                     size (not "int") to match the field in the proto struct.
 *   13 Oct 2007 TAQ - Renamed action_routines to simply actions.
 *   14 Oct 2007 TAQ - Changed unsigned short steps arguments to u_int16_t.
 *   10 May 2014 TAQ - Switched to the Eigen math library.  The separation
 *                     between the Control object's execute_action and this
 *                     class was not very good, so we now also have a method
 *                     called execute_action, which does most of the checking.
 *   17 Jun 2014 TAQ - Moved the thread pool functions into this class.
 *   21 Jun 2014 TAQ - Moved the action library in here too.
 *
 * Things to do
 *
 */

#ifndef __INC_ZONE_H__
#define __INC_ZONE_H__

#include <map>

#include "defs.h"
#include "control.h"
#include "game_obj.h"
#include "thread_pool.h"
#include "library.h"
#include "proto.h"

class Zone
{
  private:
    u_int16_t x_steps, y_steps, z_steps;
    u_int64_t x_dim, y_dim, z_dim;

    Library *action_lib;

  public:
    std::map<u_int16_t, action_rec> actions;
    std::map<u_int64_t, game_object_list_element> game_objects;
    ThreadPool<packet_list> *action_pool;   /* Takes action requests      */
    ThreadPool<Motion *> *motion_pool;      /* Processes motion/collision */
    ThreadPool<Motion *> *update_pool;      /* Prepares motion updates    */

  private:
    void load_actions(const char *);
    void create_thread_pools(void);

    static void *action_pool_worker(void *);
    static void *motion_pool_worker(void *);
    static void *update_pool_worker(void *);

  public:
    Zone(u_int64_t, u_int16_t);
    Zone(u_int64_t, u_int64_t, u_int64_t, u_int16_t, u_int16_t, u_int16_t);
    ~Zone();

    void start(void);

    /* Interface to the action pool */
    void add_action_request(u_int64_t, packet *, size_t);

    void execute_action(Control *, action_request&, size_t);
};

#endif /* __INC_ZONE_H__ */
