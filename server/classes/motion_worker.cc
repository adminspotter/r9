/* motion_worker.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 10 May 2014, 18:27:11 tquirk
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
 * This file contains the motion thread pool worker routine, and all
 * other related support routines.
 *
 * Changes
 *   11 Aug 2006 TAQ - Created the file.
 *   17 Sep 2007 TAQ - Reformatted debugging output.
 *   23 Sep 2007 TAQ - The push() method of the ThreadPool changed, so I
 *                     updated the calls here.
 *   19 Sep 2013 TAQ - Return NULL at the end of the worker routine to quiet
 *                     gcc.
 *   10 May 2014 TAQ - Renamed file not to collide with the new motion object.
 *                     We now properly use Motion objects, instead of the old
 *                     way we were doing with GameObjects.
 *
 * Things to do
 *
 * $Id$
 */

#include "zone.h"
#include "zone_interface.h"

/* ARGSUSED */
void *motion_pool_worker(void *notused)
{
    Motion *req;
    struct timeval current;
    double interval;

    syslog(LOG_DEBUG, "started a motion pool worker");
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
    syslog(LOG_DEBUG, "motion pool worker ending");
    return NULL;
}
