/* server.h
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 05 Jul 2014, 07:47:43 tquirk
 *
 * Revision IX game server
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
 * This file contains some prototypes for the server program.
 *
 * Things to do
 *
 */

#ifndef __INC_SERVER_H__
#define __INC_SERVER_H__

#include "classes/zone.h"
#include "classes/modules/db.h"

extern Zone *zone;
extern DB *database;

void set_exit_flag(void);
void complete_startup(void);
void complete_cleanup(void);

#endif /* __INC_SERVER_H__ */
