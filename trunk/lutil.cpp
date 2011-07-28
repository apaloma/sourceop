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

#include "fixdebug.h"

#include <ctype.h>

// HACK HACK HACK : BASEENTITY ACCESS
#define GAME_DLL
#include "cbase.h"
#include "baseentity.h"
// HACK HACK HACK : BASEENTITY ACCESS

#include "AdminOP.h"

#include "lua.h"

#include "lauxlib.h"
#include "luasoplibs.h"

#include "tier0/dbg.h"

static int parse_table_keyvals(lua_State *L, KeyValues *keyVals)
{
    lua_newtable(L);
    int idx = lua_gettop(L);

    for ( KeyValues *kv = keyVals->GetFirstSubKey(); kv; kv = kv->GetNextKey() )
    {
        lua_pushstring(L, kv->GetName());

        if(kv->GetFirstSubKey())
        {
            parse_table_keyvals(L, kv);
        }
        else
        {
            const char *value = kv->GetString();

            int len = Q_strlen( value );
            const char* pSEnd = value + len ; // pos where token ends
            char* pIEnd; // pos where int scan ended

            int ival = strtol( value, &pIEnd, 10 );

            if(pSEnd == pIEnd)
            {
                lua_pushinteger(L, ival);
            }
            else
            {
                lua_pushstring(L, value);
            }
        }

        lua_settable(L, idx);
    }

    return idx;
}

static int util_keyvaluestotable(lua_State *L)
{
    const char *s = luaL_checklstring(L, 1, NULL);

    KeyValues *allKeyvals = new KeyValues("util_keyvaluestotable");
    allKeyvals->LoadFromBuffer("util_keyvaluestotable", s);
    parse_table_keyvals(L, allKeyvals);
    allKeyvals->deleteThis();

    return 1;
}

static int util_precachemodel(lua_State *L)
{
    const char *s = luaL_checklstring(L, 1, NULL);

    lua_pushinteger(L, engine->PrecacheModel(s));
    return 1;
}

static int util_precachesound(lua_State *L)
{
    const char *s = luaL_checklstring(L, 1, NULL);

    lua_pushboolean(L, enginesound->PrecacheSound(s, true));
    return 1;
}

static CUtlString *TableToKeyValues(lua_State *L, const char *tablename, int indent = 0)
{
    CUtlString *ret = new CUtlString();

    for(int i = 0; i < indent; i++)
        *ret += "\t";
    *ret += UTIL_VarArgs("\"%s\"\n", tablename);
    for(int i = 0; i < indent; i++)
        *ret += "\t";
    *ret += "{\n";

    int t = lua_gettop(L);
    // table is in the stack at index `t'
    lua_pushnil(L);  // first key
    while (lua_next(L, t) != 0) {
        // `key' is at index -2 and `value' at index -1
        const char *key = lua_tolstring(L, -2, NULL);

        if(lua_type(L, -1) == LUA_TTABLE)
        {
            CUtlString *recurse = TableToKeyValues(L, key, indent + 1);
            *ret += *recurse;
            delete recurse;
        }
        else
        {
            const char *value = lua_tolstring(L, -1, NULL);

            if(key && value)
            {
                // this could be up to twice as long
                int value2_size = (strlen(value) * 2) + 1;
                char *value2 = (char *)malloc(value2_size);
                char *value3 = (char *)malloc(value2_size);
                V_StrSubst(value, "\\", "\\\\", value2, value2_size);
                V_StrSubst(value2, "\"", "\\\"", value3, value2_size);
                V_StrSubst(value3, "\n", "\\n", value2, value2_size);
                V_StrSubst(value2, "\r", "\\r", value3, value2_size);

                for(int i = 0; i < indent + 1; i++)
                    *ret += "\t";
                *ret += UTIL_VarArgs("\"%s\" \"%s\"\n", key, value3);

                free(value2);
                free(value3);
            }
        }
        lua_pop(L, 1);  // removes `value'; keeps `key' for next iteration
    }

    for(int i = 0; i < indent; i++)
        *ret += "\t";
    *ret += "}\n";

    return ret;
}

static int util_tabletokeyvalues(lua_State *L)
{
    CUtlString *ret = TableToKeyValues(L, "0");
    lua_pushstring(L, ret->Get());
    delete ret;
    return 1;
}


static const luaL_Reg utillib[] = {
    {"KeyValuesToTable",    util_keyvaluestotable},
    {"PrecacheModel",       util_precachemodel},
    {"PrecacheSound",       util_precachesound},
    {"TableToKeyValues",    util_tabletokeyvalues},
    {NULL, NULL}
};


/*
** Open library
*/
LUALIB_API int luaopen_util (lua_State *L) {
  luaL_register(L, LUA_UTILLIBNAME, utillib);
  return 1;
}
