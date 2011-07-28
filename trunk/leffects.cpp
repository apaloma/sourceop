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

#define LUA_LIB

// HACK HACK HACK : BASEENTITY ACCESS
#define GAME_DLL
#include "fixdebug.h"
#include "cbase.h"
#include "baseentity.h"
// HACK HACK HACK : BASEENTITY ACCESS

#include "AdminOP.h"

#include "lua.h"

#include "lauxlib.h"
#include "luasoplibs.h"

#include "l_class_vector.h"

#include "tier0/dbg.h"

static int effects_beam(lua_State *L)
{
    SOPVector *start = Lunar<SOPVector>::check(L, 1);
    SOPVector *end = Lunar<SOPVector>::check(L, 2);
    int index = luaL_checkinteger(L, 3);
    int haloindex = luaL_checkinteger(L, 4);
    int framestart = luaL_checkinteger(L, 5);
    int framerate = luaL_checkinteger(L, 6);
    lua_Number life = luaL_checknumber(L, 7);
    int width = luaL_checkinteger(L, 8);
    int endwidth = luaL_checkinteger(L, 9);
    int fadelength = luaL_checkinteger(L, 10);
    int noise = luaL_checkinteger(L, 11);
    int red = luaL_checkinteger(L, 12);
    int green = luaL_checkinteger(L, 13);
    int blue = luaL_checkinteger(L, 14);
    int brightness = luaL_checkinteger(L, 15);
    int speed = luaL_checkinteger(L, 16);
    
    effects->Beam(start->ToVector(), end->ToVector(), index, haloindex, framestart, framerate, life, width, endwidth, fadelength, noise, red, green, blue, brightness, speed);

    return 0;
}


static const luaL_Reg effectslib[] = {
    {"Beam",   effects_beam},
    {NULL, NULL}
};


/*
** Open library
*/
LUALIB_API int luaopen_effects (lua_State *L) {
  luaL_register(L, LUA_EFFECTSLIBNAME, effectslib);
  return 1;
}
