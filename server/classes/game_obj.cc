/* game_obj.cc
 *   by Trinity Quirk <trinity@ymb.net>
 *   last updated 10 May 2014, 17:21:42 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2004  Trinity Annabelle Quirk
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
 * This file contains the implementation of the GameObject class.
 *
 * Changes
 *   21 Jun 2000 TAQ - Created the file.
 *   23 Jun 2000 TAQ - Moved the static member initializers in here.
 *   11 Jul 2000 TAQ - Minor changes so that it compiles.  The mutex is
 *                     no longer a pointer.
 *   27 Jul 2000 TAQ - Preliminary ExecuteAction routine.
 *   20 Aug 2000 TAQ - Removed the min template function, since STL
 *                     already has it.  Tweaked ExecuteAction method.
 *   04 Apr 2006 TAQ - Added std:: namespace specifier to STL objects.
 *   15 May 2006 TAQ - Added the GPL notice.
 *   01 Jul 2006 TAQ - Got rid of the getters and setters.  It'll just be
 *                     simpler this way.
 *   04 Jul 2006 TAQ - Quick syntax fix.  Missed deleting one of the methods
 *                     which was superseded by making an attribute public.
 *                     Also changed the action routine call, since the zone
 *                     now makes the action list public.
 *   12 Jul 2006 TAQ - Removed ExecuteAction method - it's handled in the
 *                     worker routine for the action thread pool.
 *   12 Aug 2006 TAQ - We're using a u_int64_t for the object id now.
 *   16 Aug 2006 TAQ - Added the can_see method, to determine if this object
 *                     can see another given object, for purposes of sending
 *                     motion updates.
 *   29 Nov 2009 TAQ - Added set_object_id method.
 *   29 Jun 2010 TAQ - Added new members to constructor, renamed some others,
 *                     commented most of them out.
 *
 * Things to do
 *   - Implement the destructor if necessary.
 *   - Decide on a method to make sure we don't repeat object ID
 *   values (not that there's likely to be a very large chance of
 *   that happening, but you never know).
 *
 * $Id: game_obj.cc 10 2013-01-25 22:13:00Z trinity $
 */

#include <algorithm>

#include "game_obj.h"
#include "zone.h"

pthread_mutex_t GameObject::max_mutex = PTHREAD_MUTEX_INITIALIZER;
u_int64_t GameObject::max_id_value = 0LL;

u_int64_t GameObject::reset_max_id(void)
{
    u_int64_t val;

    pthread_mutex_lock(&GameObject::max_mutex);
    val = GameObject::max_id_value;
    GameObject::max_id_value = (u_int64_t)0;
    pthread_mutex_unlock(&GameObject::max_mutex);
    return val;
}

GameObject::GameObject(Geometry *g, u_int64_t newid)
{
    this->default_geometry = this->geometry = g;
    pthread_mutex_lock(&GameObject::max_mutex);
    if (newid == 0LL)
    {
        newid = GameObject::max_id_value++;
    }
    else
    {
        /* This clause is mostly for recreating an object from some
         * saved state.
         */
        GameObject::max_id_value = std::max(GameObject::max_id_value, id_value);
    }
    pthread_mutex_unlock(&GameObject::max_mutex);
    this->id_value = newid;
}

GameObject::~GameObject()
{
}

GameObject *GameObject::clone(void) const
{
    return new GameObject(default_geometry);
}

u_int64_t GameObject::get_object_id(void) const
{
    return this->id_value;
}
