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

#include "lua.h"

#include "lualib.h"
#include "luasoplibs.h"
#include "lauxlib.h"


static const luaL_Reg luasoplibs[] = {
  {LUA_BITLIBNAME, luaopen_bit},
  {LUA_CONCOMMANDLIBNAME, luaopen_concommand},
  {LUA_CONSTRAINTLIBNAME, luaopen_constraint},
  {LUA_CONVARLIBNAME, luaopen_convar},
  {LUA_EFFECTSLIBNAME, luaopen_effects},
  {LUA_ENGINELIBNAME, luaopen_engine},
  {LUA_ENTSLIBNAME, luaopen_ents},
  {LUA_FILELIBNAME, luaopen_file},
  {LUA_GAMELIBNAME, luaopen_game},
  {LUA_GAMERULESLIBNAME, luaopen_gamerules},
  {LUA_OSLIBNAME, luaopen_osext},
  {LUA_PLAYERLIBNAME, luaopen_player},
  {LUA_SOURCEOPLIBNAME, luaopen_sourceop},
  {LUA_STRLIBNAME, luaopen_stringext},
  {LUA_TABLIBNAME, luaopen_tableext},
  {LUA_TEMPENTLIBNAME, luaopen_tempent},
  {LUA_UTILLIBNAME, luaopen_util},
  {NULL, NULL}
};


LUALIB_API void luaL_opensoplibs (lua_State *L) {
  const luaL_Reg *lib = luasoplibs;
  for (; lib->func; lib++) {
    lua_pushcfunction(L, lib->func);
    lua_pushstring(L, lib->name);
    lua_call(L, 1, 0);
  }
}
