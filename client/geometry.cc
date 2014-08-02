/* geometry.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 29 Jul 2014, 18:21:09 tquirk
 *
 * Revision IX game client
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
 * Changes
 *   03 Aug 2006 TAQ - Created the file.
 *   04 Aug 2006 TAQ - Wrote the hash table - man, those things are really
 *                     simple to implement.  Wrote geometry file reading
 *                     and writing.
 *   05 Aug 2006 TAQ - Added the timestamp, which is set during the
 *                     find_geometry routine.  Makes some amount of sense
 *                     that if we're finding a geometry, we're going to
 *                     use it.  Or something like that.  Added the pruning
 *                     thread.
 *   09 Aug 2006 TAQ - Completed update_geometry.  Now notify users of
 *                     entries removed from hash table.  We no longer save
 *                     geometry files here, since they're no longer part of
 *                     the protocol.  Removed the data element in the
 *                     geometry structure.
 *   11 Jul 2014 TAQ - This is now a C++ file, and will be based on a
 *                     std::unordered_map.  Writing our own hash table is
 *                     nice, but unnecessary.  We've also got an XML file
 *                     parser as a private class.
 *   12 Jul 2014 TAQ - We want to use hex numbers in our file names/paths,
 *                     and they're XML files, so use the .xml extension.
 *   13 Jul 2014 TAQ - We were leaking lots of display lists, in both the
 *                     reaper and in the destructor.  Fixed.
 *   20 Jul 2014 TAQ - We're now using the templated object cache, instead
 *                     of having geometry and texture caches which are
 *                     basically the same thing.
 *   23 Jul 2014 TAQ - Include cleanups.
 *
 * Things to do
 *   - The parser is a massive race condition - we're constructing
 *     display lists as we see the elements.  The regular display loop
 *     could be operating at the same time, and we'd get a bunch of
 *     stuff we don't expect in our list.  A mutex may be necessary,
 *     to lock out the regular display loop while we're parsing.
 *   - Oh, so it turns out that using display lists is massively
 *     deprecated or no longer supported at all, so I guess we'll have
 *     to figure out how to use the new-style vertex stuff.
 *
 */

