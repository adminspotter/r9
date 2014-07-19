/* zone.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 10 Jul 2014, 14:34:16 trinityquirk
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
 *   17 Jun 2014 TAQ - Moved the thread pool routines inside the class as
 *                     static methods.
 *   21 Jun 2014 TAQ - Moved the action library handling into this class.
 *                     Updated the syslog to use the stream logger.
 *   06 Jul 2014 TAQ - We now reserve space in the sectors (formerly trees)
 *                     vectors for all the octrees we may need.  Also moved
 *                     some of the initialization into the init method, since
 *                     it was shared by more than one routine.  Added the
 *                     stop method to stop the thread pools.
 *   09 Jul 2014 TAQ - Simplified exception handling and logging.  A lot of
 *                     the exceptions that we get, we'll just let them
 *                     continue on up the call chain.
 *
 * Things to do
 *
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <glob.h>
#include <errno.h>

#include <sstream>
#include <stdexcept>

#include "zone.h"
#include "thread_pool.h"
#include "../config.h"
#include "../log.h"

/* Private methods */
void Zone::init(void)
{
    int i, j;

    this->load_actions(config.action_lib);
    this->create_thread_pools();
    try
    {
        sectors.reserve(x_steps);
        for (i = 0; i < x_steps; ++i)
        {
            sectors[i].reserve(y_steps);
            for (j = 0; j < y_steps; ++j)
                sectors[i][j].reserve(z_steps);
        }
    }
    catch (std::length_error& e)
    {
        std::ostringstream s;
        s << "couldn't reserve "
          << this->x_steps << "x" << this->y_steps << "x" << this->z_steps
          << " (" << this->x_steps * this->y_steps * this->z_steps
          << ") elements in the zone: " << e.what();
        throw std::runtime_error(s.str());
    }
}

void Zone::load_actions(const std::string& libname)
{
    try
    {
        this->action_lib = new Library(libname);
        action_reg_t *reg
            = (action_reg_t *)this->action_lib->symbol("actions_register");
        (*reg)(this->actions);
    }
    catch (std::exception& e)
    {
        std::clog << syslogErr
                  << "error loading actions library: " << e.what() << std::endl;
    }
}

void Zone::create_thread_pools(void)
{
    this->action_pool
        = new ThreadPool<packet_list>("action", config.action_threads);
    this->motion_pool
        = new ThreadPool<Motion *>("motion", config.motion_threads);
    this->update_pool
        = new ThreadPool<Motion *>("update", config.update_threads);
}

void *Zone::action_pool_worker(void *arg)
{
    Zone *zone = (Zone *)arg;
    packet_list req;
    u_int16_t skillid;

    std::clog << "started action pool worker" << std::endl;
    for (;;)
    {
        /* Grab the next packet off the queue */
        zone->action_pool->pop(&req);

        /* Process the packet */
        ntoh_packet(&req.buf, sizeof(packet));

        /* Make sure the action exists and is valid on this server,
         * before trying to do anything
         */
        skillid = req.buf.act.action_id;
        if (zone->actions.find(skillid) != zone->actions.end()
            && zone->actions[skillid].valid == true)
        {
            /* Make the action call.
             *
             * The action routine will handle checking the environment
             * and relevant skills of the target, and spawning of any
             * new needed subobjects.
             */
            if (((Control *)req.who)->slave->object->get_object_id()
                == req.buf.act.object_id)
                ((Control *)req.who)->execute_action(req.buf.act,
                                                     sizeof(action_request));
        }
    }
    std::clog << "action pool worker ending" << std::endl;
    return NULL;
}

void *Zone::motion_pool_worker(void *arg)
{
    Zone *zone = (Zone *)arg;
    Motion *req;
    struct timeval current;
    double interval;

    std::clog << "started a motion pool worker" << std::endl;
    for (;;)
    {
        /* Grab the next object to move off the queue */
        zone->motion_pool->pop(&req);

        /* Process the movement */
        /* Get interval since last move */
        gettimeofday(&current, NULL);
        interval = (current.tv_sec + (current.tv_usec * 1000000))
            - (req->last_updated.tv_sec
               + (req->last_updated.tv_usec * 1000000));
        /* Do the actual move, scaled by the interval */
        req->position += req->movement * interval;
        /*zone->game_objects[req->get_object_id()].orientation
            += req->rotation * interval;*/
        /* Do collisions (not yet) */
        /* Reset last update to "now" */
        memcpy(&req->last_updated, &current, sizeof(struct timeval));
        /* We've moved, so users need updating */
        zone->update_pool->push(req);
        /* If we're still moving, queue it up */
        if ((req->movement[0] != 0.0
             || req->movement[1] != 0.0
             || req->movement[2] != 0.0)
            || (req->rotation[0] != 0.0
                || req->rotation[1] != 0.0
                || req->rotation[2] != 0.0))
            zone->motion_pool->push(req);
    }
    std::clog << "motion pool worker ending" << std::endl;
    return NULL;
}

