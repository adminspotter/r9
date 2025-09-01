/* r9tcl.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *
 * Revision IX game server
 * Copyright (C) 1998-2018  Trinity Annabelle Quirk
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
 * Things to do
 *
 */

#include <sstream>
#include <stdexcept>

#include "r9tcl.h"

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

extern "C" Language *create_language(void)
{
    return new TclLanguage();
}

extern "C" void destroy_language(Language *lang)
{
    delete lang;
}
