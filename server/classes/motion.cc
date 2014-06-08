/* motion.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 10 May 2014, 18:10:45 tquirk
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
 * This file contains the motion class for the Revision IX system.
 *
 * Changes
 *   10 May 2014 TAQ - Created the file.
 *
 * Things to do
 *
 * $Id$
 */

#include "motion.h"

Motion::Motion(GameObject *go, Control *con)
{
    this->object = go;
    this->default_master = this->master = con;
}

bool Motion::connect(Control *con)
{
    /* Only one thing can control a given thing */
    if (this->default_master == this->master)
    {
        /* Permissions will be checked in the control_object action */
        this->master = con;
        return true;
    }
    return false;
}

void Motion::disconnect(Control *con)
{
    /* Should we also check whether default_master is the same?  What would
     * we want to do in that case?
     */
    if (this->master == con)
        master = default_master;
}
