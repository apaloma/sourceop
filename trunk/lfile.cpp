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

#include "AdminOP.h"

#include "lua.h"

#include "lauxlib.h"
#include "luasoplibs.h"

#include "tier0/dbg.h"

static int file_append(lua_State *L)
{
    char filepath[512];
    const char *file = luaL_checkstring(L, 1);
    const char *str = luaL_checkstring(L, 2);

    Q_snprintf(filepath, sizeof(filepath), "%s/%s/%s", pAdminOP.GameDir(), pAdminOP.DataDir(), file);
    V_FixSlashes(filepath);

    FILE *fp = fopen(filepath, "at");
    if(fp)
    {
        fputs(str, fp);
        fclose(fp);
    }
    return 0;
}

static int file_read(lua_State *L)
{
    char buf[4096];
    char filepath[512];
    const char *file = luaL_checkstring(L, 1);

    Q_snprintf(filepath, sizeof(filepath), "%s/%s/%s", pAdminOP.GameDir(), pAdminOP.DataDir(), file);
    V_FixSlashes(filepath);

    FILE *fp = fopen(filepath, "rt");
    if(fp)
    {
        int top = lua_gettop(L);
        int pushes = 0;
        int len;

        while(!feof(fp))
        {
            if(len = fread(buf, 1, sizeof(buf), fp))
            {
                luaL_checkstack(L, 1, "file read");
                lua_pushlstring(L, buf, len);
                pushes++;
            }
        }
        fclose(fp);

        if(pushes)
        {
            lua_concat(L, pushes);
            return 1;
        }
    }

    lua_pushstring(L, "");
    return 1;
}

static int file_write(lua_State *L)
{
    char filepath[512];
    const char *file = luaL_checkstring(L, 1);
    const char *str = luaL_checkstring(L, 2);

    Q_snprintf(filepath, sizeof(filepath), "%s/%s/%s", pAdminOP.GameDir(), pAdminOP.DataDir(), file);
    V_FixSlashes(filepath);

    FILE *fp = fopen(filepath, "wt");
    if(fp)
    {
        fputs(str, fp);
        fclose(fp);
    }
    return 0;
}


static const luaL_Reg filelib[] = {
    {"Append",  file_append},
    {"Read",    file_read},
    {"Write",   file_write},
    {NULL, NULL}
};


/*
** Open library
*/
LUALIB_API int luaopen_file (lua_State *L) {
  luaL_register(L, LUA_FILELIBNAME, filelib);
  return 1;
}
