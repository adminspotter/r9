/* register.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 10 May 2014, 16:02:08 tquirk
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
 * This file contains the un/registration functions for our action list.
 *
 * Changes
 *   14 Aug 2006 TAQ - Created the file.
 *   16 Aug 2006 TAQ - A quick cast to get rid of a signed/unsigned comparison
 *                     complaint by the compiler.  Added extern "C" linkage.
 *   05 Sep 2007 TAQ - Fixed a compiler complaint.
 *   11 Oct 2007 TAQ - Updated to use new action record maps.
 *   10 May 2014 TAQ - Switched to the Eigen math library.  Updated calls to
 *                     use Motion objects.
 *
 * Things to do
 *
 * $Id: register.cc 10 2013-01-25 22:13:00Z trinity $
 */

#include <map>

#include "../defs.h"
#include "register.h"

#define ENTRIES(x)  (int)(sizeof(x) / sizeof(x[0]))

extern "C"
{
    void actions_register(std::map<u_int16_t, action_rec> &am)
    {
	int i;

	for (i = 0; i < ENTRIES(actions); ++i)
	    am[actions[i].action_number].action = actions[i].action_routine;
    }

    void actions_unregister(std::map<u_int16_t, action_rec> &am)
    {
	int i;

	for (i = 0; i < ENTRIES(actions); ++i)
	    if (am[actions[i].action_number].action
		== actions[i].action_routine)
		am.erase(actions[i].action_number);
    }
}
