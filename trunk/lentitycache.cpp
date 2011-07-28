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
#include "lualib.h"

#include "lunar.h"
#include "l_class_entity.h"
#include "l_class_physobj.h"
#include "l_class_player.h"

#include "lentitycache.h"

CEntityCache g_entityCache;

CEntityCache::CEntityCache()
{
    cacheMap.RemoveAll();
    cacheMap.SetLessFunc(DefLessFunc(int));
    physCacheMap.RemoveAll();
    physCacheMap.SetLessFunc(DefLessFunc(int));
    playerCacheMap.RemoveAll();
    playerCacheMap.SetLessFunc(DefLessFunc(int));
}

void CEntityCache::PushEntity(int index)
{
    int cachehit = cacheMap.Find(index);
    if(cacheMap.IsValidIndex(cachehit))
    {
        int ref = cacheMap.Element(cachehit);
        lua_getref(pAdminOP.GetLuaState(), ref);
        if(lua_isnil(pAdminOP.GetLuaState(), -1))
        {
            Msg("Warning null cache hit: %i\n", index);
        }
    }
    else
    {
        int ref;
        //Msg("Cache miss: %i\n", index);

        // make new SOPEntity
        Lunar<SOPEntity>::push(pAdminOP.GetLuaState(), new SOPEntity(index), true);
        ref = lua_ref(pAdminOP.GetLuaState(), true);
        lua_getref(pAdminOP.GetLuaState(), ref);
        cacheMap.Insert(index, ref);
    }
}

void CEntityCache::PushPhysObj(int index)
{
    int cachehit = physCacheMap.Find(index);
    if(physCacheMap.IsValidIndex(cachehit))
    {
        int ref = physCacheMap.Element(cachehit);
        lua_getref(pAdminOP.GetLuaState(), ref);
        if(lua_isnil(pAdminOP.GetLuaState(), -1))
        {
            Msg("Warning null physobj cache hit: %i\n", index);
        }
    }
    else
    {
        int ref;
        //Msg("Physobj cache miss: %i\n", index);

        // make new SOPPhysObj
        Lunar<SOPPhysObj>::push(pAdminOP.GetLuaState(), new SOPPhysObj(index), true);
        ref = lua_ref(pAdminOP.GetLuaState(), true);
        lua_getref(pAdminOP.GetLuaState(), ref);
        physCacheMap.Insert(index, ref);
    }
}

void CEntityCache::PushPlayer(int index)
{
    int cachehit = playerCacheMap.Find(index);
    if(playerCacheMap.IsValidIndex(cachehit))
    {
        int ref = playerCacheMap.Element(cachehit);
        lua_getref(pAdminOP.GetLuaState(), ref);
        //Msg("Cache HIT player: %i\n", index);
        if(lua_isnil(pAdminOP.GetLuaState(), -1))
        {
            Msg("Warning null cache hit: %i\n", index);
        }
    }
    else
    {
        int ref;
        //Msg("Cache miss player: %i\n", index);

        // make new SOPPlayer
        Lunar<SOPPlayer>::push(pAdminOP.GetLuaState(), new SOPPlayer(index), true);
        ref = lua_ref(pAdminOP.GetLuaState(), true);
        lua_getref(pAdminOP.GetLuaState(), ref);
        playerCacheMap.Insert(index, ref);
    }
}

void CEntityCache::EntityRemoved(int index)
{
    int cachehit = cacheMap.Find(index);
    if(cacheMap.IsValidIndex(cachehit))
    {
        int ref = cacheMap.Element(cachehit);
        if(ref != LUA_REFNIL)
        {
            //Msg("Removing reference to cached entity %i\n", index);
            lua_unref(pAdminOP.GetLuaState(), ref);
        }
        cacheMap.RemoveAt(cachehit);
    }

    int physCacheHit = physCacheMap.Find(index);
    if(physCacheMap.IsValidIndex(physCacheHit))
    {
        int ref = physCacheMap.Element(physCacheHit);
        if(ref != LUA_REFNIL)
        {
            //Msg("Removing reference to cached phys object %i\n", index);
            lua_unref(pAdminOP.GetLuaState(), ref);
        }
        physCacheMap.RemoveAt(physCacheHit);
    }
}

void CEntityCache::PlayerDisconnected(int index)
{
    int cachehit = playerCacheMap.Find(index);
    if(playerCacheMap.IsValidIndex(cachehit))
    {
        int ref = playerCacheMap.Element(cachehit);
        if(ref != LUA_REFNIL)
        {
            //Msg("Removing reference to cached player %i\n", index);
            lua_unref(pAdminOP.GetLuaState(), ref);
        }
        playerCacheMap.RemoveAt(cachehit);
    }
}

void CEntityCache::InitCache()
{
    cacheMap.RemoveAll();
    physCacheMap.RemoveAll();
    playerCacheMap.RemoveAll();
}

void CEntityCache::ClearCache()
{
    InitCache();
}
