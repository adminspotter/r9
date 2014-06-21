/* zone_interface.h
 *   by Trinity Quirk <trinity@ymb.net>
 *   last updated 21 Jun 2014, 09:35:58 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2007  Trinity Annabelle Quirk
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
 * This file contains the C interfaces to the C++ zone class.  We do this
 * so we can include this file, and the extern "C" routines the zone
 * offers, in C files, without having to worry about the C++ freaking out
 * the C compiler.
 *
 * Changes
 *   02 Aug 2006 TAQ - Created the file.
 *   09 Aug 2006 TAQ - Removed add_geometry_request prototype.
 *   11 Aug 2006 TAQ - Moved the global zone pointer and thread worker
 *                     prototypes in here, with some C++ compilation guards.
 *   24 Aug 2007 TAQ - Commented add_access_request, since it will no longer
 *                     be done in the zone.
 *   05 Sep 2007 TAQ - Added access thread pool extern declaration.  Changed
 *                     add_<pool>_request prototypes to take void * as the
 *                     first arg.  Added create_send_pool, add_send_request,
 *                     and destroy_send_pool prototypes.
 *   06 Sep 2007 TAQ - Cleaned up prototype for add_send_request.
 *   08 Sep 2007 TAQ - Slight tweak to create_send_pool prototype.
 *   09 Sep 2007 TAQ - Removed some prototypes which no longer exist.
 *   10 Sep 2007 TAQ - The access pool now is of type access_list.  Added
 *                     active users list and mutex for access pool.  Cleaned
 *                     up the includes.
 *   17 Jun 2014 TAQ - Moved most of the pool worker prototypes into the
 *                     zone.
 *
 * Things to do
 *   - See if we can get rid of this altogether.  There are better ways of
 *     handling the stuff this file was originally meant to address.
 *
 */

#ifndef __INC_ZONE_INTERFACE_H__
#define __INC_ZONE_INTERFACE_H__

#ifdef __cplusplus

#include <stdio.h>
#include <pthread.h>

#include <set>

#include "defs.h"
#include "thread_pool.h"
#include "zone.h"
#include "library.h"
#include "modules/db.h"

extern Zone *zone;
extern Library *db_lib;
extern DB *database;
extern ThreadPool<access_list> *access_pool;
extern pthread_mutex_t active_users_mutex;
extern std::set<u_int64_t> *active_users;

void *access_pool_worker(void *);

extern "C"
{
#endif /* __cplusplus */

int setup_zone(void);
void cleanup_zone(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INC_ZONE_INTERFACE_H__ */
