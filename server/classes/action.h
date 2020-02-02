/* action.h                                                -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 23 Dec 2019, 19:39:18 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2019  Trinity Annabelle Quirk
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
 * This file contains a class which handles actions, including names,
 * function pointers, level minima and maxima, server validity, and
 * defaulting.
 *
 * Things to do
 *
 */

#ifndef __INC_ACTION_H__
#define __INC_ACTION_H__

#include <stdint.h>

#include <string>
#include <unordered_map>

#include "game_obj.h"

class Action
{
  public:
    std::string name;
    int (*action)(GameObject *, int, GameObject *, glm::dvec3&);
    uint16_t def;                 /* The "default" skill to use */
    int lower, upper;             /* The bounds for skill levels */
    bool valid;                   /* Is this action valid on this server? */

    Action()
        : name()
        {
            this->action = NULL;
            this->def = 0;
            this->lower = 0;
            this->upper = 0;
            this->valid = false;
        };
    ~Action()
        {
        };
};

typedef std::unordered_map<uint16_t, Action> actions_map;
typedef actions_map::iterator actions_iterator;

#endif /* __INC_ACTION_H__ */
