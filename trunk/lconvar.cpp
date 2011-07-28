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
#ifdef _L4D_PLUGIN
#include "convar_l4d.h"
#endif
#include "cbase.h"
#include "baseentity.h"
#include "player.h"
// HACK HACK HACK : BASEENTITY ACCESS

#include "AdminOP.h"
#include "cvars.h"

#include "lua.h"

#include "lauxlib.h"
#include "lstate.h"
#include "lobject.h"
#include "luasoplibs.h"

#include "tier0/dbg.h"
#include "tier0/memdbgon.h"

class CLuaCvar
{
    char name[256];
    char defaultval[256];
    int varflags;
    char helpstring[512];
    bool hasmin;
    float minval;
    bool hasmax;
    float maxval;

    lua_State *L;
    ConVar *convar;

public:
    CLuaCvar(lua_State *state, const char *pName, const char *pDefaultValue, int flags = 0, const char *pHelpString = NULL, bool bMin = false, float fMin = 0, bool bMax = false, float fMax = 0)
    {
        L = state;
        convar = NULL;

        V_strncpy(name, pName, sizeof(name));
        V_strncpy(defaultval, pDefaultValue, sizeof(name));
        varflags = flags;
        hasmin = bMin;
        minval = fMin;
        hasmax = bMax;
        maxval = fMax;

        if(pHelpString)
        {
            V_strncpy(helpstring, pHelpString, sizeof(helpstring));
        }
        else
        {
            lua_Debug ar;

            // default description
            strcpy(helpstring, "Lua ConVar added by SourceOP.");

            // or make a better one if one is available
            if (lua_getstack(L, 1, &ar)) {  // check function at level
                lua_getinfo(L, "Sln", &ar);  // get info about it
                if (ar.currentline > 0) {   // is there info?
                    if (ar.name) {
                        sprintf(helpstring, "Lua ConVar added by SourceOP (%s:%d in function %s).", V_UnqualifiedFileName(ar.short_src), ar.currentline, ar.name);
                    } else {
                        sprintf(helpstring, "Lua ConVar added by SourceOP (%s:%d).", V_UnqualifiedFileName(ar.short_src), ar.currentline);
                    }
                }
            }
        }
        Init();
    }
    ConVar *Init()
    {
        if(!convar)
            convar = new ConVar(name, defaultval, varflags, helpstring, hasmin, minval, hasmax, maxval);
        return convar;
    }
    inline const char *GetName()
    {
        return name;
    }
    void Unregister()
    {
        if(convar)
        {
            g_metamodOverrides->UnregisterConCommand(convar);
            delete convar;
            convar = NULL;
        }
    }
    ~CLuaCvar()
    {
        Unregister();
    }
};

CUtlVector <CLuaCvar *> newCvars; 

TValue *index2adr (lua_State *L, int idx) {
  if (idx > 0) {
    TValue *o = L->base + (idx - 1);
    api_check(L, idx <= L->ci->top - L->base);
    if (o >= L->top) return cast(TValue *, luaO_nilobject);
    else return o;
  }
  else if (idx > LUA_REGISTRYINDEX) {
    api_check(L, idx != 0 && -idx <= L->top - L->base);
    return L->top + idx;
  }
  else switch (idx) {  /* pseudo-indices */
    case LUA_REGISTRYINDEX: return registry(L);
    case LUA_ENVIRONINDEX: {
      Closure *func = curr_func(L);
      sethvalue(L, &L->env, func->c.env);
      return &L->env;
    }
    case LUA_GLOBALSINDEX: return gt(L);
    default: {
      Closure *func = curr_func(L);
      idx = LUA_GLOBALSINDEX - idx;
      return (idx <= func->c.nupvalues)
                ? &func->c.upvalue[idx-1]
                : cast(TValue *, luaO_nilobject);
    }
  }
}

LUALIB_API int luaL_checkboolean (lua_State *L, int narg) {
  const TValue *o = index2adr(L, narg);
  if (!ttisboolean(o) && !ttisnil(o))
    luaL_typerror(L, narg, lua_typename(L, LUA_TBOOLEAN));
  return !l_isfalse(o);
}

LUALIB_API int luaL_optbool (lua_State *L, int narg, int def) {
  if (lua_isnone(L, narg)) {
    return def;
  }
  else return luaL_checkboolean(L, narg);
}

static int convar_add(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);
    const char *defaultval = luaL_checkstring(L, 2);
    int varflags = luaL_optint(L, 3, FCVAR_NONE);
    const char *helpstring = luaL_optstring(L, 4, NULL);
    int hasmin = luaL_optbool(L, 5, 0);
    float minval = luaL_optnumber(L, 6, 0);
    int hasmax = luaL_optbool(L, 7, 0);
    float maxval = luaL_optnumber(L, 8, 0);

    CLuaCvar *newCvar = new CLuaCvar(L, name, defaultval, varflags, helpstring, hasmin != 0, minval, hasmax != 0, maxval);
    newCvars.AddToTail(newCvar);

    return 0;
}

