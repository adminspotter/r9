/* library.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 10 Jul 2014, 14:21:56 trinityquirk
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
 * This file contains the dynamic library loading class.
 *
 * Changes
 *   18 Jun 2014 TAQ - Created the file.
 *   21 Jun 2014 TAQ - Instead of returning NULL on failures, we'll now throw
 *                     strings, and leave it to our caller to log or whatever.
 *                     Updated syslog to use new stream log.
 *   22 Jun 2014 TAQ - Constructor now takes strings instead of char pointers.
 *                     Also got rid of the magic lib name behaviour, instead
 *                     just taking the raw name we get passed.
 *   09 Jul 2014 TAQ - Normalized exceptions with the rest of the codebase.
 *   10 Jul 2014 TAQ - Handle any exceptions in the destructor.
 *
 * Things to do
 *
 */

#include <dlfcn.h>

#include <sstream>
#include <stdexcept>

#include "library.h"

Library::Library(const std::string& name)
    : libname(name)
{
    this->open();
}

Library::~Library()
{
    try { this->close(); }
    catch (std::exception& e) { /* Do nothing */ }
}

void Library::open(void)
{
    char *err;

    this->lib = dlopen(this->libname.c_str(), RTLD_LAZY | RTLD_GLOBAL);
    if ((err = dlerror()) != NULL)
    {
        std::ostringstream s;
        s << "error opening library " << this->libname << ": " << err;
        throw std::runtime_error(s.str());
    }
    dlerror();
}

void *Library::symbol(const char *symname)
{
    void *sym = NULL;
    char *err;

    if (this->lib != NULL)
    {
        sym = dlsym(this->lib, symname);
        if ((err = dlerror()) != NULL)
        {
            std::ostringstream s;
            s << "error finding symbol " << symname << " in library "
              << this->libname << ": " << err;
            throw std::runtime_error(s.str());
        }
        dlerror();
    }
    return sym;
}

void Library::close(void)
{
    char *err;

    if (this->lib != NULL)
    {
        dlclose(this->lib);
        if ((err = dlerror()) != NULL)
        {
            std::ostringstream s;
            s << "error closing library " << this->libname << ": " << err;
            throw std::runtime_error(s.str());
        }
    }
    dlerror();
}
