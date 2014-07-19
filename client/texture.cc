/* texture.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 13 Jul 2014, 11:10:35 tquirk
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
 * This file contains the texture management routines for the Revision IX
 * client program.
 *
 * We will save the geometries and textures in directory structures under
 * the user's config directory ($HOME/.revision9/).  Since we will possibly
 * have a HUGE number of geometries and textures, we'll split things up
 * based on the last digit or two of the ID.  To start, we'll do the last
 * two numbers, and then inside each of those directories will be the texture
 * files.  Each texture can have two files:  the texture data itself (text),
 * and a possible texture map (a graphic file of some format, probably XPM).
 *
 * Let's keep the prefixes around so we don't have to keep recreating them.
 * The texture_prefix is the system-wide texture repository (in
 * /usr/share/revision9/texture), where the texture_cache is the user's
 * personal repository (in $HOME/.revision9/texture).  First we'll look in
 * the user's cache, then if we don't find what we need, we'll look in the
 * store.  If we *still* don't find it, we'll send out a server request.
 *
 * Changes
 *   03 Aug 2006 TAQ - Created the file.
 *   04 Aug 2006 TAQ - Wrote the hash table.
 *   05 Aug 2006 TAQ - Added time lastused timestamp to the struct, which is
 *                     set during the call to find_texture.  Started work on
 *                     the hash pruning thread.  Removed the delete_texture
 *                     routine, since it was redundant with the pruning thread.
 *   09 Aug 2006 TAQ - Fallback texture is now 18% gray.  Now notify user
 *                     of entries removed from the hash table.
 *   12 Jul 2014 TAQ - Moved over to C++, and copied most of texture.cc
 *                     into here.  I don't think a template class would work,
 *                     or a generic-ish base class, so we'll just have two
 *                     parallel implementations for the time being.
 *
 * Things to do
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>

#include <sstream>
#include <iomanip>
#include <stdexcept>

#include "texture.h"
#include "client.h"

void TextureCache::FileParser::open_texture(XNS::AttributeList& attrs)
{
    int i;
    char *str;

    if (this->current == start)
    {
        this->current = texture_en;
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
                str = XNS::XMLString::transcode(attrs.getValue(i));
                this->texid = strtoll(str, NULL, 10);
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
    }
    else
        throw std::runtime_error("Bad texture open tag");
}

void TextureCache::FileParser::close_texture(void)
{
    if (this->current == shininess || this->current == mapfile)
    {
        this->current = texture_en;
        parent->tex[this->texid] = this->tex;
    }
    else
        throw std::runtime_error("Bad texture close tag");
}

void TextureCache::FileParser::open_diffuse(XNS::AttributeList& attrs)
{
    if (this->current == texture_st)
    {
        this->current = diffuse_st;
        this->rgba_ptr = this->tex.diffuse;
    }
    else
        throw std::runtime_error("Bad diffuse open tag");
}

void TextureCache::FileParser::close_diffuse(void)
{
    if (this->current == rgba)
        this->current = diffuse_en;
    else
        throw std::runtime_error("Bad diffuse close tag");
}

void TextureCache::FileParser::open_specular(XNS::AttributeList& attrs)
{
    if (this->current == diffuse_en)
    {
        this->current = specular_st;
        this->rgba_ptr = this->tex.specular;
    }
    else
        throw std::runtime_error("Bad specular open tag");
}

void TextureCache::FileParser::close_specular(void)
{
    if (this->current == rgba)
        this->current = specular_en;
    else
        throw std::runtime_error("Bad specular close tag");
}

void TextureCache::FileParser::open_shininess(XNS::AttributeList& attrs)
{
    int i;
    char *str;

    if (this->current == specular_en)
    {
        this->current = shininess;
        for (i = 0; i < attrs.getLength(); ++i)
        {
            if (XNS::XMLString::compareIString(attrs.getName(i), this->value))
            {
                str = XNS::XMLString::transcode(attrs.getValue(i));
                this->tex.shininess = atof(str);
                XNS::XMLString::release(&str);
            }
            else
            {
                /* We don't recognize this attribute */
                std::ostringstream s;
                str = XNS::XMLString::transcode(attrs.getName(i));
                s << "Bad shininess attribute \"" << str << '"';
                XNS::XMLString::release(&str);
                throw std::runtime_error(s.str());
            }
        }
    }
    else
        throw std::runtime_error("Bad shininess tag");
}

