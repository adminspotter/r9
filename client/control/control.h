/* control.h                                               -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 12 Dec 2020, 23:14:58 tquirk
 *
 * Revision IX game client
 * Copyright (C) 2020  Trinity Annabelle Quirk
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
 * This file contains the declaration of the base control class for
 * the R9 client.  Subclasses will implement any setup and event
 * handling for their respective control devices.
 *
 * Things to do
 *
 */

#ifndef __INC_R9CLIENT_CONTROL_H__
#define __INC_R9CLIENT_CONTROL_H__

#include <vector>
#include <string>

#include <cuddly-gl/active.h>

#include "../comm.h"

class control
{
  public:
    control() {};
    virtual ~control() {};

    virtual void setup(ui::active *, Comm *) {};
    virtual void cleanup(ui::active *, Comm *) {};
};

class control_factory
{
  public:
    static std::vector<std::string> types(void);
    static control *create(const std::string&);
};

#endif /* __INC_R9CLIENT_CONTROL_H__ */
