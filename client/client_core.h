/* client_core.h
 *   by Trinity Quirk <tquirk@ymb.net>
 *
 * Revision IX game client
 * Copyright (C) 2015-2019  Trinity Annabelle Quirk
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
 * This file contains the publically-available function prototypes
 * and structures for the client program.
 *
 * Things to do
 *
 */

#ifndef __INC_R9CLIENT_CORE_H__
#define __INC_R9CLIENT_CORE_H__

#include <GL/gl.h>

#include <string>

#include "object.h"

extern ObjectCache *obj;
extern struct object *self_obj;

void init_client_core(void);
void cleanup_client_core(void);

void draw_objects(void);
void move_object(uint64_t, uint16_t,
                 float, float, float,
                 float, float, float, float,
                 float, float, float);

void resize_window(int, int);

GLuint load_shader(GLenum, const std::string&);
GLuint create_shader(GLenum, const std::string&);
GLuint create_program(GLuint, GLuint, GLuint, const char *);
std::string GLenum_to_string(GLenum);

#endif /* __INC_R9CLIENT_CORE_H__ */
