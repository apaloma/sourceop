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
#include "player.h"
// HACK HACK HACK : BASEENTITY ACCESS

#include "AdminOP.h"
#include "gamerulesproxy.h"

#include "lua.h"

#include "lauxlib.h"
#include "luasoplibs.h"

#include "tier0/dbg.h"

static int tempent_dynamiclight(lua_State *L)
{
    // TODO: Implement DynamicLight
    Msg("[SOURCEOP] DynamicLight not implemented.\n");
    return 0;
}

static const luaL_Reg tempentlib[] = {
    {"DynamicLight",    tempent_dynamiclight},
    {NULL, NULL}
};


/*
** Open library
*/
LUALIB_API int luaopen_tempent (lua_State *L) {
  luaL_register(L, LUA_TEMPENTLIBNAME, tempentlib);
  return 1;
}