void TextureCache::FileParser::open_rgba(XNS::AttributeList& attrs)
{
    int i;
    char *str;

    if (this->current == diffuse_st || this->current == specular_st)
    {
        this->current = rgba;
        for (i = 0; i < attrs.getLength(); ++i)
        {
            if (XNS::XMLString::compareIString(attrs.getName(i), this->r))
            {
                str = XNS::XMLString::transcode(attrs.getValue(i));
                this->rgba_ptr[0] = atof(str);
                XNS::XMLString::release(&str);
            }
            else if (XNS::XMLString::compareIString(attrs.getName(i), this->g))
            {
                str = XNS::XMLString::transcode(attrs.getValue(i));
                this->rgba_ptr[1] = atof(str);
                XNS::XMLString::release(&str);
            }
            else if (XNS::XMLString::compareIString(attrs.getName(i), this->b))
            {
                str = XNS::XMLString::transcode(attrs.getValue(i));
                this->rgba_ptr[2] = atof(str);
                XNS::XMLString::release(&str);
            }
            else if (XNS::XMLString::compareIString(attrs.getName(i), this->a))
            {
                str = XNS::XMLString::transcode(attrs.getValue(i));
                this->rgba_ptr[3] = atof(str);
                XNS::XMLString::release(&str);
            }
            else
            {
                /* We don't recognize this attribute */
                std::ostringstream s;
                str = XNS::XMLString::transcode(attrs.getName(i));
                s << "Bad rgba attribute \"" << str << '"';
                XNS::XMLString::release(&str);
                throw std::runtime_error(s.str());
            }
        }
    }
    else
        throw std::runtime_error("Bad rgba tag");
}

void TextureCache::FileParser::open_mapfile(XNS::AttributeList& attrs)
{
    int i;
    char *str;

    if (this->current == shininess)
    {
        this->current = mapfile;
    }
    else
        throw std::runtime_error("Bad mapfile tag");
}

TextureCache::FileParser::FileParser(TextureCache *parent)
{
    this->parent = parent;
    this->current = start;
    this->texid = 0LL;

    /* Set our parser strings, so we don't have to keep transcoding */
    this->texture = XNS::XMLString::transcode("texture");
    this->diffuse = XNS::XMLString::transcode("diffuse");
    this->specular = XNS::XMLString::transcode("specular");
    this->shininess = XNS::XMLString::transcode("shininess");
    this->rgba = XNS::XMLString::transcode("rgba");
    this->mapfile = XNS::XMLString::transcode("mapfile");
    this->version = XNS::XMLString::transcode("version");
    this->identifier = XNS::XMLString::transcode("identifier");
    this->value = XNS::XMLString::transcode("value");
    this->r = XNS::XMLString::transcode("r");
    this->g = XNS::XMLString::transcode("g");
    this->b = XNS::XMLString::transcode("b");
    this->a = XNS::XMLString::transcode("a");
    this->filename = XNS::XMLString::transcode("filename");
}

TextureCache::FileParser::~FileParser()
{
    XNS::XMLString::release(&(this->texture));
    XNS::XMLString::release(&(this->diffuse));
    XNS::XMLString::release(&(this->specular));
    XNS::XMLString::release(&(this->shininess));
    XNS::XMLString::release(&(this->rgba));
    XNS::XMLString::release(&(this->mapfile));
    XNS::XMLString::release(&(this->version));
    XNS::XMLString::release(&(this->identifier));
    XNS::XMLString::release(&(this->value));
    XNS::XMLString::release(&(this->r));
    XNS::XMLString::release(&(this->g));
    XNS::XMLString::release(&(this->b));
    XNS::XMLString::release(&(this->a));
    XNS::XMLString::release(&(this->filename));
}

void TextureCache::FileParser::characters(const XMLCh *chars,
                                          const unsigned int len)
{
    /* This should get invoked when there are free characters.  Our
     * DTD doesn't really allow any, but we might get whitespace
     * here.
     */
    if (XNS::XMLString::isAllWhiteSpace(chars))
        return;
    else
    {
        std::ostringstream s;
        char *str = XNS::XMLString::transcode(chars);
        s << "Got free text \"" << str << "\" in the texture file";
        XNS::XMLString::release(&str);
        throw std::runtime_error(s.str());
    }
}

