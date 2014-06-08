/* defs.h                                                  -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 10 May 2014, 17:59:03 tquirk
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
 * Some basic definitions and typedefs for the server classes.
 *
 * Changes
 *   28 Oct 2000 TAQ - Created the file.
 *   11 Feb 2002 TAQ - The geometry is now a vector of polygons with some
 *                     color information.
 *   20 Feb 2002 TAQ - Moved geometry into a separate file.
 *   30 Jun 2002 TAQ - Added the sequence_element, which keeps track of
 *                     sequences of frames for object movement.
 *   30 Mar 2004 TAQ - The octree now contains *pointers* to polys, instead
 *                     of the actual polys themselves.  This will save memory
 *                     and also make things really easy to change.  Also,
 *                     we're using poly pointers instead of refs in the
 *                     contents deque.
 *   02 Apr 2004 TAQ - Added the neighbor array of pointers to the octree
 *                     node structure.  We're going to precompute the neighbors
 *                     because it could get expensive doing them on the fly.
 *   10 Apr 2004 TAQ - Added a min and max point for the octree.
 *   21 Apr 2004 TAQ - The polygon got too complicated, and has become its own
 *                     class, in its own file.
 *   04 Apr 2006 TAQ - Changed the include of m3d.h, because the Math3D
 *                     includes all do <math3d/fname.h>.  Added explicit
 *                     namespace specifiers instead of "using namespace ...".
 *   01 Jul 2006 TAQ - Fleshed out the action_level (changed name from the
 *                     bicapitalized ActionLevel) to include level, improvement
 *                     points, and last-level-increase timestamp; we're going
 *                     to only allow levelling only after a certain time, or
 *                     after a user has logged out for a certain time since
 *                     his previous levelling.  The index element of the
 *                     action_level is the index into the action table;
 *                     basically, which action it is.
 *   04 Jul 2006 TAQ - Added some syntactic BS to make this compile.
 *   06 Jul 2006 TAQ - Added the C++ tag at the top to get emacs to use the
 *                     right mode.
 *   12 Jul 2006 TAQ - Added a couple args to the action routine.
 *   11 Aug 2006 TAQ - Moved some stuff from zone.h.
 *   12 Aug 2006 TAQ - Got rid of the GO_id_t typedef.
 *   16 Aug 2006 TAQ - Changed the login_list to be a packet_list, since we're
 *                     going to use it for the sending queue as well.
 *   30 Jun 2007 TAQ - Updated to reflect Sockaddr naming changes.
 *   05 Sep 2007 TAQ - Removed the sockaddr.h include.  Changed the Sockaddr
 *                     object in the packet_list structure to a Control *.
 *                     Removed GameObject declaration.
 *   06 Sep 2007 TAQ - Changed who element of packet_list to void *.  Removed
 *                     Control declaration.
 *   08 Sep 2007 TAQ - Change the who element of packet list to u_int64_t.
 *   10 Sep 2007 TAQ - Added ugly, ugly access_list struct.
 *   12 Sep 2007 TAQ - Added basesock.h include.  Changed void *parent in
 *                     access struct to a basesock *.
 *   11 Oct 2007 TAQ - Totally reworked action into class action_rec.
 *   22 Nov 2009 TAQ - Added stdlib.h include to cover the free(3) call in
 *                     the action_rec destructor.
 *   29 Jun 2010 TAQ - Changed the orientation in the game_object_list_element
 *                     type to be a quaternion, in anticipation of using
 *                     quats for orientations.  Added look to the same
 *                     struct.
 *   10 May 2014 TAQ - Switched math libraries to Eigen.
 *
 * Things to do
 *   - Flesh out the attribute and nature as needed.
 *
 * $Id: defs.h 10 2013-01-25 22:13:00Z trinity $
 */

#ifndef __INC_DEFS_H__
#define __INC_DEFS_H__

#include <deque>
#include <Eigen/Core>
#include <Eigen/Geometry>

#include <stdlib.h>

#include "proto.h"
#include "basesock.h"

/* Eliminate the multiple-include problems */
class GameObject;
class Motion;
class polygon;

typedef struct octree_tag
{
    Eigen::Vector3d center_point, min_point, max_point;
    struct octree_tag *octants[8], *parent, *neighbor[6];
    char parent_index;
    std::deque<polygon *> contents;
}
octant, *octree;

typedef struct sequence_tag
{
    int frame_number, duration;
}
sequence_element;

typedef struct game_object_tag
{
    GameObject *obj;
    Motion *mot;
    Eigen::Vector3d position, look;
    Eigen::Quaterniond orientation;
}
game_object_list_element;

typedef struct packet_list_tag
{
    packet buf;
    u_int64_t who;
}
packet_list;

typedef struct access_list_tag
{
    packet buf;
    basesock *parent;
    union
    {
	struct
	{
	    union
	    {
		struct sockaddr_in dgram;
		struct
		{
		    int sub, sock;
		}
		stream;
	    }
	    who;
	}
	login;
	struct
	{
	    u_int64_t who;
	}
	logout;
    }
    what;
}
access_list;

typedef int (*ZoneControl)(int);
class action_rec
{
  public:
    char *name;
    void (*action)(Motion *, int, Motion *, Eigen::Vector3d &);
    u_int16_t def;                /* The "default" skill to use */
    int lower, upper;             /* The bounds for skill levels */
    bool valid;                   /* Is this action valid on this server? */

    inline action_rec()
	{
	    this->name = NULL;
	    this->action = NULL;
	    this->def = 0;
	    this->lower = 0;
	    this->upper = 0;
	    this->valid = false;
	};
    inline ~action_rec()
	{
	    if (this->name != NULL)
		free(this->name);
	};
};
typedef int attribute;
typedef int nature;
typedef struct action_level_tag
{
    u_int16_t index, level, improvement;
    time_t last_level;
}
action_level;

#endif /* __INC_DEFS_H__ */
