/* control.h                                               -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 13 Dec 2019, 08:43:02 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2019  Trinity Annabelle Quirk
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
 * This file contains the declaration of the control class for the
 * Revision IX system.
 *
 * Things to do
 *
 */

#ifndef __INC_CONTROL_H__
#define __INC_CONTROL_H__

#include <sys/types.h>

#include <cstdint>
#include <unordered_map>
#include <string>

class Control;

#include "game_obj.h"

class Control
{
  public:
    typedef struct skill_level_tag
    {
        uint16_t index, level, improvement;
        time_t last_level;
    }
    skill_level;
    typedef std::unordered_map<uint16_t, skill_level> skills_map;
    typedef skills_map::iterator skills_iterator;

    uint64_t userid;
    GameObject *default_slave, *slave;
    skills_map actions;

  public:
    Control(uint64_t, GameObject *);
    virtual ~Control();

    virtual bool operator<(const Control&) const;
    virtual bool operator==(const Control&) const;
    virtual const Control& operator=(const Control&);

    bool take_over(GameObject *);
};

#endif /* __INC_CONTROL_H__ */
