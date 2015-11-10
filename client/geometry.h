/* geometry.h                                              -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 09 Nov 2015, 18:52:51 tquirk
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
 * This file contains the geometry cache class declaration.
 *
 * Things to do
 *
 */

#ifndef __INC_R9CLIENT_GEOMETRY_H__
#define __INC_R9CLIENT_GEOMETRY_H__

#include <config.h>

#include <GL/gl.h>

#if HAVE_XERCESC_SAX_ATTRIBUTELIST_HPP
#include <xercesc/sax/AttributeList.hpp>
#endif /* HAVE_XERCESC_SAX_ATTRIBUTELIST_HPP */
#define XNS XERCES_CPP_NAMESPACE
#include "dtdresolver.h"

#include "cache.h"

typedef std::vector<GLuint> geometry;

class GeometryParser : public XNS::HandlerBase, public R9Resolver
{
  private:
    enum
    {
        start, end, geometry_st, geometry_en, frame_st, frame_en, sphere_st,
        polylist_st, polylist_en, polygon_st, polygon_en,
        point_st, point_en, vertex_st, normal_st,
    }
    current;
    geometry *geom;
    GLfloat pt[3], norm[3];

    void open_geometry(XNS::AttributeList&);
    void close_geometry(void);
    void open_frame(XNS::AttributeList&);
    void close_frame(void);
    void open_sphere(XNS::AttributeList&);
    void open_polylist(XNS::AttributeList&);
    void close_polylist(void);
    void open_polygon(XNS::AttributeList&);
    void close_polygon(void);
    void open_point(XNS::AttributeList&);
    void close_point(void);
    void open_vertex(XNS::AttributeList&);
    void open_normal(XNS::AttributeList&);

  public:
    GeometryParser(geometry *);
    ~GeometryParser();

    void startElement(const XMLCh *, XNS::AttributeList&);
    void endElement(const XMLCh *);
};

struct geom_cleanup
{
    void operator()(geometry& g)
        {
            for (geometry::iterator i = g.begin(); i != g.end(); ++i)
                glDeleteLists(*i, 1);
        }
};

typedef ParsedCache<geometry, GeometryParser, geom_cleanup> GeometryCache;

#endif /* __INC_R9CLIENT_GEOMETRY_H__ */
