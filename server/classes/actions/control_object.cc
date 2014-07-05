/* control_object.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 05 Jul 2014, 08:00:43 tquirk
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
 * This file contains the action routine to take control of a given game
 * object.  We'll be using this "skill" on the login procedure, since
 * all game objects will always exist, and will only be dormant while the
 * player is logged off.
 *
 * The intensity and direction arguments will never be used.
 *
 * Changes
 *   14 Aug 2006 TAQ - Created the file.
 *   29 Sep 2007 TAQ - Fleshed things out.  This should be basically complete
 *                     for now.
 *   25 Oct 2007 TAQ - Database call now takes a userid, not a username.
 *                     Added action_uncontrol_object, to drop control of an
 *                     object.
 *   10 May 2014 TAQ - Switched to Eigen math library.
 *   28 Jun 2014 TAQ - Include fixups.
 *   05 Jul 2014 TAQ - The zone_interface is moved into server.h.
 *
 * Things to do
 *   - Need to make more database calls - we need to figure out where the
 *     object is, and then we need to create it if it doesn't already exist,
 *     then we will need to make sure it's visible and interactible, and only
 *     then can we actually take control of it.
 *
 */

#include <Eigen/Core>

#include "../game_obj.h"
#include "../motion.h"
#include "../../server.h"

/* ARGSUSED */
void action_control_object(Motion *source,
			   int intensity,
			   Motion *target,
			   Eigen::Vector3d &direction)
{
    /* Source will be a Control object ptr cast into a Motion ptr */
    Control *src = (Control *)source;
    int access_type = ACCESS_NONE;

    if (src->slave == NULL)
    {
	/* Figure out if the player actually has access to the target */
	if ((access_type
             = database->check_authorization(src->userid,
                                             target->object->get_object_id()))
	    != ACCESS_NONE)
	{
            if (target->connect(src))
                src->slave = target;
	}
	/* Let the user know how things worked out */
	src->send_ack(TYPE_ACTREQ, access_type);
    }
}

/* ARGSUSED */
void action_uncontrol_object(Motion *source,
			     int intensity,
			     Motion *target,
			     Eigen::Vector3d &direction)
{
    /* Source will be a Control object ptr cast into a Motion ptr */
    Control *src = (Control *)source;

    if (src->slave == target)
    {
        target->disconnect(src);
        src->slave = NULL;
    }
    src->send_ack(TYPE_ACTREQ, 0);
}
