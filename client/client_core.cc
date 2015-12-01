/* client_core.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 01 Dec 2015, 06:56:11 tquirk
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
 * This file contains the caches and some general drawing routines.
 *
 * Things to do
 *   - Act sensibly in draw_objects, instead of brute-forcing it.
 *
 */

#include <config.h>

#include <functional>

#include "client_core.h"

#include "texture.h"
#include "geometry.h"
#include "object.h"

TextureCache *tex = NULL;
GeometryCache *geom = NULL;
ObjectCache *obj = NULL;

void init_client_core(void)
{
    tex = new TextureCache("texture");
    geom = new GeometryCache("geometry");
    obj = new ObjectCache("object");
}

void cleanup_client_core(void)
{
    if (tex != NULL)
    {
        delete tex;
        tex = NULL;
    }
    if (geom != NULL)
    {
        delete geom;
        geom = NULL;
    }
    if (obj != NULL)
    {
        delete obj;
        obj = NULL;
    }
}

void draw_texture(uint64_t texid)
{
    texture& t = (*tex)[texid];
    glMaterialfv(GL_FRONT, GL_DIFFUSE, t.diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, t.specular);
    glMaterialf(GL_FRONT, GL_SHININESS, t.shininess);
}

void draw_geometry(uint64_t geomid, uint16_t frame)
{
    glCallList((*geom)[geomid][frame]);
}

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
}
