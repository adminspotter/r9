/* zone.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 12 May 2014, 22:16:04 tquirk
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
 * This is the implementation of the zone object.
 *
 * Changes
 *   11 Jun 2000 TAQ - Created the file.
 *   12 Jun 2000 TAQ - Lots of fleshing out.  Just need to do the sector
 *                     stuff and we're mostly done.
 *   29 Jun 2000 TAQ - LoadWorld will do the sector creation for us.
 *   10 Jul 2000 TAQ - Implemented (create|delete)_sector_grid.
 *   12 Jul 2000 TAQ - Got a good implementation of the sector_grid.
 *   24 Jul 2000 TAQ - Implemented sector_to_world and world_to_sector.
 *                     the dimension variables are now doubles.
 *   27 Jul 2000 TAQ - Added a zone object pointer.
 *   13 Aug 2000 TAQ - Minor tweak to delete_sector_grid which might
 *                     free a bit of memory.  Added sectorize_polygons
 *                     and (yz|xz|xy)_clip private methods.  Implemented
 *                     LoadWorld.
 *   20 Aug 2000 TAQ - Removed (yz|xz|xy)_clip for a single clip method
 *                     which takes an int of which element we're clipping.
 *   21 Aug 2000 TAQ - Small tweaks for proper compilation.
 *   29 Sep 2000 TAQ - Added (setup|cleanup)_zone functions with C linkage.
 *                     Added sanity checking on Add, Get, and Del methods
 *                     for zone controls, game objects, action routines,
 *                     and libraries. Added load_lib_path function to
 *                     actually load the zone control and action routine
 *                     libraries.
 *   02 Oct 2000 TAQ - Added some (open|get|close)_library methods that
 *                     take const char * instead of string.
 *   03 Oct 2000 TAQ - Removed extern "C" from the definitions of
 *                     (setup|cleanup)_zone.  They're only apparently
 *                     needed on the declarations.
 *   28 Oct 2000 TAQ - Converted to use M3d instead of point.  Fixed the
 *                     poly-clipping routines to be more efficient.  Set
 *                     a very simple funcptr type for Action, which will
 *                     need to be refined once we figure out what the
 *                     action routines actually need.  Made the ZoneControl
 *                     into a simple function typedef.  Fixed a problem
 *                     with error string variables in load_lib_path.
 *   29 Oct 2000 TAQ - Added NULL-checking for the dlsym in load_lib_path.
 *   14 Feb 2002 TAQ - Fixed some config changes with the library names and
 *                     paths.
 *   02 Aug 2003 TAQ - Had to cast away const in the destructor.
 *   06 Apr 2004 TAQ - sectorize_polygons now takes lists of polygon pointers.
 *                     A whole bunch of vars and stuff needed to change because
 *                     of this as well.
 *   04 Apr 2006 TAQ - Added namespaces for std:: and Math3d:: objects.  The
 *                     polygon object is also now an object instead of a
 *                     typedef of a vector, so a few things have changed.
 *   15 May 2006 TAQ - Added the GPL notice.
 *   04 Jul 2006 TAQ - Some fixes for our removal/commenting of funcs.  Cleanup
 *                     from changes to the nature type.  Made several members
 *                     public, and removed associated getters and setters.
 *   06 Jul 2006 TAQ - Commented the zone controls for now.  Changed dimension
 *                     elements to be u_int64_t.
 *   12 Jul 2006 TAQ - Commented all the sector stuff out, since the octrees
 *                     don't currently understand spheres, which is what the
 *                     game objects are now.  This file is more comment than
 *                     code.  Fleshed out pool worker routines.
 *   17 Jul 2006 TAQ - When putting the numbers into a position update packet,
 *                     we multiply the floating point numbers by 100, and
 *                     truncate the decimal portion.  Centimeter resolution
 *                     should be good enough.
 *   23 Jul 2006 TAQ - Removed the proto.h include, since it's in the zone
 *                     header file (and has also moved around a bit).
 *   27 Jul 2006 TAQ - We're segfaulting somewhere in the update_pool_worker
 *                     routine.  From current testing, it seems to be a
 *                     possible race condition between something in the thread
 *                     creation (perhaps the queue), and the new thread(s)
 *                     trying to access it.  If we sleep on entry into the
 *                     threads' worker routines, there are no problems.
 *   30 Jul 2006 TAQ - Changed the thread pool startup a bit - now we make an
 *                     explicit call to start() to get the routines going; that
 *                     should hopefully take care of the apparent race
 *                     condition we're seeing.
 *   09 Aug 2006 TAQ - Removed geometry pool, since we'll no longer be handling
 *                     geometry and texture requests.
 *   11 Aug 2006 TAQ - Deleted all the superfluous stuff, since it's probably
 *                     going to be drastically different by the time we need
 *                     to use it.  Moved the pool workers and support routines
 *                     into their own files, since they really don't belong
 *                     here.
 *   12 Aug 2006 TAQ - We're using a u_int64_t for the game object id now.
 *   30 Jun 2007 TAQ - Updated Sockaddr to reflect naming changes.
 *   24 Aug 2007 TAQ - Commented all references to the players map, and the
 *                     access and sending thread pools, since they're going
 *                     to be moving out of here.
 *   05 Sep 2007 TAQ - Removed references to access and sending pools, and
 *                     the players map.
 *   07 Oct 2007 TAQ - Moved thread handling in here - added
 *                     create_thread_pools and start methods.  Added some
 *                     exception handling to the constructors, since the
 *                     thread pools require it.
 *   13 Oct 2007 TAQ - Renamed action_routines to simply actions.
 *   14 Oct 2007 TAQ - Changed unsigned short steps arguments to u_int16_t.
 *   11 May 2014 TAQ - Added the execute_action method, to separate the
 *                     Control's part of executing actions from the zone's.
 *
 * Things to do
 *   - Do we even want to handle geometry here, or would that be more
 *     appropriate to handle in each of the game objects?  Saving geometry
 *     for a single object in a file might actually make sense, and allow
 *     the game object to hide that bit of complexity from everybody else.
 *     So the Load World operation becomes more of a "create this list of
 *     game objects, in these given locations".
 *   - Get a handle on our geometry file format.  We'll probably want to
 *     use the same files as the client, but we'll need the bounding boxes
 *     instead of the actual geometry of the object.  Two-level bounding
 *     objects will probably be nice; the first level will be a sphere, and
 *     the second will be a somewhat tight box (or set of boxes).
 *
 * $Id$
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <syslog.h>
#include <glob.h>
#include <dlfcn.h>
#include <errno.h>

#include "zone.h"
#include "thread_pool.h"
#include "../config.h"

/* Private methods */
void Zone::create_thread_pools(void)
    throw (int)
{
    try
    {
	this->action_pool
	    = new ThreadPool<packet_list>("action", config.action_threads);
	this->motion_pool
	    = new ThreadPool<Motion *>("motion", config.motion_threads);
	this->update_pool
	    = new ThreadPool<Motion *>("update", config.update_threads);
    }
    catch (int errval)
    {
	syslog(LOG_ERR,
	       "couldn't create zone thread pool: %s",
	       strerror(errval));
	throw;
    }
}

