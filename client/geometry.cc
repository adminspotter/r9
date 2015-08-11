/* geometry.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 10 Aug 2015, 19:01:26 tquirk
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
 * This file contains the geometry management object for the Revision IX
 * client program.
 *
 * We will save the geometries and textures in directory structures under
 * the user's config directory ($HOME/.revision9/).  Since we will possibly
 * have a HUGE number of geometries and textures, we'll split things up
 * based on the last two digits of the ID.  Each geometry file will contain
 * all the frames for that particular object.
 *
 * Once we load or retrieve the geometry, we'll make a display list out of
 * it, and only retain the display list - we don't need to keep all that
 * geometry stuff around in main memory, when we already have it encoded in
 * the display list.
 *
 * It's probably necessary, for the sake of not exhausting OpenGL's supply
 * of display lists, that we keep the geometries separate from the objects,
 * since many objects can use the same geometry, and it'd be a waste to
 * have a bunch of display lists which contain the same thing.
 *
 * We'll keep the prefixes around so we don't have to keep recreating
 * them.  The store is the system-wide geometry repository (in
 * /usr/share/revision9/geometry), where the cache is the user's
 * personal repository (in $HOME/.revision9/geometry).  First we'll
 * look in the cache, then if we don't find what we need, we'll
 * look in the store.  If we *still* don't find it, we'll send out a
 * server request.
 *
 * Things to do
 *
 */

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#include <cstdint>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <stdexcept>

#include "geometry.h"

void GeometryParser::open_geometry(XNS::AttributeList& attrs)
{
    int i, count = 0;
    char *str;

    if (this->current == start)
    {
        this->current = geometry_st;
        for (i = 0; i < attrs.getLength(); ++i)
        {
            str = XNS::XMLString::transcode(attrs.getName(i));
            if (!strcmp(str, "version"))
            {
                XNS::XMLString::release(&str);
                /* We don't care about the version number yet */
            }
            else if (!strcmp(str, "identifier"))
            {
                XNS::XMLString::release(&str);
                /* We already know the objectid
                str = XNS::XMLString::transcode(attrs.getValue(i));
                this->objid = strtoll(str, NULL, 10);
                XNS::XMLString::release(&str);*/
            }
            else if (!strcmp(str, "count"))
            {
                XNS::XMLString::release(&str);
                str = XNS::XMLString::transcode(attrs.getValue(i));
                count = atoi(str);
                XNS::XMLString::release(&str);
            }
            else
            {
                /* We don't recognize this attribute */
                std::ostringstream s;
                s << "Bad geometry attribute \"" << str << '"';
                XNS::XMLString::release(&str);
                throw std::runtime_error(s.str());
            }
        }
        if (count > 0)
            this->geom->reserve(count);
    }
    else
        throw std::runtime_error("Bad geometry open tag");
}

void GeometryParser::close_geometry(void)
{
    if (this->current == frame_en)
        this->current = geometry_en;
    else
        throw std::runtime_error("Bad geometry close tag");
}

void GeometryParser::open_frame(XNS::AttributeList& attrs)
{
    if (this->current == geometry_st)
    {
        this->current = frame_st;
        this->geom->push_back(glGenLists(1));
        /* This would be the time to grab a mutex */
        glNewList(this->geom->back(), GL_COMPILE);
    }
    else
        throw std::runtime_error("Bad frame open tag");
}

void GeometryParser::close_frame(void)
{
    if (this->current == polylist_en || this->current == sphere_st)
    {
        this->current = frame_en;
        glEndList();
        /* This would be the time to drop a mutex */
    }
    else
        throw std::runtime_error("Bad frame close tag");
}

