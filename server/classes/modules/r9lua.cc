/* r9lua.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 31 Mar 2018, 08:54:21 tquirk
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
    luaL_dostring(this->state, s.c_str());

    int top = lua_gettop(this->state);
    if (top > 0)
    {
        std::string result;

        switch (lua_type(this->state, top))
        {
          case LUA_TSTRING:
            result = lua_tostring(this->state, top);
            break;

          case LUA_TBOOLEAN:
            result = (lua_toboolean(this->state, top) ? "true" : "false");
            break;

          case LUA_TNUMBER:
            char tmp_str[32];
            snprintf(tmp_str, sizeof(tmp_str),
                     "%g", lua_tonumber(this->state, top));
            result = tmp_str;
            break;

          default:
            result = lua_typename(this->state, lua_type(this->state, top));
            break;
        }
        lua_pop(this->state, 1);
        return result;
    }
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
