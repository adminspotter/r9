/* geometry.h                                              -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 24 Jul 2015, 13:29:15 tquirk
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
 * This file contains the geometry representation of a game object (or any
 * other kind of object) for the game system.
 *
 * Changes
 *   11 Feb 2002 TAQ - Created the file.
 *   19 Feb 2002 TAQ - Added addPoly and delPoly.
 *   20 Feb 2002 TAQ - Changed delPoly to delPolys.
 *   15 Mar 2002 TAQ - Started in with simplistic bounding volumes.
 *   09 Apr 2002 TAQ - Removed addPolys prototype.
 *   24 Jun 2002 TAQ - We're now basically a collection of RAPID_models.
 *                     We may have to tweak the RAPID objects to include
 *                     things like color or texture.
 *   30 Jun 2002 TAQ - Started adding frame sequences.
 *   02 Aug 2003 TAQ - Needed to include RAPID.H.
 *   04 Apr 2006 TAQ - Added std:: namespace specifiers to STL objects.
 *   15 May 2006 TAQ - Added the GPL notice.
 *   27 Jun 2006 TAQ - We're going to not implement most of this class
 *                     for right now, just because it's going to be complex
 *                     and probably take a while.  We'll just be a sphere of
 *                     a given size.
 *   06 Jul 2006 TAQ - Added the C++ tag at the top to get emacs to use the
 *                     right mode.
 *   09 Jul 2006 TAQ - Made some members public, for simplicity.  Deleted
 *                     radius get/set.
 *   11 Aug 2006 TAQ - Removed superfluous, and likely will-never-be-used,
 *                     geometry things.
 *   10 May 2014 TAQ - Switched to Eigen math library.
 *   24 Jul 2015 TAQ - Comment cleanup.
 *
 * Things to do
 *   - Consider how we want to represent our sequences.
 *   - Consider how we want to represent our bounding volumes.  From the
 *   looks of things, we're going to attempt a two-level bounding operation:
 *   first level is a bounding sphere, since it's easy and small, and the
 *   second level will be a bounding box, or set of boxes, which is much
 *   tighter than the initial sphere.
 *
 */

#ifndef __INC_GEOMETRY_H__
#define __INC_GEOMETRY_H__

#include <Eigen/Core>

class Geometry
{
  public:
    //std::vector< std::vector<sequence_element> > sequences;
    Eigen::Vector3d center;
    double radius;

  public:
    Geometry();
    Geometry(const Geometry &);
    ~Geometry();
};

#endif /* __INC_GEOMETRY_H__ */