void GeometryParser::open_sphere(XNS::AttributeList& attrs)
{
    int i;
    char *str;
    uint64_t tid;
    GLfloat radius;

    if (this->current == frame_st)
    {
        this->current = sphere_st;
        for (i = 0; i < attrs.getLength(); ++i)
        {
            str = XNS::XMLString::transcode(attrs.getName(i));
            if (!strcmp(str, "texture"))
            {
                XNS::XMLString::release(&str);
                str = XNS::XMLString::transcode(attrs.getValue(i));
                tid = strtoll(str, NULL, 10);
                XNS::XMLString::release(&str);
            }
            else if (!strcmp(str, "radius"))
            {
                XNS::XMLString::release(&str);
                str = XNS::XMLString::transcode(attrs.getValue(i));
                radius = atof(str);
                XNS::XMLString::release(&str);
            }
            else
            {
                /* We don't recognize this attribute */
                std::ostringstream s;
                s << "Bad sphere attribute \"" << str << '"';
                XNS::XMLString::release(&str);
                throw std::runtime_error(s.str());
            }
        }
        draw_texture(tid);
        glutSolidSphere(radius, 4, 4);
    }
    else
        throw std::runtime_error("Bad sphere tag");
}

void GeometryParser::open_polylist(XNS::AttributeList& attrs)
{
    if (this->current == frame_st)
        this->current = polylist_st;
    else
        throw std::runtime_error("Bad polylist open tag");
}

void GeometryParser::close_polylist(void)
{
    if (this->current == polygon_en)
        this->current = polylist_en;
    else
        throw std::runtime_error("Bad polylist close tag");
}

void GeometryParser::open_polygon(XNS::AttributeList& attrs)
{
    int i;
    char *str;
    uint64_t tid;

    if (this->current == polylist_st || this->current == polygon_en)
    {
        this->current = polygon_st;
        for (i = 0; i < attrs.getLength(); ++i)
        {
            str = XNS::XMLString::transcode(attrs.getName(i));
            if (!strcmp(str, "texture"))
            {
                XNS::XMLString::release(&str);
                str = XNS::XMLString::transcode(attrs.getValue(i));
                tid = strtoll(str, NULL, 10);
                XNS::XMLString::release(&str);
            }
            else
            {
                /* We don't recognize this attribute */
                std::ostringstream s;
                s << "Bad polygon attribute \"" << str << '"';
                XNS::XMLString::release(&str);
                throw std::runtime_error(s.str());
            }
        }
        glBegin(GL_POLYGON);
        draw_texture(tid);
    }
    else
        throw std::runtime_error("Bad polygon open tag");
}

void GeometryParser::close_polygon(void)
{
    if (this->current == point_en)
    {
        this->current = polygon_en;
        glEnd();
    }
    else
        throw std::runtime_error("Bad polygon close tag");
}

void GeometryParser::open_point(XNS::AttributeList& attrs)
{
    if (this->current == polygon_st)
    {
        this->current = point_st;
        this->pt[0] = this->pt[1] = this->pt[2] = 0.0;
        this->norm[0] = this->norm[1] = this->norm[2] = 0.0;
    }
    else
        throw std::runtime_error("Bad point open tag");
}

void GeometryParser::close_point(void)
{
    if (this->current == normal_st)
    {
        this->current = point_en;
        glNormal3fv(this->norm);
        glVertex3fv(this->pt);
    }
    else
        throw std::runtime_error("Bad point close tag");
}

void GeometryParser::open_vertex(XNS::AttributeList& attrs)
{
    int i;
    char *str;

    if (this->current == point_st)
    {
        this->current = vertex_st;
        for (i = 0; i < attrs.getLength(); ++i)
        {
            str = XNS::XMLString::transcode(attrs.getName(i));
            if (!strcmp(str, "x"))
            {
                XNS::XMLString::release(&str);
                str = XNS::XMLString::transcode(attrs.getValue(i));
                this->pt[0] = atof(str);
                XNS::XMLString::release(&str);
            }
            else if (!strcmp(str, "y"))
            {
                XNS::XMLString::release(&str);
                str = XNS::XMLString::transcode(attrs.getValue(i));
                this->pt[1] = atof(str);
                XNS::XMLString::release(&str);
            }
            else if (!strcmp(str, "z"))
            {
                XNS::XMLString::release(&str);
                str = XNS::XMLString::transcode(attrs.getValue(i));
                this->pt[2] = atof(str);
                XNS::XMLString::release(&str);
            }
            else
            {
                /* We don't recognize this attribute */
                std::ostringstream s;
                s << "Bad vertex attribute \"" << str << '"';
                XNS::XMLString::release(&str);
                throw std::runtime_error(s.str());
            }
        }
    }
    else
        throw std::runtime_error("Bad vertex tag");
}

