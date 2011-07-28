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

static int engine_changelevel(lua_State *L)
{
    const char *pszLevel = luaL_checkstring(L, 1);

    engine->ChangeLevel(pszLevel, NULL);
    return 0;
}

static int engine_isdedicatedserver(lua_State *L)
{
    lua_pushboolean(L, engine->IsDedicatedServer());
    return 1;
}

static int engine_ismapvalid(lua_State *L)
{
    const char *pszLevel = luaL_checkstring(L, 1);

    lua_pushboolean(L, engine->IsMapValid(pszLevel));
    return 1;
}

static int engine_lightstyle(lua_State *L)
{
    int style = luaL_checkinteger(L, 1);
    const char *pszVal = STRING(AllocPooledString(luaL_checkstring(L, 2)));

    engine->LightStyle(style, pszVal);
    return 0;
}

static int engine_servercommand(lua_State *L)
{
    const char *pCommand = luaL_checkstring(L, 1);

    engine->ServerCommand(UTIL_VarArgs("%s\n", pCommand));
    return 0;
}

static const luaL_Reg enginelib[] = {
    {"ChangeLevel",     engine_changelevel},
    {"IsDedicatedServer", engine_isdedicatedserver},
    {"IsMapValid",      engine_ismapvalid},
    {"LightStyle",      engine_lightstyle},
    {"ServerCommand",   engine_servercommand},
    {NULL, NULL}
};


/*
** Open library
*/
LUALIB_API int luaopen_engine (lua_State *L) {
  luaL_register(L, LUA_ENGINELIBNAME, enginelib);
  return 1;
}
