/* texture.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 02 Aug 2014, 18:05:16 tquirk
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
 * Things to do
 *
 */

#include <cstring>
#include <sstream>
#include <iomanip>
#include <stdexcept>

#include "texture.h"

void TextureParser::open_texture(XNS::AttributeList& attrs)
{
    int i;
    char *str;

    if (this->current == start)
    {
        this->current = texture_st;
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
                this->texid = strtoll(str, NULL, 10);
                XNS::XMLString::release(&str);*/
            }
            else
            {
                /* We don't recognize this attribute */
                std::ostringstream s;
                s << "Bad texture attribute \"" << str << '"';
                XNS::XMLString::release(&str);
                throw std::runtime_error(s.str());
            }
        }
    }
    else
        throw std::runtime_error("Bad texture open tag");
}

void TextureParser::close_texture(void)
{
    if (this->current == shininess_st || this->current == mapfile_st)
        this->current = texture_en;
    else
        throw std::runtime_error("Bad texture close tag");
}

void TextureParser::open_diffuse(XNS::AttributeList& attrs)
{
    if (this->current == texture_st)
    {
        this->current = diffuse_st;
        this->rgba_ptr = this->tex->diffuse;
    }
    else
        throw std::runtime_error("Bad diffuse open tag");
}

void TextureParser::close_diffuse(void)
{
    if (this->current == rgba_st)
        this->current = diffuse_en;
    else
        throw std::runtime_error("Bad diffuse close tag");
}

void TextureParser::open_specular(XNS::AttributeList& attrs)
{
    if (this->current == diffuse_en)
    {
        this->current = specular_st;
        this->rgba_ptr = this->tex->specular;
    }
    else
        throw std::runtime_error("Bad specular open tag");
}

void TextureParser::close_specular(void)
{
    if (this->current == rgba_st)
        this->current = specular_en;
    else
        throw std::runtime_error("Bad specular close tag");
}

void TextureParser::open_shininess(XNS::AttributeList& attrs)
{
    int i;
    char *str;

    if (this->current == specular_en)
    {
        this->current = shininess_st;
        for (i = 0; i < attrs.getLength(); ++i)
        {
            str = XNS::XMLString::transcode(attrs.getName(i));
            if (!strcmp(str, "value"))
            {
                XNS::XMLString::release(&str);
                str = XNS::XMLString::transcode(attrs.getValue(i));
                this->tex->shininess = atof(str);
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

void TextureParser::open_rgba(XNS::AttributeList& attrs)
{
    int i;
    char *str;

    if (this->current == diffuse_st || this->current == specular_st)
    {
        this->current = rgba_st;
        for (i = 0; i < attrs.getLength(); ++i)
        {
            str = XNS::XMLString::transcode(attrs.getName(i));
            if (!strcmp(str, "r"))
            {
                XNS::XMLString::release(&str);
                str = XNS::XMLString::transcode(attrs.getValue(i));
                this->rgba_ptr[0] = atof(str);
                XNS::XMLString::release(&str);
            }
            else if (!strcmp(str, "g"))
            {
                XNS::XMLString::release(&str);
                str = XNS::XMLString::transcode(attrs.getValue(i));
                this->rgba_ptr[1] = atof(str);
                XNS::XMLString::release(&str);
            }
            else if (!strcmp(str, "b"))
            {
                XNS::XMLString::release(&str);
                str = XNS::XMLString::transcode(attrs.getValue(i));
                this->rgba_ptr[2] = atof(str);
                XNS::XMLString::release(&str);
            }
            else if (!strcmp(str, "a"))
            {
                XNS::XMLString::release(&str);
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

void TextureParser::open_mapfile(XNS::AttributeList& attrs)
{
    /*int i;
    char *str;*/

    if (this->current == shininess_st)
    {
        this->current = mapfile_st;
        /* TODO: figure out how to actually do a mapped image */
    }
    else
        throw std::runtime_error("Bad mapfile tag");
}

TextureParser::TextureParser(texture *obj)
{
    this->tex = obj;
    this->current = start;
}

TextureParser::~TextureParser()
{
}

void TextureParser::startElement(const XMLCh *name,
                                 XNS::AttributeList& attrs)
{
    char *str = XNS::XMLString::transcode(name);

    if (!strcmp(str, "texture"))
        this->open_texture(attrs);
    else if (!strcmp(str, "diffuse"))
        this->open_diffuse(attrs);
    else if (!strcmp(str, "specular"))
        this->open_specular(attrs);
    else if (!strcmp(str, "shininess"))
        this->open_shininess(attrs);
    else if (!strcmp(str, "rgba"))
        this->open_rgba(attrs);
    else if (!strcmp(str, "mapfile"))
        this->open_mapfile(attrs);
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

void TextureParser::endElement(const XMLCh *name)
{
    char *str = XNS::XMLString::transcode(name);

    if (!strcmp(str, "texture"))
        this->close_texture();
    else if (!strcmp(str, "diffuse"))
        this->close_diffuse();
    else if (!strcmp(str, "specular"))
        this->close_specular();
    else if (!strcmp(str, "rgba") || !strcmp(str, "shininess"))
        ; /* empty tag */
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
