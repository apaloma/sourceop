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
#include "cvars.h"

#include "lua.h"

#include "lauxlib.h"
#include "luasoplibs.h"

#include "l_class_entity.h"
#include "l_class_player.h"

#include "tier0/dbg.h"

static int sourceop_adddownloadable(lua_State *L)
{
    size_t filelen;
    const char *file = luaL_checklstring(L, 1, &filelen);
    // TODO: Optimize this to only load the table once
    INetworkStringTable *pDownloadablesTable = networkstringtable->FindTable("downloadables");
    if(pDownloadablesTable)
    {
        pDownloadablesTable->AddString(true, file, filelen);
    }
    return 0;
}

static int sourceop_addgameeventlistener(lua_State *L)
{
    const char *eventName = luaL_checkstring(L, 1);

    // TODO: L4D, put back event listeners
#ifndef _L4D_PLUGIN
    gameeventmanager->AddListener( &g_eventListener, eventName, true );
#endif
    return 0;
}

static int sourceop_addspawnedent(lua_State *L)
{
    SOPEntity *entity = Lunar<SOPEntity>::check(L, 1);

    pAdminOP.spawnedServerEnts.AddToTail(entity->EntIndex());
    return 0;
}

static int sourceop_canspawn(lua_State *L)
{
    lua_pushboolean(L, spawnlimit_server.GetInt() == -1 || pAdminOP.spawnedServerEnts.Count() < spawnlimit_server.GetInt());
    return 1;
}

static int sourceop_datadir(lua_State *L)
{
    lua_pushstring(L, pAdminOP.GameDir());
    return 1;
}

static int sourceop_featurestatus(lua_State *L)
{
    int feat = luaL_checkinteger(L, 1);

    lua_pushboolean(L, pAdminOP.FeatureStatus(feat));
    return 1;
}

static int sourceop_fullpathtodatadir(lua_State *L)
{
    char path[1024];

    V_snprintf(path, sizeof(path), "%s/%s", pAdminOP.GameDir(), pAdminOP.DataDir());
    V_FixSlashes(path);
    lua_pushstring(L, path);
    return 1;
}

static int sourceop_getcommandprefix(lua_State *L)
{
    lua_pushstring(L, command_prefix.GetString());
    return 1;
}

static int sourceop_getpropoffset(lua_State *L)
{
    const char *table = luaL_checkstring(L, 1);
    const char *propname = luaL_checkstring(L, 2);
    lua_pushinteger(L, pAdminOP.GetPropOffset(table, propname));
    return 1;
}

static int sourceop_getspawnedcount(lua_State *L)
{
    lua_pushinteger(L, pAdminOP.spawnedServerEnts.Count());
    return 1;
}

static int sourceop_playsoundall(lua_State *L)
{
    const char *pText = luaL_checkstring(L, 1);
    
    pAdminOP.PlaySoundAll(pText);
    return 0;
}

static int sourceop_saytextall(lua_State *L)
{
    const char *pText = luaL_checkstring(L, 1);
    int playerIndex = luaL_optinteger(L, 2, 0);
    
    pAdminOP.SetSayTextPlayerIndex(playerIndex);
    pAdminOP.SayTextAllChatHud(UTIL_VarArgs("[%s] %s", pAdminOP.adminname, pText));
    return 0;
}


static const luaL_Reg sourceoplib[] = {
    {"AddDownloadable",     sourceop_adddownloadable},
    {"AddGameEventListener",sourceop_addgameeventlistener},
    {"AddSpawnedEnt",       sourceop_addspawnedent},
    {"CanSpawn",            sourceop_canspawn},
    {"DataDir",             sourceop_datadir},
    {"FeatureStatus",       sourceop_featurestatus},
    {"FullPathToDataDir",   sourceop_fullpathtodatadir},
    {"GetCommandPrefix",    sourceop_getcommandprefix},
    {"GetPropOffset",       sourceop_getpropoffset},
    {"GetSpawnedCount",     sourceop_getspawnedcount},
    {"PlaySoundAll",        sourceop_playsoundall},
    {"SayTextAll",          sourceop_saytextall},
    {NULL, NULL}
};


/*
** Open library
*/
LUALIB_API int luaopen_sourceop (lua_State *L) {
  luaL_register(L, LUA_SOURCEOPLIBNAME, sourceoplib);
  return 1;
}