void *Zone::update_pool_worker(void *arg)
{
    Zone *zone = (Zone *)arg;
    Motion *req;
    u_int64_t objid;
    packet_list buf;

    std::clog << "started an update pool worker" << std::endl;
    for (;;)
    {
        zone->update_pool->pop(&req);

        /* Process the request */
        /* We won't bother to figure out who can see what just yet */
        buf.buf.pos.type = TYPE_POSUPD;
        /* We're not using a sequence number yet, but don't forget about it */
        objid = buf.buf.pos.object_id = req->object->get_object_id();
        /* We're not doing frame number yet, but don't forget about it */
        buf.buf.pos.x_pos = (u_int64_t)trunc(req->position[0] * 100);
        buf.buf.pos.y_pos = (u_int64_t)trunc(req->position[1] * 100);
        buf.buf.pos.z_pos = (u_int64_t)trunc(req->position[2] * 100);
        /*buf.buf.pos.x_orient = (int32_t)trunc(req->orientation[0] * 100);
        buf.buf.pos.y_orient = (int32_t)trunc(req->orientation[1] * 100);
        buf.buf.pos.z_orient = (int32_t)trunc(req->orientation[2] * 100);*/
        buf.buf.pos.x_look = (int32_t)trunc(req->look[0] * 100);
        buf.buf.pos.y_look = (int32_t)trunc(req->look[1] * 100);
        buf.buf.pos.z_look = (int32_t)trunc(req->look[2] * 100);
        /* Figure out who to send it to */
        /* Push the packet onto the send queue */
        /*zone->sending_pool->push(&buf, sizeof(packet));*/
    }
    std::clog << "update pool worker ending" << std::endl;
    return NULL;
}

/* Public methods */
Zone::Zone(u_int64_t dim, u_int16_t steps)
    : sectors(), actions(), game_objects()
{
    this->x_dim = this->y_dim = this->z_dim = dim;
    this->x_steps = this->y_steps = this->z_steps = steps;
    this->init();
}

Zone::Zone(u_int64_t xd, u_int64_t yd, u_int64_t zd,
           u_int16_t xs, u_int16_t ys, u_int16_t zs)
    : sectors(), actions(), game_objects()
{
    this->x_dim = xd;
    this->y_dim = yd;
    this->z_dim = zd;
    this->x_steps = xs;
    this->y_steps = ys;
    this->z_steps = zs;
    this->init();
}

Zone::~Zone()
{
    /* Stop the thread pools first; deleting them should run through the
     * destructor, which will terminate all the threads and clean up
     * the queues and stuff.
     */
    std::clog << "deleting thread pools" << std::endl;
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
    std::clog << "deleting game objects" << std::endl;
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
    if (this->action_lib != NULL)
    {
        std::clog << "cleaning up action routines" << std::endl;
        try
        {
            action_unreg_t *unreg
                = (action_unreg_t *)this->action_lib->symbol("actions_unregister");
            (*unreg)(this->actions);
        }
        catch (std::exception& e) { /* Do nothing */ }

        /* Close the actions library */
        delete this->action_lib;
    }
}

void Zone::start(void)
{
    /* Do we want to load up all the game objects here, before we start
     * up the thread pools?
     */

    this->action_pool->startup_arg = (void *)this;
    this->action_pool->start(Zone::action_pool_worker);

    this->motion_pool->startup_arg = (void *)this;
    this->motion_pool->start(Zone::motion_pool_worker);

    this->update_pool->startup_arg = (void *)this;
    this->update_pool->start(Zone::update_pool_worker);
}

void Zone::stop(void)
{
    this->action_pool->stop();
    this->motion_pool->stop();
    this->update_pool->stop();
}

void Zone::add_action_request(u_int64_t from, packet *buf, size_t len)
{
    packet_list pl;

    if (this->action_pool != NULL)
    {
        memcpy(&(pl.buf), buf, len);
        pl.who = from;
        this->action_pool->push(pl);
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
