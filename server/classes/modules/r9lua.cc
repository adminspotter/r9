/* r9lua.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 30 Mar 2018, 22:45:02 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2018  Trinity Annabelle Quirk
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
 * This file contains the class implementation for embedding lua.
 *
 * Things to do
 *
 */

#include "r9lua.h"

LuaLanguage::LuaLanguage()
{
    this->state = luaL_newstate();
    luaL_openlibs(this->state);
}

LuaLanguage::~LuaLanguage()
{
    lua_close(this->state);
}

std::string LuaLanguage::execute(const std::string &s)
{
    return "";
}

extern "C" Language *create_language(void)
{
    return new LuaLanguage();
}

extern "C" void destroy_language(Language *lang)
{
    delete lang;
}
