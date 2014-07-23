/* texture.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 23 Jul 2014, 18:02:35 tquirk
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
 *   23 Jul 2014 TAQ - We're now using the cache.h template.
 *
 * Things to do
 *   - To properly implement a fallback texture, we may need to say
 *     that a given element (say, id 0) in the cache is the fallback,
 *     and that if we don't find the one we're looking for and have to
 *     request it (or even while we're waiting to load it), we return
 *     the id 0 element, instead of the element we're actually looking
 *     for.
 *
 */

#include <sstream>
#include <iomanip>
#include <stdexcept>

#include "texture.h"

TextureCache tex_cache;

void TextureParser::open_texture(XNS::AttributeList& attrs)
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
                /* We already know the objectid
                str = XNS::XMLString::transcode(attrs.getValue(i));
                this->texid = strtoll(str, NULL, 10);
                XNS::XMLString::release(&str);*/
            }
            else
            {
                /* We don't recognize this attribute */
                std::ostringstream s;
                str = XNS::XMLString::transcode(attrs.getName(i));
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
            if (XNS::XMLString::compareIString(attrs.getName(i), this->value))
            {
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

    /* Set our parser strings, so we don't have to keep transcoding */
    this->texture_str = XNS::XMLString::transcode("texture");
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

TextureParser::~TextureParser()
{
    XNS::XMLString::release(&(this->texture_str));
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

void TextureParser::startElement(const XMLCh *name,
                                 XNS::AttributeList& attrs)
{
    if (XNS::XMLString::compareIString(name, this->texture_str))
        this->open_texture(attrs);
    else if (XNS::XMLString::compareIString(name, this->diffuse))
        this->open_diffuse(attrs);
    else if (XNS::XMLString::compareIString(name, this->specular))
        this->open_specular(attrs);
    else if (XNS::XMLString::compareIString(name, this->shininess))
        this->open_shininess(attrs);
    else if (XNS::XMLString::compareIString(name, this->rgba))
        this->open_rgba(attrs);
    else if (XNS::XMLString::compareIString(name, this->mapfile))
        this->open_mapfile(attrs);
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

void TextureParser::endElement(const XMLCh *name)
{
    if (XNS::XMLString::compareIString(name, this->texture_str))
        this->close_texture();
    else if (XNS::XMLString::compareIString(name, this->diffuse))
        this->close_diffuse();
    else if (XNS::XMLString::compareIString(name, this->specular))
        this->close_specular();
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

void draw_texture(u_int64_t texid)
{
    texture& t = tex_cache[texid];

    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, t.diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, t.specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, &t.shininess);

    /* Fallback texture */
    /*GLfloat material_diff[] = { 0.18, 0.18, 0.18, 1.0 };
    GLfloat material_shin[] = { 0.0 };

    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, material_diff);
    glMaterialfv(GL_FRONT, GL_SHININESS, material_shin);*/
}