#include <glut.h>

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
            if (XNS::XMLString::compareIString(attrs.getName(i),
                                               this->version))
            {
                /* We don't care about the version number yet */
            }
            else if (XNS::XMLString::compareIString(attrs.getName(i),
                                                    this->identifier))
            {
                /* We already know the objectid
                str = XNS::XMLString::transcode(attrs.getValue(i));
                this->objid = strtoll(str, NULL, 10);
                XNS::XMLString::release(&str);*/
            }
            else if (XNS::XMLString::compareIString(attrs.getName(i),
                                                    this->count))
            {
                str = XNS::XMLString::transcode(attrs.getValue(i));
                count = atoi(str);
                XNS::XMLString::release(&str);
            }
            else
            {
                /* We don't recognize this attribute */
                std::ostringstream s;
                str = XNS::XMLString::transcode(attrs.getName(i));
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
    u_int64_t tid;
    GLfloat radius;

    if (this->current == frame_st)
    {
        this->current = sphere_st;
        for (i = 0; i < attrs.getLength(); ++i)
        {
            if (XNS::XMLString::compareIString(attrs.getName(i), this->texture))
            {
                str = XNS::XMLString::transcode(attrs.getValue(i));
                tid = strtoll(str, NULL, 10);
                XNS::XMLString::release(&str);
            }
            else if (XNS::XMLString::compareIString(attrs.getName(i),
                                                    this->radius))
            {
                str = XNS::XMLString::transcode(attrs.getValue(i));
                radius = atof(str);
                XNS::XMLString::release(&str);
            }
            else
            {
                /* We don't recognize this attribute */
                std::ostringstream s;
                str = XNS::XMLString::transcode(attrs.getName(i));
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
    u_int64_t tid;

    if (this->current == polylist_st || this->current == polygon_en)
    {
        this->current = polygon_st;
        for (i = 0; i < attrs.getLength(); ++i)
        {
            if (XNS::XMLString::compareIString(attrs.getName(i), this->texture))
            {
                str = XNS::XMLString::transcode(attrs.getValue(i));
                tid = strtoll(str, NULL, 10);
                XNS::XMLString::release(&str);
            }
            else
            {
                /* We don't recognize this attribute */
                std::ostringstream s;
                str = XNS::XMLString::transcode(attrs.getName(i));
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
            if (XNS::XMLString::compareIString(attrs.getName(i), this->x))
            {
                str = XNS::XMLString::transcode(attrs.getValue(i));
                this->pt[0] = atof(str);
                XNS::XMLString::release(&str);
            }
            else if (XNS::XMLString::compareIString(attrs.getName(i), this->y))
            {
                str = XNS::XMLString::transcode(attrs.getValue(i));
                this->pt[1] = atof(str);
                XNS::XMLString::release(&str);
            }
            else if (XNS::XMLString::compareIString(attrs.getName(i), this->z))
            {
                str = XNS::XMLString::transcode(attrs.getValue(i));
                this->pt[2] = atof(str);
                XNS::XMLString::release(&str);
            }
            else
            {
                /* We don't recognize this attribute */
                std::ostringstream s;
                str = XNS::XMLString::transcode(attrs.getName(i));
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
            if (XNS::XMLString::compareIString(attrs.getName(i), this->x))
            {
                str = XNS::XMLString::transcode(attrs.getValue(i));
                this->norm[0] = atof(str);
                XNS::XMLString::release(&str);
            }
            else if (XNS::XMLString::compareIString(attrs.getName(i), this->y))
            {
                str = XNS::XMLString::transcode(attrs.getValue(i));
                this->norm[1] = atof(str);
                XNS::XMLString::release(&str);
            }
            else if (XNS::XMLString::compareIString(attrs.getName(i), this->z))
            {
                str = XNS::XMLString::transcode(attrs.getValue(i));
                this->norm[2] = atof(str);
                XNS::XMLString::release(&str);
            }
            else
            {
                /* We don't recognize this attribute */
                std::ostringstream s;
                str = XNS::XMLString::transcode(attrs.getName(i));
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

    /* Set our parser strings, so we don't have to keep transcoding */
    this->geometry_str = XNS::XMLString::transcode("geometry");
    this->frame = XNS::XMLString::transcode("frame");
    this->sphere = XNS::XMLString::transcode("sphere");
    this->polylist = XNS::XMLString::transcode("polylist");
    this->polygon = XNS::XMLString::transcode("polygon");
    this->point = XNS::XMLString::transcode("point");
    this->vertex = XNS::XMLString::transcode("vertex");
    this->normal = XNS::XMLString::transcode("normal");
    this->version = XNS::XMLString::transcode("version");
    this->identifier = XNS::XMLString::transcode("identifier");
    this->count = XNS::XMLString::transcode("count");
    this->radius = XNS::XMLString::transcode("radius");
    this->texture = XNS::XMLString::transcode("texture");
    this->x = XNS::XMLString::transcode("x");
    this->y = XNS::XMLString::transcode("y");
    this->z = XNS::XMLString::transcode("z");
}

GeometryParser::~GeometryParser()
{
    XNS::XMLString::release(&(this->geometry_str));
    XNS::XMLString::release(&(this->frame));
    XNS::XMLString::release(&(this->sphere));
    XNS::XMLString::release(&(this->polylist));
    XNS::XMLString::release(&(this->polygon));
    XNS::XMLString::release(&(this->point));
    XNS::XMLString::release(&(this->vertex));
    XNS::XMLString::release(&(this->normal));
    XNS::XMLString::release(&(this->version));
    XNS::XMLString::release(&(this->identifier));
    XNS::XMLString::release(&(this->count));
    XNS::XMLString::release(&(this->radius));
    XNS::XMLString::release(&(this->texture));
    XNS::XMLString::release(&(this->x));
    XNS::XMLString::release(&(this->y));
    XNS::XMLString::release(&(this->z));
}

void GeometryParser::startElement(const XMLCh *name,
                                             XNS::AttributeList& attrs)
{
    if (XNS::XMLString::compareIString(name, this->geometry_str))
        this->open_geometry(attrs);
    else if (XNS::XMLString::compareIString(name, this->frame))
        this->open_frame(attrs);
    else if (XNS::XMLString::compareIString(name, this->sphere))
        this->open_sphere(attrs);
    else if (XNS::XMLString::compareIString(name, this->polylist))
        this->open_polylist(attrs);
    else if (XNS::XMLString::compareIString(name, this->polygon))
        this->open_polygon(attrs);
    else if (XNS::XMLString::compareIString(name, this->point))
        this->open_point(attrs);
    else if (XNS::XMLString::compareIString(name, this->vertex))
        this->open_vertex(attrs);
    else if (XNS::XMLString::compareIString(name, this->normal))
        this->open_normal(attrs);
    else
    {
        /* We don't recognize whatever it is that we just got */
        std::ostringstream s;
        char *tag = XNS::XMLString::transcode(name);
        s << "Bad open tag: \"" << tag << '"';
        XNS::XMLString::release(&tag);
        throw std::runtime_error(s.str());
    }
}

void GeometryParser::endElement(const XMLCh *name)
{
    if (XNS::XMLString::compareIString(name, this->geometry_str))
        this->close_geometry();
    else if (XNS::XMLString::compareIString(name, this->frame))
        this->close_frame();
    else if (XNS::XMLString::compareIString(name, this->polylist))
        this->close_polylist();
    else if (XNS::XMLString::compareIString(name, this->polygon))
        this->close_polygon();
    else if (XNS::XMLString::compareIString(name, this->point))
        this->close_point();
    else
    {
        /* We don't recognize whatever it is that we just got */
        std::ostringstream s;
        char *tag = XNS::XMLString::transcode(name);
        s << "Bad close tag: \"" << tag << '"';
        XNS::XMLString::release(&tag);
        throw std::runtime_error(s.str());
    }
}
