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

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define lua_c

#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"
#include "luasoplibs.h"

// HACK HACK HACK : BASEENTITY ACCESS
#define GAME_DLL
#include "cbase.h"
#include "baseentity.h"
// HACK HACK HACK : BASEENTITY ACCESS
#include "in_buttons.h"

#include "AdminOP.h"
#include "gamerulesproxy.h"
#include "LuaLoader.h"
#include "l_class_entity.h"
#include "l_class_player.h"
#include "l_class_vector.h"
#include "l_class_angle.h"
#include "l_class_physobj.h"
#include "l_class_damageinfo.h"

#include "tier0/memdbgon.h"


lua_State *globalL = NULL;

const char *progname = "[SOURCEOP] lua";
char lastscriptpath[512];



static void lstop (lua_State *L, lua_Debug *ar) {
    (void)ar;  /* unused arg. */
    lua_sethook(L, NULL, 0, 0);
    luaL_error(L, "interrupted!");
}


static void laction (int i) {
    signal(i, SIG_DFL); /* if another SIGINT happens before lstop,
                        terminate process (default action) */
    lua_sethook(globalL, lstop, LUA_MASKCALL | LUA_MASKRET | LUA_MASKCOUNT, 1);
}


void l_message (const char *pname, const char *msg) {
    if (pname) CAdminOP::ColorMsg(CONCOLOR_LUA, "%s: ", pname);
    CAdminOP::ColorMsg(CONCOLOR_LUA, "%s\n", msg);
}


static int report (lua_State *L, int status) {
    if (status && !lua_isnil(L, -1)) {
        const char *msg = lua_tostring(L, -1);
        if (msg == NULL) msg = "(error object is not a string)";
        l_message(progname, msg);
        lua_pop(L, 1);
    }
    return status;
}


static int traceback (lua_State *L) {
    lua_getfield(L, LUA_GLOBALSINDEX, "debug");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return 1;
    }
    lua_getfield(L, -1, "traceback");
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 2);
        return 1;
    }
    lua_pushvalue(L, 1);  /* pass error message */
    lua_pushinteger(L, 2);  /* skip this function and traceback */
    lua_call(L, 2, 1);  /* call debug.traceback */
    return 1;
}


static int docall (lua_State *L, int narg, int clear) {
    int status;
    int base = lua_gettop(L) - narg;  /* function index */
    lua_pushcfunction(L, traceback);  /* push traceback function */
    lua_insert(L, base);  /* put it under chunk and args */
    signal(SIGINT, laction);
    status = lua_pcall(L, narg, (clear ? 0 : LUA_MULTRET), base);
    signal(SIGINT, SIG_DFL);
    lua_remove(L, base);  /* remove traceback function */
    /* force a complete garbage collection in case of errors */
    if (status != 0) lua_gc(L, LUA_GCCOLLECT, 0);
    return status;
}


static void print_version (void) {
    l_message(NULL, LUA_RELEASE "  " LUA_COPYRIGHT);
}

static int handle_script (lua_State *L, const char *script) {
    int status;
    status = luaL_loadfile(L, script);
    if (status == 0)
        status = docall(L, 0, 0); 
    return report(L, status);
}

struct Smain {
    char *script;
    int status;
};


static int lua_include(lua_State *L)
{
    char filepath[512];
    const char *script = lua_tostring(L, 1);

    if(strstr(script, "..") || strstr(script, ":") || strstr(script, "%") || script[0] == '\\' || script[0] == '/')
    {
        CAdminOP::ColorMsg(CONCOLOR_LUA_ERR, "invalid include path `%s'\n", script);
        return 0;
    }

    Q_snprintf(filepath, sizeof(filepath), "%s/%s", lastscriptpath, script);
    V_FixSlashes(filepath);

    return handle_script(L, filepath);
}

static int lua_Msg(lua_State *L) {
    const char *msg = lua_tostring(L, 1);
    if(msg) CAdminOP::ColorMsg(CONCOLOR_LUA, msg);
    return 0;
}

static int lua_Error(lua_State *L) {
    const char *msg = lua_tostring(L, 1);
    if(msg)
    {
        CAdminOP::ColorMsg(CONCOLOR_LUA_ERR, msg);
        pAdminOP.TimeLog("SourceOPLuaErrors.log", "%s", msg);
    }
    return 0;
}

static int lua_CurTime(lua_State *L) {
    lua_pushnumber(L, gpGlobals->curtime);
    return 1;
}

