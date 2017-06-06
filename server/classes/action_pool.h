/* action_pool.h                                           -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 05 Jun 2017, 23:47:08 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2017  Trinity Annabelle Quirk
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
 * This file contains the action thread pool, a wrapper around the
 * ThreadPool which adds an actions map.
 *
 * Things to do
 *
 */

#ifndef __INC_ACTION_POOL_H__
#define __INC_ACTION_POOL_H__

#include "thread_pool.h"
#include "library.h"
#include "defs.h"

#include "modules/db.h"

class ActionPool : public ThreadPool<packet_list>
{
  private:
    typedef std::map<uint16_t, action_rec> actions_map;
    typedef actions_map::iterator actions_iterator;

    Library *action_lib;

    actions_map actions;

    void load_actions(const std::string&);
    int execute_action(Control *, action_request&, size_t);

    std::map<uint64_t, GameObject *>& game_objects;

  public:
    ActionPool(const std::string&, unsigned int,
               std::map<uint64_t, GameObject *>&, DB *);
    ~ActionPool();


    static void *action_pool_worker(void *);

    void populate_server_actions(DB&);
};

#endif /* __INC_ACTION_POOL_H__ */
