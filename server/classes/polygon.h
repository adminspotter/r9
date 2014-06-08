/* polygon.h                                               -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 20 May 2014, 17:49:13 tquirk
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
 * This class comprises the polygon for the Revision 9 game server.  It used
 * to be a struct, but some of the computations didn't belong anywhere else.
 *
 * Changes
 *   21 Apr 2004 TAQ - Created the file.
 *   04 Apr 2006 TAQ - Changed the include for m3d.h.  Also using explicit
 *                     namespace, instead of "using namespace Math3d".  Also
 *                     added std:: namespace specifier for STL objects.
 *   06 Jul 2006 TAQ - Added the C++ tag at the top to get emacs to use the
 *                     right mode.
 *   20 May 2014 TAQ - Switched from Math3d to Eigen library.
 *
 * Things to do
 *   - Make sure this thing does everything we need it to do.  We might think
 *     about keeping point-normals, in addition to the main normal, for purty-
 *     picture purposes (client-side smoothing).
 *
 *   - See if we can make the actual data members private.
 *
 *   - Consider adding in the texture here, since an object can certainly have
 *     more than one texture, and that'll surely be divided up by polys.
 *     Consider the cleanest way to do this; maybe subobjects with a single
 *     texture?
 *
 * $Id$
 */

#ifndef __INC_POLYGON_H__
#define __INC_POLYGON_H__

#include <vector>
#include <Eigen/Core>

class polygon
{
  public:
    std::vector<Eigen::Vector3d> points;
    Eigen::Vector3d normal;
    double d;

    polygon();
    ~polygon();

    void calculate_normal(void);
};

#endif /* __INC_POLYGON_H__ */
