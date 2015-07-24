/* r9client.h
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 24 Jul 2015, 12:27:10 tquirk
 *
 * Revision IX game client
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
 * This file contains the publically-available function prototypes
 * and structures for the client program.
 *
 * Changes
 *   26 Sep 1998 TAQ - Created the file.
 *   18 Jul 2006 TAQ - Changed it to client.h, and included all the routines
 *                     that need to be available everywhere.  Added GPL notice.
 *   19 Jul 2006 TAQ - Added socket creation prototype.  Added about creation
 *                     callback.
 *   30 Jul 2006 TAQ - Comm prototypes changed a bit.
 *   03 Aug 2006 TAQ - Reworked the geometry and texture management protos.
 *                     Added some default value defines.
 *   04 Aug 2006 TAQ - Added draw_texture prototype.  A couple other geo/tex
 *                     prototypes changed.
 *   09 Aug 2006 TAQ - The geometry and texture update routines have changed.
 *   10 Aug 2006 TAQ - Added draw_geometry prototype.
 *   12 Sep 2013 TAQ - Added linux/limits.h include for PATH_MAX.
 *   19 Jul 2014 TAQ - Copied from client.h and stripped out all OS-specific
 *                     stuff.
 *   23 Jul 2014 TAQ - Removed more functions which are now inside classes.
 *   26 Jul 2014 TAQ - Removed more stuff which moved inside classes.
 *   24 Jul 2015 TAQ - Converted to stdint types
 *
 * Things to do
 *
 */

#ifndef __INC_R9CLIENT_H__
#define __INC_R9CLIENT_H__

#include "../proto/proto.h"

#ifndef STORE_PREFIX
#define STORE_PREFIX   "/usr/share/r9/"
#endif /* STORE_PREFIX */

void draw_geometry(uint64_t, uint16_t);
void draw_texture(uint64_t);

void move_object(uint64_t, uint16_t, double, double, double, double, double, double);

#endif /* __INC_R9CLIENT_H__ */
