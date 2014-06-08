/* action.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 10 May 2014, 18:12:41 tquirk
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
 * This file contains the action thread pool worker routine, and all
 * other related support routines.
 *
 * Changes
 *   11 Aug 2006 TAQ - Created the file.
 *   16 Aug 2006 TAQ - Massively simplified this, because it's going
 *                     to be controlled mostly by the control and associated
 *                     game object, since they directly know most about
 *                     themselves.  We now take packet_list instead of packet.
 *   05 Sep 2007 TAQ - Removed references to the players list in the zone,
 *                     since we'll no longer be handling things that way.
 *   12 Sep 2007 TAQ - Changed type of from argument in add_action_request
 *                     from void * to u_int64_t.
 *   17 Sep 2007 TAQ - Reformatted debugging output.
 *   23 Sep 2007 TAQ - The push() member of ThreadPool changed, so updated
 *                     the call.
 *   13 Oct 2007 TAQ - Moved the checking of the zone's action routine list
 *                     from the control object into here.  The zone's action
 *                     routine list is now just called actions.
 *   19 Sep 2013 TAQ - Return NULL at the end of the worker routine to quiet
 *                     gcc.
 *   10 May 2014 TAQ - Action calls changed a bit, so made the pointers
 *                     pointing at the right things.
 *
 * Things to do
 *
 * $Id$
 */

#include "zone.h"
#include "zone_interface.h"
#include "control.h"

void add_action_request(u_int64_t from, packet *buf, size_t len)
{
    packet_list pl;

    if (zone->action_pool != NULL)
    {
	memcpy(&(pl.buf), buf, len);
	pl.who = from;
	zone->action_pool->push(pl);
    }
}

/* ARGSUSED */
void *action_pool_worker(void *notused)
{
    packet_list req;
    u_int16_t skillid;

    syslog(LOG_DEBUG, "started action pool worker");
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
    syslog(LOG_DEBUG, "action pool worker ending");
    return NULL;
}
