/* sector.cc
 *   by Trinity Quirk <trinity@ymb.net>
 *   last updated 04 Jul 2006, 11:17:35 trinity
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
 * Implementation of the sector.
 *
 * Changes
 *   12 Jul 2000 TAQ - Created the file.
 *   24 Jul 2000 TAQ - Finalized (Add|Move|Del)Object prototypes.
 *   03 Apr 2004 TAQ - Removed the massive memory leaking in the SetSpace
 *                     and DelSpace routines, now that the octree_delete
 *                     routine exists.
 *   04 Apr 2006 TAQ - Added namespace for std:: and Math3d:: objects.
 *   15 May 2006 TAQ - Added the GPL notice.
 *   04 Jul 2006 TAQ - Cleaned up changes to the nature type.
 *
 * Things to do
 *   - Allow putting things into the octree.
 *
 * $Id: sector.cc 10 2013-01-25 22:13:00Z trinity $
 */

#include "sector.h"

Sector::Sector()
{
}

Sector::~Sector()
{
}

void Sector::DoObjectCollisions(void) const
{
}
