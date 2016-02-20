/* register.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 20 Feb 2016, 10:12:44 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2016  Trinity Annabelle Quirk
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
 * This file contains the un/registration functions for our action list.
 *
 * Things to do
 *
 */

#include <cstdint>
#include <map>

#include "../defs.h"
#include "register.h"

#define ENTRIES(x)  (int)(sizeof(x) / sizeof(x[0]))

extern "C"
{
    void actions_register(std::map<uint16_t, action_rec> &am)
    {
        int i;

        for (i = 0; i < ENTRIES(actions); ++i)
        {
            action_rec& ar = am[actions[i].action_number];

            ar.action = actions[i].action_routine;
            if (ar.name.empty())
                ar.name = actions[i].action_name;
        }
    }

    void actions_unregister(std::map<uint16_t, action_rec> &am)
    {
        int i;

        for (i = 0; i < ENTRIES(actions); ++i)
            if (am[actions[i].action_number].action
                == actions[i].action_routine)
                am.erase(actions[i].action_number);
    }
}
