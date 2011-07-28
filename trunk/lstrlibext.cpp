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

#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"
#include "luasoplibs.h"

#include "strtools.h"

#include "tier0/dbg.h"

static int string_fixslashes(lua_State *L)
{
    const char *str = luaL_checkstring(L, 1);
    char fixedstr[1024];

    V_strncpy(fixedstr, str, sizeof(fixedstr));
    V_FixSlashes(fixedstr);
    
    lua_checkstack(L, 1);
    lua_pushstring(L, fixedstr);
    return 1;
}

static int string_strtrim(lua_State *L)
{
    int front = 0, back;
    const char *str = luaL_checklstring(L, 1, (size_t *)(&back)); /* Should probably change front & back to long long in case there is ever a string longer than 2GiB */
    const char *del = luaL_optstring(L, 2, "\t\n\r ");
    --back;

    while (front <= back && strchr(del, str[front]))
        ++front;
    while (back > front && strchr(del, str[back]))
        --back;
    
    lua_checkstack(L, 1);
    lua_pushlstring(L, &str[front], back - front + 1);
    return 1;
}

/* strsplit & strjoin adapted from code by Norganna */
static int string_strsplit(lua_State *L)
{
    const char *sep = luaL_checkstring(L, 1);
    const char *str = luaL_checkstring(L, 2);
    int limit = luaL_optint(L, 3, 0);
    int count = 0;
    /* Set the stack to a predictable size */
    lua_settop(L, 0);
    /* Initialize the result count */
    /* Tokenize the string */
    if(!limit || limit > 1) {
        const char *end = str;
        while(*end) {
            int issep = 0;
            const char *s = sep;
            for(; *s; ++s) {
                if(*s == *end) {
                    issep = 1;
                    break;
                }
            }
            if(issep) {
                lua_checkstack(L, count+1);
                lua_pushlstring(L, str, (end-str));
                ++count;
                str = end+1;
                if(count == (limit-1)) {
                    break;
                }
            }
            ++end;
        }
    }
    /* Add the remainder */
    lua_checkstack(L, count+1);
    lua_pushstring(L, str);
    ++count;
    /* Return with the number of values found */
    return count;
}

static int string_strjoin(lua_State *L)
{
    int seplen;
    const char *sep = luaL_checklstring(L, 1, (size_t *)(&seplen));
    /* Get the top of the stack */
    int top = lua_gettop(L);
    /* Work out how many entries we need to concatenate */
    int entries = top - 1;
    int i;
    /* If there's no seperator, then this is the same as a concat */
    if(seplen == 0) {
        lua_concat(L, top);
        return 1;
    }
    /* If there's no entries then we can't concatenate anything */
    if(entries == 0) {
        lua_pushstring(L, "");
        return 1;
    }
    /* Make sure there's enough room for the entries, (entries-1) seperators, and 1 seperator */
    lua_checkstack(L, 2*entries);
    /* Foreach entry (except the last one) */
    for(i = 1; i < entries; ++i) {
        /* Push the seperator onto the stack and then move it into position */
        lua_pushlstring(L, sep, seplen);
        lua_insert(L, i*2 + 1);
    }
    /* Concatenate the entries + (entries-1) seperators in the stack, together */
    lua_concat(L, 2*entries - 1);
    return 1;
}

static int string_strconcat(lua_State *L)
{
    lua_concat(L, lua_gettop(L));
    return 1;
}

static const luaL_Reg stringext_funcs[] = {
  {"FixSlashes", string_fixslashes},
  {"strtrim", string_strtrim},
  {"strsplit", string_strsplit},
  {"strjoin", string_strjoin},
  {"strconcat", string_strconcat},
  {NULL, NULL}
};


LUALIB_API int luaopen_stringext (lua_State *L) {
  luaL_register(L, LUA_STRLIBNAME, stringext_funcs);
  return 1;
}
