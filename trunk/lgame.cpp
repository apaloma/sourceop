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
#include "mapcycletracker.h"

#include "lua.h"

#include "lauxlib.h"
#include "luasoplibs.h"

#include "tier0/dbg.h"

static int game_consolecommand(lua_State *L)
{
    const char *pCommand = luaL_checkstring(L, 1);

    engine->ServerCommand(UTIL_VarArgs("%s\n", pCommand));
    return 0;
}

static int game_curtime(lua_State *L)
{
    lua_pushnumber(L, gpGlobals->curtime);
    return 1;
}

static int game_fireevent(lua_State *L)
{
    const char *eventName = luaL_checkstring(L, 1);
    IGameEvent *event = gameeventmanager->CreateEvent(eventName);

    if(!event)
        return 0;

    int t = lua_gettop(L);
    // table is in the stack at index `t'
    lua_pushnil(L);  // first key
    while (lua_next(L, t) != 0)
    {
        // `key' is at index -2 and `value' at index -1
        const char *key = lua_tolstring(L, -2, NULL);
        int valType = lua_type(L, -1);

        if(valType == LUA_TBOOLEAN)
        {
            event->SetBool(key, lua_toboolean(L, -1) != 0);
        }
        if(valType == LUA_TNUMBER)
        {
            int intnum = lua_tointeger(L, -1);
            double dblnum = lua_tonumber(L, -1);

            if(intnum == dblnum)
            {
                event->SetInt(key, intnum);
            }
            else
            {
                event->SetFloat(key, dblnum);
            }
        }
        else if(valType == LUA_TSTRING)
        {
            const char *val = lua_tolstring(L, -1, NULL);
            if(val)
            {
                event->SetString(key, val);
            }
        }
        lua_pop(L, 1);  // removes `value'; keeps `key' for next iteration
    }

    gameeventmanager->FireEvent( event );
    return 0;
}

static int game_getlevelfromcycle(lua_State *L)
{
    char levelname[32];
    int next = luaL_optinteger(L, 1, 0);

    pAdminOP.MapCycleTracker()->GetNextLevelName(levelname, sizeof(levelname), next);
    lua_pushstring(L, levelname);
    return 1;
}

static int game_getmap(lua_State *L)
{
    lua_pushstring(L, pAdminOP.CurrentMap());
    return 1;
}

static int game_getmapnext(lua_State *L)
{
    lua_pushstring(L, pAdminOP.NextMap());
    return 1;
}

static int game_gettimelimit(lua_State *L)
{
    lua_pushinteger(L, pAdminOP.GetTimeLimit());
    return 1;
}

static int game_gettimeremaining(lua_State *L)
{
    lua_pushinteger(L, pAdminOP.GetMapTimeRemaining());
    return 1;
}

static int game_iscstrike(lua_State *L)
{
    lua_pushboolean(L, pAdminOP.isCstrike);
    return 1;
}

static int game_isdod(lua_State *L)
{
    lua_pushboolean(L, pAdminOP.isDod);
    return 1;
}

static int game_istf2(lua_State *L)
{
    lua_pushboolean(L, pAdminOP.isTF2);
    return 1;
}

static int game_loadnextmap(lua_State *L)
{
    if(pAdminOP.GameRules() && pAdminOP.GameRules()->IsInitialized())
    {
        pAdminOP.GameRules()->ChangeLevel();
    }
    return 0;
}

static int game_replicateconvar(lua_State *L)
{
    const char *var = luaL_checkstring(L, 1);
    const char *value = luaL_checkstring(L, 2);

    pAdminOP.ReplicateCVar(var, value);

    return 0;
}

static int game_serverexecute(lua_State *L)
{
    engine->ServerExecute();
    return 0;
}

static const luaL_Reg gamelib[] = {
    {"ConsoleCommand",      game_consolecommand},
    {"CurTime",             game_curtime},
    {"FireEvent",           game_fireevent},
    {"GetLevelFromCycle",   game_getlevelfromcycle},
    {"GetMap",              game_getmap},
    {"GetMapNext",          game_getmapnext},
    {"GetTimeLimit",        game_gettimelimit},
    {"GetTimeRemaining",    game_gettimeremaining},
    {"IsCStrike",           game_iscstrike},
    {"IsDOD",               game_isdod},
    {"IsTF2",               game_istf2},
    {"LoadNextMap",         game_loadnextmap},
    {"ReplicateConVar",     game_replicateconvar},
    {"ServerExecute",       game_serverexecute},
    {NULL, NULL}
};


/*
** Open library
*/
LUALIB_API int luaopen_game (lua_State *L) {
  luaL_register(L, LUA_GAMELIBNAME, gamelib);
  return 1;
}