void GeometryParser::open_normal(XNS::AttributeList& attrs)
{
    int i;
    char *str;

    if (this->current == vertex_st)
    {
        this->current = normal_st;
        for (i = 0; i < attrs.getLength(); ++i)
        {
            str = XNS::XMLString::transcode(attrs.getName(i));
            if (!strcmp(str, "x"))
            {
                XNS::XMLString::release(&str);
                str = XNS::XMLString::transcode(attrs.getValue(i));
                this->norm[0] = atof(str);
                XNS::XMLString::release(&str);
            }
            else if (!strcmp(str, "y"))
            {
                XNS::XMLString::release(&str);
                str = XNS::XMLString::transcode(attrs.getValue(i));
                this->norm[1] = atof(str);
                XNS::XMLString::release(&str);
            }
            else if (!strcmp(str, "z"))
            {
                XNS::XMLString::release(&str);
                str = XNS::XMLString::transcode(attrs.getValue(i));
                this->norm[2] = atof(str);
                XNS::XMLString::release(&str);
            }
            else
            {
                /* We don't recognize this attribute */
                std::ostringstream s;
                s << "Bad normal attribute \"" << str << '"';
                XNS::XMLString::release(&str);
                throw std::runtime_error(s.str());
            }
        }
    }
    else
        throw std::runtime_error("Bad normal tag");
}

GeometryParser::GeometryParser(geometry *elem)
{
    this->geom = elem;
    this->current = start;
}

GeometryParser::~GeometryParser()
{
}

void GeometryParser::startElement(const XMLCh *name,
                                  XNS::AttributeList& attrs)
{
    char *str = XNS::XMLString::transcode(name);

    if (!strcmp(str, "geometry"))
        this->open_geometry(attrs);
    else if (!strcmp(str, "frame"))
        this->open_frame(attrs);
    else if (!strcmp(str, "sphere"))
        this->open_sphere(attrs);
    else if (!strcmp(str, "polylist"))
        this->open_polylist(attrs);
    else if (!strcmp(str, "polygon"))
        this->open_polygon(attrs);
    else if (!strcmp(str, "point"))
        this->open_point(attrs);
    else if (!strcmp(str, "vertex"))
        this->open_vertex(attrs);
    else if (!strcmp(str, "normal"))
        this->open_normal(attrs);
    else
    {
        /* We don't recognize whatever it is that we just got */
        std::ostringstream s;
        s << "Bad open tag: \"" << str << '"';
        XNS::XMLString::release(&str);
        throw std::runtime_error(s.str());
    }
    XNS::XMLString::release(&str);
}

void GeometryParser::endElement(const XMLCh *name)
{
    char *str = XNS::XMLString::transcode(name);

    if (!strcmp(str, "geometry"))
        this->close_geometry();
    else if (!strcmp(str, "frame"))
        this->close_frame();
    else if (!strcmp(str, "polylist"))
        this->close_polylist();
    else if (!strcmp(str, "polygon"))
        this->close_polygon();
    else if (!strcmp(str, "point"))
        this->close_point();
    else if (!strcmp(str, "sphere")
             || !strcmp(str, "vertex")
             || !strcmp(str, "normal"))
        ; /* Empty tag */
    else
    {
        /* We don't recognize whatever it is that we just got */
        std::ostringstream s;
        s << "Bad close tag: \"" << str << '"';
        XNS::XMLString::release(&str);
        throw std::runtime_error(s.str());
    }
    XNS::XMLString::release(&str);
}
