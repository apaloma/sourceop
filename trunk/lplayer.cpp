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

#include "lentitycache.h"

#include "tier0/dbg.h"

static int player_find(lua_State *L)
{
    int playerList[128];
    int playerCount = 0;

    size_t l;
    const char *s = luaL_checklstring(L, 1, &l);

    playerCount = pAdminOP.FindPlayer(playerList, s);

    lua_newtable(L);
    int idx = lua_gettop(L);

    if(l > 0)
    {
        for(int i = 0; i < playerCount; i++)
        {
            lua_pushinteger(L, i+1);
            g_entityCache.PushPlayer(playerList[i]);
            lua_settable(L, idx);
        }
    }

    return 1;
}

static int player_getall(lua_State *L)
{
    lua_newtable(L);
    int idx = lua_gettop(L);

    for(int i = 0; i < pAdminOP.GetMaxClients(); i++)
    {
        if(pAdminOP.pAOPPlayers[i].GetPlayerState() >= 2)
        {
            lua_pushinteger(L, i+1);
            g_entityCache.PushPlayer(i+1);
            lua_settable(L, idx);
        }
    }

    return 1;
}

static int player_getbyid(lua_State *L)
{
    int index = luaL_checkinteger(L, 1);

    if(index > 0 && index <= pAdminOP.GetMaxClients())
    {
        g_entityCache.PushPlayer(index);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}

static int player_getbyuserid(lua_State *L)
{
    int iPlayer = luaL_checkinteger(L, 1);

    if(iPlayer > 0)
    {
        int playerList[128];
        int playerCount = 0;

        playerCount = pAdminOP.FindPlayerByUserID(playerList, iPlayer);
        if(playerCount == 1)
        {
            g_entityCache.PushPlayer(playerList[0]);
            return 1;
        }
    }
    lua_pushnil(L);
    return 1;
}


static const luaL_Reg playerlib[] = {
    {"Find",        player_find},
    {"GetAll",      player_getall},
    {"GetByID",     player_getbyid},
    {"GetByUserID", player_getbyuserid},
    {NULL, NULL}
};


/*
** Open library
*/
LUALIB_API int luaopen_player (lua_State *L) {
  luaL_register(L, LUA_PLAYERLIBNAME, playerlib);
  return 1;
}
