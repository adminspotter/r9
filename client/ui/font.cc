/* font.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 18 Jun 2016, 18:30:53 tquirk
 *
 * Revision IX game client
 * Copyright (C) 2016  Trinity Annabelle Quirk
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
 * This file contains the implementation for the font handling, via
 * the Freetype library.
 *
 * We include a static Freetype library singleton with reference
 * counting in here, so that we don't have to think about the library
 * anywhere else.
 *
 * Since we have kerning data available to us in the font, and it
 * would be difficult to represent that to an external caller, we'll
 * go ahead and render strings here.
 *
 * We want to seamlessly handle mixed L-to-R and R-to-L text.  There
 * may be cases in an L-to-R language, in which R-to-L text is
 * included, or vice versa, that we want to handle.  It seems like
 * most of the vertical languages have horizontal forms, so we're
 * going to make a command decision and render everything in
 * horizontal mode.  It would be nice to handle verticals natively,
 * but it's going to be a nightmare to implement in a reasonable way.
 *
 * There do not appear to be any horizontal scripts which flow their
 * lines B-to-T, so we can at least be safe with a T-to-B line flow
 * assumption.
 *
 * Ref: http://www.omniglot.com/writing/direction.htm
 *
 * Things to do
 *
 */

#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdexcept>
#include <algorithm>

#include "font.h"

#include "../configdata.h"
#include "../l10n.h"

/* We'll keep a single library handle, and a refcount.  This will all
 * be handled within the same thread, so no multi-thread weirdness
 * should intrude.
 */
static FT_Library ft_lib;
static int ft_lib_count = 0;

static FT_Library *init_freetype(void)
{
    if (ft_lib_count++ == 0)
        FT_Init_FreeType(&ft_lib);

    return &ft_lib;
}

static void cleanup_freetype(void)
{
    if (--ft_lib_count == 0)
        FT_Done_FreeType(ft_lib);
}

std::string Font::search_path(std::string& font_name,
                              std::vector<std::string>& paths)
{
    std::vector<std::string>::iterator i;
    struct stat st;

    for (i = paths.begin(); i != paths.end(); ++i)
    {
        std::string path = *i;
        std::string::size_type pos;

        /* Make sure the path doesn't have a ~, which stat won't
         * understand
         */
        if ((pos = path.find('~')) != std::string::npos)
        {
            std::string home = getenv("HOME");

            if (home.size() == 0)
                throw std::runtime_error(_("Could not find home directory"));
            path.replace(pos, 1, home);
        }
        path += '/' + font_name;
        if (stat(path.c_str(), &st) != -1)
            return path;
    }
    throw std::runtime_error(_("Could not find font ") + font_name);
}

void Font::load_glyph(FT_ULong code)
{
    FT_GlyphSlot slot = this->face->glyph;

    FT_Load_Char(this->face, code, FT_LOAD_RENDER);
    Glyph& g = this->glyphs[code];
    /* Advance is represented in 26.6 format, so throw away the
     * lowest-order 6 bits.
     */
    g.x_advance = slot->advance.x >> 6;
    g.y_advance = slot->advance.y >> 6;
    g.width = slot->bitmap.width;
    g.height = slot->bitmap.rows;
    g.left = slot->bitmap_left;
    g.top = slot->bitmap_top;
    g.pitch = slot->bitmap.pitch;
    g.bitmap = new unsigned char[abs(g.pitch) * g.height];
    memcpy(g.bitmap, slot->bitmap.buffer, abs(g.pitch) * g.height);
}

void Font::kern(FT_ULong a, FT_ULong b, FT_Vector *k)
{
    if (FT_Get_Kerning(this->face, a, b, FT_KERNING_DEFAULT, k))
        k->x = k->y = 0;
}

/* This gets a little complicated, because a glyph which has no
 * descender could have an overall height that is equal to a shorter
 * glyph that has a descender.  They would evaluate as equal, but the
 * descender would expand the line height, and we wouldn't be able to
 * account for that with a single value.  So we need two values for
 * the vertical axis; the values for vertical size are the ascender
 * and descender respectively.
 *
 * Return values come back in the req_size argument.  First element is
 * width, second is ascender, third is descender.
 *
 * It would be nice if we could support vertical lines, but it's going
 * to be way too difficult, and most of the currently-used vertical
 * languages have horizontal usage nowadays.
 */
