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

#ifndef __linux__
#include <windows.h>
#include <time.h>

#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif
 
struct timezone 
{
  int  tz_minuteswest; /* minutes W of Greenwich */
  int  tz_dsttime;     /* type of dst correction */
};
 
int gettimeofday(struct timeval *tv, struct timezone *tz)
{
  FILETIME ft;
  unsigned __int64 tmpres = 0;
  static int tzflag;
 
  if (NULL != tv)
  {
    GetSystemTimeAsFileTime(&ft);
 
    tmpres |= ft.dwHighDateTime;
    tmpres <<= 32;
    tmpres |= ft.dwLowDateTime;
 
    /*converting file time to unix epoch*/
    tmpres /= 10;  /*convert into microseconds*/
    tmpres -= DELTA_EPOCH_IN_MICROSECS; 
    tv->tv_sec = (long)(tmpres / 1000000UL);
    tv->tv_usec = (long)(tmpres % 1000000UL);
  }
 
  if (NULL != tz)
  {
    if (!tzflag)
    {
      _tzset();
      tzflag++;
    }
    tz->tz_minuteswest = _timezone / 60;
    tz->tz_dsttime = _daylight;
  }
 
  return 0;
}
#else
#include <sys/time.h>
#include <time.h>
#endif

#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"
#include "luasoplibs.h"

static int os_gettimeofday (lua_State *L) {
    struct timeval now;

    gettimeofday(&now, NULL); 
    lua_pushnumber(L, (lua_Integer)now.tv_sec);
    lua_pushnumber(L, (lua_Integer)now.tv_usec);

    return 2;
}

static int os_microtime (lua_State *L) {
    struct timeval now;

    gettimeofday(&now, NULL); 
    lua_pushnumber(L, (lua_Number)now.tv_sec + (lua_Number)now.tv_usec / 1000000UL);

    return 1;
}

static const luaL_Reg osext_funcs[] = {
  {"gettimeofday",  os_gettimeofday},
  {"microtime",     os_microtime},
  {NULL, NULL}
};


LUALIB_API int luaopen_osext (lua_State *L) {
  luaL_register(L, LUA_OSLIBNAME, osext_funcs);
  return 1;
}