static int pmain (lua_State *L) {
    struct Smain *s = (struct Smain *)lua_touserdata(L, 1);
    char *script = s->script;
    globalL = L;

    s->status = handle_script(L, script);

    return s->status;
}

#define lua_pushglobalinteger(x) \
    lua_pushstring(L, #x); \
    lua_pushinteger(L, x); \
    lua_settable(L, LUA_GLOBALSINDEX)

#define lua_pushglobalinteger_renamed(x, name) \
    lua_pushstring(L, name); \
    lua_pushinteger(L, x); \
    lua_settable(L, LUA_GLOBALSINDEX)


lua_State *luaopen (void) {
    lua_State *L = lua_open();  /* create state */
    if (L == NULL) {
        l_message(progname, "cannot create state: not enough memory");
        return NULL;
    }

    lua_gc(L, LUA_GCSTOP, 0);  /* stop collector during initialization */
    luaL_openlibs(L);  /* open libraries */
    luaL_opensoplibs(L); /* 0.o */
    lua_SOPAngle_register(L);
    lua_SOPVector_register(L);
    lua_SOPDamageInfo_register(L);
    lua_SOPEntity_register(L);
    lua_SOPPlayer_register(L);
    lua_SOPPhysObj_register(L);
    lua_register(L, "include", lua_include);
    lua_register(L, "Msg", lua_Msg);
    lua_register(L, "Error", lua_Error);
    lua_register(L, "CurTime", lua_CurTime);

    /*lua_pushstring(L, "LUAVERSION");
    lua_pushstring(L, LUA_RELEASE "  " LUA_COPYRIGHT);
    lua_settable(L, LUA_GLOBALSINDEX);*/
    lua_pushglobalinteger(HUD_PRINTNOTIFY);
    lua_pushglobalinteger(HUD_PRINTCONSOLE);
    lua_pushglobalinteger(HUD_PRINTTALK);
    lua_pushglobalinteger(HUD_PRINTCENTER);

    lua_pushglobalinteger(MASK_ALL);
    lua_pushglobalinteger(MASK_SOLID);
    lua_pushglobalinteger(MASK_PLAYERSOLID);
    lua_pushglobalinteger(MASK_NPCSOLID);
    lua_pushglobalinteger(MASK_WATER);
    lua_pushglobalinteger(MASK_OPAQUE);
    lua_pushglobalinteger(MASK_OPAQUE_AND_NPCS);
    lua_pushglobalinteger(MASK_BLOCKLOS);
    lua_pushglobalinteger(MASK_BLOCKLOS_AND_NPCS);
    lua_pushglobalinteger(MASK_VISIBLE);
    lua_pushglobalinteger(MASK_VISIBLE_AND_NPCS);
    lua_pushglobalinteger(MASK_SHOT);
    lua_pushglobalinteger(MASK_SHOT_HULL);
    lua_pushglobalinteger(MASK_SHOT_PORTAL);
    lua_pushglobalinteger(MASK_SOLID_BRUSHONLY);
    lua_pushglobalinteger(MASK_PLAYERSOLID_BRUSHONLY);
    lua_pushglobalinteger(MASK_NPCSOLID_BRUSHONLY);
    lua_pushglobalinteger(MASK_NPCWORLDSTATIC);
    lua_pushglobalinteger(MASK_SPLITAREAPORTAL);
    lua_pushglobalinteger(MASK_CURRENT);
    lua_pushglobalinteger(MASK_DEADSOLID);

    lua_pushglobalinteger(SOLID_NONE);
    lua_pushglobalinteger(SOLID_BSP);
    lua_pushglobalinteger(SOLID_BBOX);
    lua_pushglobalinteger(SOLID_OBB);
    lua_pushglobalinteger(SOLID_OBB_YAW);
    lua_pushglobalinteger(SOLID_CUSTOM);
    lua_pushglobalinteger(SOLID_VPHYSICS);
    lua_pushglobalinteger(SOLID_LAST);

    lua_pushglobalinteger(COLLISION_GROUP_NONE);
    lua_pushglobalinteger(COLLISION_GROUP_DEBRIS);
    lua_pushglobalinteger(COLLISION_GROUP_DEBRIS_TRIGGER);
    lua_pushglobalinteger(COLLISION_GROUP_INTERACTIVE_DEBRIS);
    lua_pushglobalinteger(COLLISION_GROUP_INTERACTIVE);
    lua_pushglobalinteger(COLLISION_GROUP_PLAYER);
    lua_pushglobalinteger(COLLISION_GROUP_BREAKABLE_GLASS);
    lua_pushglobalinteger(COLLISION_GROUP_VEHICLE);
    lua_pushglobalinteger(COLLISION_GROUP_PLAYER_MOVEMENT);
    lua_pushglobalinteger(COLLISION_GROUP_NPC);
    lua_pushglobalinteger(COLLISION_GROUP_IN_VEHICLE);
    lua_pushglobalinteger(COLLISION_GROUP_WEAPON);
    lua_pushglobalinteger(COLLISION_GROUP_VEHICLE_CLIP);
    lua_pushglobalinteger(COLLISION_GROUP_PROJECTILE);
    lua_pushglobalinteger(COLLISION_GROUP_DOOR_BLOCKER);
    lua_pushglobalinteger(COLLISION_GROUP_PASSABLE_DOOR);
    lua_pushglobalinteger(COLLISION_GROUP_DISSOLVING);
    lua_pushglobalinteger(COLLISION_GROUP_PUSHAWAY);
    lua_pushglobalinteger(COLLISION_GROUP_NPC_ACTOR);
    lua_pushglobalinteger(COLLISION_GROUP_NPC_SCRIPTED);
    lua_pushglobalinteger_renamed(COLLISION_GROUP_DEBRIS, "COLLISION_GROUP_WORLD");

    lua_pushglobalinteger(EF_BONEMERGE);
    lua_pushglobalinteger(EF_BRIGHTLIGHT);
    lua_pushglobalinteger(EF_DIMLIGHT);
    lua_pushglobalinteger(EF_NOINTERP);
    lua_pushglobalinteger(EF_NOSHADOW);
    lua_pushglobalinteger(EF_NODRAW);
    lua_pushglobalinteger(EF_NORECEIVESHADOW);
    lua_pushglobalinteger(EF_BONEMERGE_FASTCULL);
    lua_pushglobalinteger(EF_ITEM_BLINK);
    lua_pushglobalinteger(EF_PARENT_ANIMATES);

    lua_pushglobalinteger(FL_ONGROUND);
    lua_pushglobalinteger(FL_DUCKING);
    lua_pushglobalinteger(FL_WATERJUMP);
    lua_pushglobalinteger(FL_ONTRAIN);
    lua_pushglobalinteger(FL_INRAIN);
    lua_pushglobalinteger(FL_FROZEN);
    lua_pushglobalinteger(FL_ATCONTROLS);
    lua_pushglobalinteger(FL_FAKECLIENT);
    lua_pushglobalinteger(FL_INWATER);
    lua_pushglobalinteger(FL_FLY);
    lua_pushglobalinteger(FL_SWIM);
    lua_pushglobalinteger(FL_CONVEYOR);
    lua_pushglobalinteger(FL_NPC);
    lua_pushglobalinteger(FL_GODMODE);
    lua_pushglobalinteger(FL_NOTARGET);
    lua_pushglobalinteger(FL_AIMTARGET);
    lua_pushglobalinteger(FL_PARTIALGROUND);
    lua_pushglobalinteger(FL_STATICPROP);
    lua_pushglobalinteger(FL_GRAPHED);
    lua_pushglobalinteger(FL_GRENADE);
    lua_pushglobalinteger(FL_STEPMOVEMENT);
    lua_pushglobalinteger(FL_DONTTOUCH);
    lua_pushglobalinteger(FL_BASEVELOCITY);
    lua_pushglobalinteger(FL_WORLDBRUSH);
    lua_pushglobalinteger(FL_OBJECT);
    lua_pushglobalinteger(FL_KILLME);
    lua_pushglobalinteger(FL_ONFIRE);
    lua_pushglobalinteger(FL_DISSOLVING);
    lua_pushglobalinteger(FL_TRANSRAGDOLL);
    lua_pushglobalinteger(FL_UNBLOCKABLE_BY_PLAYER);

    lua_pushglobalinteger(MOVETYPE_NONE);
    lua_pushglobalinteger(MOVETYPE_ISOMETRIC);
    lua_pushglobalinteger(MOVETYPE_WALK);
    lua_pushglobalinteger(MOVETYPE_STEP);
    lua_pushglobalinteger(MOVETYPE_FLY);
    lua_pushglobalinteger(MOVETYPE_FLYGRAVITY);
    lua_pushglobalinteger(MOVETYPE_VPHYSICS);
    lua_pushglobalinteger(MOVETYPE_PUSH);
    lua_pushglobalinteger(MOVETYPE_NOCLIP);
    lua_pushglobalinteger(MOVETYPE_LADDER);
    lua_pushglobalinteger(MOVETYPE_OBSERVER);
    lua_pushglobalinteger(MOVETYPE_CUSTOM);

    lua_pushglobalinteger(FEAT_CREDITS);
    lua_pushglobalinteger(FEAT_ADMINCOMMANDS);
    lua_pushglobalinteger(FEAT_ENTCOMMANDS);
    lua_pushglobalinteger(FEAT_ADMINSAYCOMMANDS);
    lua_pushglobalinteger(FEAT_PLAYERSAYCOMMANDS);
    lua_pushglobalinteger(FEAT_KILLSOUNDS);
    lua_pushglobalinteger(FEAT_MAPVOTE);
    lua_pushglobalinteger(FEAT_CVARVOTE);
    lua_pushglobalinteger(FEAT_REMOTE);
    lua_pushglobalinteger(FEAT_HOOK);
    lua_pushglobalinteger(FEAT_JETPACK);
    lua_pushglobalinteger(FEAT_SNARK);
    lua_pushglobalinteger(FEAT_RADIO);
    lua_pushglobalinteger(FEAT_LUA);

    lua_pushglobalinteger(FCVAR_NONE);
    lua_pushglobalinteger(FCVAR_UNREGISTERED);
    lua_pushglobalinteger(FCVAR_DEVELOPMENTONLY);
    lua_pushglobalinteger(FCVAR_GAMEDLL);
    lua_pushglobalinteger(FCVAR_CLIENTDLL);
    lua_pushglobalinteger(FCVAR_HIDDEN);
    lua_pushglobalinteger(FCVAR_PROTECTED);
    lua_pushglobalinteger(FCVAR_SPONLY);
    lua_pushglobalinteger(FCVAR_ARCHIVE);
    lua_pushglobalinteger(FCVAR_NOTIFY);
    lua_pushglobalinteger(FCVAR_USERINFO);
    lua_pushglobalinteger(FCVAR_CHEAT);
    lua_pushglobalinteger(FCVAR_PRINTABLEONLY);
    lua_pushglobalinteger(FCVAR_NEVER_AS_STRING);
    lua_pushglobalinteger(FCVAR_REPLICATED);
    lua_pushglobalinteger(FCVAR_DEMO);
    lua_pushglobalinteger(FCVAR_DONTRECORD);
    lua_pushglobalinteger(FCVAR_NOT_CONNECTED);
    lua_pushglobalinteger(FCVAR_ARCHIVE_XBOX);
    lua_pushglobalinteger(FCVAR_SERVER_CAN_EXECUTE);
    lua_pushglobalinteger(FCVAR_SERVER_CANNOT_QUERY);
    lua_pushglobalinteger(FCVAR_CLIENTCMD_CAN_EXECUTE);

    lua_pushglobalinteger(STALEMATE_JOIN_MID);
    lua_pushglobalinteger(STALEMATE_TIMER);
    lua_pushglobalinteger(STALEMATE_SERVER_TIMELIMIT);
    lua_pushglobalinteger(NUM_STALEMATE_REASONS);

    lua_pushglobalinteger(DMG_GENERIC);
    lua_pushglobalinteger(DMG_CRUSH);
    lua_pushglobalinteger(DMG_BULLET);
    lua_pushglobalinteger(DMG_SLASH);
    lua_pushglobalinteger(DMG_BURN);
    lua_pushglobalinteger(DMG_VEHICLE);
    lua_pushglobalinteger(DMG_FALL);
    lua_pushglobalinteger(DMG_BLAST);
    lua_pushglobalinteger(DMG_CLUB);
    lua_pushglobalinteger(DMG_SHOCK);
    lua_pushglobalinteger(DMG_SONIC);
    lua_pushglobalinteger(DMG_ENERGYBEAM);
    lua_pushglobalinteger(DMG_PREVENT_PHYSICS_FORCE);
    lua_pushglobalinteger(DMG_NEVERGIB);
    lua_pushglobalinteger(DMG_ALWAYSGIB);
    lua_pushglobalinteger(DMG_DROWN);
    lua_pushglobalinteger(DMG_PARALYZE);
    lua_pushglobalinteger(DMG_NERVEGAS);
    lua_pushglobalinteger(DMG_POISON);
    lua_pushglobalinteger(DMG_RADIATION);
    lua_pushglobalinteger(DMG_DROWNRECOVER);
    lua_pushglobalinteger(DMG_ACID);
    lua_pushglobalinteger(DMG_SLOWBURN);
    lua_pushglobalinteger(DMG_REMOVENORAGDOLL);
    lua_pushglobalinteger(DMG_PHYSGUN);
    lua_pushglobalinteger(DMG_PLASMA);
    lua_pushglobalinteger(DMG_AIRBOAT);
    lua_pushglobalinteger(DMG_DISSOLVE);
    lua_pushglobalinteger(DMG_BLAST_SURFACE);
    lua_pushglobalinteger(DMG_DIRECT);
    lua_pushglobalinteger(DMG_BUCKSHOT);
    lua_pushglobalinteger(DMG_LASTGENERICFLAG);

    lua_pushglobalinteger_renamed((1<<24), "DMG_CUSTOM_TF2_IGNITE");

    lua_pushglobalinteger(eQueryCvarValueStatus_ValueIntact);
    lua_pushglobalinteger(eQueryCvarValueStatus_CvarNotFound);
    lua_pushglobalinteger(eQueryCvarValueStatus_NotACvar);
    lua_pushglobalinteger(eQueryCvarValueStatus_CvarProtected);

    lua_pushglobalinteger(IN_ATTACK);
    lua_pushglobalinteger(IN_JUMP);
    lua_pushglobalinteger(IN_DUCK);
    lua_pushglobalinteger(IN_FORWARD);
    lua_pushglobalinteger(IN_BACK);
    lua_pushglobalinteger(IN_USE);
    lua_pushglobalinteger(IN_CANCEL);
    lua_pushglobalinteger(IN_LEFT);
    lua_pushglobalinteger(IN_RIGHT);
    lua_pushglobalinteger(IN_MOVELEFT);
    lua_pushglobalinteger(IN_MOVERIGHT);
    lua_pushglobalinteger(IN_ATTACK2);
    lua_pushglobalinteger(IN_RUN);
    lua_pushglobalinteger(IN_RELOAD);
    lua_pushglobalinteger(IN_ALT1);
    lua_pushglobalinteger(IN_ALT2);
    lua_pushglobalinteger(IN_SCORE);
    lua_pushglobalinteger(IN_SPEED);
    lua_pushglobalinteger(IN_WALK);
    lua_pushglobalinteger(IN_ZOOM);
    lua_pushglobalinteger(IN_WEAPON1);
    lua_pushglobalinteger(IN_WEAPON2);
    lua_pushglobalinteger(IN_BULLRUSH);
    lua_pushglobalinteger(IN_GRENADE1);
    lua_pushglobalinteger(IN_GRENADE2);

    lua_pushglobalinteger(TF2_CLASS_SCOUT);
    lua_pushglobalinteger(TF2_CLASS_SNIPER);
    lua_pushglobalinteger(TF2_CLASS_SOLDIER);
    lua_pushglobalinteger(TF2_CLASS_DEMOMAN);
    lua_pushglobalinteger(TF2_CLASS_MEDIC);
    lua_pushglobalinteger(TF2_CLASS_HEAVY);
    lua_pushglobalinteger(TF2_CLASS_PYRO);
    lua_pushglobalinteger(TF2_CLASS_SPY);
    lua_pushglobalinteger(TF2_CLASS_ENGINEER);
    lua_pushglobalinteger(TF2_CLASS_CIVILIAN);

    lua_pushglobalinteger(OBS_MODE_NONE);
    lua_pushglobalinteger(OBS_MODE_DEATHCAM);
    lua_pushglobalinteger(OBS_MODE_FREEZECAM);
    lua_pushglobalinteger(OBS_MODE_FIXED);
    lua_pushglobalinteger(OBS_MODE_IN_EYE);
    lua_pushglobalinteger(OBS_MODE_CHASE);
    lua_pushglobalinteger(OBS_MODE_ROAMING);
    lua_pushglobalinteger(NUM_OBSERVER_MODES);

    lua_gc(L, LUA_GCRESTART, 0);

    luaexec(L, "package.path =  string.FixSlashes(sourceop.FullPathToDataDir() .. \"\\\\lua\\\\includes\\\\modules\\\\?.lua;\") .. package.path");

    return L;
}

void luaexec(lua_State *L, const char *pszLua)
{
    luaL_loadstring(L, pszLua);
    lua_call(L, 0, 0);
}

int luascript (lua_State *L, char *script) {
    int status;
    struct Smain s;
    if(L == NULL) return 0;
    s.script = script;
    strncpy(lastscriptpath, script, sizeof(lastscriptpath));
    V_StripFilename(lastscriptpath);
    status = lua_cpcall(L, &pmain, &s);
    report(L, status);
    return (status || s.status) ? 0 : 1;
}
