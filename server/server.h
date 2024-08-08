/* server.h                                                -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 07 Aug 2024, 09:16:10 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2024  Trinity Annabelle Quirk
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

#include <atomic>

#include "classes/zone.h"
#include "classes/action_pool.h"
#include "classes/motion_pool.h"
#include "classes/update_pool.h"
#include "classes/modules/db.h"

extern Zone *zone;
extern DB *database;
extern ActionPool *action_pool;
extern MotionPool *motion_pool;
extern UpdatePool *update_pool;
extern std::atomic<int> main_loop_exit_flag;

void set_exit_flag(void);
void complete_startup(void);
void complete_cleanup(void);

#endif /* __INC_SERVER_H__ */
