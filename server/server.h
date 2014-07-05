/* server.h
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 05 Jul 2014, 07:47:43 tquirk
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
 * This file contains some prototypes for the server program.
 *
 * Changes
 *   11 Apr 1998 TAQ - Created the file.
 *   17 Apr 1998 TAQ - Added the SERVER_ROOT and SERVER_CONFIG defines.
 *   10 May 1998 TAQ - Added CVS ID string.
 *   24 May 1998 TAQ - Removed the ENTRIES macro, since it won't work with
 *                     dynamically-allocated blocks; sizeof only works
 *                     with automatic blocks.
 *   14 Feb 1999 TAQ - Removed some meaningless defines.
 *   20 Feb 1999 TAQ - Added the cleanup_sockets prototype.
 *   16 Apr 2000 TAQ - Reset the CVS id string.
 *   27 Jul 2000 TAQ - Removed SERVER_CONF_FILE define since it was unused.
 *   14 Oct 2000 TAQ - Added complete_(startup|cleanup).
 *   29 Oct 2000 TAQ - Added load_zone and unload_zone prototypes.
 *   15 May 2006 TAQ - Added the GPL notice.
 *   13 Aug 2006 TAQ - Removed the load/unload_zone prototypes.
 *   23 Aug 2007 TAQ - Removed cleanup_sockets prototype.  Added
 *                     set_exit_flag prototype.
 *   24 Aug 2007 TAQ - Removed EXPANSION define.
 *   30 Aug 2007 TAQ - Added start_(stream|dgram)_server prototypes.
 *   04 Sep 2007 TAQ - Added create_socket prototype.
 *   13 Sep 2007 TAQ - Removed create_socket prototype, since it's being
 *                     done elsewhere now.
 *   23 Jun 2014 TAQ - Removed nonexistent prototypes.
 *
 * Things to do
 *
 */

#ifndef __INC_SERVER_H__
#define __INC_SERVER_H__

#include "classes/zone.h"
#include "classes/modules/db.h"

extern Zone *zone;
extern DB *database;

void set_exit_flag(void);
void complete_startup(void);
void complete_cleanup(void);

#endif /* __INC_SERVER_H__ */
