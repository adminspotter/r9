/* library.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 21 Jun 2014, 18:22:38 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2007  Trinity Annabelle Quirk
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
 *
 * Things to do
 *
 */

#include <dlfcn.h>

#include "library.h"
#include "../log.h"

Library::Library(const char *name)
    : libname(name)
{
    this->open();
}

Library::~Library()
{
    this->close();
}

void Library::open(void)
{
    std::string fname = "libr9_" + libname + ".so";
    char *err;

    std::clog << "loading " << this->libname << " lib" << std::endl;
    this->lib = dlopen(fname.c_str(), RTLD_LAZY | RTLD_GLOBAL);
    if ((err = dlerror()) != NULL)
        throw std::string(err);
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
            throw std::string(err);
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
            throw std::string(err);
    }
    dlerror();
}
