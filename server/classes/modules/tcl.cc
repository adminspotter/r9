/* tcl.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 08 Aug 2014, 18:14:44 tquirk
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
 * This files contains the class to embed tcl within our server program.
 *
 * Changes
 *   25 Jul 1998 TAQ - Created the file.
 *   05 Sep 1998 TAQ - Minor text change.
 *   15 May 2006 TAQ - Added the GPL notice.
 *   13 Aug 2006 TAQ - Small compiler error fixes.
 *   12 Jun 2007 TAQ - A few minor tweaks.
 *   16 Jul 2014 TAQ - This is now a C++ class, a descendent of Language.
 *
 * Things to do
 *
 */

#include <sstream>
#include <stdexcept>

#include "tcl.h"

TclLanguage::TclLanguage()
{
    if ((this->interp = Tcl_CreateInterp()) == NULL)
    {
        std::ostringstream s;
        s << "couldn't create tcl interpreter";
        throw std::runtime_error(s.str());
    }
}

TclLanguage::~TclLanguage()
{
    Tcl_DeleteInterp(this->interp);
    this->interp = NULL;
}

std::string TclLanguage::execute(const std::string& cmd)
{
    Tcl_Eval(this->interp, cmd.c_str());
    return std::string(Tcl_GetStringResult(this->interp));
}

Language *create_language(void)
{
    return new TclLanguage();
}

void destroy_language(Language *lang)
{
    delete lang;
}
