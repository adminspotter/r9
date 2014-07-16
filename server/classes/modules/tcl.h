/* tcl.h                                                   -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 16 Jul 2014, 10:08:30 trinityquirk
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
 * This file contains the class for embedding tcl.
 *
 * Changes
 *   16 Jul 2014 TAQ - Created the file.
 *
 * Things to do
 *
 */

#ifndef __INC_R9TCL_H__
#define __INC_R9TCL_H__

#include <tcl.h>

#include "language.h"

class TclLanguage : public Language
{
  private:
    TclInterp *interp;

  public:
    TclLanguage();
    ~TclLanguage();

    std::string execute(const std::string&);
};

#endif /* __INC_R9TCL_H__ */
