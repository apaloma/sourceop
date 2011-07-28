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

#include "fixdebug.h"

#ifdef _L4D_PLUGIN
#include "convar_l4d.h"
#endif

#include "utlvector.h"
#include "recipientfilter.h"

// HACK HACK HACK : BASEENTITY ACCESS
#define GAME_DLL
#include "cbase.h"
#include "baseentity.h"
#include "basegrenade_shared.h"
#include "player.h"
// HACK HACK HACK : BASEENTITY ACCESS

#include "vmatrix.h"
#include "util.h"
#include "vphysics_interface.h"
#include "vcollide_parse.h"

#include <stdio.h>
#include <time.h>

#include "AdminOP.h"
#include "sourcehooks.h"
#include "bitbuf.h"
#include "CAOPEntity.h"
#include "CAOPLuaNormal.h"
#include "vfuncs.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "lunar.h"
#include "l_class_entity.h"
#include "lentitycache.h"

#include "tier0/memdbgon.h"

static ConVar lua_entity_thinkfreq("DF_lua_entity_thinkfreq", "0.1", 0, "How often Lua entities think in seconds.");

BEGIN_DATADESC( CAOPLuaNormal )
END_DATADESC()

SOP_LINK_ENTITY_TO_CLASS_FEAT(sop_luaent, CAOPLuaNormal, FEAT_LUA);

static CUtlMap<const char *, int> g_LuaEnts;

class CSOPLuaEntityStringPool : public CStringPool, public CAutoGameSystem
{
    virtual char const *Name() { return "CSOPLuaEntityStringPool"; }

    virtual void Shutdown() 
    {
        FreeAll();
    }
};

static CSOPLuaEntityStringPool g_LuaEntityStringPool;

void AddLuaNormalEntity(const char *pszName, int reference)
{
    static bool bSetLessFunc = false;
    if(!bSetLessFunc)
    {
        //SetDefLessFunc( g_LuaEnts );
        g_LuaEnts.SetLessFunc(CaselessStringLessThan);
        bSetLessFunc = true;
    }
    g_LuaEnts.Insert(g_LuaEntityStringPool.Allocate(pszName), reference);
    AddEntityToInstallList(&sop_luaent, pszName, FEAT_LUA);
}

CAOPLuaNormal::CAOPLuaNormal()
{
    m_iRef = LUA_REFNIL;
    m_LuaEnt = NULL;
}

CAOPLuaNormal::~CAOPLuaNormal()
{
    if(m_iRef != LUA_REFNIL)
    {
        lua_unref(pAdminOP.GetLuaState(), m_iRef);
    }
}

void CAOPLuaNormal::PostConstructor( const char *szClassname )
{
    int pos = g_LuaEnts.Find(szClassname);
    if(g_LuaEnts.IsValidIndex(pos))
    {
        lua_State *L = pAdminOP.GetLuaState();

        const char *szPooledClassname = g_LuaEntityStringPool.Find(szClassname);
        if(!szPooledClassname)
        {
            pAdminOP.ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Pooled classname for lua entity %s not found.\n", szClassname);
            szPooledClassname = g_LuaEntityStringPool.Allocate(szClassname);
        }
        m_LuaEnt = new SOPEntity(GetIndex(), szPooledClassname);
        Lunar<SOPEntity>::push(L, m_LuaEnt, true, szPooledClassname);
        m_iRef = lua_ref(L, true);
        m_szClassname = szPooledClassname;
    }
    else
    {
        if(!FStrEq(szClassname, "sop_luaent"))
        {
            CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] No Lua ref for classname %s\n", szClassname);
        }
        m_iRef = LUA_REFNIL;
        m_szClassname = "UNKNOWN";
    }
}

void CAOPLuaNormal::Spawn()
{
    CBaseEntity *pBase = GetBase();
    VFuncs::SetHealth(pBase, BIG_HEALTH);

    m_flNextThink = gpGlobals->curtime;
    MakeMyThink(&CAOPLuaNormal::Think);
    pAdminOP.myThinkEnts.AddToTail((CAOPEntity *)this);
    MakeTouch(&CAOPLuaNormal::Touch);

    if(m_iRef != LUA_REFNIL)
    {
        lua_State *L = pAdminOP.GetLuaState();

        lua_getref(L, m_iRef);
        if(Lunar<SOPEntity>::call(L, "Initialize", m_szClassname) != 0)
            CAdminOP::ColorMsg(CONCOLOR_LUA_ERR, "[SOURCEOP] Lua: error running entity function `%s:Initialize':\n %s\n", m_szClassname, lua_tostring(L, -1));

        RETURN_META(MRES_SUPERCEDE);
    }
    RETURN_META(MRES_IGNORED);
}

