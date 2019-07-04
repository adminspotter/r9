/* control.h                                               -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 02 Jul 2019, 07:56:03 tquirk
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
#include <map>
#include <string>

class Control;

#include "game_obj.h"

class Control
{
  public:
    uint64_t userid;
    GameObject *default_slave, *slave;
    std::map<uint16_t, action_level> actions;

    typedef std::map<uint16_t, action_level>::iterator actions_iterator;

  public:
    Control(uint64_t, GameObject *);
    virtual ~Control();

    virtual bool operator<(const Control&) const;
    virtual bool operator==(const Control&) const;
    virtual const Control& operator=(const Control&);

    bool take_over(GameObject *);
};

#endif /* __INC_CONTROL_H__ */
