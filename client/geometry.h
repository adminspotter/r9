/* geometry.h                                              -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 12 Jul 2014, 14:48:23 tquirk
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
 * This file contains the geometry cache class declaration.
 *
 * Changes
 *   12 Jul 2014 TAQ - Created the file.
 *
 * Things to do
 *   - This might almost work well as a base template for both geometries and
 *     textures.  We'd need to determine how to properly handle the domain-
 *     specific parsers.
 *
 */

#ifndef __INC_R9CLIENT_GEOMETRY_H__
#define __INC_R9CLIENT_GEOMETRY_H__

#include <time.h>
#include <pthread.h>
#include <GL.h>

#include <string>
#include <unordered_map>
#include <vector>

#include <xercesc/sax/HandlerBase.hpp>

typedef struct geometry_tag
{
    struct timeval lastused;
    GLuint disp_list;
}
geometry;

class GeometryCache
{
  private:
    /* SAX XML parser for reading geometry files */
#define XNS XERCES_CPP_NAMESPACE
    class FileParser : public XNS::HandlerBase
    {
      private:
        enum
        {
            start, end, geometry_st, geometry_en, frame_st, frame_en, sphere,
            polylist_st, polylist_en, polygon_st, polygon_en,
            point_st, point_en, vertex, normal,
        }
        current;
        XMLCh *geometry, *frame, *sphere, *polylist, *polygon, *point;
        XMLCh *vertex, *normal;
        XMLCh *version, *identifier, *count, *radius, *texture, *x, *y, *z,
        GeometryCache *parent;
        u_int64_t objid;
        u_int16_t frameid;
        geometry geom;
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
        FileParser(GeometryCache *);
        ~FileParser();

        void characters(const XMLCh *, const unsigned int);
        void startElement(const XMLCh *, XNS::AttributeList&);
        void endElement(const XMLCh *);
    };

  public:
    const int PRUNE_INTERVAL;
    const int PRUNE_LIFETIME;

    std::unordered_map<u_int64_t, std::vector<geometry> > geom;

  private:
    std::string store;
    std::string cache;
    pthread_t prune_thread;

    static void *prune_worker(void *);
    bool parse_file(std::string&);

  public:
    GeometryCache();
    ~GeometryCache();

    void load(u_int64_t);
    geometry *fetch(u_int64_t, u_int16_t);
};

#endif /* __INC_R9CLIENT_GEOMETRY_H__ */
