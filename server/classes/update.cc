/* update.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 10 May 2014, 19:08:01 tquirk
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
 * This file contains the update thread pool worker routine, and all
 * other related support routines.
 *
 * Changes
 *   11 Aug 2006 TAQ - Created the file.
 *   21 Jun 2007 TAQ - Added math.h include.
 *   05 Sep 2007 TAQ - Commented out the sending pool, since we're changing
 *                     the way those things occur.
 *   17 Sep 2007 TAQ - Reformatted debugging output.
 *   19 Sep 2013 TAQ - Return NULL at the end of the worker routine to quiet
 *                     gcc.
 *
 * Things to do
 *
 * $Id: update.cc 10 2013-01-25 22:13:00Z trinity $
 */

#include <math.h>

#include "zone.h"
#include "zone_interface.h"

/* ARGSUSED */
void *update_pool_worker(void *notused)
{
    Motion *req;
    u_int64_t objid;
    packet_list buf;

    syslog(LOG_DEBUG, "started an update pool worker");
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
    syslog(LOG_DEBUG, "update pool worker ending");
    return NULL;
}