void Font::get_string_size(const std::u32string& str,
                           std::vector<int>& req_size)
{
    std::u32string::const_iterator i;

    req_size[0] = req_size[1] = req_size[2] = 0;
    for (i = str.begin(); i != str.end(); ++i)
    {
        FT_Vector kerning = {0, 0};
        Glyph& g = (*this)[*i];

        if (g.x_advance == 0 && g.y_advance != 0)
            throw std::runtime_error(_("This font is not supported."));
        if (i != str.begin())
            this->kern(*(i - 1), *i, &kerning);

        /* We're only going to do horizontal text */
        req_size[0] += kerning.x;
        if (i != str.begin())
            req_size[0] += g.left;
        if (i + 1 == str.end())
            req_size[0] += g.width;
        else
            req_size[0] += g.x_advance;
        req_size[1] = std::max(req_size[1], g.top);
        req_size[2] = std::max(req_size[2], g.height - g.top);
    }
}

Font::Font(std::string& font_name,
           int pixel_size,
           std::vector<std::string>& paths)
    : glyphs(font_name + " glyphs")
{
    FT_Library *lib = init_freetype();
    std::string font_path = this->search_path(font_name, paths);

    if (FT_New_Face(*lib, font_path.c_str(), 0, &this->face))
        throw std::runtime_error(_("Could not load font ") + font_name);
    FT_Set_Pixel_Sizes(this->face, 0, pixel_size);
}

Font::~Font()
{
    FT_Done_Face(this->face);
    cleanup_freetype();
}

struct Glyph& Font::operator[](FT_ULong code)
{
    Glyph& g = this->glyphs[code];

    if (g.bitmap == NULL)
        this->load_glyph(code);
    return g;
}

unsigned char *Font::render_string(const std::u32string& str,
                                   unsigned int& w,
                                   unsigned int& h)
{
    std::vector<int> req_size = {0, 0, 0};
    unsigned char *img = NULL;
    std::u32string::const_iterator i = str.begin();
    bool l_to_r = (*this)[*i].is_l_to_r();
    int pos = (l_to_r ? 0 : w - 1), save_pos = pos;

    this->get_string_size(str, req_size);
    w = req_size[0];
    h = req_size[1] + req_size[2];
    img = new unsigned char[w * h];
    memset(img, 0, w * h);

    /* GL does positive y as up, so it makes more sense to just draw
     * the buffer upside-down.  All the glyphs are already
     * upside-down.
     */
    while (i != str.end())
    {
        Glyph& g = (*this)[*i];
        int j, k, bottom_row = req_size[2] + g.top - g.height;
        int row_offset, glyph_offset;
        FT_Vector kerning = {0, 0};
        bool same_dir = (l_to_r == g.is_l_to_r());

        /* Kerning, if available */
        if (i != str.begin())
            this->kern(*(i - 1), *i, &kerning);

        /* If we're dealing with wrong-direction text, scoot things over */
        if (!same_dir && pos != save_pos)
        {
            int x_move = abs(pos - save_pos);
            int x_distance = g.x_advance + kerning.x;
            int start = std::min(pos, save_pos);
            for (j = 0; j < h; ++j)
            {
                row_offset = (w * j) + start;
                memmove(&img[row_offset - x_distance],
                        &img[row_offset],
                        x_move);
                memset(&img[start], 0, x_distance);
            }
        }
        else
            save_pos = pos;
        if (!l_to_r)
        {
            if (same_dir)
            {
                pos += g.x_advance + kerning.x;
                save_pos = pos;
            }
            else
                pos -= g.x_advance + kerning.x;
        }
        for (j = 0; j < g.height; ++j)
        {
            row_offset = (bottom_row + j + kerning.y) * w + save_pos + kerning.x
                - (!l_to_r && !same_dir ? g.x_advance : 0);
            if (i != str.begin())
                row_offset += g.left;
            glyph_offset = (g.height - 1 - j) * g.width;
            for (k = 0; k < g.width; ++k)
                img[row_offset + k] |= g.bitmap[glyph_offset + k];
        }
        if (l_to_r)
        {
            if (same_dir)
            {
                pos += g.x_advance + kerning.x;
                save_pos = pos;
            }
            else
                pos -= g.x_advance + kerning.x;
        }

        ++i;
    }
    return img;
}