void TextureCache::FileParser::startElement(const XMLCh *name,
                                            XNS::AttributeList& attrs)
{
    if (XNS::XMLString::compareIString(name, this->texture))
        this->open_texture(attrs);
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

void TextureCache::FileParser::endElement(const XMLCh *name)
{
    if (XNS::XMLString::compareIString(name, this->texture))
        this->close_texture(attrs);
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

const int TextureCache::PRUNE_INTERVAL = 600;
const int TextureCache::PRUNE_LIFETIME = 1200;

void *TextureCache::prune_worker(void *arg)
{
    TextureCache *tc = (TextureCache *)arg;
    struct timeval limit;
    std::unordered_map<u_int64_t, texture >::iterator i;
    int count;

    for (;;)
    {
        sleep(TextureCache::PRUNE_INTERVAL);
        if (!gettimeofday(&limit, NULL))
        {
            std::ostringstream s;
            s << "Texture reaper thread couldn't get time of day: "
              << strerror(errno) << " (" << errno << ")";
            main_post_message(s.str());
            continue;
        }
        limit.tv_sec -= TextureCache::PRUNE_LIFETIME;
        count = 0;
        for (i = tc->geom.begin(); i != tc->geom.end(); ++i)
        {
            if (i->lastused.tv_sec >= limit.tv_sec)
            {
                tc->tex.erase(i--);
                ++count;
            }
        }

        if (count > 0)
        {
            std::ostringstream s;
            s << "Removed " << count << " entities from texture cache";
            main_post_message(s.str());
        }
    }
}

TextureCache::TextureCache()
    : tex(), store(TEXTURE_PREFIX), cache()
{
    int ret;

    this->cache = getenv("HOME");
    this->cache += "/.revision9/texture";

    /* We might consider doing a simple file mtime comparison between
     * everything in the cache and everything in the store, and deleting
     * stuff from the cache which is newer in the store.
     */

    if ((ret = pthread_create(&(this->prune_thread),
                              NULL,
                              this->prune_worker,
                              (void *)this)) != 0)
    {
        std::ostringstream s;
        s << "Couldn't start texture cleanup thread: "
          << strerror(ret) << " (" << ret << ")";
        throw std::runtime_error(s.str());
    }
}

TextureCache::~TextureCache()
{
    int ret;

    if ((ret = pthread_cancel(this->prune_thread)) != 0)
    {
        std::ostringstream s;
        s << "Couldn't cancel texture prune thread: "
          << strerror(ret) << " (" << ret << ")";
        main_post_message(s.str());
    }
    sleep(0);
    if ((ret = pthread_join(this->prune_thread, NULL)) != 0)
    {
        std::ostringstream s;
        s << "Couldn't join texture prune thread: "
          << strerror(ret) << " (" << ret << ")";
        main_post_message(s.str());
    }
    this->tex.clear();
}

void TextureCache::load(u_int64_t texid)
{
    /* Look in the user cache, since it could possibly be updated more
     * recently than the system store.
     */
    std::ostringstream user_fname;
    user_fname << this->cache << '/' << std::hex << texid & 0xFF << '/'
               << texid << ".xml";
    if (this->parse_file(user_fname.str()))
        return;

    /* Didn't find it in the user cache; now look in the system store */
    std::ostringstream store_fname;
    store_fname << this->store << '/' << std::hex << texid & 0xFF << '/'
                << texid << ".xml";
    if (this->parse_file(store_fname.str()))
        return;

    /* If we can't find the texture and have to request it, we should
     * have some sort of fallback texture to use in the meantime
     * before we receive the new stuff.
     */
    /*send_texture_request(texid, frame);*/
}

texture *TextureCache::fetch(u_int64_t texid)
{
    std::unordered_map<u_int64_t, texture>::iterator tex;

    if ((tex = this->tex.find(texid)) != this->tex.end())
    {
        gettimeofday(&(tex->lastused), NULL);
        return &(*tex);
    }
    this->load(texid);
    return NULL;
}

void draw_texture(u_int64_t textid)
{
    texture *t = this->fetch(textid);

    if (t != NULL)
    {
        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, t->diffuse);
        glMaterialfv(GL_FRONT, GL_SPECULAR, t->specular);
        glMaterialfv(GL_FRONT, GL_SHININESS, &t->shininess);
    }
    else
    {
        /* Fallback texture */
        GLfloat material_diff[] = { 0.18, 0.18, 0.18, 1.0 };
        GLfloat material_shin[] = { 0.0 };

        glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, material_diff);
        glMaterialfv(GL_FRONT, GL_SHININESS, material_shin);
    }
}
