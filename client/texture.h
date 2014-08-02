/* texture.h                                               -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 01 Aug 2014, 18:30:36 tquirk
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
 * This file contains the texture cache class declaration.
 *
 * Changes
 *   12 Jul 2014 TAQ - Created the file.
 *   20 Jul 2014 TAQ - We're now using the templated version of the cache.
 *   23 Jul 2014 TAQ - The parser now has a pointer to the object, rather
 *                     than an object that we insert into the cache.
 *
 * Things to do
 *
 */

#ifndef __INC_R9CLIENT_TEXTURE_H__
#define __INC_R9CLIENT_TEXTURE_H__

#include <gl.h>

#include <xercesc/sax/AttributeList.hpp>

#include "cache.h"

struct texture
{
    GLfloat diffuse[4], specular[4], shininess;
    /* Some sort of texture map in here too */
};

#define XNS XERCES_CPP_NAMESPACE
class TextureParser : public XNS::HandlerBase
{
  private:
    enum
    {
        start, end, texture_st, texture_en, diffuse_st, diffuse_en,
        specular_st, specular_en, shininess_st, rgba_st, mapfile_st,
    }
    current;
    XMLCh *texture_str, *diffuse, *specular, *shininess, *rgba, *mapfile;
    XMLCh *version, *identifier, *value, *r, *g, *b, *a, *filename;
    texture *tex;
    GLfloat *rgba_ptr;

    void open_texture(XNS::AttributeList&);
    void close_texture(void);
    void open_diffuse(XNS::AttributeList&);
    void close_diffuse(void);
    void open_specular(XNS::AttributeList&);
    void close_specular(void);
    void open_shininess(XNS::AttributeList&);
    void open_rgba(XNS::AttributeList&);
    void open_mapfile(XNS::AttributeList&);

  public:
    TextureParser(texture *);
    ~TextureParser();

    void startElement(const XMLCh *, XNS::AttributeList&);
    void endElement(const XMLCh *);
};

typedef ObjectCache<texture, TextureParser> TextureCache;

#endif /* __INC_R9CLIENT_TEXTURE_H__ */