/* Public methods */
Zone::Zone(u_int64_t dim, u_int16_t steps)
    throw (int)
    : actions(), game_objects()
{
    this->x_dim = this->y_dim = this->z_dim = dim;
    this->x_steps = this->y_steps = this->z_steps = steps;
    try { this->create_thread_pools(); }
    catch (int errval) { throw; }
}

Zone::Zone(u_int64_t xd, u_int64_t yd, u_int64_t zd,
	   u_int16_t xs, u_int16_t ys, u_int16_t zs)
    throw (int)
    : actions(), game_objects()
{
    this->x_dim = xd;
    this->y_dim = yd;
    this->z_dim = zd;
    this->x_steps = xs;
    this->y_steps = ys;
    this->z_steps = zs;
    try { this->create_thread_pools(); }
    catch (int errval) { throw; }
}

Zone::~Zone()
{
    /* Stop the thread pools first; deleting them should run through the
     * destructor, which will terminate all the threads and clean up
     * the queues and stuff.
     */
    syslog(LOG_DEBUG, "deleting thread pools");
    if (this->action_pool != NULL)
    {
	delete this->action_pool;
	this->action_pool = NULL;
    }
    if (this->motion_pool != NULL)
    {
	delete this->motion_pool;
	this->motion_pool = NULL;
    }
    if (this->update_pool != NULL)
    {
	delete this->update_pool;
	this->update_pool = NULL;
    }

    /* Delete all the game objects. */
    syslog(LOG_DEBUG, "deleting game objects");
    if (this->game_objects.size())
    {
	std::map<u_int64_t, game_object_list_element>::iterator i;

	for (i = this->game_objects.begin();
	     i != this->game_objects.end();
	     ++i)
	    delete i->second.obj;
	/* Maybe save the game objects' locations before deleting them? */
	this->game_objects.erase(this->game_objects.begin(),
				 this->game_objects.end());
    }

    /* Unregister all the action routines. */
    syslog(LOG_DEBUG, "cleaning up action routines");
    if (this->actions.size())
	this->actions.erase(this->actions.begin(), this->actions.end());
}

void Zone::start(void *(*action)(void *),
		 void *(*motion)(void *),
		 void *(*update)(void *))
    throw (int)
{
    /* Do we want to load up all the game objects here, before we start
     * up the thread pools?
     */

    try
    {
	this->action_pool->start(action);
	this->motion_pool->start(motion);
	this->update_pool->start(update);
    }
    catch (int errval)
    {
	syslog(LOG_ERR,
	       "couldn't start zone thread pool: %s",
	       strerror(errval));
	throw;
    }
}

void Zone::execute_action(Control *con, action_request& req, size_t len)
{
    /* On entry, the control object has already determined if it has
     * the skill in question, and has scaled the power level of the
     * request to its skill level.
     */
    std::map<u_int16_t, action_rec>::iterator i
        = this->actions.find(req.action_id);
    Eigen::Vector3d vec;

    vec << req.x_pos_dest, req.y_pos_dest, req.z_pos_dest;
    if (i != this->actions.end())
    {
        /* If it's not valid on this server, it should at least have
         * a default.
         */
        if (!i->second.valid)
        {
            req.action_id = i->second.def;
            i = this->actions.find(req.action_id);
        }

        req.power_level = std::max<u_int8_t>(req.power_level, i->second.lower);
        req.power_level = std::min<u_int8_t>(req.power_level, i->second.upper);

        (*(i->second.action))(con->slave,
                              req.power_level,
                              this->game_objects.find(req.dest_object_id)->second.mot,
                              vec);
    }
}
