/* register.h
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 28 Jun 2014, 09:39:33 tquirk
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
 * Things to do
 *
 */

#ifndef __INC_REGISTER_H__
#define __INC_REGISTER_H__

#include "../motion.h"

typedef void action_routine_t(Motion *, int, Motion *, Eigen::Vector3d&);

/* Prototypes for each action function */
void action_control_object(Motion *, int, Motion *, Eigen::Vector3d &);
void action_uncontrol_object(Motion *, int, Motion *, Eigen::Vector3d &);
void action_move(Motion *, int, Motion *, Eigen::Vector3d &);
void action_rotate(Motion *, int, Motion *, Eigen::Vector3d &);

struct action_routines_list_tag
{
    int action_number;
    const char *action_name;
    action_routine_t *action_routine;
}
actions[] =
{
    { 1, "Control object",   action_control_object   },
    { 2, "Uncontrol object", action_uncontrol_object },
    { 3, "Move",             action_move             },
    { 4, "Rotate",           action_rotate           }
};

#endif /* __INC_REGISTER_H__ */
