/* register.h
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 10 May 2014, 15:56:31 tquirk
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
 * This file contains the un/registration function prototypes and the table
 * of actions that we support.
 *
 * Changes
 *   14 Aug 2006 TAQ - Created the file.
 *   11 Oct 2007 TAQ - Updated action_routine element of list.
 *   20 Oct 2009 TAQ - Added rotate action.
 *   22 Nov 2009 TAQ - Added const to the action_name element of the
 *                     action_routines_list struct.
 *   29 Nov 2009 TAQ - Added uncontrol object action.
 *   10 May 2014 TAQ - Switched to the Eigen math library.  Also updated
 *                     action routines to take Motion objects instead of
 *                     GameObjects.
 *
 * Things to do
 *
 * $Id$
 */

#ifndef __INC_REGISTER_H__
#define __INC_REGISTER_H__

#include "motion.h"

/* Prototypes for each action function */
void action_control_object(Motion *, int, Motion *, Eigen::Vector3d &);
void action_uncontrol_object(Motion *, int, Motion *, Eigen::Vector3d &);
void action_move(Motion *, int, Motion *, Eigen::Vector3d &);
void action_rotate(Motion *, int, Motion *, Eigen::Vector3d &);

struct action_routines_list_tag
{
    int action_number;
    const char *action_name;
    void (*action_routine)(Motion *, int, Motion *, Eigen::Vector3d&);
}
actions[] =
{
    { 1, "Control object",   action_control_object   },
    { 2, "Uncontrol object", action_uncontrol_object },
    { 3, "Move",             action_move             },
    { 4, "Rotate",           action_rotate           }
};

#endif /* __INC_REGISTER_H__ */
