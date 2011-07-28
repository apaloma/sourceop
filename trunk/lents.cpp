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

#include "vfuncs.h"

#include "lua.h"

#include "lauxlib.h"
#include "luasoplibs.h"

#include "lunar.h"
#include "l_class_entity.h"
#include "l_class_vector.h"
#include "l_class_physobj.h"
#include "lentitycache.h"

#include "tier0/dbg.h"

LUALIB_API int luaL_checkboolean (lua_State *L, int narg);
LUALIB_API int luaL_optbool (lua_State *L, int narg, int def);

static int ents_create(lua_State *L)
{
    const char *classname = luaL_checkstring(L, 1);
    CBaseEntity *pEnt = CreateEntityByName( classname );
    if(pEnt)
    {
        g_entityCache.PushEntity(VFuncs::entindex(pEnt));
        return 1;
    }
    lua_pushnil(L);
    return 1;
}

static int ents_findinsphere(lua_State *L)
{
    SOPVector *sopcenter = Lunar<SOPVector>::check(L, 1);
    Vector center = sopcenter->ToVector();
    lua_Number radius = luaL_checknumber(L, 2);
    bool physonly = luaL_optbool(L, 3, 0) != 0;

    CBaseEntity *pEntity = NULL;
    int i = 0;
    lua_newtable(L);
    int idx = lua_gettop(L);
    while((pEntity = gEntList.FindEntityInSphere(pEntity, center, radius)) != NULL)
    {
        if(physonly && VFuncs::VPhysicsGetObject(pEntity) == NULL) continue;

        int entindex = VFuncs::entindex(pEntity);
        i++;

        lua_pushinteger(L, i);
        g_entityCache.PushEntity(entindex);
        lua_settable(L, idx);
    }
    return 1;
}

static int ents_findbyclass(lua_State *L)
{
    const char *classname = luaL_checkstring(L, 1);

    CBaseEntity *pEntity = NULL;
    int i = 0;
    lua_newtable(L);
    int idx = lua_gettop(L);
    while((pEntity = gEntList.FindEntityByClassname(pEntity, classname)) != NULL)
    {
        int entindex = VFuncs::entindex(pEntity);
        i++;
        lua_pushinteger(L, i);

        g_entityCache.PushEntity(entindex);
        lua_settable(L, idx);
    }
    return 1;
}

static int ents_findbyname(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);

    CBaseEntity *pEntity = NULL;
    int i = 0;
    lua_newtable(L);
    int idx = lua_gettop(L);
    while((pEntity = gEntList.FindEntityByName(pEntity, name)) != NULL)
    {
        int entindex = VFuncs::entindex(pEntity);
        i++;
        lua_pushinteger(L, i);

        g_entityCache.PushEntity(entindex);
        lua_settable(L, idx);
    }
    return 1;
}

static int ents_findbytarget(lua_State *L)
{
    const char *target = luaL_checkstring(L, 1);

    CBaseEntity *pEntity = NULL;
    int i = 0;
    lua_newtable(L);
    int idx = lua_gettop(L);
    while((pEntity = gEntList.FindEntityByTarget(pEntity, target)) != NULL)
    {
        int entindex = VFuncs::entindex(pEntity);
        i++;
        lua_pushinteger(L, i);

        g_entityCache.PushEntity(entindex);
        lua_settable(L, idx);
    }
    return 1;
}

static int ents_getbyindex(lua_State *L)
{
    int index = luaL_checkinteger(L, 1);

    if(pAdminOP.GetEntity(index))
    {
        g_entityCache.PushEntity(index);
    }
    else
    {
        lua_pushnil(L);
    }
    return 1;
}


static const luaL_Reg entslib[] = {
    {"Create",          ents_create},
    {"FindInSphere",    ents_findinsphere},
    {"FindByClass",     ents_findbyclass},
    {"FindByName",      ents_findbyname},
    {"FindByTarget",    ents_findbytarget},
    {"GetByIndex",      ents_getbyindex},
    {NULL, NULL}
};


/*
** Open library
*/
LUALIB_API int luaopen_ents (lua_State *L) {
  luaL_register(L, LUA_ENTSLIBNAME, entslib);
  return 1;
}
