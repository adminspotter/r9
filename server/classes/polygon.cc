/* polygon.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 20 May 2014, 18:18:40 tquirk
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
 * This file contains the polygon implementation.  Not too much in here now
 * beside the normal calculation.
 *
 * Changes
 *   21 Apr 2004 TAQ - Created the file.
 *   10 May 2006 TAQ - Tweaked with the calculation of d.
 *   20 May 2014 TAQ - Switched from Math3d to Eigen vector library.
 *
 * Things to do
 *
 * $Id$
 */

#include "polygon.h"

polygon::polygon()
{
    normal << 0, 0, 0;
}

polygon::~polygon()
{
}

void polygon::calculate_normal(void)
{
    std::vector<Eigen::Vector3d>::iterator i;

    /* 3 points define a plane, and so we can only compute a normal with 3
     * or more points.
     */
    if (points.size() >= 3)
    {
        normal << 0, 0, 0;
        for (i = points.begin(); i != points.end(); ++i)
            normal += *i;
        normal /= points.size();
	d = (-points[0]).dot(normal);
    }
}
