/* control.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 02 Jan 2021, 08:48:37 tquirk
 *
 * Revision IX game client
 * Copyright (C) 2021  Trinity Annabelle Quirk
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
 * This file contains the control factory for the R9 client.
 *
 * Things to do
 *
 */

#include <config.h>

#include <glob.h>
#include <dlfcn.h>

#include <sstream>
#include <stdexcept>

#include "control.h"
#include "l10n.h"

static std::string fname("");
static void *lib = NULL;

std::vector<std::string> control_factory::types(void)
{
    int errno;
    glob_t mods;
    std::vector<std::string> mod_types;

    if ((errno = glob(CLIENT_LIB_DIR "/*" LT_MODULE_EXT, 0, NULL, &mods)) != 0)
    {
        std::ostringstream s;

        s << _("Could not find control modules: ");
        switch (errno)
        {
          case 1:   s << "Out of memory";             break;
          case 2:   s << "Read error";                break;
          case 3:   s << "No match found";            break;
          case 4:   s << "Function not implemented";  break;
          default:  s << "Unknown error";             break;
        }
        s << "(" << errno << ")";
        throw std::runtime_error(s.str());
    }
    for (int i = 0; i < mods.gl_pathc; ++i)
    {
        std::string mod(mods.gl_pathv[i]);
        std::string::size_type found;

        if ((found = mod.find_last_of("/")) != std::string::npos)
            mod = mod.substr(found);
        if ((found = mod.find_last_of(".")) != std::string::npos)
            mod = mod.substr(0, found);
        mod_types.push_back(mod);
    }
    globfree(&mods);
    return mod_types;
}

control *control_factory::create(const std::string& control_type)
{
    control *(*sym)(void);
    char *err;

    if (fname != "" && lib != NULL)
    {
        dlclose(lib);
        if ((err = dlerror()) != NULL)
        {
            std::ostringstream s;
            s << "Could not close library " << fname << ": " << err;
            throw std::runtime_error(s.str());
        }
    }
    fname = std::string(CLIENT_LIB_DIR) + "/" + control_type + LT_MODULE_EXT;
    lib = dlopen(fname.c_str(), RTLD_LAZY | RTLD_GLOBAL);
    if ((err = dlerror()) != NULL)
    {
        std::ostringstream s;
        s << "Could not open library " << fname << ": " << err;
        throw std::runtime_error(s.str());
    }
    dlerror();
    sym = (control *(*)(void))dlsym(lib, "create");
    if ((err = dlerror()) != NULL)
    {
        std::ostringstream s;
        s << "Could not find create symbol: " << err;
        throw std::runtime_error(s.str());
    }
    dlerror();
    return (*sym)();
}
