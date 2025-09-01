/* update_pool.h                                           -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *
 * Revision IX game server
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
 * This file contains the update thread pool, a pretty thin wrapper
 * around the ThreadPool, to simplify the interface and allow
 * reasonable testing.
 */

#ifndef __INC_UPDATE_POOL_H__
#define __INC_UPDATE_POOL_H__

#include "thread_pool.h"
#include "game_obj.h"

class UpdatePool : public ThreadPool<GameObject *>
{
  public:
    UpdatePool(const char *, unsigned int);
    ~UpdatePool();

    void start(void);
    void start(void *(*)(void *));

    static void *update_pool_worker(void *);
};

#endif /* __INC_UPDATE_POOL_H__ */
