/* ui.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 08 Mar 2016, 16:56:32 tquirk
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
 * This file contains the ui context method definitions for the R9 UI
 * widget set.
 *
 * Things to do
 *
 */

#include <string>
#include <algorithm>

#include "ui.h"
#include "panel.h"
#include "shader.h"

int ui::context::get_size(GLuint t, void *v)
{
    int ret = 0;

    switch (t)
    {
      case ui::size::width:   *((GLuint *)v) = this->width;  break;
      case ui::size::height:  *((GLuint *)v) = this->height; break;
      default:                ret = 1;                       break;
    }
    return ret;
}

int ui::context::get_attribute(GLuint t, void *v)
{
    int ret = 0;

    switch (t)
    {
      case ui::attribute::position:
        *((GLuint *)v) = this->pos_attr;
        break;
      case ui::attribute::normal:
        *((GLuint *)v) = this->norm_attr;
        break;
      case ui::attribute::color:
        *((GLuint *)v) = this->color_attr;
        break;
      default: ret = 1; break;
    }
    return ret;
}

ui::context::context(GLuint w, GLuint h)
    : children()
{
    this->width = w;
    this->height = h;

    this->vert_shader = load_shader(GL_VERTEX_SHADER,
                                    SHADER_SRC_PATH "/ui_vertex.glsl");
    this->frag_shader = load_shader(GL_FRAGMENT_SHADER,
                                    SHADER_SRC_PATH "/ui_fragment.glsl");
    this->shader_pgm = create_program(vert_shader, 0, frag_shader, "color");
    this->pos_attr = glGetAttribLocation(shader_pgm, "position");
    this->norm_attr = glGetAttribLocation(shader_pgm, "normal");
    this->color_attr = glGetAttribLocation(shader_pgm, "color");
}

ui::context::~context()
{
    glDeleteShader(this->vert_shader);
    glDeleteShader(this->frag_shader);
    glUseProgram(0);
    glDeleteProgram(this->shader_pgm);
}

int ui::context::get(GLuint e, GLuint t, void *v)
{
    int ret;

    switch (e)
    {
      case ui::element::size:       ret = this->get_size(t, v);      break;
      case ui::element::attribute:  ret = this->get_attribute(t, v); break;
      default:                      ret = 1;                         break;
    }
    return ret;
}

void ui::context::draw(void)
{
    glUseProgram(this->shader_pgm);
    for (auto i = this->children.begin(); i != this->children.end(); ++i)
        (*i)->draw();
}

ui::context& ui::context::operator+=(ui::panel *p)
{
    auto found = std::find(this->children.begin(), this->children.end(), p);
    if (found == this->children.end())
        this->children.push_back(p);
    return *this;
}

ui::context& ui::context::operator-=(ui::panel *p)
{
    this->children.remove(p);
    return *this;
}
