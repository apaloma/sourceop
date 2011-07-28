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

#ifndef LSOPLIBS_H
#define LSOPLIBS_H

#include "lua.h"

#define LUA_BITLIBNAME          "bit"
LUALIB_API int (luaopen_bit) (lua_State *L);

#define LUA_CONCOMMANDLIBNAME   "concommand"
LUALIB_API int (luaopen_concommand) (lua_State *L);

#define LUA_CONSTRAINTLIBNAME   "constraint"
LUALIB_API int (luaopen_constraint) (lua_State *L);

#define LUA_CONVARLIBNAME       "convar"
LUALIB_API int (luaopen_convar) (lua_State *L);

#define LUA_EFFECTSLIBNAME      "effects"
LUALIB_API int (luaopen_effects) (lua_State *L);

#define LUA_ENGINELIBNAME       "engine"
LUALIB_API int (luaopen_engine) (lua_State *L);

#define LUA_ENTSLIBNAME         "ents"
LUALIB_API int (luaopen_ents) (lua_State *L);

#define LUA_FILELIBNAME         "file"
LUALIB_API int (luaopen_file) (lua_State *L);

#define LUA_GAMELIBNAME         "game"
LUALIB_API int (luaopen_game) (lua_State *L);

#define LUA_GAMERULESLIBNAME    "gamerules"
LUALIB_API int (luaopen_gamerules) (lua_State *L);

//                              "os"
LUALIB_API int (luaopen_osext) (lua_State *L);

#define LUA_PLAYERLIBNAME       "player"
LUALIB_API int (luaopen_player) (lua_State *L);

#define LUA_SOURCEOPLIBNAME     "sourceop"
LUALIB_API int (luaopen_sourceop) (lua_State *L);

//                              "string"
LUALIB_API int (luaopen_stringext) (lua_State *L);

//                              "table"
LUALIB_API int (luaopen_tableext) (lua_State *L);

#define LUA_TEMPENTLIBNAME      "tempent"
LUALIB_API int (luaopen_tempent) (lua_State *L);

#define LUA_UTILLIBNAME         "util"
LUALIB_API int (luaopen_util) (lua_State *L);


/* open all previous libraries */
LUALIB_API void (luaL_opensoplibs) (lua_State *L); 

#endif
