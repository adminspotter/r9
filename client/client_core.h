/* client_core.h
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 01 Dec 2015, 06:55:37 tquirk
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
 * This file contains the publically-available function prototypes
 * and structures for the client program.
 *
 * Things to do
 *
 */

#ifndef __INC_R9CLIENT_CORE_H__
#define __INC_R9CLIENT_CORE_H__

void init_client_core(void);
void cleanup_client_core(void);

void draw_texture(uint64_t);
void draw_geometry(uint64_t, uint16_t);

void draw_objects(void);
void move_object(uint64_t, uint16_t,
                 double, double, double, double, double, double);

#endif /* __INC_R9CLIENT_CORE_H__ */
