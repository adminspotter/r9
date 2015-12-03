/* control_object.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 02 Dec 2015, 17:48:02 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2015  Trinity Annabelle Quirk
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
 * Things to do
 *   - Need to make more database calls - we need to figure out where the
 *     object is, and then we need to create it if it doesn't already exist,
 *     then we will need to make sure it's visible and interactible, and only
 *     then can we actually take control of it.
 *
 */

#include <Eigen/Core>

#include "../game_obj.h"
#include "../../server.h"

/* ARGSUSED */
int action_control_object(GameObject *source,
                          int intensity,
                          GameObject *target,
                          Eigen::Vector3d &direction)
{
    /* Source will be a Control object ptr cast into a GameObject ptr */
    Control *src = (Control *)source;
    int access_type = ACCESS_NONE;

    if (src->slave == NULL && target != NULL)
    {
        /* Figure out if the player actually has access to the target */
        if ((access_type
             = database->check_authorization(src->userid,
                                             target->get_object_id()))
            != ACCESS_NONE)
        {
            if (target->connect(src))
            {
                target->natures.erase("invisible");
                target->natures.erase("non-interactive");
                src->slave = target;
            }
        }
        /* Let the user know how things worked out */
        return access_type;
    }
    return -1;
}

/* ARGSUSED */
int action_uncontrol_object(GameObject *source,
                            int intensity,
                            GameObject *target,
                            Eigen::Vector3d &direction)
{
    /* Source will be a Control object ptr cast into a GameObject ptr */
    Control *src = (Control *)source;

    if (src->slave == target)
    {
        target->disconnect(src);
        src->slave = NULL;
        return 0;
    }
    return -1;
}
