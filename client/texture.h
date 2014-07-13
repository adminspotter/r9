/* texture.h                                               -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 13 Jul 2014, 10:53:41 tquirk
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
 *
 * Things to do
 *
 */

#ifndef __INC_R9CLIENT_TEXTURE_H__
#define __INC_R9CLIENT_TEXTURE_H__

#include <time.h>
#include <pthread.h>
#include <GL.h>

#include <string>
#include <unordered_map>
#include <vector>

#include <xercesc/sax/HandlerBase.hpp>

typedef struct texture_tag
{
    struct timeval lastused;
    GLfloat diffuse[4], specular[4], shininess;
    /* Some sort of texture map in here too */
}
texture;

class TextureCache
{
  private:
#define XNS XERCES_CPP_NAMESPACE
    class FileParser : public XNS::HandlerBase
    {
      private:
        enum
        {
            start, end, texture_st, texture_en, diffuse_st, diffuse_en,
            specular_st, specular_en, shininess, rgba, mapfile,
        }
        current;
        XMLCh *texture, *diffuse, *specular, *shininess, *rgba, *mapfile;
        XMLCh *version, *identifier, *value, *r, *g, *b, *a, *filename;
        u_int64_t texid;
        texture tex;
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
        FileParser(TextureCache *);
        ~FileParser();

        void characters(const XMLCh *, const unsigned int);
        void startElement(const XMLCh *, XNS::AttributeList&);
        void endElement(const XMLCh *);
    };

  public:
    const int PRUNE_INTERVAL;
    const int PRUNE_LIFETIME;

    std::unordered_map<u_int64_t, texture> tex;

  private:
    std::string store;
    std::string cache;
    pthread_t prune_thread;

    static void *prune_worker(void *);
    bool parse_file(std::string&);

  public:
    TextureCache();
    ~TextureCache();

    void load(u_int64_t);
    texture *fetch(u_int64_t);
};

#endif /* __INC_R9CLIENT_TEXTURE_H__ */
