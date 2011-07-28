/*
    This file is part of SourceOP.

    SourceOP is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SourceOP is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with SourceOP.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"
#include "luasoplibs.h"

#include "tier0/dbg.h"

static int table_count(lua_State *L)
{
    int count = 0;
    /* table is in the stack at index `t' */
    lua_pushnil(L);  /* first key */
    while (lua_next(L, 1) != 0) {
        count++;
        lua_pop(L, 1);  /* removes `value'; keeps `key' for next iteration */
    }
    lua_pushinteger(L, count);
    return 1;
}

static int table_hasvalue(lua_State *L)
{
    bool match = 0;
    /* table is in the stack at index `t' */
    lua_pushnil(L);  /* first key */
    while (lua_next(L, 1) != 0) {
        /* `key' is at index -2 and `value' at index -1 */
        if(lua_equal(L, 2, -1)) match = 1;

        //Msg("%s - %s\n",
        //  lua_typename(L, lua_type(L, -2)), lua_typename(L, lua_type(L, -1)));
        lua_pop(L, 1);  /* removes `value'; keeps `key' for next iteration */
    }
    lua_pushboolean(L, match);
    return 1;
}

static const luaL_Reg tabext_funcs[] = {
  {"Count", table_count},
  {"HasValue", table_hasvalue},
  {NULL, NULL}
};


LUALIB_API int luaopen_tableext (lua_State *L) {
  luaL_register(L, LUA_TABLIBNAME, tabext_funcs);
  return 1;
}
