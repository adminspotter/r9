/* geometry.c
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 24 Jul 2015, 13:28:42 tquirk
 *
 * Revision IX game server
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
 * This file contains the geometry class implementation.
 *
 * Changes
 *   19 Feb 2002 TAQ - Created the file.
 *   15 Mar 2002 TAQ - Started messing with bounding volumes.
 *   09 Apr 2002 TAQ - Removed addPolys routine.
 *   24 Jun 2002 TAQ - Converted to using RAPID.
 *   30 Jun 2002 TAQ - Started adding frame sequences.
 *   02 Aug 2003 TAQ - Worked a little more with the frame sequences.  Also
 *                     fixed a couple problems with the RAPID passthrough
 *                     functions.
 *   15 May 2006 TAQ - Added the GPL notice.
 *   27 Jun 2006 TAQ - For the time being, we're going to keep this class
 *                     extremely basic - we'll just be a sphere of some
 *                     radius, and that's it.
 *   09 Jul 2006 TAQ - Removed methods which are no longer needed.
 *   11 Aug 2006 TAQ - Removed unneeded methods.
 *   24 Jul 2015 TAQ - Comment cleanup.
 *
 * Things to do
 *
 */

#include "geometry.h"
#include "../config.h"

Geometry::Geometry()
{
}

Geometry::Geometry(const Geometry& geo)
{
    radius = geo.radius;
}

Geometry::~Geometry()
{
}
