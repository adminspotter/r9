/* object.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 16 Aug 2015, 08:40:02 tquirk
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
 * This file contains some basic object-management routines.
 *
 * Things to do
 *   - Act sensibly in draw_objects, instead of brute-forcing it.
 *
 */

#include <config.h>

#include <functional>

#include "object.h"

extern void expose_main_view(void);

extern ObjectCache *obj;

struct draw_object
{
    void operator()(object& o)
        {
            glPushMatrix();
            glTranslated(o.position[0],
                         o.position[1],
                         o.position[2]);
            glRotated(o.orientation[0], 1.0, 0.0, 0.0);
            glRotated(o.orientation[1], 0.0, 1.0, 0.0);
            glRotated(o.orientation[2], 0.0, 0.0, 1.0);
            draw_geometry(o.geometry_id, o.frame_number);
            glPopMatrix();
        }
};

void draw_objects(void)
{
    /* Brute force, baby - ya just can't beat it */
    std::function<void(object&)> draw = draw_object();
    obj->each(draw);
}

void move_object(uint64_t objectid, uint16_t frame,
                 double xpos, double ypos, double zpos,
                 double xori, double yori, double zori)
{
    object& oref = (*obj)[objectid];

    /* Update the object's position */
    oref.frame_number = frame;
    oref.position[0] = xpos;
    oref.position[1] = ypos;
    oref.position[2] = zpos;
    oref.orientation[0] = xori;
    oref.orientation[1] = yori;
    oref.orientation[2] = zori;

    /* Post an expose for the screen */
    expose_main_view();
}
