/* library.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 27 Dec 2020, 14:14:59 tquirk
 *
 * Revision IX game server
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
 * This file contains the dynamic library loading class.
 *
 * Things to do
 *
 */

#include <dlfcn.h>
#include <glob.h>

#include <sstream>
#include <stdexcept>

#include "library.h"

Library::Library()
    : libname("none")
{
    this->lib = NULL;
}

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

    if (this->libname[0] != '/')
        this->libname.insert(0, std::string(SERVER_LIB_DIR) + "/");
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

void find_libraries(const std::string& glob_path,
                    std::vector<Library *>& result)
{
    glob_t libs;
    int err;

    if ((err = glob(glob_path.c_str(), 0, NULL, &libs)) != 0)
    {
        std::ostringstream s;

        s << "error locating libraries: ";
        switch (err)
        {
          case 1:   s << "Out of memory";             break;
          case 2:   s << "Read error";                break;
          case 3:   s << "No match found";            break;
          case 4:   s << "Function not implemented";  break;
          default:  s << "Unknown error";             break;
        }
        s << " (" << err << ")";
        throw std::runtime_error(s.str());
    }
    while (result.size())
    {
        delete result.back();
        result.pop_back();
    }
    result.reserve(libs.gl_pathc);
    for (int i = 0; i < libs.gl_pathc; ++i)
        result.push_back(new Library(std::string(libs.gl_pathv[i])));
    globfree(&libs);
}
