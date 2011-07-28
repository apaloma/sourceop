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
#include "gamerulesproxy.h"

#include "lua.h"

#include "lauxlib.h"
#include "luasoplibs.h"

LUALIB_API int luaL_checkboolean (lua_State *L, int narg);
LUALIB_API int luaL_optbool (lua_State *L, int narg, int def);

static int gamerules_advancemapcycle(lua_State *L)
{
    if(pAdminOP.GameRules() && pAdminOP.GameRules()->IsInitialized())
    {
        pAdminOP.GameRules()->AdvanceMapCycle();
    }
    return 0;
}

static int gamerules_changelevel(lua_State *L)
{
    if(pAdminOP.GameRules() && pAdminOP.GameRules()->IsInitialized())
    {
        pAdminOP.GameRules()->ChangeLevel();
    }
    return 0;
}

static int gamerules_getindexedteamname(lua_State *L)
{
    int team = luaL_checkinteger(L, 1);

    if(pAdminOP.GameRules() && pAdminOP.GameRules()->IsInitialized())
    {
        const char *pszTeamName = pAdminOP.GameRules()->GetIndexedTeamName(team);
        if(pszTeamName && strlen(pszTeamName) > 0)
        {
            lua_pushstring(L, pszTeamName);
        }
        else
        {
            lua_pushnil(L);
        }
    }
    else
    {
        lua_pushnil(L);
    }
    return 1;
}

static int gamerules_getteamindex(lua_State *L)
{
    size_t l;
    const char *s = luaL_checklstring(L, 1, &l);

    if(pAdminOP.GameRules() && pAdminOP.GameRules()->IsInitialized())
    {
        lua_pushinteger(L, pAdminOP.GameRules()->GetTeamIndex(s));
    }
    else
    {
        lua_pushnil(L);
    }
    return 1;
}

static int gamerules_handleswitchteams(lua_State *L)
{
    if(pAdminOP.GameRules() && pAdminOP.GameRules()->IsInitialized())
    {
        pAdminOP.GameRules()->HandleSwitchTeams();
    }
    return 0;
}

static int gamerules_isvalidteamnumber(lua_State *L)
{
    if(lua_isnumber(L, 1))
    {
        int team = luaL_checknumber(L, 1);
        if(team >= MIN_TEAM_NUM && pAdminOP.GameRules() && pAdminOP.GameRules()->IsInitialized())
        {
            // if it's not empty string, then the team is valid
            if(strcmp(pAdminOP.GameRules()->GetIndexedTeamName(team), ""))
            {
                lua_pushboolean(L, 1);
                return 1;
            }
        }
    }
    lua_pushboolean(L, 0);
    return 1;
}

static int gamerules_setscrambleteams(lua_State *L)
{
    if(pAdminOP.GameRules() && pAdminOP.GameRules()->IsInitialized())
    {
        bool bScramble = luaL_checkboolean(L, 2) != 0;
        pAdminOP.GameRules()->SetScrambleTeams(bScramble);
    }
    return 0;
}

static int gamerules_setstalemate(lua_State *L)
{
    if(pAdminOP.GameRules() && pAdminOP.GameRules()->IsInitialized())
    {
        int iReason = luaL_checknumber(L, 1);
        bool bForceMapReset = luaL_checkboolean(L, 2) != 0;
        pAdminOP.GameRules()->SetStalemate(iReason, bForceMapReset, false);
    }
    return 0;
}

static int gamerules_setswitchteams(lua_State *L)
{
    if(pAdminOP.GameRules() && pAdminOP.GameRules()->IsInitialized())
    {
        bool bSwitch = luaL_checkboolean(L, 2) != 0;
        pAdminOP.GameRules()->SetSwitchTeams(bSwitch);
    }
    return 0;
}


static const luaL_Reg gameruleslib[] = {
    {"AdvanceMapCycle",     gamerules_advancemapcycle},
    {"ChangeLevel",         gamerules_changelevel},
    {"GetIndexedTeamName",  gamerules_getindexedteamname},
    {"GetTeamIndex",        gamerules_getteamindex},
    {"HandleSwitchTeams",   gamerules_handleswitchteams},
    {"IsValidTeamNumber",   gamerules_isvalidteamnumber},
    {"SetScrambleTeams",    gamerules_setscrambleteams},
    {"SetStalemate",        gamerules_setstalemate},
    {"SetSwitchTeams",    gamerules_setscrambleteams},
    {NULL, NULL}
};


/*
** Open library
*/
LUALIB_API int luaopen_gamerules (lua_State *L) {
  luaL_register(L, LUA_GAMERULESLIBNAME, gameruleslib);
  return 1;
}