static int convar_exists(lua_State *L)
{
    const char *szCmd = luaL_checkstring(L, 1);

    ConVar *cvCvar = cvar->FindVar(szCmd);
    if(cvCvar && !cvCvar->IsCommand())
    {
        lua_pushboolean(L, true);
        return 1;
    }
    lua_pushboolean(L, false);
    return 1;
}

static int convar_getbool(lua_State *L)
{
    const char *szCmd = luaL_checkstring(L, 1);

    ConVar *cvCvar = cvar->FindVar(szCmd);
    if(cvCvar && !cvCvar->IsCommand())
    {
        lua_pushboolean(L, cvCvar->GetBool());
        return 1;
    }
    lua_pushboolean(L, false);
    return 1;
}

static int convar_getinteger(lua_State *L)
{
    const char *szCmd = luaL_checkstring(L, 1);

    ConVar *cvCvar = cvar->FindVar(szCmd);
    if(cvCvar && !cvCvar->IsCommand())
    {
        lua_pushinteger(L, cvCvar->GetInt());
        return 1;
    }
    lua_pushnil(L);
    return 1;
}


static int convar_getnumber(lua_State *L)
{
    const char *szCmd = luaL_checkstring(L, 1);

    ConVar *cvCvar = cvar->FindVar(szCmd);
    if(cvCvar && !cvCvar->IsCommand())
    {
        lua_pushnumber(L, cvCvar->GetFloat());
        return 1;
    }
    lua_pushnil(L);
    return 1;
}

static int convar_getstring(lua_State *L)
{
    const char *szCmd = luaL_checkstring(L, 1);

    ConVar *cvCvar = cvar->FindVar(szCmd);
    if(cvCvar && !cvCvar->IsCommand())
    {
        lua_pushstring(L, cvCvar->GetString());
        return 1;
    }
    lua_pushnil(L);
    return 1;
}

static int convar_remove(lua_State *L)
{
    const char *name = luaL_checkstring(L, 1);

    for(int i = 0; i < newCvars.Count(); i++)
    {
        CLuaCvar *cvar = newCvars[i];
        if(!strcmp(cvar->GetName(), name))
        {
            cvar->Unregister();
            newCvars.Remove(i);
            delete cvar;
            return 0;
        }
    }
    lua_pushstring(L, "convar.Remove did not find the specified variable\n");
    lua_error(L);
    return 0;
}

static int convar_removeall(lua_State *L)
{
    for(int i = 0; i < newCvars.Count(); i++)
    {
        CLuaCvar *cvar = newCvars[i];
        cvar->Unregister();
        delete cvar;
    }
    newCvars.RemoveAll();
    return 0;
}

static int convar_setinteger(lua_State *L)
{
    const char *szCmd = luaL_checkstring(L, 1);
    int iValue = luaL_checkinteger(L, 2);

    ConVar *cvCvar = cvar->FindVar(szCmd);
    if(cvCvar && !cvCvar->IsCommand())
    {
        cvCvar->SetValue(iValue);
    }
    return 0;
}

static int convar_setnumber(lua_State *L)
{
    const char *szCmd = luaL_checkstring(L, 1);
    float flValue = luaL_checknumber(L, 2);

    ConVar *cvCvar = cvar->FindVar(szCmd);
    if(cvCvar && !cvCvar->IsCommand())
    {
        cvCvar->SetValue(flValue);
    }
    return 0;
}

static int convar_setstring(lua_State *L)
{
    const char *szCmd = luaL_checkstring(L, 1);
    const char *szValue = luaL_checkstring(L, 2);

    ConVar *cvCvar = cvar->FindVar(szCmd);
    if(cvCvar && !cvCvar->IsCommand())
    {
        cvCvar->SetValue(szValue);
    }
    return 0;
}

static const luaL_Reg convarlib[] = {
    {"Add",         convar_add},
    {"Exists",      convar_exists},
    {"GetBool",     convar_getbool},
    {"GetInteger",  convar_getinteger},
    {"GetNumber",   convar_getnumber},
    {"GetString",   convar_getstring},
    {"Remove",      convar_remove},
    {"RemoveAll",   convar_removeall},
    {"SetInteger",  convar_setinteger},
    {"SetNumber",   convar_setnumber},
    {"SetString",   convar_setstring},
    {NULL, NULL}
};


/*
** Open library
*/
LUALIB_API int luaopen_convar (lua_State *L) {
  luaL_register(L, LUA_CONVARLIBNAME, convarlib);
  return 1;
}