void CAOPLuaNormal::Precache()
{
    if(m_iRef != LUA_REFNIL)
    {
        lua_State *L = pAdminOP.GetLuaState();

        lua_getref(L, m_iRef);
        int r = Lunar<SOPEntity>::call(L, "Precache", m_szClassname);
        if(r != 0 && r != -2)
            CAdminOP::ColorMsg(CONCOLOR_LUA_ERR, "[SOURCEOP] Lua: error running entity function `%s:Precache':\n %s\n", m_szClassname, lua_tostring(L, -1));
    }
}

void CAOPLuaNormal::Think()
{
    bool bOverrideThink = false;
    if(m_iRef != LUA_REFNIL)
    {
        lua_State *L = pAdminOP.GetLuaState();

        double startTime = Plat_FloatTime();
        lua_getref(L, m_iRef);
        int r = Lunar<SOPEntity>::call(L, "Think", m_szClassname);
        if(r == 0 || r == 1)
        {
            bool ret;
            if(r == 0)
            {
                ret = false;
            }
            else
            {
                ret = (lua_toboolean(L, -1) != 0);
                lua_pop(L, 1);
            }

            bOverrideThink = ret;
        }
        else
        {
            CAdminOP::ColorMsg(CONCOLOR_LUA_ERR, "[SOURCEOP] Lua: error running entity function `%s:Think':\n %s\n", m_szClassname, lua_tostring(L, -1));
        }
        double time = ( Plat_FloatTime() - startTime ) * 1000.0f;
        if(time > 20.0f)
        {
            Msg("[SOURCEOP] Lua: '%s' entity thinking for %.02f ms!\n", m_szClassname, time); 
        }
    }
    if(!bOverrideThink)
    {
        m_flNextThink = gpGlobals->curtime + lua_entity_thinkfreq.GetFloat();
    }
}

void CAOPLuaNormal::Touch(CBaseEntity *pOther)
{
    if(m_iRef != LUA_REFNIL)
    {
        lua_State *L = pAdminOP.GetLuaState();
        int otherindex = VFuncs::entindex(pOther);

        lua_getref(L, m_iRef);
        g_entityCache.PushEntity(otherindex);
        if(Lunar<SOPEntity>::call(L, "Touch", m_szClassname, 1) != 0)
            CAdminOP::ColorMsg(CONCOLOR_LUA_ERR, "[SOURCEOP] Lua: error running entity function `%s:Touch':\n %s\n", m_szClassname, lua_tostring(L, -1));
    }
}

bool CAOPLuaNormal::AcceptInput( const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t Value, int outputID )
{
    if(m_iRef != LUA_REFNIL)
    {
        lua_State *L = pAdminOP.GetLuaState();
        int actindex = pActivator ? VFuncs::entindex(pActivator) : -1;
        int callindex = pCaller ? VFuncs::entindex(pCaller) : -1;

        lua_getref(L, m_iRef);
        lua_pushstring(L, szInputName);
        if(actindex > -1)
            g_entityCache.PushEntity(actindex);
        else
            lua_pushnil(L);
        if(callindex > -1)
            g_entityCache.PushEntity(callindex);
        else
            lua_pushnil(L);
        int r = Lunar<SOPEntity>::call(L, "AcceptInput", m_szClassname, 3);
        if(r == 0)
        {
            bool r = (lua_toboolean(L, -1) != 0);
            lua_pop(L, 1);

            if(r)
            {
                RETURN_META_VALUE(MRES_SUPERCEDE, true);
            }
        }
        else if(r != -2)
        {
            CAdminOP::ColorMsg(CONCOLOR_LUA_ERR, "[SOURCEOP] Lua: error running entity function `%s:AcceptInput':\n %s\n", m_szClassname, lua_tostring(L, -1));
        }
    }
    RETURN_META_VALUE(MRES_IGNORED, false);
}

void CAOPLuaNormal::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
    if(m_iRef != LUA_REFNIL)
    {
        lua_State *L = pAdminOP.GetLuaState();
        int actindex = VFuncs::entindex(pActivator);
        int callindex = VFuncs::entindex(pCaller);

        g_entityCache.PushEntity(actindex);
        g_entityCache.PushEntity(callindex);
        if(Lunar<SOPEntity>::call(L, "Use", m_szClassname) != 0)
            CAdminOP::ColorMsg(CONCOLOR_LUA_ERR, "[SOURCEOP] Lua: error running entity function `%s:Use':\n %s\n", m_szClassname, lua_tostring(L, -1));

        RETURN_META(MRES_SUPERCEDE);
    }
    BaseClass::Use(pActivator, pCaller, useType, value);
}
