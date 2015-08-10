/* r9tcl.h                                                 -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 10 Aug 2015, 07:44:38 tquirk
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
 * This file contains the class for embedding tcl.
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
    Tcl_Interp *interp;

  public:
    TclLanguage();
    ~TclLanguage();

    std::string execute(const std::string&);
};

#endif /* __INC_R9TCL_H__ */
