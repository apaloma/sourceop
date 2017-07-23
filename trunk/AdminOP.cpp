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
#ifdef _L4D_PLUGIN
#include "datamap_l4d.h"
#include "convar_l4d.h"
#include "dt_send_l4d.h"
#endif

#include "utlvector.h"
#include "recipientfilter.h"

#include "cbase.h"
#include "baseentity.h"
// HACK HACK HACK : BASEENTITY ACCESS

#define PVFN( classptr , offset ) ((*(DWORD*) classptr ) + offset) 
#define VFN( classptr , offset ) *(DWORD*)PVFN( classptr , offset ) 
#include "beam_flags.h" 

#include "vmatrix.h"
#include "util.h"
#include "vphysics_interface.h"
//#include "object_hash.h"
#include "viewport_panel_names.h"
#include "inetchannel.h"
#include "ivoiceserver.h"
#include "particle_parse.h"
#include "toolframework/itoolentity.h"
#include "cdll_int.h"

#include <stdio.h>
#include <ctype.h>

#include <time.h>
#ifndef __linux__
#include <IO.H>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifdef WIN32
#ifdef INVALID_HANDLE_VALUE
#undef INVALID_HANDLE_VALUE
#endif
#include <windows.h>
#undef GetClassName
#endif

// Steam
#include "steam/steam_api.h"

#include "AdminOP.h"
#include "sourcehooks.h"
#include "admincommands.h"
#include "sourceopadmin.h"
#include "recipientfilter.h"
#include "bitbuf.h"
#include "cvars.h"
#include "download.h"
#include "CAOPEntity.h"
#include "funcdetours.h"
#include "CDetourDis.h"

#include "mempatcher.h"
#include "vfuncs.h"
#include "sigmgr.h"
#include "clientcommands.h"
#include "reservedslots.h"
#include "mapcycletracker.h"
#include "gamerulesproxy.h"
#include "isopgamesystem.h"
#include "specialitems.h"
#include "tf2items.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "lunar.h"
#include "l_class_player.h"
#include "lentitycache.h"

#include "LuaLoader.h"

#include "tier0/memdbgon.h"

LUALIB_API int luaL_checkboolean (lua_State *L, int narg);
LUALIB_API int luaL_optbool (lua_State *L, int narg, int def);

char msg[2048];
ITempEntsSystem* te = NULL;
INetworkStringTable *g_pStringTableParticleEffectNames = NULL;

static int g_physgunBeam;

#ifdef CopyFile
#undef CopyFile
#endif

char *sTeamNamesHL2MP[] =
{
    "Unassigned",
    "Spectator",
    "Combine",
    "Rebels",
    UNKNOWN_TEAM_NAME,
    UNKNOWN_TEAM_NAME
};

char *sTeamNamesCS[] =
{
    "Unassigned",
    "Spectator",
    "Terrorist",
    "Counter-Terrorist",
    UNKNOWN_TEAM_NAME,
    UNKNOWN_TEAM_NAME
};

char *sTeamNamesDOD[] =
{
    "Unassigned",
    "Spectator",
    "Allies",
    "Axis",
    UNKNOWN_TEAM_NAME,
    UNKNOWN_TEAM_NAME
};

char *sTeamNamesFF[] =
{
    "Unassigned",
    "Spectator",
    "Blue",
    "Red",
    "Yellow",
    "Green"
};

char *sTeamNamesTF2[] =
{
    "Unassigned",
    "Spectator",
    "Red",
    "Blue",
    UNKNOWN_TEAM_NAME,
    UNKNOWN_TEAM_NAME
};

char *sTeamNamesDefault[] =
{
    "Unassigned",
    "Spectator",
    "Blue",
    "Red",
    UNKNOWN_TEAM_NAME"3",
    UNKNOWN_TEAM_NAME"4"
};

char linConColors[16][7] = {"22;30m", "22;34m", "22;32m", "22;36m", "22;31m", "22;35m", "22;33m", "22;37m",
                            "01;30m", "01;34m", "01;32m", "01;36m", "01;31m", "01;35m", "01;33m", "01;37m"};

short g_sModelIndexSmoke;
extern IEntityFactoryDictionary *g_entityFactoryDictionary;

bool hookedWorld = 0;
bool hookedEnts = 0;
int hooks;
int unhooks;
int baseserverconnectclienthook = 0;
int baseservergetfreeclienthook = 0;

// Now using VP hooks
CUtlVector<int> g_hookIDS;
CUtlVector<int> g_tempHookIDS;
CUtlVector<void *> g_hookedVTables;
SH_DECL_MANUALHOOK0_void(CBaseEntity_Spawn, 0, 0, 0);
SH_DECL_MANUALHOOK0_void(CBaseEntity_Precache, 0, 0, 0);
SH_DECL_MANUALHOOK5(CBaseEntity_AcceptInput, 0, 0, 0, bool, const char *, CBaseEntity *, CBaseEntity *, variant_t, int);
SH_DECL_MANUALHOOK1_void(CBaseEntity_Touch, 0, 0, 0, CBaseEntity*);
SH_DECL_MANUALHOOK1_void(CBaseEntity_StartTouch, 0, 0, 0, CBaseEntity*);
SH_DECL_MANUALHOOK0_void(CBaseEntity_Think, 0, 0, 0);
SH_DECL_MANUALHOOK2(CBaseEntity_KeyValue, 0, 0, 0, bool, const char *, const char *);
SH_DECL_MANUALHOOK4_void(CBaseEntity_Use, 0, 0, 0, CBaseEntity *, CBaseEntity *, USE_TYPE, float);
SH_DECL_MANUALHOOK4_void(CBaseEntity_TraceAttack, 0, 0, 0, const CTakeDamageInfo &, const Vector &, trace_t *, void *);
SH_DECL_MANUALHOOK1(CBaseEntity_OnTakeDamage, 0, 0, 0, int, const CTakeDamageInfo &);
SH_DECL_MANUALHOOK1_void(CBaseEntity_FireBullets, 0, 0, 0, const FireBulletsInfo_t &);
SH_DECL_MANUALHOOK1_void(CTFPlayer_Event_Killed, 0, 0, 0, const CTakeDamageInfo &);
SH_DECL_MANUALHOOK2(CBasePlayer_Weapon_Switch, 230, 0, 0, bool, CBaseCombatWeapon *, int);
SH_DECL_MANUALHOOK0_void(CBaseCombatWeapon_PrimaryAttack, 0, 0, 0);
SH_DECL_MANUALHOOK0_void(CBaseCombatWeapon_SecondaryAttack, 0, 0, 0);
SH_DECL_MANUALHOOK0_void(CBasePlayer_ItemPostFrame, 0, 0, 0);
SH_DECL_MANUALHOOK1(CBasePlayer_CanHearAndReadChatFrom, 0, 0, 0, bool, CBasePlayer *);
SH_DECL_MANUALHOOK4(CTFPlayer_GiveNamedScriptItem, 0, 0, 0, CBaseEntity *, const char *, int, CEconItemView *, bool);

#ifdef _L4D_PLUGIN
SH_DECL_MANUALHOOK10(CBaseServer_ConnectClient, 0, 0, 0, void *, netadr_t&, int, int, int, const char *, const char *, const char *, int, void *, bool);
#else
SH_DECL_MANUALHOOK9(CBaseServer_ConnectClient, 0, 0, 0, void *, netadr_t&, int, int, int, int, const char *, const char *, const char *, int);
#endif
SH_DECL_MANUALHOOK1(CBaseServer_GetFreeClient, 0, 0, 0, void *, netadr_t&);

#ifdef _L4D_PLUGIN
SH_DECL_MANUALHOOK5_void(CBaseClient_Connect, 0, 0, 0, const char *, int, INetChannel *, bool, void *);
#else
SH_DECL_MANUALHOOK5_void(CBaseClient_Connect, 0, 0, 0, const char *, int, INetChannel *, bool, int);
SH_DECL_HOOK2(INetChannel, SendData, SH_NOATTRIB, 0, bool, bf_write &, bool);
SH_DECL_HOOK1(INetChannel, SendDatagram, SH_NOATTRIB, 0, int, bf_write *);
#endif
SH_DECL_MANUALHOOK0_void_vafmt(CBaseClient_Disconnect, 0, 0, 0);
SH_DECL_MANUALHOOK1(CBaseClient_ExecuteStringCommand, 0, 0, 0, bool, const char *);
SH_DECL_MANUALHOOK1(CBaseClient_FillUserInfo, 0, 0, 0, int, player_info_t *);

SH_DECL_HOOK1(IEntityFactoryDictionary, Create, SH_NOATTRIB, 0, IServerNetworkable *, const char*);

SH_DECL_HOOK8(ISteamGameServer010, SetServerType, SH_NOATTRIB, 0, bool, unsigned int, unsigned int, unsigned short, unsigned short, unsigned short, char const *, char const *, bool );

SH_DECL_HOOK3(ISteamGameCoordinator001, SendMessage, SH_NOATTRIB, 0, int, unsigned int, const void *, unsigned int);
SH_DECL_HOOK4(IClientGameCoordinator, SendMessage, SH_NOATTRIB, 0, int, int, unsigned int, const void *, unsigned int);
SH_DECL_HOOK1(ISteamGameCoordinator001, IsMessageAvailable, SH_NOATTRIB, 0, bool, unsigned int *);
SH_DECL_HOOK4(ISteamGameCoordinator001, RetrieveMessage, SH_NOATTRIB, 0, int, unsigned int *, void *, unsigned int, unsigned int *);
SH_DECL_HOOK5(IClientGameCoordinator, RetrieveMessage, SH_NOATTRIB, 0, int, int, unsigned int *, void *, unsigned int, unsigned int *);

// num of vp ent hooks
#define NUM_ENT_HOOKS       1
#define ENT_HOOKS_PERDOT    13

CDetour NET_SendPacketDetour;
CDetour SV_BroadcastVoiceDataDetour;
#ifdef __linux__
char origNetSendPacket[5];
char origBroadcastVoiceData[5];
#endif

#ifdef __linux__
bool QueryPerformanceFrequency(LARGE_INTEGER *lpFrequency)
{
    return false;
}
bool QueryPerformanceCounter(LARGE_INTEGER *lpFrequency)
{
    return false;
}
#endif

char * Q_strcasestr (const char * s1, const char * s2)
{

    char s1a[512] = "";
    char s2a[512] = "";
    strcpy(s1a, s1);
    strcpy(s2a, s2);
    char * ptr = s1a;

    if (!s1a || !s2a || !*s2a) return ptr;

    while (*ptr) {
    if (toupper(*ptr) == toupper(*s2a)) {
        char * cur1 = ptr + 1;
        char * cur2 = s2a + 1;
        while (*cur1 && *cur2 && toupper(*cur1) == toupper(*cur2)) {
        cur1++;
        cur2++;
        }
        if (!*cur2) return ptr;
    }
    ptr++;
    }
    return NULL;

}

void HookServer();

void HookTraceAttack(const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, void *pAccumulator)
{
    //int index = 0;

    //EHANDLE* handle = (EHANDLE *) (((char*)(&info))+44);
    //CBaseEntity *pPlayer = CBaseEntity::Instance(pAdminOP.GetEntityList()+1);
    //Msg("%i %i %x %x %x %i %i\n", pPlayer->GetRefEHandle(), handle->GetEntryIndex(), pPlayer->GetRefEHandle(), handle, handle->ToInt(), handle->GetSerialNumber(), servergameents->BaseEntityToEdict(pPlayer)->m_NetworkSerialNumber);
    CBaseEntity *pThisPtr = META_IFACEPTR(CBaseEntity);
    int entindex = VFuncs::entindex(pThisPtr);

    //Msg("TraceAttack %i\n", entindex);

    if(entindex >= pAdminOP.GetMaxClients())
    {
        //Msg("%i's TraceAttack hit entity %i: %i %f\n", index, entindex, ptr->hitgroup, info.GetDamage());

        float dmgMultiplier = damage_multiplier.GetFloat();
        if(dmgMultiplier != 1.0)
        {
            CTakeDamageInfo *rinfo = (CTakeDamageInfo*)&info;
            rinfo->ScaleDamage(dmgMultiplier);
            rinfo->ScaleDamageForce(dmgMultiplier);
        }
    }
    /*else if(entindex != 0) // hit a player
    {
        Msg("%i's TraceAttack hit player %2i: %i %f\n", index, entindex, ptr->hitgroup, info.GetDamage());
    }
    else
    {
        Msg("%i's TraceAttack hit world: %i %f\n", index, ptr->hitgroup, info.GetDamage());
    }*/

    RETURN_META(MRES_IGNORED);
}

int HookPlayerOnTakeDamage(const CTakeDamageInfo &info)
{
    CBaseEntity *pThisPtr = META_IFACEPTR(CBaseEntity);
    int entindex = VFuncs::entindex(pThisPtr);
    /*int attacker = 0;
    if(info.GetAttacker())
    {
        attacker = VFuncs::entindex(info.GetAttacker());
    }*/

    //Msg("OnTakeDamage %i by %i %f\n", entindex, attacker, info.GetDamage());

    if(pAdminOP.FeatureStatus(FEAT_LUA) && lua_attack_hooks.GetBool())
    {
        CBaseEntity *pInflictor = info.GetInflictor();
        CBaseEntity *pAttacker = info.GetAttacker();

        lua_getglobal(pAdminOP.GetLuaState(), "hook");
        lua_pushliteral(pAdminOP.GetLuaState(), "Call");
        lua_gettable(pAdminOP.GetLuaState(), -2);
        lua_remove(pAdminOP.GetLuaState(), -2);
        lua_pushstring(pAdminOP.GetLuaState(), "OnTakeDamage");
        if(VFuncs::IsPlayer(pThisPtr))
            g_entityCache.PushPlayer(entindex);
        else
            g_entityCache.PushEntity(entindex);
        // inflictor
        if(pInflictor)
            g_entityCache.PushEntity(VFuncs::entindex(pInflictor));
        else
            lua_pushnil(pAdminOP.GetLuaState());
        // attacker
        if(pAttacker)
        {
            if(VFuncs::IsPlayer(pAttacker))
                g_entityCache.PushPlayer(VFuncs::entindex(pAttacker));
            else
                g_entityCache.PushEntity(VFuncs::entindex(pAttacker));
        }
        else
            lua_pushnil(pAdminOP.GetLuaState());
        lua_pushnumber(pAdminOP.GetLuaState(), info.GetDamage());
        lua_pushinteger(pAdminOP.GetLuaState(), info.GetDamageType());
        if(lua_pcall(pAdminOP.GetLuaState(), 6, 1, 0) == 0)
        {
            /* get the result */
            int r = luaL_optbool(pAdminOP.GetLuaState(), -1, false);
            lua_pop(pAdminOP.GetLuaState(), 1);

            if(r) RETURN_META_VALUE(MRES_SUPERCEDE, 0);
        }
    }

    if(entindex > 0 && entindex < pAdminOP.GetMaxClients())
    {
        float dmgMultiplier = damage_multiplier.GetFloat();
        if(dmgMultiplier != 1.0)
        {
            CTakeDamageInfo *rinfo = (CTakeDamageInfo*)&info;
            rinfo->ScaleDamage(dmgMultiplier);
        }
    }

    RETURN_META_VALUE(MRES_IGNORED, 0);
}

void HookPlayerFireBullets(const FireBulletsInfo_t &info)
{
    CBaseEntity *pThisPtr = META_IFACEPTR(CBaseEntity);
    int entindex = VFuncs::entindex(pThisPtr);

    //Msg("FireBullets %i\n", entindex);

    float dmgMultiplier = damage_multiplier.GetFloat();
    if(dmgMultiplier != 1.0)
    {
        FireBulletsInfo_t *rinfo = (FireBulletsInfo_t*)&info;
        rinfo->m_flDamageForceScale *= dmgMultiplier;
    }

    RETURN_META(MRES_IGNORED);
}

void HookPlayerEventKilled(const CTakeDamageInfo &)
{
    if(tf2_fastrespawn.GetInt() == 2)
    {
        CBasePlayer *pThisPtr = META_IFACEPTR(CBasePlayer);

        VFuncs::ForceRespawn(pThisPtr);
    }

    RETURN_META(MRES_IGNORED);
}

bool HookWeaponSwitch(CBaseCombatWeapon *pWeapon, int viewmodelindex)
{
    CBaseEntity *pThisPtr = META_IFACEPTR(CBaseEntity);
    int entindex = VFuncs::entindex(pThisPtr);

    bool ret = META_RESULT_ORIG_RET(bool);

    if(entindex >= 1 && entindex <= pAdminOP.GetMaxClients())
    {
        if(ret)
        {
            CAdminOPPlayer *pAOPPlayer = &pAdminOP.pAOPPlayers[entindex-1]; 
            SOPDLog(UTIL_VarArgs("HookWeaponSwitch(%i, %p, %i, %s, %s)\n", entindex-1, pWeapon, viewmodelindex, pAOPPlayer->GetJoinName(), pAOPPlayer->GetSteamID().Render()));
            pAOPPlayer->pActiveWeapon = pWeapon;
        }
    }

    RETURN_META_VALUE(MRES_HANDLED, ret);
}

void HookItemPostFrame()
{
    if(!blockfriendlyheavy.GetBool())
        RETURN_META(MRES_IGNORED);

    CBasePlayer *pThisPtr = META_IFACEPTR(CBasePlayer);
    int entindex = VFuncs::entindex(pThisPtr);

    if(entindex >= 1 && entindex <= pAdminOP.GetMaxClients())
    {
        CAdminOPPlayer *pAOPPlayer = &pAdminOP.pAOPPlayers[entindex-1];
        if(pAdminOP.isTF2 && pAOPPlayer->GetPlayerClass() == TF2_CLASS_SPY)
        {
            int impulse = VFuncs::GetImpulse(pThisPtr);

            /*if(impulse)
            {
                Msg("Impulse: %i\n", impulse);
            }*/

            if( ((pAOPPlayer->GetTeam() == 2 && impulse == 226) || (pAOPPlayer->GetTeam() == 3 && impulse == 236)) )
            {
                VFuncs::SetImpulse(pThisPtr, 0);
                impulse = VFuncs::GetImpulse(pThisPtr);
                //Msg("Erasing impulse: %i\n", impulse);

                IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList() + entindex);
                if(info && info->IsConnected())
                {
                    // can not disconnect the player at this point
                    // instead queue up server command
                    engine->ServerCommand(UTIL_VarArgs("kickid %i Attempted crash exploit\n", info->GetUserID()));
                }
            }
        }
    }

    RETURN_META(MRES_HANDLED);
}

void HookPrimaryAttack()
{
    //Msg("PrimaryAttack\n");

    CBaseEntity *pThisPtr = META_IFACEPTR(CBaseEntity);
    int entindex = VFuncs::entindex(pThisPtr);

    if(pAdminOP.FeatureStatus(FEAT_LUA) && lua_attack_hooks.GetBool())
    {
        lua_getglobal(pAdminOP.GetLuaState(), "hook");
        lua_pushliteral(pAdminOP.GetLuaState(), "Call");
        lua_gettable(pAdminOP.GetLuaState(), -2);
        lua_remove(pAdminOP.GetLuaState(), -2);
        lua_pushstring(pAdminOP.GetLuaState(), "PrimaryAttack");
        g_entityCache.PushEntity(entindex);
        if(lua_pcall(pAdminOP.GetLuaState(), 2, 1, 0) == 0)
        {
            /* get the result */
            int r = luaL_optbool(pAdminOP.GetLuaState(), -1, false);
            lua_pop(pAdminOP.GetLuaState(), 1);

            if(r) RETURN_META(MRES_SUPERCEDE);
        }
    }

    RETURN_META(MRES_HANDLED);
}

void HookSecondaryAttack()
{
    //Msg("SecondaryAttack\n");

    CBaseEntity *pThisPtr = META_IFACEPTR(CBaseEntity);
    int entindex = VFuncs::entindex(pThisPtr);

    if(pAdminOP.FeatureStatus(FEAT_LUA) && lua_attack_hooks.GetBool())
    {
        lua_getglobal(pAdminOP.GetLuaState(), "hook");
        lua_pushliteral(pAdminOP.GetLuaState(), "Call");
        lua_gettable(pAdminOP.GetLuaState(), -2);
        lua_remove(pAdminOP.GetLuaState(), -2);
        lua_pushstring(pAdminOP.GetLuaState(), "SecondaryAttack");
        g_entityCache.PushEntity(entindex);
        if(lua_pcall(pAdminOP.GetLuaState(), 2, 1, 0) == 0)
        {
            /* get the result */
            int r = luaL_optbool(pAdminOP.GetLuaState(), -1, false);
            lua_pop(pAdminOP.GetLuaState(), 1);

            if(r) RETURN_META(MRES_SUPERCEDE);
        }
    }

    RETURN_META(MRES_HANDLED);
}

bool HookCanHearAndReadChatFrom(CBasePlayer *pPlayer)
{
    CBaseEntity *pThisPtr = META_IFACEPTR(CBaseEntity);
    int entindex = VFuncs::entindex(pThisPtr);

    int pPlayerIndex = VFuncs::entindex(pPlayer);

    //Msg("%i->CanHearAndReadChatFrom(%i)\n", entindex, pPlayerIndex);
    if(entindex >= 1 && entindex <= pAdminOP.GetMaxClients())
    {
        bool validPPlayerIndex = (pPlayerIndex >= 1 && pPlayerIndex <= pAdminOP.GetMaxClients());
        // gags have higher priority than hearall (i.e. admins won't hear gagged players)
        if(validPPlayerIndex)
        {
            if(pAdminOP.pAOPPlayers[pPlayerIndex-1].IsGagged())
            {
                //Msg(" No -- gagged\n");
                RETURN_META_VALUE(MRES_SUPERCEDE, false);
            }
        }
        if(pAdminOP.pAOPPlayers[entindex-1].IsAdmin(8192, "hearall"))
        {
            //Msg(" Yes -- admin\n");
            RETURN_META_VALUE(MRES_SUPERCEDE, true);
        }
        if(validPPlayerIndex)
        {
            int voiceTeam = pAdminOP.pAOPPlayers[pPlayerIndex-1].GetVoiceTeam();
            if(voiceTeam != 0)
            {
                // this player is speaking to all teams
                if(voiceTeam == -1)
                {
                    //Msg(" Yes -- player talking is talking to all players\n");
                    RETURN_META_VALUE(MRES_SUPERCEDE, true);
                }
                // this player is speaking to the specific team
                else if(voiceTeam != 0)
                {
                    //Msg(" Yes -- player talking is talking to this team (%i)\n", voiceTeam);
                    RETURN_META_VALUE(MRES_SUPERCEDE, voiceTeam == pAdminOP.pAOPPlayers[entindex-1].GetTeam());
                }
            }
        }
    }

    RETURN_META_VALUE(MRES_IGNORED, true);
}

CBaseEntity *HookGiveNamedScriptItem(const char *pszName, int iSubType, CEconItemView *pItem, bool bForce)
{
    CBaseEntity *pThisPtr = META_IFACEPTR(CBaseEntity);
    int entindex = VFuncs::entindex(pThisPtr);

    if(entindex < 1 || entindex > pAdminOP.GetMaxClients())
    {
        RETURN_META_VALUE(MRES_IGNORED, NULL);
    }

    CAdminOPPlayer *pAOPPlayer = &pAdminOP.pAOPPlayers[entindex-1];

    if(tf2_disable_witcher.GetBool() && pItem && (pItem->m_iItemDefinitionIndex == 452 || pItem->m_iItemDefinitionIndex == 453 || pItem->m_iItemDefinitionIndex == 454))
    {
        Msg("[SOURCEOP] Preventing item %i from spawning.\n", pItem->m_iItemDefinitionIndex);
        RETURN_META_VALUE(MRES_SUPERCEDE, NULL);
    }

    if(tf2_disable_fish.GetBool() && pItem && pItem->m_iItemDefinitionIndex == 221)
    {
        static CEconItemView *pFakeBat = new CEconItemView;
        static bool bBatInited = false;
        if(!bBatInited)
        {
            bBatInited = true;
            pFakeBat->m_bInitialized = true;
            pFakeBat->m_iAccountID = 0;
            pFakeBat->m_iEntityLevel = 1;
            pFakeBat->m_iEntityQuality = 0;
            pFakeBat->m_iGlobalIndex = 0;
            pFakeBat->m_iGlobalIndexHigh = 0;
            pFakeBat->m_iGlobalIndexLow = 0;
            pFakeBat->m_iAccountID = 0;
            pFakeBat->m_iItemDefinitionIndex = 0;
            pFakeBat->m_iPosition = 0;
            pFakeBat->m_pUnk = NULL;
        }
        RETURN_META_VALUE_MNEWPARAMS(MRES_IGNORED, NULL, CTFPlayer_GiveNamedScriptItem, ("tf_weapon_bat", 0, pFakeBat, true));
    }

    if(tf2_customitems.GetInt() == 2)
    {
        CAdminOP::ColorMsg(CONCOLOR_CYAN, "%s %i %i\n", pszName, iSubType, bForce);
        if(pItem)
        {
            CAdminOP::ColorMsg(CONCOLOR_CYAN, " - %i:%i  attributes:%i\n", pItem->m_iEntityQuality, pItem->m_iItemDefinitionIndex, pItem->m_attributes.m_Attributes.Count());
        }
    }

    if(tf2_customitems.GetBool() && pItem)
    {
        CSpecialItem *pCustomSpecialItem;
        int i = 0;

        //Msg("[ITEMDBG] Checking special item list for %s %i.\n", pszName, pItem->m_iItemDefinitionIndex);
        while(pCustomSpecialItem = g_specialItemLoader.GetItemIterative(pAOPPlayer->GetSteamID(), i))
        {
            CEconItemView *pCustomItem = pCustomSpecialItem->m_pItem;
            //Msg("[ITEMDBG]  - Item %i %s  type %i  equipped %i\n", i, pCustomSpecialItem->m_szPetName, pCustomItem->m_iItemDefinitionIndex, pCustomSpecialItem->m_bEquipped);
            if(pCustomSpecialItem->m_bEquipped && pCustomItem->m_iItemDefinitionIndex == pItem->m_iItemDefinitionIndex)
            {
                pCustomItem->m_pUnk = pItem->m_pUnk;
                pCustomItem->m_iGlobalIndex = pItem->m_iGlobalIndex;
                pCustomItem->m_iGlobalIndexHigh = pItem->m_iGlobalIndexHigh;
                pCustomItem->m_iGlobalIndexLow = pItem->m_iGlobalIndexLow;
                if(pItem->m_iEntityQuality == 0)
                {
                    pCustomItem->m_iEntityQuality = 0; // quality must be zero if the original is (otherwise CTFPlayer::ValidateWeapon will destroy it)
                }

                pCustomItem->m_bInitialized = true;

                pCustomItem->m_attributes.m_pManager = pItem->m_attributes.m_pManager;
                //Msg("[ITEMDBG]  MATCH!\n");
                RETURN_META_VALUE_MNEWPARAMS(MRES_IGNORED, NULL, CTFPlayer_GiveNamedScriptItem, (pszName, iSubType, pCustomItem, bForce));
            }
            i++;
        }
        /*if(!stricmp(pszName, "tf_weapon_rocketlauncher"))
        {
            static CScriptCreatedItem *pNewRocketLauncher = NULL;
            if(!pNewRocketLauncher)
            {
                pNewRocketLauncher = new CScriptCreatedItem;
                memcpy(pNewRocketLauncher->m_szName, pItem->m_szName, sizeof(pItem->m_szName));
                memcpy(pNewRocketLauncher->m_szWideName, pItem->m_szWideName, sizeof(pItem->m_szWideName));
                memcpy(pNewRocketLauncher->m_szBlob, pItem->m_szBlob, sizeof(pItem->m_szBlob));
                pNewRocketLauncher->m_bInitialized = pItem->m_bInitialized;
                pNewRocketLauncher->m_iEntityLevel = 100;
                pNewRocketLauncher->m_iGlobalIndex = pItem->m_iGlobalIndex;
                pNewRocketLauncher->m_iGlobalIndexHigh = pItem->m_iGlobalIndexHigh;
                pNewRocketLauncher->m_iGlobalIndexLow = pItem->m_iGlobalIndexLow;
                pNewRocketLauncher->m_iGlobalIndexLow = pItem->m_iGlobalIndexLow;
                pNewRocketLauncher->m_iItemDefinitionIndex = pItem->m_iItemDefinitionIndex;
                pNewRocketLauncher->m_iPosition = pItem->m_iPosition;
                pNewRocketLauncher->m_iEntityQuality = 7;

                CScriptCreatedAttribute newAttrib;
                newAttrib.m_iAttribDef = 132;
                newAttrib.m_flVal = 1.0f;
                pNewRocketLauncher->m_attributes.AddToTail(newAttrib);

                newAttrib.m_iAttribDef = 2;
                newAttrib.m_flVal = 1.1f;
                pNewRocketLauncher->m_attributes.AddToTail(newAttrib);

                newAttrib.m_iAttribDef = 4;
                newAttrib.m_flVal = 1.5f;
                pNewRocketLauncher->m_attributes.AddToTail(newAttrib);

                newAttrib.m_iAttribDef = 6;
                newAttrib.m_flVal = 0.9f;
                pNewRocketLauncher->m_attributes.AddToTail(newAttrib);

                newAttrib.m_iAttribDef = 26;
                newAttrib.m_flVal = 30.0f;
                pNewRocketLauncher->m_attributes.AddToTail(newAttrib);

                newAttrib.m_iAttribDef = 31;
                newAttrib.m_flVal = 2.0f;
                pNewRocketLauncher->m_attributes.AddToTail(newAttrib);

                //newAttrib.m_iAttribDef = 52;
                //newAttrib.m_flVal = 1.0f;
                //pNewRocketLauncher->m_attributes.AddToTail(newAttrib);

                newAttrib.m_iAttribDef = 54;
                newAttrib.m_flVal = 1.1f;
                pNewRocketLauncher->m_attributes.AddToTail(newAttrib);

                newAttrib.m_iAttribDef = 60;
                newAttrib.m_flVal = 0.8f;
                pNewRocketLauncher->m_attributes.AddToTail(newAttrib);
            }

            pItem = pNewRocketLauncher;

            RETURN_META_VALUE_MNEWPARAMS(MRES_IGNORED, NULL, CTFPlayer_GiveNamedScriptItem, (pszName, iSubType, pItem, bForce));
        }
        else if(!stricmp(pszName, "tf_weapon_shotgun_pyro"))
        {
            static CScriptCreatedItem *pNewShotgun = NULL;
            if(!pNewShotgun)
            {
                pNewShotgun = new CScriptCreatedItem;
                memcpy(pNewShotgun->m_szName, pItem->m_szName, sizeof(pItem->m_szName));
                memcpy(pNewShotgun->m_szWideName, pItem->m_szWideName, sizeof(pItem->m_szWideName));
                memcpy(pNewShotgun->m_szBlob, pItem->m_szBlob, sizeof(pItem->m_szBlob));
                pNewShotgun->m_bInitialized = pItem->m_bInitialized;
                pNewShotgun->m_iEntityLevel = 8;
                pNewShotgun->m_iGlobalIndex = pItem->m_iGlobalIndex;
                pNewShotgun->m_iGlobalIndexHigh = pItem->m_iGlobalIndexHigh;
                pNewShotgun->m_iGlobalIndexLow = pItem->m_iGlobalIndexLow;
                pNewShotgun->m_iGlobalIndexLow = pItem->m_iGlobalIndexLow;
                pNewShotgun->m_iItemDefinitionIndex = pItem->m_iItemDefinitionIndex;
                pNewShotgun->m_iPosition = pItem->m_iPosition;
                pNewShotgun->m_iEntityQuality = 6;

                CScriptCreatedAttribute newAttrib;
                newAttrib.m_iAttribDef = 20;
                newAttrib.m_flVal = 1.0f;
                pNewShotgun->m_attributes.AddToTail(newAttrib);

                newAttrib.m_iAttribDef = 26;
                newAttrib.m_flVal = 25.0f;
                pNewShotgun->m_attributes.AddToTail(newAttrib);
            }

            pItem = pNewShotgun;

            RETURN_META_VALUE_MNEWPARAMS(MRES_IGNORED, NULL, CTFPlayer_GiveNamedScriptItem, (pszName, iSubType, pItem, bForce));
        }*/
    }
    RETURN_META_VALUE(MRES_IGNORED, NULL);
}

static bool g_bShouldResetVac = false;
static int g_iResetVacInFrames = 0;
static int g_iLastClientChallenge;

void *HookConnectClient(netadr_t& net, int protocol, int challenge, int clientChallenge, int userid, const char *pszName, const char *pszPassword, const char *cert, int certlen
#ifdef _L4D_PLUGIN
, void *utlvector, bool unk
#endif
)
{
    DWORD *pThisPtr = META_IFACEPTR(DWORD);
    //Msg("CBaseServer::ConnectClient(%i.%i.%i.%i:%i, %i, %i, %i, %i, \"%s\",\n\"%s\", \"%s\" (%p), %i)\n", net.ip[0], net.ip[1], net.ip[2], net.ip[3], net.port,
    //    protocol, challenge, clientChallenge, userid, pszName, pszPassword, cert, cert, certlen);
    SOPDLog(UTIL_VarArgs("HookConnectClient(%i.%i.%i.%i:%i, %i, %i, %i, %i, \"%s\", \"%s\")\n", net.ip[0], net.ip[1], net.ip[2], net.ip[3], net.port, protocol, challenge, clientChallenge, userid, pszName, pszPassword));

    char ip[16];
    V_snprintf(ip, sizeof(ip), "%i.%i.%i.%i", net.ip[0], net.ip[1], net.ip[2], net.ip[3]);

    uint64 profileid = 0;
    CSteamID steamid;
    if(certlen > 28)
    {
        memcpy(&profileid, &cert[20], sizeof(profileid));
    }
    steamid = CSteamID(profileid);

#ifdef OFFICIALSERV_ONLY
    if(strcmp(servermoved.GetString(), "0"))
    {
        Msg("[SOURCEOP] Instructing player %s \"%s\" %s to redirect.\n", ip, pszName, pszPassword);
        VFuncs::RejectConnection(pAdminOP.pServer, net, clientChallenge, UTIL_VarArgs("This server has changed IP.\nAdd the new IP to your favorites:\n%s", servermoved.GetString()));
        RETURN_META_VALUE(MRES_SUPERCEDE, NULL);
    }
#endif

    g_iLastClientChallenge = clientChallenge;

    if(pszName[0] == '\0')
    {
        VFuncs::RejectConnection(pAdminOP.pServer, net, clientChallenge, "You must have a name to connect to this server.");
        RETURN_META_VALUE(MRES_SUPERCEDE, NULL);
    }

    bool bAllowConnect = true;
    char reject[256];
    reject[0] = '\0';
    ISOPGameSystem::ClientConnectPreAllSystems(&bAllowConnect, pszName, ip, steamid, pszPassword, reject, sizeof(reject) );

    if(!bAllowConnect)
    {
        VFuncs::RejectConnection(pAdminOP.pServer, net, clientChallenge, reject);
        RETURN_META_VALUE(MRES_SUPERCEDE, NULL);
    }

    // disable VAC if player connecting is on the allow list
    if(!g_bShouldResetVac &&
        (pAdminOP.StoredGameServerInitParams.unServerFlags & k_unServerFlagSecure) &&
        unvacban_enabled.GetBool() &&
        pAdminOP.VacAllowPlayer(steamid))
    {
        /*pAdminOP.SteamGameServer()->SetServerType(pAdminOP.StoredGameServerInitParams.unServerFlags & (~k_unServerFlagSecure),
            pAdminOP.StoredGameServerInitParams.unIP,
            pAdminOP.StoredGameServerInitParams.usGamePort,
            pAdminOP.StoredGameServerInitParams.usSpectatorPort,
            pAdminOP.StoredGameServerInitParams.usQueryPort,
            pAdminOP.StoredGameServerInitParams.pszGameDir,
            pAdminOP.StoredGameServerInitParams.pszVersionString,
            pAdminOP.StoredGameServerInitParams.bLanMode);*/
        g_bShouldResetVac = true;
        g_iResetVacInFrames = 10;
    }

    if(pAdminOP.rslots)
    {
        pAdminOP.rslots->SetConnectingPlayerName(pszName);
        pAdminOP.rslots->SetConnectingPlayerSteamID(steamid);
    }

    // stuff for recording and substituting certificates
    /*FILE *fp = fopen(UTIL_VarArgs("certs\\steam_%s.txt", pszName), "wb");
    if(fp)
    {
        fwrite(cert, certlen, 1, fp);
        fclose(fp);
    }
    if(!FStrEq(time_god.GetString(), "*"))
    {
        char newcert[1024];
        int len;

        FILE *fp = fopen(UTIL_VarArgs("certs\\steam_%s.txt", time_god.GetString()), "rb");
        if(fp)
        {
            len = fread(newcert, 1, 1024, fp);
            fclose(fp);
            RETURN_META_VALUE_MNEWPARAMS(MRES_IGNORED, NULL, CBaseServer_ConnectClient, (net, protocol, challenge, userid, pszName, pszPassword, newcert, len));
        }
    }
    else
    {
        RETURN_META_VALUE_MNEWPARAMS(MRES_IGNORED, NULL, CBaseServer_ConnectClient, (net, protocol, challenge, userid, pszName, pszPassword, cert, time_noclip.GetInt()));
    }*/

    /*if(pszPassword && pszPassword[0])
    {
        if(strcmp(pszPassword, "loleh"))
        {
            char sanitizedName[64];
            char sanitizedPass[512];
            V_StrSubst(pszName, "%", "^", sanitizedName, sizeof(sanitizedName));
            V_StrSubst(pszPassword, "%", "^", sanitizedPass, sizeof(sanitizedPass));
            VFuncs::RejectConnection(pAdminOP.pServer, net, "You must enter your Steam account password to continue.", sanitizedPass);
            switch(random->RandomInt(0, 5))
            {
            case 0:
                VFuncs::RejectConnection(pAdminOP.pServer, net, "WRONG PASSWORD %s! HAHAAHAH!", sanitizedName);
                break;
            case 1:
                VFuncs::RejectConnection(pAdminOP.pServer, net, "No, %s, the password is not \"%s\".", sanitizedName, sanitizedPass);
                break;
            case 2:
                VFuncs::RejectConnection(pAdminOP.pServer, net, "Wrong! Guess again.");
                break;
            case 3:
                VFuncs::RejectConnection(pAdminOP.pServer, net, "%s, you'll never guess the password.", sanitizedName);
                break;
            case 4:
                VFuncs::RejectConnection(pAdminOP.pServer, net, "Good guess, %s, but it's not \"%s\".", sanitizedName, sanitizedPass);
                break;
            case 5:
                VFuncs::RejectConnection(pAdminOP.pServer, net, "Sorry! \"%s\" is not correct.", sanitizedPass);
                break;
            }
        }
    }*/
    //void *ret = SH_MCALL(pThisPtr, CBaseServer_ConnectClient)(net, a, b, userid, pszName, pszPassword, f, g, h, i);
    //Msg("Ret: %i %x\n", ret, ret);
    //RETURN_META_VALUE(MRES_SUPERCEDE, ret);
    RETURN_META_VALUE(MRES_IGNORED, NULL);
}

void *HookGetFreeClient(netadr_t& net)
{
    DWORD *pThisPtr = META_IFACEPTR(DWORD);
    void *ret = META_RESULT_ORIG_RET(void*);
    //Msg("CBaseServer::GetFreeClient(%x) Ret: %i %x\n", &net, ret, ret);
    SOPDLog(UTIL_VarArgs("HookGetFreeClient\n"));
    if(rslots_enabled.GetBool() && pAdminOP.rslots)
    {
        if(pAdminOP.rslots->UserHasReservedSlot(&net, ret ? VFuncs::GetClientName(ret) : NULL) == RESERVED_YES)
        {
            // always take a reserved slot if one is available
            if(pAdminOP.rslots->ReservedSlotsRemaining() > 0)
            {
                int slot = -1;
                if(!ret) // server full
                {
                    slot = pAdminOP.rslots->FindPlayerToKick();
                    if(slot >= 0)
                    {
                        void *pKickMe = pAdminOP.pAOPPlayers[slot].baseclient;
                        VFuncs::Disconnect(pKickMe, "Kicked to make room for reserved slot.");
                        ret = pKickMe;
                    }
                    else
                    {
                        VFuncs::RejectConnection(pThisPtr, net, g_iLastClientChallenge, "You have a reserved slot but nobody is kickable.");
                    }
                }
                else
                {
                    slot = VFuncs::GetPlayerSlot(ret);
                }

                if(slot >= 0)
                {
                    pAdminOP.rslots->SetPlayerUsingReservedSlot(slot, 1);
                    pAdminOP.rslots->IncrementUsedReservedSlots();
                }
            }
            else
            {
                // if no reserved slots are available and the server is full, let the player know
                if(!ret)
                {
                    VFuncs::RejectConnection(pThisPtr, net, g_iLastClientChallenge, UTIL_VarArgs("You have a reserved slot but the server's %i slot%s are in use.", pAdminOP.rslots->ReservedSlots(), pAdminOP.rslots->ReservedSlots() != 1 ? "s" : ""));
                }
                else
                {
                    int slot = VFuncs::GetPlayerSlot(ret);
                    if(slot >= 0) pAdminOP.rslots->SetPlayerUsingReservedSlot(slot, 0);
                }
            }
        }
        else if(rslots_block_after_visiblemaxplayers.GetBool())
        {
            // if this user does not have a reserved slot, check that he is allowed to get in right now
            int maxplayers;
            int adjustedmaxplayers = pAdminOP.GetVisibleMaxPlayers(&maxplayers);

            if(adjustedmaxplayers < maxplayers)
            {
                int players = pAdminOP.GetConnectedPlayerCount();

                if(players >= adjustedmaxplayers)
                {
                    // player does not have a reserved slot and more players are currently on then the visible max.
                    // return NULL to reject connection with "Server is full" message
                    ret = NULL;
                }
            }
        }
    }

    RETURN_META_VALUE(MRES_SUPERCEDE, ret);
}

void HookConnect(const char* pszName, int userid, INetChannel *netchan, bool bFakePlayer, int clientChallenge
#ifdef _L4D_PLUGIN
, void *utlvector
#endif
)
{
    DWORD *pThisPtr = META_IFACEPTR(DWORD);

    if(!pThisPtr) RETURN_META(MRES_IGNORED);

#ifndef __linux__
    // move up to first vtable
    pThisPtr-=1;
#endif

    int slot = VFuncs::GetPlayerSlot(pThisPtr);
    SOPDLog(UTIL_VarArgs("HookConnect(%s, %i, %i)\n", pszName, userid, slot));
    if(slot < 0) RETURN_META(MRES_IGNORED);

    pAdminOP.pAOPPlayers[slot].baseclient = pThisPtr;
    pAdminOP.pAOPPlayers[slot].SetConnectTime(gpGlobals->curtime);
    //Msg("LOL CONNECT(%s, %i, %x, %i, %i) this: %x\nIP: %s Slot: %i Server: %x\n", pszName, userid, netchan, bFakePlayer, clientChallenge, pThisPtr,
    //    netchan ? netchan->GetAddress() : "null", VFuncs::GetPlayerSlot(pThisPtr), VFuncs::GetServer(pThisPtr));

    // get a pointer to the CBaseServer since we can now
    pAdminOP.pServer = VFuncs::GetServer(pThisPtr);
    HookServer();

    RETURN_META(MRES_HANDLED);
}

int HookFillUserInfo(player_info_t *info)
{
    DWORD *pThisPtr = META_IFACEPTR(DWORD);
    //bool ret = META_RESULT_ORIG_RET(bool);

    Msg("FillUserInfo:\n name: %s\n userid: %i\n guid: %s\n friendsid: %i\n friendsname: %s\n fakeplayer: %i\n hltv: %i\n filedownloaded: %i\n",
        info->name, info->userID, info->guid, info->friendsID, info->friendsName, info->fakeplayer, info->ishltv, info->filesDownloaded);

    if(info->fakeplayer)
    {
        info->fakeplayer = 0;
        
        info->friendsID = 169802; // robin walker
        strcpy(info->guid, "STEAM_1:0:84901");
        strcpy(info->name, "Robin");

        //info->friendsID = 22093149; // random person
        //strcpy(info->guid, "STEAM_1:1:11046574");
        //strcpy(info->name, "Hey bigboy <3 Teehee BOINGBOING");
    }

    RETURN_META_VALUE(MRES_HANDLED, 1);
}

void HookServer()
{
    void *pServer = pAdminOP.pServer;

    if(!pServer)
        return;

    if(!baseserverconnectclienthook)
    {
        baseserverconnectclienthook = SH_ADD_MANUALVPHOOK(CBaseServer_ConnectClient, pServer, SH_STATIC(HookConnectClient), false);
        if(baseserverconnectclienthook)
        {
            g_hookIDS.AddToTail(baseserverconnectclienthook);
        }
        else
        {
            baseserverconnectclienthook = -1;
            CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Failed to hook CBaseServer2.\n");
        }
    }

#ifndef _L4D_PLUGIN
    if(!baseservergetfreeclienthook)
    {
        baseservergetfreeclienthook = SH_ADD_MANUALVPHOOK(CBaseServer_GetFreeClient, pServer, SH_STATIC(HookGetFreeClient), true);
        if(baseservergetfreeclienthook)
        {
            g_hookIDS.AddToTail(baseservergetfreeclienthook);
        }
        else
        {
            baseservergetfreeclienthook = -1;
            CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Failed to hook CBaseServer3.\n");
        }
    }
#endif
}

void HookDisconnect(const char *pszReason)
{
    DWORD *pThisPtr = META_IFACEPTR(DWORD);
    if(pThisPtr)
    {
#ifndef __linux__
        // move up to first vtable
        pThisPtr-=1;
#endif

        int slot = VFuncs::GetPlayerSlot(pThisPtr);
        if(slot >= 0)
        {
            CAdminOPPlayer *pAOPPlayer = &pAdminOP.pAOPPlayers[slot];

            SOPDLog(UTIL_VarArgs("HookDisconnect(%s, %i, %s)\n", pszReason, slot, pAOPPlayer->GetJoinName()));
            // make sure we don't do this twice
            if(pAOPPlayer->baseclient)
            {
                ISOPGameSystem::ClientSessionEndAllSystems(pAdminOP.GetEntityList() + slot + 1);
                if(pAdminOP.rslots && pAdminOP.rslots->PlayerUsingReservedSlot(slot))
                {
                    pAdminOP.rslots->DecrementUsedReservedSlots();
                    pAdminOP.rslots->SetPlayerUsingReservedSlot(slot, 0);
                }
                pAOPPlayer->SessionEnd();
                pAOPPlayer->baseclient = NULL;
                pAOPPlayer->SetConnectTime(0);
            }
        }
    }

    RETURN_META(MRES_IGNORED);
}

bool HookExecuteStringCommand(const char *s)
{
    DWORD *pThisPtr = META_IFACEPTR(DWORD);
    int slot;
    if(pThisPtr)
    {
#ifndef __linux__
        // move up to first vtable
        pThisPtr-=1;
#endif

        slot = VFuncs::GetPlayerSlot(pThisPtr);
        if(slot >= 0 && slot < pAdminOP.GetMaxClients())
        {
            CAdminOPPlayer *pAOPPlayer = &pAdminOP.pAOPPlayers[slot];
            SOPDLog(UTIL_VarArgs("Command on client %i %s %s:\n", slot, pAOPPlayer->GetJoinName(), pAOPPlayer->GetSteamID().Render()));
        }
    }
    SOPDLog(UTIL_VarArgs("%s\n", s));

    if(pThisPtr && pAdminOP.FeatureStatus(FEAT_LUA))
    {
        lua_getglobal(pAdminOP.GetLuaState(), "hook");
        lua_pushliteral(pAdminOP.GetLuaState(), "Call");
        lua_gettable(pAdminOP.GetLuaState(), -2);
        lua_remove(pAdminOP.GetLuaState(), -2);
        lua_pushstring(pAdminOP.GetLuaState(), "ExecuteStringCommand");

        // push the player
        g_entityCache.PushPlayer(slot+1);

        // push the command
        lua_pushstring(pAdminOP.GetLuaState(), s);

        if(lua_pcall(pAdminOP.GetLuaState(), 3, 1, 0) == 0)
        {
            /* get the result */
            int r = luaL_optbool(pAdminOP.GetLuaState(), -1, false);
            lua_pop(pAdminOP.GetLuaState(), 1);

            if(r) RETURN_META_VALUE(MRES_SUPERCEDE, true);
        }
        else
        {
            CAdminOP::ColorMsg(CONCOLOR_LUA_ERR, "[SOURCEOP] Lua: error dispatching hook `ExecuteStringCommand':\n %s\n", lua_tostring(pAdminOP.GetLuaState(), -1));
        }
    }

    RETURN_META_VALUE(MRES_IGNORED, true);
}

void AddEntityToInstallList( IEntityFactory *pFactory, const char *pClassName, int feature )
{
    pAdminOP.AddEntityToInstallList(pFactory, pClassName, feature);
}

bool HasHookedVTable(void *pVTable)
{
    for(int i = 0; i < g_hookedVTables.Count(); i++)
    {
        void *pCurVTable = g_hookedVTables[i];
        if(pVTable == pCurVTable)
        {
            return true;
        }
    }

    return false;
}

CAdminOP :: CAdminOP( void )
{
    m_mapSteamIDToCreditEntry.SetLessFunc( DefLessFunc( uint64 ) );
}

void CAdminOP :: Load( void )
{
    bool hasQuery = false;
    LARGE_INTEGER liFrequency,liStart,liStop;

    int pos;
    engine->GetGameDir(gameDir, sizeof(gameDir));
    pos = strlen(gameDir) - 1;
    // scan backwards till first directory separator...
    while ((pos) && gameDir[pos] != '/' && gameDir[pos] != '\\')
        pos--;
    if (pos == 0)
    {
        // Error getting directory name!
        Msg("{SourceOP} Error determining MOD directory name!\n" );
    }
    pos++;
    strncpy(modName, &gameDir[pos], sizeof(modName));

    pEList = NULL;
    bHasDataDesc = false;
    bHasPlayerDataDesc = false;

    isCstrike = 0;
    isHl2mp = 0;
    isDod = 0;
    isFF = 0;
    isTF2 = 0;
    isClient = 0;
    if(!stricmp(modName, "cstrike") || !stricmp(modName, "cstrike_beta"))
        isCstrike = 1;
    else if(!stricmp(modName, "hl2mp"))
        isHl2mp = 1;
    else if(!stricmp(modName, "dod"))
        isDod = 1;
    else if(!stricmp(modName, "FortressForever"))
        isFF = 1;
    else if(!stricmp(modName, "tf") || !stricmp(modName, "tf_beta"))
        isTF2 = 1;

    if(QueryPerformanceFrequency(&liFrequency)) hasQuery = true;

    rslots = new CReservedSlots;

    if(hasQuery) QueryPerformanceCounter(&liStart);
    GrabSendTables();
    if(hasQuery) QueryPerformanceCounter(&liStop);
    Msg("[SOURCEOP] \"Console<0><Console><Console>\" loaded tables in \"%f\" seconds.\n", hasQuery ? (double)(liStop.QuadPart - liStart.QuadPart) / (double) liFrequency.QuadPart : -1);

    if(hasQuery) QueryPerformanceCounter(&liStart);
    VFuncs::LoadOffsets();

    SH_MANUALHOOK_RECONFIGURE(CBaseEntity_Spawn, offs[OFFSET_SPAWN], 0, 0);
    SH_MANUALHOOK_RECONFIGURE(CBaseEntity_Precache, offs[OFFSET_PRECACHE], 0, 0);
    SH_MANUALHOOK_RECONFIGURE(CBaseEntity_AcceptInput, offs[OFFSET_ACCEPTINPUT], 0, 0);
    SH_MANUALHOOK_RECONFIGURE(CBaseEntity_Touch, offs[OFFSET_TOUCH], 0, 0);
    SH_MANUALHOOK_RECONFIGURE(CBaseEntity_StartTouch, offs[OFFSET_STARTTOUCH], 0, 0);
    SH_MANUALHOOK_RECONFIGURE(CBaseEntity_Think, offs[OFFSET_THINK], 0, 0);
    SH_MANUALHOOK_RECONFIGURE(CBaseEntity_KeyValue, offs[OFFSET_KEYVALUE], 0, 0);
    SH_MANUALHOOK_RECONFIGURE(CBaseEntity_Use, offs[OFFSET_USE], 0, 0);
    SH_MANUALHOOK_RECONFIGURE(CBaseEntity_TraceAttack, offs[OFFSET_TRACEATTACK], 0, 0);
    SH_MANUALHOOK_RECONFIGURE(CBaseEntity_OnTakeDamage, offs[OFFSET_ONTAKEDAMAGE], 0, 0);
    SH_MANUALHOOK_RECONFIGURE(CBaseEntity_FireBullets, offs[OFFSET_FIREBULLETS], 0, 0);
    SH_MANUALHOOK_RECONFIGURE(CTFPlayer_Event_Killed, offs[OFFSET_EVENTKILLED], 0, 0);
    SH_MANUALHOOK_RECONFIGURE(CBasePlayer_Weapon_Switch, offs[OFFSET_WEAPONSWITCH], 0, 0);
    SH_MANUALHOOK_RECONFIGURE(CBaseCombatWeapon_PrimaryAttack, offs[OFFSET_PRIMARYATTACK], 0, 0);
    SH_MANUALHOOK_RECONFIGURE(CBaseCombatWeapon_SecondaryAttack, offs[OFFSET_SECONDARYATTACK], 0, 0);
    SH_MANUALHOOK_RECONFIGURE(CBasePlayer_ItemPostFrame, offs[OFFSET_ITEMPOSTFRAME], 0, 0);
    SH_MANUALHOOK_RECONFIGURE(CBasePlayer_CanHearAndReadChatFrom, offs[OFFSET_CANHEARANDREADCHATFROM], 0, 0);

    if(pAdminOP.isTF2)
    {
        SH_MANUALHOOK_RECONFIGURE(CTFPlayer_GiveNamedScriptItem, offs[OFFSET_GIVENAMEDSCRIPTITEM], 0, 0);
    }

    SH_MANUALHOOK_RECONFIGURE(CBaseClient_Connect, offs[OFFSET_CONNECT], 0, 0);
    SH_MANUALHOOK_RECONFIGURE(CBaseClient_Disconnect, offs[OFFSET_DISCONNECT], 0, 0);
    SH_MANUALHOOK_RECONFIGURE(CBaseClient_ExecuteStringCommand, offs[OFFSET_EXECSTRINGCMD], 0, 0);
    SH_MANUALHOOK_RECONFIGURE(CBaseClient_FillUserInfo, offs[OFFSET_FILLUSERINFO], 0, 0);

    SH_MANUALHOOK_RECONFIGURE(CBaseServer_ConnectClient, offs[OFFSET_CONNECTCLIENT], 0, 0);
    SH_MANUALHOOK_RECONFIGURE(CBaseServer_GetFreeClient, offs[OFFSET_GETFREECLIENT], 0, 0);

    if(hasQuery) QueryPerformanceCounter(&liStop);
    Msg("[SOURCEOP] \"Console<0><Console><Console>\" loaded offsets in \"%f\" seconds.\n", hasQuery ? (double)(liStop.QuadPart - liStart.QuadPart) / (double) liFrequency.QuadPart : -1);

    gameRules = new CSOPGameRulesProxy();
    mapCycleTracker = new CMapCycleTracker();

    if(hasQuery) QueryPerformanceCounter(&liStart);
    if(MemPatcher())
    {
        if(!isCstrike && !isTF2 && _CreateCombineBall == NULL)
            TimeLog("SourceOPErrors.log", "CreateCombineBall was not found inside of the server dll.\n");
        if(servertools == NULL && _CreateEntityByName == NULL)
            TimeLog("SourceOPErrors.log", "CreateEntityByName was not found inside of the server dll.\n");
        if(g_nServerToolsVersion < 2 && ( _ClearMultiDamage == NULL || _ApplyMultiDamage == NULL ))
            TimeLog("SourceOPErrors.log", "Either ClearMultiDamage or ApplyMultiDamage was not found inside of the server dll.\n");
        if(_RadiusDamage == NULL)
            TimeLog("SourceOPErrors.log", "RadiusDamage was not found inside of the server dll.\n");
        if(g_nServerToolsVersion < 2 && _SetMoveType == NULL)
            TimeLog("SourceOPErrors.log", "SetMoveType was not found inside of the server dll. Will use fallback method.\n");
        if(g_nServerToolsVersion < 2 && _ResetSequence == NULL)
            TimeLog("SourceOPErrors.log", "ResetSequence was not found inside of the server dll.\n");
        if(_NET_SendPacket == NULL)
            TimeLog("SourceOPErrors.log", "NET_SendPacket was not found inside of the engine dll.\n");
        if(_SV_BroadcastVoiceData == NULL)
            TimeLog("SourceOPErrors.log", "SV_BroadcastVoiceData was not found inside of the engine dll.\n");
    }
    else
    {
        TimeLog("SourceOPErrors.log", "There was an error trying to find certain functions inside of the server dll.\n");
    }
    if(hasQuery) QueryPerformanceCounter(&liStop);
    Msg("[SOURCEOP] \"Console<0><Console><Console>\" ran patcher in \"%f\" seconds.\n", hasQuery ? (double)(liStop.QuadPart - liStart.QuadPart) / (double) liFrequency.QuadPart : -1);

    // VCR recvfrom
    VCR_Hook_recvfrom = g_pVCR->Hook_recvfrom;
    g_pVCR->Hook_recvfrom = SOP_recvfrom;


    if(featureStatus[FEAT_REMOTE])
    {
        SOPAdminStart();
    }

    sprintf(adminname, "SourceOP");
    getCreateEdict = 0;

    sTeamNames = sTeamNamesDefault;
    if(isHl2mp)
        sTeamNames = sTeamNamesHL2MP;
    else if(isCstrike)
        sTeamNames = sTeamNamesCS;
    else if(isDod)
        sTeamNames = sTeamNamesDOD;
    else if(isFF)
        sTeamNames = sTeamNamesFF;
    else if(isTF2)
        sTeamNames = sTeamNamesTF2;

    if( g_nServerToolsVersion >= 2 )
    {
        te = servertools->GetTempEntsSystem();
    }

#ifdef __linux__
    // retrieved earlier using dlsym
    if(!te && isFF)
    {
        DWORD step1 = ((*(DWORD*) effects ) + 0x10);
        DWORD step2 = (*(DWORD*)step1);
        te = **(ITempEntsSystem***)( step2 + 0x42 );
    }
#else
    if(!te && isFF)
    {
        DWORD step1 = ((*(DWORD*) effects ) + 0x0C);
        DWORD step2 = (*(DWORD*)step1) + 1;
        DWORD step3 = (*(DWORD*)step2);
        DWORD step4 = step2 + step3 + 4;
        te = **(ITempEntsSystem***)( step4 + 0x6B );
    }
#endif

#ifdef DFENTS
    memset(DFEntities, 0, sizeof(DFEntities));
    for(int i=0;i<DF_MAX_ENTS;i++)
    {
        DFEntities[i] = NULL;
    }
#endif

    maxClients = 0;
    saveCredits = 0;
    attemptedCreditLoad = 0;
    creditList.Purge();
    m_mapSteamIDToCreditEntry.Purge();
    entList.Purge();
    myEntList.Purge();
    myThinkEnts.Purge();
    radioLoops.Purge();
    dataDesc.Purge();
    installedFactories.Purge();

    vacAllowList.Purge();
#ifdef OFFICIALSERV_ONLY
    // Drunken F00l
    vacAllowList.AddToTail(CSteamID(76561197961366266ULL));
    // Rukes
    vacAllowList.AddToTail(CSteamID(76561197961036195ULL));
#endif

    memset(&pAdminOP.StoredGameServerInitParams, 0, sizeof(pAdminOP.StoredGameServerInitParams));

    hasDownloadUsers = 0;
    jetpackVoteNextTime = 0;
    inRound = isCstrike ? 0 : 1;
    blockLogOutputToClient = -1;
    m_bEndRoundAllTalk = false;
    bPlayerResourceCached = 0;
    pPlayerResource = NULL;
    if(rslots)
    {
        rslots->Reset();
        rslots->SetReservedSlots(rslots_slots.GetInt());
    }
    m_flNextSlotsRefresh = 0.0f;

    hookedEnts = 0;
    hookedWorld = 0;
    hooks = 0;
    unhooks = 0;
    g_hookedVTables.Purge();

    m_flNextHeartbeat = 0.0f;

    overrideMapTime = 0;
    overrideMapTimeSetTime = 0;

    luaState = NULL;
    if(featureStatus[FEAT_LUA])
    {
        ColorMsg(CONCOLOR_LUA, "[SOURCEOP] \"Console<0><Console><Console>\" Loading Lua...\n");
        if(hasQuery) QueryPerformanceCounter(&liStart);
        LoadLua();
        if(hasQuery) QueryPerformanceCounter(&liStop);
        if(luaState)
            Msg("[SOURCEOP] \"Console<0><Console><Console>\" loaded Lua in \"%f\" seconds.\n", hasQuery ? (double)(liStop.QuadPart - liStart.QuadPart) / (double) liFrequency.QuadPart : -1);
        else
            Msg("[SOURCEOP] \"Console<0><Console><Console>\" failed to load Lua. Took \"%f\" seconds.\n", hasQuery ? (double)(liStop.QuadPart - liStart.QuadPart) / (double) liFrequency.QuadPart : -1);
    }

    clientCommands = new ClientCommands;
    clientCommands->Init();

    ProcessInstallList();

    if(isClient)
        pAdminOP.HookSteam();

    SOPDLog("Plugin loaded.\n");
}

void CAdminOP :: LevelInit( char const *pMapName )
{
    char strMapTo[512];
    char strMapFrom[512];
    bool hasQuery = false;
    LARGE_INTEGER liFrequency,liStart,liStop;

    SOPDLog("LevelInit\n");

    sv_cheats = cvar->FindVar("sv_cheats");
    nextlevel = cvar->FindVar("nextlevel");
    mapcyclefile = cvar->FindVar("mapcyclefile");

#if defined(_SOPDEBUG) && defined(OFFICIALSERV_ONLY)
    GrabSendTablesPrecalc();
#endif

    // Valve's map cycle will now advance to the map chosen when manually changing level
    // this is a fix for that
    int startingPosition = mapCycleTracker->GetCurrentCycleIndex();
    int currentPosition = startingPosition;
    int numAdvances = 0;
    bool brokeOnPosition = false;
    bool brokeOnInfiniteLoop = false;
    while(true)
    {
        char currentCycleMap[256];
        mapCycleTracker->GetCurrentLevelName(currentCycleMap, sizeof(currentCycleMap));
        if(!stricmp(currentCycleMap, pMapName))
            break;

        numAdvances++;
        mapCycleTracker->AdvanceCycle(true);
        currentPosition = mapCycleTracker->GetCurrentCycleIndex();

        if(currentPosition == startingPosition)
        {
            brokeOnPosition = true;
            break;
        }

        if(numAdvances >= 5000)
        {
            brokeOnInfiniteLoop = true;
            break;
        }
    }

    if(brokeOnInfiniteLoop)
    {
        ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Map cycle tracker tried advancing to the current map\n           but failed. This shouldn't happen. Please report it.\n           %i items in list. Started at %i.\n", mapCycleTracker->GetNumMapsInList(), startingPosition);
    }
    else if (brokeOnPosition)
    {
        ColorMsg(CONCOLOR_DARKGRAY, "[SOURCEOP] Map cycle tracker did not find the current map in the map cycle.\n");
    }
    else if (numAdvances > 0)
    {
        ColorMsg(CONCOLOR_DARKGRAY, "[SOURCEOP] Map cycle tracker advanced the cycle %i times to the current map.\n", numAdvances);
    }

    if(QueryPerformanceFrequency(&liFrequency)) hasQuery = true;

    /************************\
    |Code from the server dll|
    |   to get the nextmap   |
    \************************/
    char szNextMap[32];
    char szFirstMapInList[32];
    Q_strncpy( szFirstMapInList, "hldm1" ,sizeof(szFirstMapInList));  // the absolute default level is hldm1

    // find the map to change to

    const char *mapcfile = mapcyclefile->GetString();
    //Assert( mapcfile != NULL );
    Q_strncpy( szNextMap, pMapName ,sizeof(szNextMap));
    Q_strncpy( szFirstMapInList, pMapName ,sizeof(szFirstMapInList));

    int length;
    char *pFileList;
    char *aFileList = pFileList = (char*)UTIL_LoadFileForMe( mapcfile, &length );
    if ( pFileList && length )
    {
        // the first map name in the file becomes the default
        sscanf( pFileList, " %31s", szNextMap );
        if ( DFIsMapValid( szNextMap ) )
            Q_strncpy( szFirstMapInList, szNextMap ,sizeof(szFirstMapInList));

        // keep pulling mapnames out of the list until the current mapname
        // if the current mapname isn't found,  load the first map in the list
        bool next_map_is_it = false;
        while ( 1 )
        {
            while ( *pFileList && isspace( *pFileList ) ) pFileList++; // skip over any whitespace
            if ( !(*pFileList) )
                break;

            char cBuf[32];
            int ret = sscanf( pFileList, " %31s", cBuf );
            // Check the map name is valid
            if ( ret != 1 || *cBuf < 13 )
                break;

            if ( next_map_is_it )
            {
                // check that it is a valid map file
                if ( DFIsMapValid( cBuf ) )
                {
                    Q_strncpy( szNextMap, cBuf ,sizeof(szNextMap));
                    break;
                }
            }

            if ( FStrEq( cBuf, pMapName ) )
            {  // we've found our map;  next map is the one to change to
                next_map_is_it = true;
            }

            pFileList += strlen( cBuf );
        }

        UTIL_FreeFile( (byte *)aFileList );
    }

    if ( !DFIsMapValid(szNextMap) )
        Q_strncpy( szNextMap, szFirstMapInList ,sizeof(szNextMap));

    strcpy(nextmap, szNextMap);
    // end nextmap code

    Q_strncpy( szCurrentMap, pMapName ,sizeof(szCurrentMap));
    TimeStarted = engine->Time();

    PropogateMapList();

    if(featureStatus[FEAT_ENTCOMMANDS] || featureStatus[FEAT_REMOTE])
    {
        if(hasQuery) QueryPerformanceCounter(&liStart);
        LoadSpawnAlias();
        if(hasQuery) QueryPerformanceCounter(&liStop);
        engine->LogPrint(UTIL_VarArgs("[SOURCEOP] \"Console<0><Console><Console>\" loaded spawn alias file in \"%f\" seconds. \"%i\" entries.\n", hasQuery ? (double)(liStop.QuadPart - liStart.QuadPart) / (double) liFrequency.QuadPart : -1, spawnAlias.Count()));
    }

    if(hasQuery) QueryPerformanceCounter(&liStart);
    LoadAdmins();
    if(hasQuery) QueryPerformanceCounter(&liStop);
    engine->LogPrint(UTIL_VarArgs("[SOURCEOP] \"Console<0><Console><Console>\" loaded admins file in \"%f\" seconds\n", hasQuery ? (double)(liStop.QuadPart - liStart.QuadPart) / (double) liFrequency.QuadPart : -1));

    if(featureStatus[FEAT_CREDITS])
    {
        if(hasQuery) QueryPerformanceCounter(&liStart);
        LoadRank();
        if(hasQuery) QueryPerformanceCounter(&liStop);
        engine->LogPrint(UTIL_VarArgs("[SOURCEOP] \"Console<0><Console><Console>\" loaded ranks file in \"%f\" seconds\n", hasQuery ? (double)(liStop.QuadPart - liStart.QuadPart) / (double) liFrequency.QuadPart : -1));
    }

    if(featureStatus[FEAT_RADIO])
    {
        if(hasQuery) QueryPerformanceCounter(&liStart);
        LoadRadioLoops();
        if(hasQuery) QueryPerformanceCounter(&liStop);
        engine->LogPrint(UTIL_VarArgs("[SOURCEOP] \"Console<0><Console><Console>\" loaded radio loops file in \"%f\" seconds\n", hasQuery ? (double)(liStop.QuadPart - liStart.QuadPart) / (double) liFrequency.QuadPart : -1));
    }

    if(featureStatus[FEAT_CREDITS])
    {
        if(creditList.Count() == 0)
        {
            if(hasQuery) QueryPerformanceCounter(&liStart);
            LoadCreditsFromFile();
            if(hasQuery) QueryPerformanceCounter(&liStop);
            engine->LogPrint(UTIL_VarArgs("[SOURCEOP] \"Console<0><Console><Console>\" loaded credits file in \"%f\" seconds\n", hasQuery ? (double)(liStop.QuadPart - liStart.QuadPart) / (double) liFrequency.QuadPart : -1));
        }
        FOR_EACH_VEC( creditList, i )
        {
            creditsram_t *ramCredits = &creditList.Element(i);
            memset(&ramCredits->thismap, 0, sizeof(ramCredits->thismap));
        }
    }

    UpdateSourceOPMaster(pMapName);

    spawnedServerEnts.Purge();

    bool bAnyFileCopied = false;
    filesystem->CreateDirHierarchy("cfg/SourceOP", "MOD");
    sprintf(strMapFrom, "%s/cfg/sourceop.cfg", DataDir());
    if(filesystem->FileExists(strMapFrom))
    {
        engine->CopyFile(strMapFrom, "cfg/SourceOP/sourceop.cfg");
        engine->ServerCommand("exec SourceOP/sourceop.cfg\n");
        bAnyFileCopied = true;
    }
    sprintf(strMapFrom, "%s/cfg/%s.cfg", DataDir(), pMapName);
    if(filesystem->FileExists(strMapFrom))
    {
        sprintf(strMapTo, "cfg/SourceOP/%s.cfg", pMapName);
        engine->CopyFile(strMapFrom, strMapTo);
        engine->ServerCommand(UTIL_VarArgs("exec SourceOP/%s.cfg\n", pMapName));
        bAnyFileCopied = true;
    }
    if(bAnyFileCopied)
    {
        engine->ServerExecute();
    }
    else
    {
        Msg("[SOURCEOP] No SourceOP cfg files found.\n");
    }

    //DFAdminOPAdminInit(); // start the AdminOP Admin server if not already running

    g_CheckClient.LevelInitPreEntity();

    // we only need to hook entities for admin copyent currently
    // also need it for TakeDamage multiplier, but thats ok
    g_hookIDS.Purge();
    if(!bHasDataDesc)
    {
        GrabDataDesc(NULL);
        bHasDataDesc = true;
    }
    VFuncs::LoadDataDescOffsets();
    if(featureStatus[FEAT_ENTCOMMANDS])
    {
        if(!hookedEnts)
        {
            if(isTF2)
            {
                // needed before hooking entities
                enginesound->PrecacheSound("weapons/sapper_timer.wav", true);
                enginesound->PrecacheSound("weapons/dispenser_idle.wav", true);
            }
            HookEntities();
            hookedEnts = 1;
        }
    }

    ShuttingDown = 0;
    MapShutDown = 0;
    m_iSayTextPlayerIndex = 0;
    Kills.firstblood = 0;
    m_flNextHeartbeat = 0.0f;
    changeMapTime = 0;
    m_bEndRoundAllTalk = false;
    mapVote.EndVote();
    mapVote.lastVote = engine->Time();
    mapVote.hasVoted = 0;
    voteExtendCount = 0;
    nextMapPosUpdate = 0;
    jetpackVoteNextTime = gpGlobals->curtime + jetpackvote_freq.GetFloat();
    cvarVote.EndVote();
    cvarVote.lastVote = engine->Time();
    cvarVote.hasVoted = 0;

    getCreateEdict = 0;
    entList.Purge();
    for(int i=0; i < myEntList.Count(); i++)
    {
        if(myEntList[i] != NULL)
        {
            // should not see me
            // this might occur after a map that, when loading, causes:
            // [SOURCEOP] VFuncs::entindex got a null pEnt for ent info_camera_link (CBaseEntity).
            // or whatever entity
            // to remedy: add entity to deny list
            ColorMsg(CONCOLOR_LIGHTRED, "Inconsistency with what has been added to ent list and those removed.\n", i);
        }
    }
    //myEntList.Purge();

    bPlayerResourceCached = 0;
    pPlayerResource = NULL;

    if(rslots)
    {
        // update number of slots regardless of the enabled cvar
        rslots->SetReservedSlots(rslots_slots.GetInt());
        if(rslots_enabled.GetBool())
        {
            if(!rslots->LoadReservedUsersInThread())
            {
                ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Error starting reserve slot loader thread.\n");
            }
        }
    }
    m_flNextSlotsRefresh = 0.0f;

    if(featureStatus[FEAT_LUA])
    {
        lua_getglobal(GetLuaState(), "hook");
        lua_pushliteral(GetLuaState(), "Call");
        lua_gettable(GetLuaState(), -2);
        lua_remove(GetLuaState(), -2);
        lua_pushstring(GetLuaState(), "LevelInit");
        lua_pushstring(GetLuaState(), pMapName);
        lua_pcall(GetLuaState(), 2, 0, 0);

        lua_gc(luaState, LUA_GCCOLLECT, 0);
    }

    SOPDLog("LevelInit finished\n");
}

void CAdminOP :: ServerActivate( edict_t *pEdictList, int edictCount, int clientMax )
{
    bool hasQuery = false;
    LARGE_INTEGER liFrequency,liStart,liStop;
    int i = 0;

    SOPDLog("ServerActivate\n");
/*#ifdef OFFICIALSERV_ONLY
    GetVTables(NULL);
#endif*/

    if(QueryPerformanceFrequency(&liFrequency)) hasQuery = true;

    pEList = pEdictList;
    maxClients = clientMax;

    for(i = 0; i < clientMax; i++)
    {
        pAOPPlayers[i].SetEntID(i+1);
    }
    memset(userMessages, 0, sizeof(userMessages));
    for(int i = 0; i < USR_MSGS_MAX; i++)
    {
        char name[64];
        int size;
        int returned;
        returned = servergame->GetUserMessageInfo(i, name, sizeof(name), size);
        if(returned > 0)
        {
            Q_snprintf(userMessages[i], sizeof(userMessages[0]), "%s", name);
        }
    }

    gameRules->Init();

    // Get nextmap from game rules
    // nextmap is retrieved earlier from maplist in levelinit
    // this method is more reliable
    if(gameRules->IsInitialized())
    {
        gameRules->GetNextLevelName(nextmap, sizeof(nextmap));
    }

    if(hasQuery) QueryPerformanceCounter(&liStart);
    LoadPrecached();
    if(hasQuery) QueryPerformanceCounter(&liStop);
    engine->LogPrint(UTIL_VarArgs("[SOURCEOP] \"Console<0><Console><Console>\" loaded precache file in \"%f\" seconds\n", hasQuery ? (double)(liStop.QuadPart - liStart.QuadPart) / (double) liFrequency.QuadPart : -1));

    if(hasQuery) QueryPerformanceCounter(&liStart);
    LoadForceDownload();
    if(hasQuery) QueryPerformanceCounter(&liStop);
    engine->LogPrint(UTIL_VarArgs("[SOURCEOP] \"Console<0><Console><Console>\" loaded force download file in \"%f\" seconds\n", hasQuery ? (double)(liStop.QuadPart - liStart.QuadPart) / (double) liFrequency.QuadPart : -1));

    for(unsigned short i = precached.Head(); i != precached.InvalidIndex(); i = precached.Next(i))
    {
        precached_t *precache = &precached.Element(i);
        if(precache->Type == 1)
            engine->PrecacheModel(precache->FileName, true);
        else if(precache->Type == 2)
            enginesound->PrecacheSound(precache->FileName, true);
    }

    g_sModelIndexSmoke = engine->PrecacheModel("sprites/steam1.vmt");
    g_physgunBeam = engine->PrecacheModel(PHYSGUN_BEAM_SPRITE);

    if(featureStatus[FEAT_JETPACK])
    {
        engine->PrecacheModel("particle/particle_smokegrenade.vmt");
        engine->PrecacheModel("sprites/heatwave.vmt");
    }

    enginesound->PrecacheSound("physics/body/body_medium_impact_hard6.wav", true);
    enginesound->PrecacheSound("ambient/gas/cannister_loop.wav", true);
    enginesound->PrecacheSound("common/null.wav", true);
    enginesound->PrecacheSound("^weapons/explode3.wav", true);
    enginesound->PrecacheSound("^weapons/explode4.wav", true);
    enginesound->PrecacheSound("^weapons/explode5.wav", true);
    engine->PrecacheModel("models/props_junk/sawblade001a.mdl", true);
    engine->PrecacheModel("models/props_wasteland/coolingtank02.mdl", true);

    if(featureStatus[FEAT_KILLSOUNDS])
    {
        enginesound->PrecacheSound("SourceOP/dominating.wav", true);
        enginesound->PrecacheSound("SourceOP/doublekill.wav", true);
        enginesound->PrecacheSound("SourceOP/firstblood.wav", true);
        enginesound->PrecacheSound("SourceOP/godlike.wav", true);
        enginesound->PrecacheSound("SourceOP/killingspree.wav", true);
        enginesound->PrecacheSound("SourceOP/monsterkill.wav", true);
        enginesound->PrecacheSound("SourceOP/multikill.wav", true);
        enginesound->PrecacheSound("SourceOP/rampage.wav", true);
        enginesound->PrecacheSound("SourceOP/ultrakill.wav", true);
        enginesound->PrecacheSound("SourceOP/unstoppable.wav", true);
    }

    PrecacheInstallList();

    INetworkStringTable *pDownloadablesTable = networkstringtable->FindTable("downloadables");

    g_pStringTableParticleEffectNames = networkstringtable->FindTable("ParticleEffectNames");
    if(pDownloadablesTable)
    {
        char file[256];
        bool save = engine->LockNetworkStringTables(false);
        for(unsigned short i = downloads.Head(); i != downloads.InvalidIndex(); i = downloads.Next(i))
        {
            pDownloadablesTable->AddString(true, downloads.Element(i));
        }

        sprintf(file, "scripts/vehicles/airboat.txt");
        pDownloadablesTable->AddString(true, file, sizeof(file));
        sprintf(file, "scripts/vehicles/jeep_test.txt");
        pDownloadablesTable->AddString(true, file, sizeof(file));
        if(featureStatus[FEAT_KILLSOUNDS])
        {
            sprintf(file, "sound/SourceOP/dominating.wav");
            pDownloadablesTable->AddString(true, file, sizeof(file));
            sprintf(file, "sound/SourceOP/doublekill.wav");
            pDownloadablesTable->AddString(true, file, sizeof(file));
            sprintf(file, "sound/SourceOP/firstblood.wav");
            pDownloadablesTable->AddString(true, file, sizeof(file));
            sprintf(file, "sound/SourceOP/godlike.wav");
            pDownloadablesTable->AddString(true, file, sizeof(file));
            sprintf(file, "sound/SourceOP/killingspree.wav");
            pDownloadablesTable->AddString(true, file, sizeof(file));
            sprintf(file, "sound/SourceOP/monsterkill.wav");
            pDownloadablesTable->AddString(true, file, sizeof(file));
            sprintf(file, "sound/SourceOP/multikill.wav");
            pDownloadablesTable->AddString(true, file, sizeof(file));
            sprintf(file, "sound/SourceOP/rampage.wav");
            pDownloadablesTable->AddString(true, file, sizeof(file));
            sprintf(file, "sound/SourceOP/ultrakill.wav");
            pDownloadablesTable->AddString(true, file, sizeof(file));
            sprintf(file, "sound/SourceOP/unstoppable.wav");
            pDownloadablesTable->AddString(true, file, sizeof(file));
        }

        engine->LockNetworkStringTables(save);
    }

    HookSteamGameServer();

    if(featureStatus[FEAT_LUA])
    {
        lua_getglobal(GetLuaState(), "hook");
        lua_pushliteral(GetLuaState(), "Call");
        lua_gettable(GetLuaState(), -2);
        lua_remove(GetLuaState(), -2);
        lua_pushstring(GetLuaState(), "ServerActivate");
        lua_pushinteger(GetLuaState(), edictCount);
        lua_pushinteger(GetLuaState(), clientMax);
        lua_pcall(GetLuaState(), 3, 0, 0);

        lua_gc(luaState, LUA_GCCOLLECT, 0);
    }

    // TODO: seniorproj: Init remote server's list of CVARs
    SOPDLog("ServerActivate finish\n");
}

void CAdminOP :: GameFrame( bool simulating )
{
#ifdef OFFICIALSERV_ONLY
    static int frameCount = 0;

    frameCount++;
    if(frameCount >= 100)
    {
        SOPDLog("GameFrame * 100\n");
        frameCount = 0;
    }
#endif
    static int steamTries = 0;
    if(!_BGetCallback && steamTries < 5)
    {
        HookSteam();
        steamTries++;
    }

    if(g_bShouldResetVac)
    {
        g_iResetVacInFrames--;
        if(g_iResetVacInFrames <= 0)
        {
            /*this->SteamGameServer()->SetServerType(pAdminOP.StoredGameServerInitParams.unServerFlags,
                pAdminOP.StoredGameServerInitParams.unIP,
                pAdminOP.StoredGameServerInitParams.usGamePort,
                pAdminOP.StoredGameServerInitParams.usSpectatorPort,
                pAdminOP.StoredGameServerInitParams.usQueryPort,
                pAdminOP.StoredGameServerInitParams.pszGameDir,
                pAdminOP.StoredGameServerInitParams.pszVersionString,
                pAdminOP.StoredGameServerInitParams.bLanMode);*/

            g_bShouldResetVac = false;
        }
    }

    if(featureStatus[FEAT_LUA])
    {
        static int sinceLast = 0;
        lua_getglobal(GetLuaState(), "hook");
        lua_pushliteral(GetLuaState(), "Call");
        lua_gettable(GetLuaState(), -2);
        lua_remove(GetLuaState(), -2);
        lua_pushstring(GetLuaState(), "Think");
        lua_pcall(GetLuaState(), 1, 0, 0);

        if(lua_gc_stepeveryframe.GetBool())
        {
            lua_gc(GetLuaState(), LUA_GCSTEP, 10);
        }
    }
    if(featureStatus[FEAT_REMOTE])
    {
        SOPAdminFrame();
    }
    for(int i = 0; i < maxClients; i++)
    {
        if(pAOPPlayers[i].GetPlayerState() == 2)
        {
        //IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pEList+i+1);
        //if(info)
        //{
            //if(info->IsConnected())
            //{
                if(pAOPPlayers[i].nextthink <= Plat_FloatTime())
                {
                    pAOPPlayers[i].Think();
                }
                pAOPPlayers[i].HighResolutionThink();
            //}
        //}
        }
    }

    gEntList.CleanupDeleteList();

    extern void ServiceEventQueue( void );
    ServiceEventQueue();

    if(mapVote.VoteInProgress())
    {
        if(mapVote.endTime <= engine->Time())
        {
            EndMapVote();
        }
    }
    if(cvarVote.VoteInProgress())
    {
        if(cvarVote.endTime <= engine->Time())
        {
            EndCvarVote();
        }
    }

    if(m_flNextHeartbeat <= gpGlobals->curtime && extra_heartbeat.GetFloat() >= 0.5f)
    {
        engine->ServerCommand("heartbeat\n");
        m_flNextHeartbeat = gpGlobals->curtime + extra_heartbeat.GetFloat();
    }

    if(m_flNextSlotsRefresh <= gpGlobals->curtime && rslots_refresh.GetFloat() >= 0.5f)
    {
        if(rslots && rslots_enabled.GetBool())
        {
            if(!rslots->LoadReservedUsersInThread())
            {
                ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Error starting reserve slot loader thread.\n");
            }
        }

        m_flNextSlotsRefresh = gpGlobals->curtime + rslots_refresh.GetFloat();
    }

    if(changeMapTime)
    {
        if(changeMapTime <= engine->Time())
        {
            changeMapTime = 0;
            engine->ServerCommand(UTIL_VarArgs("changelevel %s\n", szChangeMap));
        }
    }
    for(int i = 0; i < myThinkEnts.Count(); i++)
    {
        if(myThinkEnts[i]->GetBase())
            myThinkEnts[i]->MyThink();
    }
    if(isTF2)
    {
        if(tf2_fastrespawn.GetInt() >= 1)
        {
            if(mp_respawnwavetime) mp_respawnwavetime->SetValue(-20);
        }
    }

#ifdef OFFICIALSERV_ONLY
    static bool filepathfilled = false;
    static char filepath[512];

    if(!filepathfilled)
    {
        Q_snprintf(filepath, sizeof(filepath), "%s/%s/attack_log.txt", gameDir, DataDir());
        V_FixSlashes(filepath);
        filepathfilled = true;
    }

    if(g_iConnectionlessThisFrame > (unsigned int)serverquery_maxconnectionless.GetInt())
    {
        g_iConsecutiveConnectionlessOverLimit++;
        if(g_iConsecutiveConnectionlessOverLimit > 10 && g_bShouldWriteOverLimitLog)
        {
            FILE *fp = fopen(filepath, "a");
            if(fp)
            {
                static char fullstring[1280];
                struct tm timestruct;

                g_pVCR->Hook_LocalTime(&timestruct);
                int hours_ampm = timestruct.tm_hour % 12;
                const char * str_ampm = (timestruct.tm_hour >= 0 && timestruct.tm_hour < 12) ? "AM" : "PM";
                if ( hours_ampm == 0 )
                    hours_ampm = 12;

                V_snprintf(fullstring, sizeof(fullstring), "[%02d/%02d/%02d %02d:%02d:%02d %s] Connectionless packet spam attack detected.\n", timestruct.tm_mon+1, timestruct.tm_mday, timestruct.tm_year % 100, hours_ampm, timestruct.tm_min, timestruct.tm_sec, str_ampm);
                fputs(fullstring, fp);

                fclose(fp);
            }

            g_bShouldWriteOverLimitLog = 0;
        }
    }
    else
    {
        // an attack doesn't have to affect every frame for it to be persistent
        // so only decrement consecutive here
        if(g_iConsecutiveConnectionlessOverLimit > 0 && (frameCount % 3) == 0)
        {
            g_iConsecutiveConnectionlessOverLimit--;
        }

        if(g_iConsecutiveConnectionlessOverLimit == 0 && !g_bShouldWriteOverLimitLog)
        {
            FILE *fp = fopen(filepath, "a");
            if(fp)
            {
                static char fullstring[1280];
                struct tm timestruct;

                g_pVCR->Hook_LocalTime(&timestruct);
                int hours_ampm = timestruct.tm_hour % 12;
                const char * str_ampm = (timestruct.tm_hour >= 0 && timestruct.tm_hour < 12) ? "AM" : "PM";
                if ( hours_ampm == 0 )
                    hours_ampm = 12;

                V_snprintf(fullstring, sizeof(fullstring), "[%02d/%02d/%02d %02d:%02d:%02d %s] Connectionless packet spam attack seems to have stopped.\n", timestruct.tm_mon+1, timestruct.tm_mday, timestruct.tm_year % 100, hours_ampm, timestruct.tm_min, timestruct.tm_sec, str_ampm);
                fputs(fullstring, fp);

                fclose(fp);
            }

            g_bShouldWriteOverLimitLog = 1;
        }
    }

    g_iConnectionlessThisFrame = 0;
#endif
    g_iA2SInfoCacheSize = -1;
}

void CAdminOP :: RoundStart( void )
{
    inRound = 1;
    m_bEndRoundAllTalk = false;

    if(featureStatus[FEAT_LUA])
    {
        lua_getglobal(GetLuaState(), "hook");
        lua_pushliteral(GetLuaState(), "Call");
        lua_gettable(GetLuaState(), -2);
        lua_remove(GetLuaState(), -2);
        lua_pushstring(GetLuaState(), "RoundStart");
        lua_pcall(GetLuaState(), 1, 0, 0);
    }
}

void CAdminOP :: ArenaRoundStart( void )
{
    if(featureStatus[FEAT_LUA])
    {
        lua_getglobal(GetLuaState(), "hook");
        lua_pushliteral(GetLuaState(), "Call");
        lua_gettable(GetLuaState(), -2);
        lua_remove(GetLuaState(), -2);
        lua_pushstring(GetLuaState(), "ArenaRoundStart");
        lua_pcall(GetLuaState(), 1, 0, 0);
    }
}

void CAdminOP :: RoundEnd( void )
{
    inRound = 0;
    for(int i = 1; i <= maxClients; i++)
    {
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pEList+i);
        if(info)
        {
            if(info->IsConnected())
            {
                pAOPPlayers[i-1].RoundEnd();
            }
        }
    }

    if(tf2_roundendalltalk.GetBool())
    {
        m_bEndRoundAllTalk = true;
    }

    if(featureStatus[FEAT_LUA])
    {
        lua_getglobal(GetLuaState(), "hook");
        lua_pushliteral(GetLuaState(), "Call");
        lua_gettable(GetLuaState(), -2);
        lua_remove(GetLuaState(), -2);
        lua_pushstring(GetLuaState(), "RoundEnd");
        lua_pcall(GetLuaState(), 1, 0, 0);
    }
}

void CAdminOP :: SetupStart( void )
{
    if(featureStatus[FEAT_LUA])
    {
        lua_getglobal(GetLuaState(), "hook");
        lua_pushliteral(GetLuaState(), "Call");
        lua_gettable(GetLuaState(), -2);
        lua_remove(GetLuaState(), -2);
        lua_pushstring(GetLuaState(), "SetupStart");
        lua_pcall(GetLuaState(), 1, 0, 0);
    }
}

void CAdminOP :: SetupEnd( void )
{
    if(featureStatus[FEAT_LUA])
    {
        lua_getglobal(GetLuaState(), "hook");
        lua_pushliteral(GetLuaState(), "Call");
        lua_gettable(GetLuaState(), -2);
        lua_remove(GetLuaState(), -2);
        lua_pushstring(GetLuaState(), "SetupEnd");
        lua_pcall(GetLuaState(), 1, 0, 0);
    }
}

void CAdminOP :: GameEvent( IGameEvent *gameEvent )
{
    if(featureStatus[FEAT_LUA] && gameeventmanager_old)
    {
        const char *eventName = gameEvent->GetName();
        KeyValues *keyVals = gameeventmanager_old->GetEvent(eventName);
        if(keyVals)
        {
            lua_getglobal(GetLuaState(), "hook");
            lua_pushliteral(GetLuaState(), "Call");
            lua_gettable(GetLuaState(), -2);
            lua_remove(GetLuaState(), -2);
            lua_pushstring(GetLuaState(), "GameEvent");

            // event name
            lua_pushstring(GetLuaState(), eventName);

            // event keyvals
            lua_newtable(GetLuaState());
            int idx = lua_gettop(GetLuaState());
            for ( KeyValues *kv = keyVals->GetFirstSubKey(); kv; kv = kv->GetNextKey() )
            {
                const char *keyName = kv->GetName();

                lua_pushstring(GetLuaState(), keyName);

                int valueType = kv->GetInt();
                switch(valueType)
                {
                case 1:
                    lua_pushstring(GetLuaState(), gameEvent->GetString(keyName));
                    break;
                case 2:
                    lua_pushnumber(GetLuaState(), gameEvent->GetFloat(keyName));
                    break;
                case 3:
                case 4:
                case 5:
                    lua_pushinteger(GetLuaState(), gameEvent->GetInt(keyName));
                    break;
                case 6:
                    lua_pushboolean(GetLuaState(), gameEvent->GetBool(keyName));
                    break;
                default:
                    lua_pushnil(GetLuaState());
                }

                lua_settable(GetLuaState(), idx);
            }

            // Do not delete keyVals. The engine maintains one instance of it.

            if(lua_pcall(GetLuaState(), 3, 0, 0) != 0)
            {
                CAdminOP::ColorMsg(CONCOLOR_LUA_ERR, "[SOURCEOP] Lua: error dispatching hook `GameEvent':\n %s\n", lua_tostring(GetLuaState(), -1));
            }
        }
    }
}

void CAdminOP :: LevelShutdown( void )
{
    bool hasQuery = false;
    LARGE_INTEGER liFrequency,liStart,liStop;

    SOPDLog("LevelShutdown\n");

    if(QueryPerformanceFrequency(&liFrequency)) hasQuery = true;

    changeMapTime = 0;
    ShuttingDown = 1;
    MapShutDown = 1;
    for(int i = 1; i <= maxClients; i++)
    {
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pEList+i);
        if(info)
        {
            if(info->IsConnected())
            {
                pAOPPlayers[i-1].LevelShutdown();
            }
        }
    }

    if(featureStatus[FEAT_CREDITS])
    {
        if(creditList.Count())
        {
            if(hasQuery) QueryPerformanceCounter(&liStart);
            SaveCreditsToFile();
            if(hasQuery) QueryPerformanceCounter(&liStop);
            engine->LogPrint(UTIL_VarArgs("[SOURCEOP] \"Console<0><Console><Console>\" saved credits to file in \"%f\" seconds\n", hasQuery ? (double)(liStop.QuadPart - liStart.QuadPart) / (double) liFrequency.QuadPart : -1));
        }
    }

    if(!DFIsAdminTutLocked())
    {
        LockAdminTut();
    }

    if(featureStatus[FEAT_LUA])
    {
        lua_getglobal(GetLuaState(), "hook");
        lua_pushliteral(GetLuaState(), "Call");
        lua_gettable(GetLuaState(), -2);
        lua_remove(GetLuaState(), -2);
        lua_pushstring(GetLuaState(), "LevelShutdown");
        lua_pcall(GetLuaState(), 1, 0, 0);
    }

    gameRules->LevelShutdown();

    SOPDLog("LevelShutdown finish.\n");
}

void CAdminOP :: PreShutdown( void )
{
    RemoveInstalledFactories();
}

void CAdminOP :: Unload( void )
{
    SOPDLog("Unloading plugin.\n");
    if(featureStatus[FEAT_LUA])
    {
        CloseLua();
    }
    if(featureStatus[FEAT_REMOTE])
    {
        SOPAdminEnd();
    }
    if(pAdminOP.FeatureStatus(FEAT_CREDITS)) SaveCreditsToFile(1);
    for(int i = 0; i < myEntList.Count(); i++)
    {
        CAOPEntity *myEnt = myEntList[i];
        if(myEnt)
        {
            //Msg("Unloading %i\n", VFuncs::entindex(myEnt->GetBase()));
            UnhookEnt(myEnt->GetBase(), myEnt, i);
        }
    }

    ColorMsg(CONCOLOR_DARKGRAY, "[SOURCEOP] Removing entity hooks.");
    for(int i = 0; i < g_hookIDS.Count(); i++)
    {
        if(i%(ENT_HOOKS_PERDOT*NUM_ENT_HOOKS) == 0) ColorMsg(CONCOLOR_DARKGRAY, ".");
        SH_REMOVE_HOOK_ID(g_hookIDS.Element(i));
    }
    Msg("\n");

    hookedEnts = 0;
    hookedWorld = 0;
    g_hookedVTables.Purge();

    if(VCR_Hook_recvfrom)
    {
        g_pVCR->Hook_recvfrom = VCR_Hook_recvfrom;
    }

    RemoveTrampolines();

    // TODO: Remove unused detour code
    if(NET_SendPacketDetour.Applied())
    {
        NET_SendPacketDetour.Remove();
    }
    if(SV_BroadcastVoiceDataDetour.Applied())
    {
        SV_BroadcastVoiceDataDetour.Remove();
    }

    if(clientCommands)
    {
        clientCommands->RemoveAll();
        delete clientCommands;
    }

    if(rslots)
    {
        rslots->SaveReservedUsersIfLoaded();
        delete rslots;
        rslots = NULL;
    }

    if(mapCycleTracker)
    {
        delete mapCycleTracker;
        mapCycleTracker = NULL;
    }

    if(gameRules)
    {
        gameRules->PluginUnloading();
        delete gameRules;
        gameRules = NULL;
    }

    SOPDLog("Unload finished.\n");
}

void CAdminOP :: LoadCreditsFromFile( void )
{
    char gamedir[256];
    char filepath[512];
    creditsram_t newCredits;
    FILE *fp;

    if(!featureStatus[FEAT_CREDITS])
    {
        return;
    }

    engine->GetGameDir(gamedir, sizeof(gamedir));

    Q_snprintf(filepath, sizeof(filepath), "%s/%s/DF_credits.cdb", gamedir, DataDir());
    V_FixSlashes(filepath);
    fp = fopen(filepath, "rb");

    saveCredits = 1;
    attemptedCreditLoad = 1;

    if(fp)
    {
        int numMerged = 0;
        credits_t CreditsInfo;
        creditsver_t verCheck;
        memset(&verCheck,0,sizeof(verCheck));

        creditList.Purge();
        m_mapSteamIDToCreditEntry.Purge();
        m_mapSteamIDToCreditEntry.SetLessFunc( DefLessFunc( uint64 ) );

        if(fread(&verCheck, sizeof(verCheck), 1, fp))
        {
            if(strcmp(verCheck.type, "SOPCRED") || verCheck.ver != 1)
            {
                engine->LogPrint(UTIL_VarArgs("[SOURCEOP] \"Console<0><Console><Console>\" incorrect credit file version\n"));
                TimeLog("SourceOPErrors.log", "Incorrect credit file version\n");
                saveCredits = 0;
                if(fp) fclose(fp);
                return;
            }
        }
        else
        {
            engine->LogPrint(UTIL_VarArgs("[SOURCEOP] \"Console<0><Console><Console>\" failed to load credit file version\n"));
            TimeLog("SourceOPErrors.log", "Failed to load credit file version\n");
            saveCredits = 0;
            if(fp) fclose(fp);
            return;
        }

        while (!feof(fp) ) 
        {
            if(fread(&CreditsInfo,sizeof(CreditsInfo),1,fp))
            {
                memset(&newCredits, 0, sizeof(newCredits));
                newCredits.credits = CreditsInfo.credits;
                strcpy(newCredits.CurrentName, CreditsInfo.CurrentName);
                strcpy(newCredits.FirstName, CreditsInfo.FirstName);
                newCredits.iuser1 = CreditsInfo.iuser1;
                strcpy(newCredits.LastName, CreditsInfo.LastName);
                newCredits.lastsave = CreditsInfo.lastsave;
                newCredits.timeonserver = CreditsInfo.timeonserver;
                newCredits.totalconnects = CreditsInfo.totalconnects;

                // Skip STEAM_ID_LAN
                if ( FStrEq( CreditsInfo.WonID, "STEAM_ID_LAN" ) )
                {
                    continue;
                }

                // Convert SteamID format
                char pszSteamIDError[64];
                if ( !IsValidSteamID( CreditsInfo.WonID, pszSteamIDError, sizeof( pszSteamIDError ) ) )
                {
                    CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Invalid SteamID %s in credits file: %s\n", CreditsInfo.WonID, pszSteamIDError );
                    saveCredits = 0;
                    continue;
                }

                CSteamID steamIDCredits( CreditsInfo.WonID, k_EUniversePublic );
                strcpy(newCredits.WonID, steamIDCredits.Render() );
                newCredits.steamid = steamIDCredits;

                int iMap = m_mapSteamIDToCreditEntry.Find( steamIDCredits.ConvertToUint64() );
                if ( iMap == m_mapSteamIDToCreditEntry.InvalidIndex() )
                {
                    int iVec = creditList.AddToTail(newCredits);
                    m_mapSteamIDToCreditEntry.Insert( steamIDCredits.ConvertToUint64(), iVec );
                }
                else
                {
                    int iVec = m_mapSteamIDToCreditEntry[iMap];

                    // Merge with the old entry
                    creditsram_t *oldCredits = creditList.Base() + iVec; // fast access

                    CAdminOP::ColorMsg(CONCOLOR_YELLOW, "[SOURCEOP] Merging duplicate credit entry %s at %i\n", newCredits.WonID, iVec );
                    CAdminOP::ColorMsg(CONCOLOR_YELLOW, "[SOURCEOP]  - Credits: add %6i credits  to existing %6i\n", newCredits.credits, oldCredits->credits );
                    CAdminOP::ColorMsg(CONCOLOR_YELLOW, "[SOURCEOP]  - Credits: add %6i time     to existing %6i\n", newCredits.timeonserver, oldCredits->timeonserver );
                    CAdminOP::ColorMsg(CONCOLOR_YELLOW, "[SOURCEOP]  - Credits: add %6i connects to existing %6i\n", newCredits.totalconnects, oldCredits->totalconnects );

                    oldCredits->credits += newCredits.credits;
                    if ( newCredits.lastsave > oldCredits->lastsave )
                    {
                        strcpy(oldCredits->CurrentName, newCredits.CurrentName);
                        oldCredits->lastsave = newCredits.lastsave;
                    }

                    oldCredits->timeonserver += newCredits.timeonserver;
                    oldCredits->totalconnects += newCredits.totalconnects;

                    numMerged++;
                }
            }
            /*else
            {
                engine->LogPrint(UTIL_VarArgs("[SOURCEOP] \"Console<0><Console><Console>\" failed to load credit entry \"%i\"\n", creditList.Count() + 1));
                TimeLog("SourceOPErrors.log", "Failed to load credit entry for \"%i\"\n", creditList.Count() + 1);
            }*/
        }
        fclose(fp);

        ColorMsg(CONCOLOR_DARKGRAY, "[SOURCEOP] Merged %i credit entries.\n", numMerged);
    }
    else
    {
        if(filesystem->FileExists(filepath))
        {
            saveCredits = 0;
            engine->LogPrint(UTIL_VarArgs("[SOURCEOP] \"Console<0><Console><Console>\" failed to load credit file, but file exists\n"));
            TimeLog("SourceOPErrors.log", "Failed to load credit file, but file exists\n");
            return;
        }
        else
        {
            engine->LogPrint(UTIL_VarArgs("[SOURCEOP] \"Console<0><Console><Console>\" found no credit database. A new one will be created.\n"));
            return;
        }
    }
    engine->LogPrint(UTIL_VarArgs("[SOURCEOP] \"Console<0><Console><Console>\" loaded \"%i\" credit entries\n", creditList.Count()));
    ColorMsg(CONCOLOR_DARKGRAY, "[SOURCEOP] Loaded %i credit entries.\n", creditList.Count());
}

void CAdminOP :: SaveCreditsToFile( bool isUnloading )
{
    char gamedir[256];
    char filepath[512];
    credits_t writeCredits;
    FILE *fp;

    if(!featureStatus[FEAT_CREDITS])
    {
        return;
    }

    if(!saveCredits)
    {
        if(attemptedCreditLoad)
        {
            engine->LogPrint(UTIL_VarArgs("[SOURCEOP] \"Console<0><Console><Console>\" due to load errors, credits are not saving\n"));
            TimeLog("SourceOPErrors.log", "Due to load errors, credits are not saving\n");
        }
        return;
    }

    engine->GetGameDir(gamedir, sizeof(gamedir));

    Q_snprintf(filepath, sizeof(filepath), "%s/%s/DF_credits.cdb", gamedir, DataDir());
    V_FixSlashes(filepath);
    fp = fopen(filepath, "wb");

    if(fp)
    {
        creditsver_t credVer;
        memset(&credVer,0,sizeof(credVer));
        strcpy(credVer.type, "SOPCRED");
        credVer.ver = 1;
        if(fwrite(&credVer, sizeof(credVer), 1, fp) == 0)
        {
            if(!isUnloading) engine->LogPrint(UTIL_VarArgs("[SOURCEOP] \"Console<0><Console><Console>\" failed to save credit entry for \"version\"\n"));
            TimeLog("SourceOPErrors.log", "Failed to save credit entry for \"version\"\n");
        }
        FOR_EACH_VEC( creditList, i )
        {
            creditsram_t *ramCredits = &creditList.Element(i);
            memset(&writeCredits, 0, sizeof(writeCredits));
            writeCredits.credits = ramCredits->credits;
            strcpy(writeCredits.CurrentName, ramCredits->CurrentName);
            strcpy(writeCredits.FirstName, ramCredits->FirstName);
            writeCredits.iuser1 = ramCredits->iuser1;
            strcpy(writeCredits.LastName, ramCredits->LastName);
            writeCredits.lastsave = ramCredits->lastsave;
            writeCredits.timeonserver = ramCredits->timeonserver;
            writeCredits.totalconnects = ramCredits->totalconnects;
            strcpy(writeCredits.WonID, ramCredits->WonID);
            if(fwrite(&writeCredits, sizeof(writeCredits), 1, fp) == 0)
            {
                if(!isUnloading) engine->LogPrint(UTIL_VarArgs("[SOURCEOP] \"Console<0><Console><Console>\" failed to save credit entry for \"%s;%i;%s\"\n", writeCredits.WonID, writeCredits.credits, writeCredits.CurrentName));
                TimeLog("SourceOPErrors.log", "Failed to save credit entry for \"%s;%i;%s\"\n", writeCredits.WonID, writeCredits.credits, writeCredits.CurrentName);
            }
        }
        if(!isUnloading) engine->LogPrint(UTIL_VarArgs("[SOURCEOP] \"Console<0><Console><Console>\" saved \"%i\" credit entries\n", creditList.Count()));
        fclose(fp);
    }
    else
    {
        if(!isUnloading) engine->LogPrint("[SOURCEOP] \"Console<0><Console><Console>\" failed to save credits\n");
    }
}

bool CAdminOP :: UnhideDevCvars( void )
{
    if(!cvar) return false;

    bool returnme = false;
    ConCommandBase *cmd = pAdminOP.GetCommands();
    for ( ; cmd; cmd = cmd->GetNext() )
    {
        if ( !cmd->IsCommand() )
        {
            if(cmd->IsFlagSet(FCVAR_DEVELOPMENTONLY))
            {
                const char *pszHelp = cmd->GetHelpText();
                CAdminOP::ColorMsg(CONCOLOR_DARKGRAY, "[SOURCEOP] Unhid CVAR: %s\n", cmd->GetName());
                if(pszHelp && pszHelp[0]) CAdminOP::ColorMsg(CONCOLOR_DARKGRAY, "[SOURCEOP]  - %s\n", pszHelp);
                //cmd->m_nFlags &= ~FCVAR_DEVELOPMENTONLY;
                char *stage2 = (char *)cmd;
                stage2 += 0x14;
                int *m_nFlags = (int *)stage2;
                //int *m_nFlags = (int *)((*(char **)cmd) + 0x14);
                (*m_nFlags) &= ~FCVAR_DEVELOPMENTONLY;
                if(cmd->IsFlagSet(FCVAR_DEVELOPMENTONLY)) CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP]  BUT IT DIDN'T WORK!\n");
                //cvar->RegisterConCommand(cmd);
                returnme = true;
            }
        }
    }
    return returnme;
}

void CAdminOP :: GrabDataDesc( CBaseEntity *pEntity )
{
    CBaseEntity *pTest = NULL;
    if(!pEntity)
    {
        pTest = CreateEntityByName("grenade");
    }
    else
    {
        pTest = pEntity;
    }
    if(!pTest) return;

    //FILE *fp = fopen("datadesc.txt", "w");
    datamap_t *datamap = VFuncs::GetDataDescMap(pTest);
    if(datamap) RecurseDataMap(datamap);
    //fclose(fp);
    if(!pEntity)
    {
        UTIL_Remove(pTest);
    }

    dataDesc.Compact();
}

void CAdminOP :: RecurseDataMap( datamap_t* datamap, FILE *fp, int level )
{
    if(datamap->baseMap) RecurseDataMap(datamap->baseMap, fp, level+1);
    if(fp)
    {
        for(int j = 0; j < level * 2; j++) fprintf(fp, " ");
        fprintf(fp, "Class: %s\n", datamap->dataClassName);
    }
    for(int i = 0; i < datamap->dataNumFields; i++)
    {
        typedescription_t *type = &datamap->dataDesc[i];
        if(type->fieldType != FIELD_VOID)
        {
            int offset = type->fieldOffset[TD_OFFSET_NORMAL];

            if(fp)
            {
                for(int j = 0; j < level * 2; j++) fprintf(fp, " ");
                fprintf(fp, "%s: %i\n", type->fieldName, offset);
            }
            datadesc_t datadesc;
            strncpy(datadesc.classname, datamap->dataClassName, MAX_DATADESC_CLASSNAME);
            strncpy(datadesc.member, type->fieldName, MAX_DATADESC_MEMBERLEN);
            datadesc.offset = offset;
            datadesc.recursive_level = level;
            dataDesc.AddToTail(datadesc);
            if(type->fieldType == FIELD_EMBEDDED)
            {
                if(fp)
                {
                    for(int j = 0; j < level * 2; j++) fprintf(fp, " ");
                    fprintf(fp, "FIELD_EMBEDDED %s\n", type->fieldName);
                }
                if(type->td) RecurseDataMap(type->td, fp, level+1);
            }
        }
    }
}

int CAdminOP :: GetDataDescFieldAnyClass(const char *pszMember)
{
    for(int i = 0; i < dataDesc.Count(); i++)
    {
        datadesc_t *datadesc = dataDesc.Base() + i; // fast access
        if(!strcmp(datadesc->member, pszMember))
        {
            return datadesc->offset;
        }
    }
    return 0;
}

void CAdminOP :: GrabSendTables( void )
{
    ServerClass *serverclass = servergame->GetAllServerClasses();
    while(serverclass)
    {
        GrabSendProps(serverclass->m_pTable);
        serverclass = serverclass->m_pNext;
    }

    sendProps.Compact();
}

class CRecvPropExtra_UtlVector
{
public:
    void* m_DataTableProxyFn;	// If it's a datatable, then this is the proxy they specified.
    void* m_ProxyFn;				// If it's a non-datatable, then this is the proxy they specified.
    void* m_ResizeFn;			// The function used to resize the CUtlVector.
    //void* m_EnsureCapacityFn; // one of these was removed, this is just a guess
    int m_ElementStride;					// Distance between each element in the array.
    int m_Offset;							// Offset of the CUtlVector from its parent structure.
    int m_nMaxElements;						// For debugging...
};

#ifdef GetProp
#undef GetProp
#endif
void CAdminOP :: GrabSendProps(SendTable *pTable, int recursion)
{
    for(int i = 0; i < pTable->GetNumProps(); i++)
    {
        //bool unique = true;
        sendprop_t sendprop;
        strncpy(sendprop.table, pTable->GetName(), MAX_SENDTBL_LEN);
        SendProp *pProp = pTable->GetProp(i);
        strncpy(sendprop.prop, pProp->GetName(), MAX_SENDPROP_LEN);
        CRecvPropExtra_UtlVector *extra = (CRecvPropExtra_UtlVector *)pProp->GetExtraData();
        if((!strcmp(sendprop.table, "DT_ScriptCreatedItem") || !strncmp(sendprop.table, "_ST_", 4) || !strcmp(sendprop.table, "DT_ScriptCreatedAttribute")) && extra)
        {
            //Msg("%s\\%s: %i  %i\n", sendprop.table, sendprop.prop, extra->m_Offset, extra->m_ElementStride);
            sendprop.offset = extra->m_Offset;
            sendprop.stride = extra->m_ElementStride;
        }
        else
        {
            sendprop.offset = pProp->GetOffset();
            sendprop.stride = pProp->GetElementStride();
        }

        sendprop.type = pProp->GetType();

        if(pProp->GetType() != DPT_DataTable)
        {
            sendprop.proxyfn = pProp->GetProxyFn();
            sendprop.tableproxyfn = NULL;
        }
        else
        {
            sendprop.proxyfn = NULL;
            sendprop.tableproxyfn = pProp->GetDataTableProxyFn();
        }
        sendprop.recursive_level = recursion;
        sendprop.arraylenproxyfn = pProp->GetArrayLengthProxy();
        // this is nice when printing out the list, but not really necessary otherwise
        // searching the list for every sendprop was really slow
        // get a faster way to search or leave it out
        // leaving it out will increase searching time since there will be a lot of duplicate data
        // but there are few searches and all of them are done right after this
        /*for(int j = 0; j < sendProps.Count(); j++)
        {
            sendprop_t *sendpropcur = sendProps.Base() + j; // fast access
            if(FStrEq(sendpropcur->table, sendprop.table) && FStrEq(sendpropcur->prop, sendprop.prop))
            {
                unique = false;
                break;
            }
        }*/
        //if(unique)
            sendProps.AddToTail(sendprop);

        if(pProp->GetDataTable())
        {
            GrabSendProps(pProp->GetDataTable(), recursion+1);
        }
    }
}

#if defined(_SOPDEBUG) && defined(OFFICIALSERV_ONLY)
// CSendTablePrecalc is the table that actually gets used during networking.
// Source engine networking refers to class props by index, and this table
// contains the ordered list of props that gets used.
class CSendTablePrecalc
{
public:
    virtual                         ~CSendTablePrecalc();

    CUtlVector<const void*>     m_unk1;
    CUtlVector<const void*>     m_unk2;
    CUtlVector<const SendProp*>     m_Props;
    CUtlVector<const void*>     m_unk3;
    CUtlVector<const void*> m_unk4;

};

void CAdminOP :: GrabSendTablesPrecalc( void )
{
    FILE *fp = fopen("sendprops.dat", "wb");

    if(fp)
    {
        fwrite("0000", 1, 4, fp);

        ServerClass *serverclass = servergame->GetAllServerClasses();
        int numberServerClasses = 0;
        while(serverclass)
        {
            numberServerClasses++;
            serverclass = serverclass->m_pNext;
        }

        fwrite(&numberServerClasses, sizeof(numberServerClasses), 1, fp);

        serverclass = servergame->GetAllServerClasses();
        while(serverclass)
        {
            fwrite(&serverclass->m_ClassID, sizeof(serverclass->m_ClassID), 1, fp);
            int netnamelen = strlen(serverclass->m_pNetworkName) + 1;
            fwrite(&netnamelen, sizeof(netnamelen), 1, fp);
            fwrite(serverclass->m_pNetworkName, 1, netnamelen, fp);

            GrabSendTablePrecalc(serverclass->m_pTable, fp);
            serverclass = serverclass->m_pNext;
        }

        fclose(fp);
    }
    else
    {
        Msg("Error writing to sendprops.dat\n");
    }
}

void DumpProp(const SendProp *pProp, FILE *fp)
{
    unsigned char propType = pProp->GetType();
    int flags = pProp->GetFlags();

    fwrite(&propType, 1, 1, fp);
    fwrite(&pProp->m_nBits, sizeof(pProp->m_nBits), 1, fp);
    fwrite(&pProp->m_fLowValue, sizeof(pProp->m_fLowValue), 1, fp);
    fwrite(&pProp->m_fHighValue, sizeof(pProp->m_fHighValue), 1, fp);
    int propnamelen = strlen(pProp->m_pVarName) + 1;
    fwrite(&propnamelen, sizeof(propnamelen), 1, fp);
    fwrite(pProp->m_pVarName, propnamelen, 1, fp);

    int propparentarraypropname = 0;
    if(pProp->m_pParentArrayPropName)
    {
        propparentarraypropname = strlen(pProp->m_pParentArrayPropName) + 1;
        fwrite(&propparentarraypropname, sizeof(propparentarraypropname), 1, fp);
        fwrite(pProp->m_pParentArrayPropName, propparentarraypropname, 1, fp);
    }
    else
    {
        fwrite(&propparentarraypropname, sizeof(propparentarraypropname), 1, fp);
    }

    fwrite(&flags, sizeof(flags), 1, fp);


    if(pProp->GetType() == DPT_Array)
    {
        int numElements = pProp->GetNumElements();
        fwrite(&numElements, sizeof(numElements), 1, fp);
        int hasLengthProxy = (pProp->GetArrayLengthProxy() != NULL);
        fwrite(&hasLengthProxy, 1, 1, fp);

        DumpProp(pProp->GetArrayProp(), fp);
    }
}

void CAdminOP :: GrabSendTablePrecalc(SendTable *pTable, FILE *fp)
{
    CSendTablePrecalc *precalc = pTable->m_pPrecalc;

    const char *pszTableName = pTable->GetName();
    int tablenamelen = strlen(pszTableName) + 1;
    fwrite(&tablenamelen, sizeof(tablenamelen), 1, fp);
    fwrite(pszTableName, 1, tablenamelen, fp);

    int count = precalc->m_Props.Count();
    fwrite(&count, sizeof(count), 1, fp);

    for(int i = 0; i < count; i++)
    {
        const SendProp *pProp = precalc->m_Props[i];

        DumpProp(pProp, fp);
    }

}
#endif // defined(_SOPDEBUG) && defined(OFFICIALSERV_ONLY)

SendTableProxyFn CAdminOP :: GetPropDataTableProxyFn(const char *pszTable, const char *pszProp)
{
    for(int i = 0; i < pAdminOP.sendProps.Count(); i++)
    {
        sendprop_t *sendprop = pAdminOP.sendProps.Base() + i; // fast access
        if(!strcmp(sendprop->table, pszTable) && !strcmp(sendprop->prop, pszProp))
        {
            return sendprop->tableproxyfn;
        }
    }
    return NULL;
}

int CAdminOP :: GetPropOffset(const char *pszTable, const char *pszProp)
{
    for(int i = 0; i < pAdminOP.sendProps.Count(); i++)
    {
        sendprop_t *sendprop = pAdminOP.sendProps.Base() + i; // fast access
        if(!strcmp(sendprop->table, pszTable) && !strcmp(sendprop->prop, pszProp))
        {
            return sendprop->offset;
        }
    }
    return 0;
}

int CAdminOP :: GetPropOffsetAnyTable(const char *pszProp)
{
    for(int i = 0; i < pAdminOP.sendProps.Count(); i++)
    {
        sendprop_t *sendprop = pAdminOP.sendProps.Base() + i; // fast access
        if(strncmp(sendprop->table, "DT_TE", 5) && !strcmp(sendprop->prop, pszProp))
        {
            return sendprop->offset;
        }
    }
    return 0;
}

int CAdminOP :: GetPropOffsetNotTable(const char *pszTable, const char *pszProp)
{
    for(int i = 0; i < pAdminOP.sendProps.Count(); i++)
    {
        sendprop_t *sendprop = pAdminOP.sendProps.Base() + i; // fast access
        if(strcmp(sendprop->table, pszTable) && !strcmp(sendprop->prop, pszProp))
        {
            return sendprop->offset;
        }
    }
    return 0;
}

void CAdminOP :: GetVTables( CBasePlayer *pPlayer )
{
    /*CBaseEntity *pTest = CreateEntityByName("info_target");
    CServerNetworkProperty * prop;
    prop = (CServerNetworkProperty *)(((unsigned int)pTest)+(GetPropOffset("DT_BaseEntity", "m_hOwnerEntity") - sizeof(CServerNetworkProperty)));
    Msg("%i %i\t%i %i\n", GetPropOffset("DT_BaseEntity", "m_hOwnerEntity") - sizeof(CServerNetworkProperty),
        GetPropOffset("DT_BaseEntity", "m_Collision") + sizeof(CCollisionProperty),
        sizeof(CServerNetworkProperty), sizeof(CCollisionProperty));
    UTIL_Remove(pTest);*/
#ifdef __linux__
    char gamedir[256];
    char filepath[512];
    void *handle;

    engine->GetGameDir(gamedir, sizeof(gamedir));
    Q_snprintf(filepath, sizeof(filepath), "%s/bin/server_i486.so", gamedir);

    handle = dlopen(filepath, RTLD_LAZY | RTLD_NOLOAD);
    if(handle)
    {
        char buf[512];
        CBaseEntity *pTest = CreateEntityByName("info_target");
        Dl_info info;
        FILE *fp = fopen("vtables_cbaseentity.txt", "wt");
        if(fp)
        {
            for(int i = 0; i < 4000; i+=4)
            {
                if(dladdr(VFN(pTest, i), &info))
                {
                    if(info.dli_sname)
                    {
                        if(strncmp(info.dli_sname, "_ZTVN", 5))
                        {
                            int soff = info.dli_saddr - VFN(pTest, i);
                            sprintf(buf, "%3i: %s%s\n", i/4, info.dli_sname, soff ? UTIL_VarArgs(" + %i", soff) : "");
                            fputs(buf, fp);
                            //Msg(buf);
                        }
                        else
                        {
                            break;
                        }
                    }
                    else
                    {
                        Msg("Unknown %3i: %s\n", i/4, info.dli_sname);
                    }
                }
            }
            fclose(fp);
        }
        UTIL_Remove(pTest);

        pTest = CreateEntityByName("grenade");
        fp = fopen("vtables_cbasegrenade.txt", "wt");
        if(fp)
        {
            for(int i = 0; i < 4000; i+=4)
            {
                if(dladdr(VFN(pTest, i), &info))
                {
                    if(info.dli_sname)
                    {
                        if(strncmp(info.dli_sname, "_ZTVN", 5))
                        {
                            int soff = info.dli_saddr - VFN(pTest, i);
                            sprintf(buf, "%3i: %s%s\n", i/4, info.dli_sname, soff ? UTIL_VarArgs(" + %i", soff) : "");
                            fputs(buf, fp);
                            //Msg(buf);
                        }
                        else
                        {
                            break;
                        }
                    }
                    else
                    {
                        Msg("Unknown %3i: %s\n", i/4, info.dli_sname);
                    }
                }
            }
            fclose(fp);
        }
        UTIL_Remove(pTest);

        if(pPlayer)
        {
            fp = fopen("vtables_cbaseplayer.txt", "wt");
            if(fp)
            {
                for(int i = 0; i < 4000; i+=4)
                {
                    if(dladdr(VFN(pPlayer, i), &info))
                    {
                        if(info.dli_sname)
                        {
                            if(strncmp(info.dli_sname, "_ZTVN", 5))
                            {
                                int soff = info.dli_saddr - VFN(pPlayer, i);
                                sprintf(buf, "%3i: %s%s\n", i/4, info.dli_sname, soff ? UTIL_VarArgs(" + %i", soff) : "");
                                fputs(buf, fp);
                                //Msg(buf);
                            }
                            else
                            {
                                break;
                            }
                        }
                    }
                }
                fclose(fp);
            }
        }
    }
#endif
}

#ifndef __linux__
bool CAdminOP :: MemPatcher( void )
{
    FuncSigMgr f;
    unsigned int len = 0;

    const char plantSig[] = "\x83\xEC\x0C\x8D\x96\x3C\x03\x00"; // len: 8
    //const char plantSig[] = "\x56\x6A\x00\x68\x6C\x80\x37\x22"; // len: 8

    // Fixes Pickup_ForcePlayerToDropThisObject in server code. causes this function to just return since
    // it shouldn't be used in multiplayer anyways.
    const char fptdropSig[] = "\x56\x8B\x74\x24\x08\x85\xF6\x74\x23"; // len: 9
    BYTE fptdropRep[] = "\xC3\xCC\xCC\xCC\xCC"; // len 5 (this causes the function to immediately return)

    //PROCESSENTRY32        procentry;
    MODULEENTRY32       modlentry;
    CMemoryPatcher      patcher;
    DWORD               processID = GetCurrentProcessId();

    if(!patcher.Init())
    {
        TimeLog("SourceOPErrors.log", "Failed to init mempatch.\n");
        Msg("Failed to init mempatch.\n");
        return 0;
    }

    if(!patcher.FindModuleInProcess("server_srv.dll", processID, &modlentry) && !patcher.FindModuleInProcess("server.dll", processID, &modlentry))
    {
        TimeLog("SourceOPErrors.log", "Found no server.dll inside of HL2 process\n");
        Msg("Found no server.dll inside of HL2 process\n");
        return 0;
    }
    char *pCheck = (char *)modlentry.hModule;
    //Msg("Looking: %x %i %i %i\n", pCheck, pCheck[0], modlentry.modBaseAddr, modlentry.modBaseSize);

    if(isCstrike)
    {
        for(unsigned int i = 0; i<(modlentry.modBaseSize-8); i++)
        {
            if(pCheck[i] == plantSig[0] && pCheck[i+1] == plantSig[1] && pCheck[i+2] == plantSig[2] && pCheck[i+3] == plantSig[3] && pCheck[i+4] == plantSig[4] && pCheck[i+5] == plantSig[5] && pCheck[i+6] == plantSig[6] && pCheck[i+7] == plantSig[7])
            {
                //Msg("Plant bomb\n\n\n\n*********\n\n\n");
                PlantBomb = (PlantBombFunc)(i+modlentry.modBaseAddr);
                break;
            }
        }
    }

    // skip combine ball on cstrike and tf2
    if(!isCstrike && !isTF2)
    {
        const char *createCombineBallSig = f.GetSignature("CreateCombineBall", &len);
        const char *createCombineBallMatch = f.GetMatchString("CreateCombineBall");
        if(len > 0)
        {
            for(unsigned int i = 0; i<(modlentry.modBaseSize-len); i++)
            {
                if(f.IsMatch(pCheck, i, len, createCombineBallSig, createCombineBallMatch))
                {
                    _CreateCombineBall = (_CreateCombineBallFunc)(i+modlentry.modBaseAddr);
                    break;
                }
            }
        }
    }

    const char *namedSig = f.GetSignature("CreateEntityByName", &len);
    const char *namedSigMatch = f.GetMatchString("CreateEntityByName");
    int namedEntFactDict = atoi(f.GetExtra("CreateEntFactDict"));
    if ( len > 0 )
    {
        for ( unsigned int i = 0; i < (modlentry.modBaseSize - len); i++ )
        {
            if ( f.IsMatch( pCheck, i, len, namedSig, namedSigMatch ) )
            {
                _CreateEntityByName = (_CreateEntityByNameFunc)(i + modlentry.modBaseAddr);
                if ( namedEntFactDict && g_entityFactoryDictionary == NULL )
                {
                    const char *namedEntFactSig = f.GetSignature( "CreateEntFactDict", &len );
                    const char *namedEntFactMatch = f.GetMatchString( "CreateEntFactDict" );
                    for ( unsigned int j = i; j < i + 0x5F; j++ )
                    {
                        if ( f.IsMatch( pCheck, j, len, namedEntFactSig, namedEntFactMatch ) )
                        {
                            int *dictOffPtr = (int *)(modlentry.modBaseAddr + j + len);
                            int dictAddr;
                            if ( namedEntFactDict == 1 )
                                dictAddr = ((int)modlentry.modBaseAddr) + j + len + 4 + *dictOffPtr;
                            else
                                dictAddr = *dictOffPtr;

                            typedef IEntityFactoryDictionary* (__cdecl* EntFactDictFunc)(void);
                            EntFactDictFunc EntFactDict;
                            EntFactDict = (EntFactDictFunc)dictAddr;
                            g_entityFactoryDictionary = (CEntityFactoryDictionary *)EntFactDict();
                        }
                    }
                }
                break;
            }
        }
    }

    // doesn't work in TF2, may not be necessary
    if(!isTF2)
    {
        for(unsigned int i = 0; i<(modlentry.modBaseSize-9); i++)
        {
            if(pCheck[i] == fptdropSig[0] && pCheck[i+1] == fptdropSig[1] && pCheck[i+2] == fptdropSig[2] && pCheck[i+3] == fptdropSig[3] &&
               pCheck[i+4] == fptdropSig[4] && pCheck[i+5] == fptdropSig[5] && pCheck[i+6] == fptdropSig[6] && pCheck[i+7] == fptdropSig[7] && pCheck[i+8] == fptdropSig[8])
            {
                patcher.WriteProcess(processID, i+modlentry.modBaseAddr, fptdropRep, sizeof(fptdropRep)-1);
                break;
            }
        }
    }

    const char *clearMultiDmgSig = f.GetSignature("ClearMultiDamage", &len);
    const char *clearMultiDmgMatch = f.GetMatchString("ClearMultiDamage");
    if ( len > 0 )
    {
        for ( unsigned int i = 0; i < (modlentry.modBaseSize - len); i++ )
        {
            if ( f.IsMatch( pCheck, i, len, clearMultiDmgSig, clearMultiDmgMatch ) )
            {
                _ClearMultiDamage = (_ClearMultiDamageFunc)(i + modlentry.modBaseAddr);
                break;
            }
        }
    }

    const char *applyMultiDmgSig = f.GetSignature("ApplyMultiDamage", &len);
    const char *applyMultiDmgMatch = f.GetMatchString("ApplyMultiDamage");
    if ( len > 0 )
    {
        for ( unsigned int i = 0; i < (modlentry.modBaseSize - len); i++ )
        {
            if ( f.IsMatch( pCheck, i, len, applyMultiDmgSig, applyMultiDmgMatch ) )
            {
                _ApplyMultiDamage = (_ApplyMultiDamageFunc)(i + modlentry.modBaseAddr);
                break;
            }
        }
    }

    const char *radiusDamageSig = f.GetSignature("RadiusDamage", &len);
    const char *radiusDamageMatch = f.GetMatchString("RadiusDamage");
    if ( len > 0 )
    {
        for ( unsigned int i = 0; i < (modlentry.modBaseSize - len); i++ )
        {
            if ( f.IsMatch( pCheck, i, len, radiusDamageSig, radiusDamageMatch ) )
            {
                _RadiusDamage = (_RadiusDamageFunc)(i + modlentry.modBaseAddr);
                break;
            }
        }
    }

    const char *setMoveTypeSig = f.GetSignature("SetMoveType", &len);
    const char *setMoveTypeMatch = f.GetMatchString("SetMoveType");
    if ( len > 0 )
    {
        for ( unsigned int i = 0; i < (modlentry.modBaseSize - len); i++ )
        {
            if ( f.IsMatch( pCheck, i, len, setMoveTypeSig, setMoveTypeMatch ) )
            {
                _SetMoveType = (_SetMoveTypeFunc)(i + modlentry.modBaseAddr);
                break;
            }
        }
    }

    const char *resetSequenceSig = f.GetSignature("ResetSequence", &len);
    const char *resetSequenceMatch = f.GetMatchString("ResetSequence");
    if(len > 0)
    {
        for(unsigned int i = 0; i<(modlentry.modBaseSize-len); i++)
        {
            if(f.IsMatch(pCheck, i, len, resetSequenceSig, resetSequenceMatch))
            {
                _ResetSequence = (_ResetSequenceFunc)(i+modlentry.modBaseAddr);
                break;
            }
        }
    }
    else
    {
        ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Missing signature for ResetSequence.\n");
        TimeLog("SourceOPErrors.log", "Missing signature for ResetSequence.\n");
    }

    if(patcher.FindModuleInProcess("client.dll", processID, &modlentry))
    {
        isClient = 1;
    }


    if(!patcher.FindModuleInProcess("engine_srv.dll", processID, &modlentry) && !patcher.FindModuleInProcess("engine.dll", processID, &modlentry))
    {
        TimeLog("SourceOPErrors.log", "Found no engine.dll inside of HL2 process\n");
        Msg("Found no engine.dll inside of HL2 process\n");
    }
    else
    {
        pCheck = (char *)modlentry.hModule;

        // NET_SendPacket
        const char *sendPacketSig = f.GetSignature("SendPacket", &len);
        const char *sendPacketMatch = f.GetMatchString("SendPacket");
        for(unsigned int i = 0; i<(modlentry.modBaseSize-len); i++)
        {
            if(f.IsMatch(pCheck, i, len, sendPacketSig, sendPacketMatch))
            {
                _NET_SendPacket = (_SendPacketFunc)(i+modlentry.modBaseAddr);
                //NET_SendPacketDetour.Detour((BYTE *)_NET_SendPacket, (BYTE *)&SOP_NET_SendPacket, true, true, true);
                //NET_SendPacketDetour.Apply();

                SetupTrampoline((BYTE *)_NET_SendPacket, (BYTE *)&SOP_NET_SendPacket, (BYTE *)&SOP_NET_SendPacketTrampoline);

                /*
                // make first 5 bytes of trampoline the first five of instruction
                CDetourDis Dis(NULL, NULL);

                BYTE *pbOrgDst = (BYTE *)&SOP_NET_SendPacketTrampoline;
                BYTE *pbDst = pbOrgDst;
                BYTE *pbSrc = (BYTE *)_NET_SendPacket;

                DWORD prev;
                VirtualProtect(pbOrgDst, 16, PAGE_EXECUTE_READWRITE, &prev);

                while((pbDst - pbOrgDst) < 5)
                {
                    BYTE* pbNew = Dis.CopyInstruction(pbDst, pbSrc);
                    pbDst += (pbNew - pbSrc);
                    Msg("Copied %i bytes into trampoline.\n", (pbNew - pbSrc));
                    pbSrc = pbNew;
                }

                int bytesWritten = (pbDst - pbOrgDst);

                // make trampoline jump to target (+what we wrote) next
                // (to target from wherever we left off)
                int tjump = (int) (((BYTE *)_NET_SendPacket) + bytesWritten) - ((int)pbDst) - 5;
                BYTE *pTjump = (BYTE *)&tjump;
                char *twritepos = (char *)pbDst;
                DWORD tprev;
                VirtualProtect(twritepos, 5, PAGE_EXECUTE_READWRITE, &tprev);
                twritepos[0] = '\xE9';
                twritepos[1] = pTjump[0];
                twritepos[2] = pTjump[1];
                twritepos[3] = pTjump[2];
                twritepos[4] = pTjump[3];

                // make target jump to hook
                int vjump = (int)((BYTE *)&SOP_NET_SendPacket) - (int)_NET_SendPacket - 5;
                BYTE *pVjump = (BYTE *)&vjump;
                char *vwritepos = (char *)_NET_SendPacket;
                DWORD vprev;
                VirtualProtect(vwritepos, 5, PAGE_EXECUTE_READWRITE, &vprev);
                //memcpy(origNetSendPacket, _NET_SendPacket, 5);
                vwritepos[0] = '\xE9';
                vwritepos[1] = pVjump[0];
                vwritepos[2] = pVjump[1];
                vwritepos[3] = pVjump[2];
                vwritepos[4] = pVjump[3];
                */

                break;
            }
        }

        // CBaseEntity::SendServerInfo
        const char *ssiTblFunc = f.GetSignature("SendServerInfo", &len);
        const char *ssiTblFuncMatch = f.GetMatchString("SendServerInfo");
        int ssiExtra = atoi(f.GetExtra("SendServerInfo"));
        int ssiOccurence = 0;

        for(unsigned int i = 0; i<(modlentry.modBaseSize-len); i++)
        {
            if(f.IsMatch(pCheck, i, len, ssiTblFunc, ssiTblFuncMatch))
            {
                // found signature for a SendServerInfo function
                int address = ((int)modlentry.modBaseAddr) + i;
                char *addrptr = (char *)&address;
                for(unsigned int j = 0; j<(modlentry.modBaseSize-4); j++)
                {
                    if(pCheck[j] == addrptr[0] && pCheck[j+1] == addrptr[1] && pCheck[j+2] == addrptr[2] && pCheck[j+3] == addrptr[3])
                    {
                        // found the vtable that this function is in
                        int vtbladdr = ((int)modlentry.modBaseAddr) + j;
                        char *vtbladdrptr = (char *)&vtbladdr;
                        ssiOccurence++;
                        //Msg("Occurence %i: %x\n", ssiOccurence, vtbladdr);
                        if(ssiOccurence != ssiExtra) continue;
                        vtbladdr -= offs[OFFSET_SENDSERVERINFO]*4; // to get to the first
                        //Msg("vtable found: %x\n", vtbladdr);
                        //g_hookIDS.AddToTail(SH_ADD_MANUALDVPHOOK(CBaseClient_SendServerInfo, (void*)vtbladdr, SH_STATIC(HookSendServerInfo), false));
                        //g_hookIDS.AddToTail(SH_ADD_MANUALDVPHOOK(CBaseClient_FillUserInfo, (void*)vtbladdr, SH_STATIC(HookFillUserInfo), true));
                        vtbladdr += offs[OFFSET_VTABLE2FROM3]*4; // vtable above (offs[OFFSET_VTABLE2FROM3] should be negative)
                        g_hookIDS.AddToTail(SH_ADD_MANUALDVPHOOK(CBaseClient_Connect, (void*)vtbladdr, SH_STATIC(HookConnect), false));
                        g_hookIDS.AddToTail(SH_ADD_MANUALDVPHOOK(CBaseClient_Disconnect, (void*)vtbladdr, SH_STATIC(HookDisconnect), false));
                        g_hookIDS.AddToTail(SH_ADD_MANUALDVPHOOK(CBaseClient_ExecuteStringCommand, (void*)vtbladdr, SH_STATIC(HookExecuteStringCommand), false));
                        break;
                    }
                }
            }
        }

        // SV_BroadcastVoiceData
        const char *bvdTblFunc = f.GetSignature("SV_BroadcastVoiceData", &len);
        const char *bvdTblFuncMatch = f.GetMatchString("SV_BroadcastVoiceData");
        
        for(unsigned int i = 0; i<(modlentry.modBaseSize-len); i++)
        {
            if(f.IsMatch(pCheck, i, len, bvdTblFunc, bvdTblFuncMatch))
            {
                _SV_BroadcastVoiceData = (_SV_BVDFunc)(i+modlentry.modBaseAddr);

                SetupTrampoline((BYTE *)_SV_BroadcastVoiceData, (BYTE *)&SOP_BroadcastVoice, (BYTE *)&SOP_BroadcastVoiceTrampoline);
                break;
            }
        }
    }

    return 1;
}
#else // __linux__
bool CAdminOP :: MemPatcher( void )
{
    char gamedir[256];
    char filepath[512];
    void *handle;

    engine->GetGameDir(gamedir, sizeof(gamedir));
    Q_snprintf(filepath, sizeof(filepath), "%s/bin/server_srv.so", gamedir);

    handle = dlopen(filepath, RTLD_LAZY | RTLD_NOLOAD);
    if(handle)
    {
        Msg("[SOURCEOP] Linux MemPatcher opened %s\n", filepath);
        _CreateEntityByName = (_CreateEntityByNameFunc)ResolveSymbol(handle, "_Z18CreateEntityByNamePKci");
        _CreateCombineBall = (_CreateCombineBallFunc)ResolveSymbol(handle, "_Z17CreateCombineBallRK6VectorS1_fffP11CBaseEntity");
        _ClearMultiDamage = (_ClearMultiDamageFunc)ResolveSymbol(handle, "_Z16ClearMultiDamagev");
        _ApplyMultiDamage = (_ApplyMultiDamageFunc)ResolveSymbol(handle, "_Z16ApplyMultiDamagev");
        _RadiusDamage = (_RadiusDamageFunc)ResolveSymbol(handle, "_Z12RadiusDamageRK15CTakeDamageInfoRK6VectorfiP11CBaseEntity");
        _SetMoveType = (_SetMoveTypeFunc)ResolveSymbol(handle, "_ZN11CBaseEntity11SetMoveTypeE10MoveType_t13MoveCollide_t");
        _ResetSequence = (_ResetSequenceFunc)ResolveSymbol(handle, "_ZN14CBaseAnimating13ResetSequenceEi");

        te = *(ITempEntsSystem**)ResolveSymbol(handle, "te");
        dlclose(handle);
    }
    else
    {
        Msg("[SOURCEOP] Linux MemPatcher failed to open %s\n", filepath);
    }

    Q_snprintf(filepath, sizeof(filepath), "bin/engine_srv.so");

    handle = dlopen(filepath, RTLD_LAZY | RTLD_NOLOAD);
    if(handle)
    {
        Msg("[SOURCEOP] Linux MemPatcher opened %s\n", filepath);

        // +8 brings us to the first virtual function entry (skipping the abi stuff and whatnot)
        void *vtbladdr = ResolveSymbol(handle, "_ZTV11CGameClient") + 8;
        //TODO: L4D may be acting up with these. Test removing it.
//#ifndef _L4D_PLUGIN
        g_hookIDS.AddToTail(SH_ADD_MANUALDVPHOOK(CBaseClient_Connect, vtbladdr, SH_STATIC(HookConnect), false));
        g_hookIDS.AddToTail(SH_ADD_MANUALDVPHOOK(CBaseClient_Disconnect, vtbladdr, SH_STATIC(HookDisconnect), false));
        g_hookIDS.AddToTail(SH_ADD_MANUALDVPHOOK(CBaseClient_ExecuteStringCommand, vtbladdr, SH_STATIC(HookExecuteStringCommand), false));
        //g_hookIDS.AddToTail(SH_ADD_MANUALDVPHOOK(CBaseClient_FillUserInfo, vtbladdr, SH_STATIC(HookFillUserInfo), true));
//#endif

        _SV_BroadcastVoiceData = (_SV_BVDFunc)ResolveSymbol(handle, "_Z21SV_BroadcastVoiceDataP7IClientiPcx");
        SetupTrampoline((BYTE *)_SV_BroadcastVoiceData, (BYTE *)&SOP_BroadcastVoice, (BYTE *)&SOP_BroadcastVoiceTrampoline);

        _NET_SendPacket = (_SendPacketFunc)ResolveSymbol(handle, "_Z14NET_SendPacketP11INetChanneliRK8netadr_sPKhiP8bf_writeb");
        SetupTrampoline((BYTE *)_NET_SendPacket, (BYTE *)&SOP_NET_SendPacket, (BYTE *)&SOP_NET_SendPacketTrampoline);

        pServer = ResolveSymbol(handle, "sv");
        HookServer();

        dlclose(handle);
    }
    else
    {
        Msg("[SOURCEOP] Linux MemPatcher failed to open %s\n", filepath);
    }

    Q_snprintf(filepath, sizeof(filepath), "bin/dedicated_srv.so");
    handle = dlopen(filepath, RTLD_LAZY | RTLD_NOLOAD);
    if(handle)
    {
        Msg("[SOURCEOP] Linux MemPatcher opened %s\n", filepath);

        _dedicated_usleep = (_usleepfunc)dlsym(handle, "usleep");
        if(_dedicated_usleep)
        {
            SetupTrampoline((BYTE *)_dedicated_usleep, (BYTE *)&SOP_usleep, (BYTE *)&SOP_usleepTrampoline);
        }
        else
        {
            ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Failed to hook dedicated's usleep.\n");
        }
    }
    else
    {
        Msg("[SOURCEOP] Linux MemPatcher failed to open %s\n", filepath);
    }
    return 1;
}

#endif

CON_COMMAND(DF_hooksteam_now, "Hooks steam now")
{
    if(_BGetCallback)
    {
        Msg("[SOURCEOP] Steam was already hooked.\n");
    }

    pAdminOP.HookSteam();
    Msg("[SOURCEOP] Steam hooked.\n");
}

bool CAdminOP :: HookSteamFromGameServerInit( void )
{
    return HookSteam();
}

#ifndef __linux__
bool CAdminOP :: HookSteam( void )
{

    MODULEENTRY32       modlentry;
    CMemoryPatcher      patcher;
    DWORD               processID = GetCurrentProcessId();

    if(!patcher.Init())
    {
        TimeLog("SourceOPErrors.log", "Failed to init mempatch.\n");
        Msg("Failed to init mempatch.\n");
        return false;
    }

    if(!patcher.FindModuleInProcessLast("steamclient.dll", processID, &modlentry))
    {
        TimeLog("SourceOPErrors.log", "Found no steamclient.dll inside of HL2 process\n");
        Msg("Found no steamclient.dll inside of HL2 process\n");
        return false;
    }

    CreateInterfaceFn steamFactory = (CreateInterfaceFn)GetProcAddress(modlentry.hModule, "CreateInterface");

    if(!_BGetCallback)
    {
        _BGetCallback = (PFNSteam_BGetCallback)GetProcAddress(modlentry.hModule, "Steam_BGetCallback");
        _FreeLastCallback = (PFNSteam_FreeLastCallback)GetProcAddress(modlentry.hModule, "Steam_FreeLastCallback");
        if(_BGetCallback)
        {
            SetupTrampoline((BYTE *)_BGetCallback, (BYTE *)&SOP_BGetCallback, (BYTE *)&SOP_BGetCallbackTrampoline, true);
            if(!_FreeLastCallback)
            {
                Msg("[SOURCEOP] Found Steam_BGetCallback but Steam_FreeLastCallback was NULL!\n");
            }
        }
        else
        {
            Msg("[SOURCEOP] Failed to find Steam's callback function.\n");
        }
    }



    if(!patcher.FindModuleInProcess("steam_api.dll", processID, &modlentry))
    {
        TimeLog("SourceOPErrors.log", "Found no steam_api.dll inside of HL2 process\n");
        Msg("Found no steam_api.dll inside of HL2 process\n");
        return false;
    }

    typedef HSteamPipe (*PFNSteamGameServer_GetHSteamPipe)( void );
    typedef HSteamUser (*PFNSteamGameServer_GetHSteamUser)( void );
    PFNSteamGameServer_GetHSteamPipe SteamGameServer_GetHSteamPipe;
    PFNSteamGameServer_GetHSteamUser SteamGameServer_GetHSteamUser;

    if(isClient)
    {
        Msg("[SOURCEOP] Hooking Steam from client.\n");
        SteamGameServer_GetHSteamPipe = (PFNSteamGameServer_GetHSteamPipe)GetProcAddress(modlentry.hModule, "SteamAPI_GetHSteamPipe");
        SteamGameServer_GetHSteamUser = (PFNSteamGameServer_GetHSteamUser)GetProcAddress(modlentry.hModule, "SteamAPI_GetHSteamUser");
    }
    else
    {
        SteamGameServer_GetHSteamPipe = (PFNSteamGameServer_GetHSteamPipe)GetProcAddress(modlentry.hModule, "SteamGameServer_GetHSteamPipe");
        SteamGameServer_GetHSteamUser = (PFNSteamGameServer_GetHSteamUser)GetProcAddress(modlentry.hModule, "SteamGameServer_GetHSteamUser");
    }

    if(steamFactory && SteamGameServer_GetHSteamPipe && SteamGameServer_GetHSteamUser)
    {
        ISteamClient008 *pSteamClientGameServer = (ISteamClient008 *)steamFactory(STEAMCLIENT_INTERFACE_VERSION_V008, NULL);
        if(pSteamClientGameServer)
        {
            HSteamPipe pipe = SteamGameServer_GetHSteamPipe();
            HSteamUser user = SteamGameServer_GetHSteamUser();

            if(isClient)
            {
                    ISteamUser *pSteamUser = (ISteamUser*)pSteamClientGameServer->GetISteamUser( user, pipe, STEAMUSER_INTERFACE_VERSION );
                    m_pSteamUser = pSteamUser;
            }
            else
            {
                    ISteamGameServer010 *pSteamGameServer = (ISteamGameServer010*)pSteamClientGameServer->GetISteamGameServer( user, pipe, "SteamGameServer010" );
                    m_pSteamGameServer = pSteamGameServer;

                    static bool hookedGameServer = false;
                    if(!hookedGameServer)
                    {
                        int r = SH_ADD_HOOK_STATICFUNC(ISteamGameServer010, SetServerType, pSteamGameServer, HookSetServerType, false);
                        if(r) g_hookIDS.AddToTail(r);

                        hookedGameServer = true;
                    }
            }

            static bool hookedCoordinator = false;
            static bool hookedClientCoordinator = false;
            if(!hookedCoordinator)
            {
                ISteamGameCoordinator001 *pCoordinator;

                pCoordinator = (ISteamGameCoordinator001 *)pSteamClientGameServer->GetISteamGenericInterface(user, pipe, STEAMGAMECOORDINATOR_INTERFACE_VERSION_001);

                if(pCoordinator)
                {
                    int r;
                    r = SH_ADD_HOOK_STATICFUNC(ISteamGameCoordinator001, SendMessage, pCoordinator, HookGCSendMessage, true);
                    if(r) g_hookIDS.AddToTail(r);
                    r = SH_ADD_HOOK_STATICFUNC(ISteamGameCoordinator001, RetrieveMessage, pCoordinator, HookGCRetrieveMessage, true);
                    if(r) g_hookIDS.AddToTail(r);
                    hookedCoordinator = true;
                }
                else
                {
                    Msg("[SOURCEOP] Failed to find coordinator.\n");
                }

                if(isClient && !hookedClientCoordinator)
                {
                    IClientEngine *pClientEngine;
                    pClientEngine = (IClientEngine *)steamFactory(CLIENTENGINE_INTERFACE_VERSION, NULL);

                    if(pClientEngine)
                    {
                        IClientGameCoordinator *pClientCoordinator = (IClientGameCoordinator *)pClientEngine->GetIClientGameCoordinator(user, pipe, CLIENTGAMECOORDINATOR_INTERFACE_VERSION);

                        if(pClientCoordinator == (void *)pCoordinator)
                        {
                            Msg("[SOURCEOP] Client coordinator is same as regular coordinator.\n");
                            pClientCoordinator = NULL;
                        }

                        if(pClientCoordinator)
                        {
                            int r;
                            r = SH_ADD_HOOK_STATICFUNC(IClientGameCoordinator, SendMessage, pClientCoordinator, HookClientGCSendMessage, true);
                            if(r) g_hookIDS.AddToTail(r);
                            r = SH_ADD_HOOK_STATICFUNC(IClientGameCoordinator, RetrieveMessage, pClientCoordinator, HookClientGCRetrieveMessage, true);
                            if(r) g_hookIDS.AddToTail(r);
                            hookedClientCoordinator = true;
                        }
                        else
                        {
                            Msg("[SOURCEOP] Failed to find client coordinator.\n");
                        }
                    }
                    else
                    {
                        Msg("[SOURCEOP] Failed to find client engine.\n");
                    }
                }
            }
        }
        else
        {
            Msg("[SOURCEOP] Failed to find Steam API's steam client interface.\n");
        }
    }
    else
    {
        Msg("[SOURCEOP] Failed to find Steam API's game server functions.\n");
    }

    return false;
}

bool CAdminOP :: HookSteamGameServer( void )
{
    static bool bHookedSteamGameServer = false;
    if(bHookedSteamGameServer)
        return false;

    MODULEENTRY32       modlentry;
    CMemoryPatcher      patcher;
    DWORD               processID = GetCurrentProcessId();

    if(!patcher.Init())
    {
        TimeLog("SourceOPErrors.log", "Failed to init mempatch.\n");
        Msg("Failed to init mempatch.\n");
        return false;
    }

    if(!patcher.FindModuleInProcess("steam_api.dll", processID, &modlentry))
    {
        TimeLog("SourceOPErrors.log", "Found no steam_api.dll inside of HL2 process\n");
        Msg("Found no steam_api.dll inside of HL2 process\n");
        return false;
    }

    _SteamGameServer_InitSafe = (PFNSteam_GameServer_InitSafe)GetProcAddress(modlentry.hModule, "SteamGameServer_InitSafe");

    Msg("[SOURCEOP] Hooking steam_api.\n");

    if(_SteamGameServer_InitSafe)
    {
        SetupTrampoline((BYTE *)_SteamGameServer_InitSafe, (BYTE *)&SOP_SteamGameServer_InitSafe, (BYTE *)&SOP_SteamGameServer_InitSafeTrampoline, false);
        bHookedSteamGameServer = true;
    }
    else
    {
        Msg("[SOURCEOP] Failed to find SteamGameServer_InitSafe.\n");
    }

    return false;
}
#else
bool CAdminOP :: HookSteam( void )
{
    char gamedir[256];
    char filepath[512];
    void *handle;
    CreateInterfaceFn steamFactory = NULL;

    engine->GetGameDir(gamedir, sizeof(gamedir));
    Q_snprintf(filepath, sizeof(filepath), "bin/steamclient.so");

    handle = dlopen(filepath, RTLD_LAZY | RTLD_NOLOAD);
    if(handle)
    {
        Msg("[SOURCEOP] Linux MemPatcher opened %s\n", filepath);
        steamFactory = (CreateInterfaceFn)dlsym(handle, "CreateInterface");
        _BGetCallback = (PFNSteam_BGetCallback)dlsym(handle, "Steam_BGetCallback");
        _FreeLastCallback = (PFNSteam_FreeLastCallback)dlsym(handle, "Steam_FreeLastCallback");
        if(_BGetCallback)
        {
            SetupTrampoline((BYTE *)_BGetCallback, (BYTE *)&SOP_BGetCallback, (BYTE *)&SOP_BGetCallbackTrampoline, true);
            if(!_FreeLastCallback)
            {
                Msg("[SOURCEOP] Found Steam_BGetCallback but Steam_FreeLastCallback was NULL!\n");
            }
        }
        else
        {
            Msg("[SOURCEOP] Failed to find Steam's callback function.\n");
        }

        dlclose(handle);
    }
    else
    {
        Msg("[SOURCEOP] Linux MemPatcher failed to open %s\n", filepath);
    }

    engine->GetGameDir(gamedir, sizeof(gamedir));
    Q_snprintf(filepath, sizeof(filepath), "bin/libsteam_api.so");

    handle = dlopen(filepath, RTLD_LAZY | RTLD_NOLOAD);
    if(handle)
    {
        Msg("[SOURCEOP] Linux MemPatcher opened %s\n", filepath);

        typedef HSteamPipe (*PFNSteamGameServer_GetHSteamPipe)( void );
        typedef HSteamUser (*PFNSteamGameServer_GetHSteamUser)( void );
        PFNSteamGameServer_GetHSteamPipe SteamGameServer_GetHSteamPipe;
        PFNSteamGameServer_GetHSteamUser SteamGameServer_GetHSteamUser;

        SteamGameServer_GetHSteamPipe = (PFNSteamGameServer_GetHSteamPipe)dlsym(handle, "SteamGameServer_GetHSteamPipe");
        SteamGameServer_GetHSteamUser = (PFNSteamGameServer_GetHSteamUser)dlsym(handle, "SteamGameServer_GetHSteamUser");

        if(steamFactory && SteamGameServer_GetHSteamPipe && SteamGameServer_GetHSteamUser)
        {
            ISteamClient008 *pSteamClientGameServer = (ISteamClient008 *)steamFactory(STEAMCLIENT_INTERFACE_VERSION_V008, NULL);
            if(pSteamClientGameServer)
            {
                HSteamPipe pipe = SteamGameServer_GetHSteamPipe();
                HSteamUser user = SteamGameServer_GetHSteamUser();

                m_pSteamGameServer = (ISteamGameServer010*)pSteamClientGameServer->GetISteamGameServer( user, pipe, "SteamGameServer010" );

                static bool hookedGameServer = false;
                if(!hookedGameServer)
                {
                    int r = SH_ADD_HOOK_STATICFUNC(ISteamGameServer010, SetServerType, m_pSteamGameServer, HookSetServerType, false);
                    if(r) g_hookIDS.AddToTail(r);

                    hookedGameServer = true;
                }

                static bool hookedCoordinator = false;
                if(!hookedCoordinator)
                {
                    ISteamGameCoordinator001 *pCoordinator = (ISteamGameCoordinator001 *)pSteamClientGameServer->GetISteamGenericInterface(user, pipe, STEAMGAMECOORDINATOR_INTERFACE_VERSION_001);

                    if(pCoordinator)
                    {
                        int r;
                        r = SH_ADD_HOOK(ISteamGameCoordinator001, SendMessage, pCoordinator, SH_STATIC(HookGCSendMessage), true);
                        if(r) g_hookIDS.AddToTail(r);
                        r = SH_ADD_HOOK(ISteamGameCoordinator001, RetrieveMessage, pCoordinator, SH_STATIC(HookGCRetrieveMessage), true);
                        if(r) g_hookIDS.AddToTail(r);
                        hookedCoordinator = true;
                    }
                    else
                    {
                        Msg("[SOURCEOP] Failed to find coordinator.\n");
                    }
                }

            }
            else
            {
                Msg("[SOURCEOP] Failed to find Steam API's steam client interface.\n");
            }
        }
        else
        {
            Msg("[SOURCEOP] Failed to find Steam API's game server functions.\n");
        }

        dlclose(handle);
    }
    else
    {
        Msg("[SOURCEOP] Linux MemPatcher failed to open %s\n", filepath);
    }

}

bool CAdminOP :: HookSteamGameServer( void )
{
    static bool bHookedSteamGameServer = false;
    if(bHookedSteamGameServer)
        return false;

    char gamedir[256];
    char filepath[512];
    void *handle;

    engine->GetGameDir(gamedir, sizeof(gamedir));
    Q_snprintf(filepath, sizeof(filepath), "bin/libsteam_api.so");

    handle = dlopen(filepath, RTLD_LAZY | RTLD_NOLOAD);
    if(handle)
    {
        _SteamGameServer_InitSafe = (PFNSteam_GameServer_InitSafe)dlsym(handle, "SteamGameServer_InitSafe");

        Msg("[SOURCEOP] Hooking steam_api.\n");

        if(_SteamGameServer_InitSafe)
        {
            SetupTrampoline((BYTE *)_SteamGameServer_InitSafe, (BYTE *)&SOP_SteamGameServer_InitSafe, (BYTE *)&SOP_SteamGameServer_InitSafeTrampoline, false);
            bHookedSteamGameServer = true;
        }
        else
        {
            Msg("[SOURCEOP] Failed to find SteamGameServer_InitSafe.\n");
        }

        dlclose(handle);
    }
    else
    {
        Msg("[SOURCEOP] Linux MemPatcher failed to open %s\n", filepath);
    }

    return false;
}

bool isFuncGetPcThunkBx( BYTE *pbFunc )
{
    char szGetPcThunkBx[] = "\x8B\x1C\x24\xC3";
    return memcmp(pbFunc, szGetPcThunkBx, 4) == 0;
}
#endif

void CAdminOP :: SetupTrampoline( BYTE *pbTarget, BYTE *pbHook, BYTE *pbTrampoline, bool bIsInSteamLib )
{
    // check for stub function
    if(*(char *)pbTrampoline == '\xE9')
    {
        pbTrampoline++;
        // not + 5 since we already ++'d
        pbTrampoline = pbTrampoline + 4 + (*(DWORD *)(pbTrampoline));
    }

    // make first 5 bytes of trampoline the first five of instruction
    CDetourDis Dis(NULL, NULL);

    BYTE *pbDst = pbTrampoline;
    BYTE *pbSrc = pbTarget;

    DWORD prev;
    VirtualProtect(pbTrampoline, 16, PAGE_EXECUTE_READWRITE, &prev);

    while((pbSrc - pbTarget) < 5)
    {
        BYTE* pbNew = Dis.CopyInstruction(pbDst, pbSrc);
        pbDst += (pbNew - pbSrc);

#ifdef __linux__
        if(((char *)pbSrc)[0] == '\xE8')
        {
            int callOffset = *((int *)&pbSrc[1]);
            int location = (int)pbNew + callOffset;

            // If the call is to __i686.get_pc_thunk.bx, the value of ebx
            // needs to be adjusted. The __i686.get_pc_thunk.bx function stores
            // the program counter into the ebx register. The program counter
            // will not be the expected value at this point since the call
            // instruction has moved from one place to another. This add
            // instruction that is being added here fixes the value in ebx to
            // match what would have been in ebx if the call to
            // __i686.get_pc_thunk.bx had never moved.
            //
            // 10/13/11 - GCC generates inline thunks of the format call 0; pop ebx
            // -- this pushes eip into ebx just like a normal thunk, so we need to
            // adjust for it as well
            bool bInlineThunk = (callOffset == 0);
            if(bInlineThunk || isFuncGetPcThunkBx((BYTE *)location))
            {
                int adjustment = (pbNew - pbDst);
                BYTE *pbAdjust = (BYTE *)&adjustment;
                // add to ebx
                // We pop ebx, fix it up, then push it.
                // this is because the 'pop ebx' step of the inline
                // thunk will be in the returned function, and skips
                // the ugly logic of copying an additional instruction
                // over and then modifying ebx after it.
                if (bInlineThunk)
                {
                    memset(pbDst - 4, 0, 4); // Undo CDetourDis's call correction, we want it to remain in the tramp
                    *(pbDst++) = 0x5B; // pop ebx
                }
                pbDst[0] = '\x81';
                pbDst[1] = '\xC3';
                pbDst[2] = pbAdjust[0];
                pbDst[3] = pbAdjust[1];
                pbDst[4] = pbAdjust[2];
                pbDst[5] = pbAdjust[3];
                pbDst += 6;
                if (bInlineThunk) *(pbDst++) = 0x53; // push ebx
            }
        }
#endif

        pbSrc = pbNew;
    }

    int bytesRead = (pbSrc - pbTarget);
    int bytesWritten = (pbDst - pbTrampoline);

    // make trampoline jump to target (+what we read) next
    // (to target from wherever we left off)
    int tjump = (int) (pbTarget + bytesRead) - ((int)pbDst) - 5;
    BYTE *pTjump = (BYTE *)&tjump;
    char *twritepos = (char *)pbDst;
    DWORD tprev;
    VirtualProtect(twritepos, 6, PAGE_EXECUTE_READWRITE, &tprev);
    twritepos[0] = '\xE9';
    twritepos[1] = pTjump[0];
    twritepos[2] = pTjump[1];
    twritepos[3] = pTjump[2];
    twritepos[4] = pTjump[3];
    twritepos[5] = '\x90';

    // make target jump to hook
    int vjump = (int)(pbHook) - (int)pbTarget - 5;
    BYTE *pVjump = (BYTE *)&vjump;
    char *vwritepos = (char *)pbTarget;
    DWORD vprev;
    VirtualProtect(vwritepos, bytesRead, PAGE_EXECUTE_READWRITE, &vprev);

    trampolineinfo_t trampData;
    trampData.length = bytesRead;
    trampData.location = (unsigned char *)vwritepos;
    trampData.olddata = (unsigned char *)malloc(bytesRead);
    trampData.insteam = bIsInSteamLib;
    memcpy(trampData.olddata, vwritepos, bytesRead);
    trampolines.AddToTail(trampData);

    vwritepos[0] = '\xE9';
    vwritepos[1] = pVjump[0];
    vwritepos[2] = pVjump[1];
    vwritepos[3] = pVjump[2];
    vwritepos[4] = pVjump[3];
    // nop unfilled bytes
    for(int i = 5; i < bytesRead; i++)
    {
        vwritepos[i] = '\x90';
    }
}

void CAdminOP :: RemoveTrampolines()
{
    bool steamIsLoaded = IsSteamLibLoaded();
    for(unsigned short i = 0; i < trampolines.Count(); i++)
    {
        trampolineinfo_t *trampData = trampolines.Base() + i; // fast access
        if(trampData->insteam && !steamIsLoaded)
            continue;

        memcpy(trampData->location, trampData->olddata, trampData->length);
        free(trampData->olddata);
    }
    trampolines.Purge();
}

bool CAdminOP :: IsSteamLibLoaded()
{
#ifndef __linux__
    MODULEENTRY32       modlentry;
    CMemoryPatcher      patcher;
    DWORD               processID = GetCurrentProcessId();

    if(!patcher.Init())
    {
        TimeLog("SourceOPErrors.log", "Failed to init mempatch.\n");
        Msg("Failed to init mempatch.\n");
        return true;
    }

    return patcher.FindModuleInProcess("steamclient.dll", processID, &modlentry);
#else
    char gamedir[256];
    char filepath[512];
    void *handle;

    engine->GetGameDir(gamedir, sizeof(gamedir));
    Q_snprintf(filepath, sizeof(filepath), "bin/steamclient.so");

    handle = dlopen(filepath, RTLD_LAZY | RTLD_NOLOAD);
    return handle != NULL;
#endif
}

const char *CAdminOP :: GetGameDescription( void )
{
    if(modify_gamedescription.GetBool())
    {
        static char retVal[256];
        const char *gameDescription = SH_CALL(servergame_cc, &IServerGameDLL::GetGameDescription)();
        Q_snprintf(retVal, 256, "SourceOP - %s", gameDescription);
        retVal[255] = '\0';
        RETURN_META_VALUE(MRES_OVERRIDE, retVal);
    }
    RETURN_META_VALUE(MRES_IGNORED, NULL);
}

// This function is no longer hooked.
// If needed, rehook it.
bool CAdminOP :: ServerGameDLLLevelInit( const char *pMapName, char const *pMapEntities, char const *pOldLevel, char const *pLandmarkName, bool loadGame, bool background )
{
    RETURN_META_VALUE(MRES_IGNORED, 0);
}

// This function is no longer hooked.
// If needed, rehook it.
edict_t *CAdminOP :: CreateEdictPre( int iForceEdictIndex )
{
    RETURN_META_VALUE(MRES_IGNORED, NULL);
}

// This function is no longer hooked.
// If needed, rehook it.
edict_t *CAdminOP :: CreateEdict( int iForceEdictIndex )
{
    edict_t *pRet = *(edict_t **)SH_GLOB_SHPTR->GetOrigRet();

    /*if(!pEList)
        pEList = pRet-33;*/

    //Msg("Create %i\n", pRet-pEList);
    if(getCreateEdict)
    {
        pCreateEdict = pRet;
        RETURN_META_VALUE(MRES_HANDLED,NULL);
    }
    RETURN_META_VALUE(MRES_IGNORED,NULL);
}

void CAdminOP :: RemoveEdictPre(edict_t *e)
{
    //CBaseEntity *pEntity = servergameents->EdictToBaseEntity(e);
    if(!e)
    {
        ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] RemoveEdictPre got null edict_t.\n");
    }
    IServerNetworkable *pNet = e->GetNetworkable();
    CBaseEntity *pEntity = pNet ? VFuncs::GetBaseEntity(pNet) : NULL;

    if(pNet && pEntity)
    {
        int entindex = ENTINDEX(e);

        if(entindex >= 0 && entindex < gpGlobals->maxEntities)
        {
            if(myEntList.IsValidIndex(entindex))
            {
                CAOPEntity *pEnt = myEntList[entindex];
                if(pEnt)
                {
                    pEnt->UpdateOnRemove();
                }
            }

            gEntList.RemoveEntity(CBaseHandle(entindex, 0));
            if(featureStatus[FEAT_LUA])
            {
                g_entityCache.EntityRemoved(entindex);
            }
            RETURN_META(MRES_HANDLED);
        }
        else
        {
            CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] RemoveEdictPre got an invalid entindex %i.\n", entindex);
            RETURN_META(MRES_IGNORED);
        }
    }
    else
    {
        CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] RemoveEdictPre got an invalid baseent or networkable.\n");
        RETURN_META(MRES_IGNORED);
    }
    RETURN_META(MRES_IGNORED);
}

IServerNetworkable *CAdminOP :: EntityCreated( const char *pClassName )
{
    IServerNetworkable *pRet = META_RESULT_ORIG_RET(IServerNetworkable *);

    if(!pRet)
        RETURN_META_VALUE(MRES_IGNORED,NULL);

    CBaseEntity *pEntity = VFuncs::GetBaseEntity(pRet);
    edict_t *pEdict = VFuncs::GetEdict(pRet);

    if(!pEntity || !pEdict)
        RETURN_META_VALUE(MRES_IGNORED,NULL);

    CBaseHandle handle = VFuncs::GetRefEHandle(pEntity);
    //Msg("%s %p created\n", pClassName, pEntity);

    int entindex = handle.GetEntryIndex();
    // HACKHACKHACK: This is a hack.
    if(!pEList)
    {
        pEList = pEdict - entindex;
    }
    if(entindex < 0 || entindex >= gpGlobals->maxEntities)
    {
        RETURN_META_VALUE(MRES_IGNORED,NULL);
        //CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] EntityCreated got an invalid entindex (%i) for ent %s (%s).\n", entindex, VFuncs::GetClassname(pEntity), VFuncs::GetClassName(pEntity));
        //pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] EntityCreated got an invalid entindex (%i) for ent %s (%s).\n", entindex, VFuncs::GetClassname(pEntity), VFuncs::GetClassName(pEntity));
    }

    if(VFuncs::IsEFlagSet(pEntity, EFL_SERVER_ONLY))
    {
        gEntList.AddNonNetworkableEntity( pEntity );
    }
    else
    {
        if(pEdict)
        {
            gEntList.AddNetworkableEntity( pEntity, entindex, handle.GetSerialNumber() );
        }
    }

    RETURN_META_VALUE(MRES_HANDLED,NULL);
}

void CAdminOP :: FreeEntPrivateData( void *pEntity )
{
    RETURN_META(MRES_IGNORED);
}

// FreeContainingEntity seems that it can be called on spawn and also on delete
void CAdminOP :: FreeContainingEntity( edict_t *pEntity )
{
    if(!pEntity)
        return;

    int EntIndex = ENTINDEX(pEntity);
#ifdef DFENTS
    if(DFEntities[EntIndex])
    {
#ifndef __linux__
        //DFTimeLog("tfc\\AdminOPErrors.log", "Warning: %s not null OnFreeEntPrivateData on %f map %s...freeing\n", STRING(pEnt->v.classname), gpGlobals->time, STRING(gpGlobals->mapname));
#else
        //DFTimeLog("tfc/AdminOPErrors.log", "Warning: %s not null OnFreeEntPrivateData on %f map %s...freeing\n", STRING(pEnt->v.classname), gpGlobals->time, STRING(gpGlobals->mapname));
#endif
        delete DFEntities[EntIndex];
        DFEntities[EntIndex] = NULL;
    }
#endif
    for(int i = 1; i <= maxClients; i++)
    {
        if(pAOPPlayers[i-1].GetPlayerState())
        {
            pAOPPlayers[i-1].FreeContainingEntity(pEntity);
        }
    }
    // TODO: seniorproj: Update remote client entity list
    int spawnedEntsElement = spawnedServerEnts.Find(EntIndex);

    if(spawnedEntsElement != spawnedServerEnts.InvalidIndex())
    {
        spawnedServerEnts.Remove(spawnedEntsElement);
    }

    RETURN_META(MRES_IGNORED);
}

void CAdminOP :: MarkEntitiesAsTouching( edict_t *e1, edict_t *e2 )
{
    CBaseEntity *entity = CBaseEntity::Instance( e1 );
    CBaseEntity *entityTouched = CBaseEntity::Instance( e2 );
    if ( entity && entityTouched )
    {
        // HACKHACK: UNDONE: Pass in the trace here??!?!?
        trace_t tr;
        UTIL_ClearTrace( tr );
        tr.endpos = (VFuncs::GetAbsOrigin(entity) + VFuncs::GetAbsOrigin(entityTouched)) * 0.5;
        entity->PhysicsMarkEntitiesAsTouching( entityTouched, tr );
    }

    RETURN_META(MRES_HANDLED);
}

void CAdminOP :: GetPlayerLimits( int& minplayers, int& maxplayers, int &defaultMaxPlayers )
{
#ifdef _L4D_PLUGIN
    RETURN_META(MRES_IGNORED);
#else
#ifdef OFFICIALSERV_ONLY
    maxplayers = 192;
#else
    if(maxplayers < 128) maxplayers = 128;
#endif
    RETURN_META(MRES_HANDLED);
#endif
}

bool CAdminOP :: SetClientListening(int iReceiver, int iSender, bool bListen)
{
    //Msg("SetClientListening(%i, %i, %i)\n", iReceiver, iSender, bListen);
    
    if(iReceiver >= 1 && iReceiver <= MAX_AOP_PLAYERS && iSender >= 1 && iSender <= MAX_AOP_PLAYERS)
    {
        // gags have higher priority than hearall (i.e. admins won't hear gagged players)
        if(pAdminOP.pAOPPlayers[iSender-1].IsGagged() && pAdminOP.pAOPPlayers[iSender-1].GetPlayerState())
        {
            // force listening off
            RETURN_META_VALUE_NEWPARAMS(MRES_HANDLED, true, &IVoiceServer::SetClientListening, (iReceiver, iSender, false));
        }
        if(m_bEndRoundAllTalk)
        {
            RETURN_META_VALUE_NEWPARAMS(MRES_HANDLED, true, &IVoiceServer::SetClientListening, (iReceiver, iSender, true));
        }
        if(pAdminOP.pAOPPlayers[iReceiver-1].IsAdmin(8192, "hearall") && pAdminOP.pAOPPlayers[iSender-1].GetPlayerState())
        {
            // force listening on
            RETURN_META_VALUE_NEWPARAMS(MRES_HANDLED, true, &IVoiceServer::SetClientListening, (iReceiver, iSender, true));
        }

        int voiceTeam = pAdminOP.pAOPPlayers[iSender-1].GetVoiceTeam();
        if(voiceTeam != 0)
        {
            // this player is speaking to all teams
            if(voiceTeam == -1)
            {
                //Msg(" Yes -- player talking is talking to all players\n");
                RETURN_META_VALUE_NEWPARAMS(MRES_HANDLED, true, &IVoiceServer::SetClientListening, (iReceiver, iSender, true));
            }
            // this player is speaking to the specific team
            else if(voiceTeam != 0)
            {
                RETURN_META_VALUE_NEWPARAMS(MRES_HANDLED, true, &IVoiceServer::SetClientListening, (iReceiver, iSender, voiceTeam == pAdminOP.pAOPPlayers[iReceiver-1].GetTeam()));
            }
        }
    }

    RETURN_META_VALUE(MRES_HANDLED, true);
}

int CAdminOP :: ParmValue( const char *psz, int nDefaultVal )
{
#ifdef OFFICIALSERV_ONLY
    if(FStrEq(psz, "-maxplayers"))
    {
        if(maxplayers_force.GetInt() > 0)
        {
            Msg("[SOURCEOP] Overriding -maxplayers parameter to %i.\n", maxplayers_force.GetInt());
            RETURN_META_VALUE(MRES_SUPERCEDE, maxplayers_force.GetInt());
        }
    }
#endif
    RETURN_META_VALUE(MRES_IGNORED, nDefaultVal);
}

bool CAdminOP :: PrecacheSound( const char *pSample, bool bPreload, bool bIsUISound )
{
    if ( tf2_disable_mvmprecache.GetBool() && Q_stristr(pSample, "/mvm/" ) )
    {
        //Msg("Blocking precache of %s\n", pSample);
        RETURN_META_VALUE(MRES_SUPERCEDE, true);
    }

    RETURN_META_VALUE(MRES_IGNORED, true);
}

void CAdminOP :: MaxPlayersDispatch( const CCommand &command )
{
    CCommand *ret = MaxPlayersDispatchHandler(command);
    if(ret)
    {
        RETURN_META_NEWPARAMS(MRES_HANDLED, &ConCommand::Dispatch, (*ret));
    }
    RETURN_META(MRES_IGNORED);
}

CCommand *CAdminOP :: MaxPlayersDispatchHandler( const CCommand &command )
{
#ifdef OFFICIALSERV_ONLY
    if(command.ArgC() == 2 && maxplayers_force.GetInt() > 0)
    {
        static CCommand newParam;
        Msg("[SOURCEOP] Overriding maxplayers command to %i.\n", maxplayers_force.GetInt());
        newParam.Tokenize(UTIL_VarArgs("maxplayers \"%s\"", maxplayers_force.GetString()));
        return &newParam;
    }
#endif

    return NULL;
}

void CAdminOP :: LogPrint( const char *msg)
{
    char newlogstring[1024];
    char datestring[32];
    tm curtime;

    if(isCstrike && !strcmp(msg, "World triggered \"Round_Start\"\n"))
    {
        RoundStart();
    }

    if(!strcmp(msg, "World triggered \"Round_Setup_Begin\"\n"))
    {
        SetupStart();
    }

    if(!strcmp(msg, "World triggered \"Round_Setup_End\"\n"))
    {
        SetupEnd();
    }

    g_pVCR->Hook_LocalTime( &curtime );

    Q_snprintf( datestring, sizeof( datestring ), "L %02i/%02i/%04i - %02i:%02i:%02i: ",
        curtime.tm_mon+1, curtime.tm_mday, 1900 + curtime.tm_year,
        curtime.tm_hour, curtime.tm_min, curtime.tm_sec);

    V_snprintf(newlogstring, sizeof(newlogstring), "%s%s", datestring, msg);
    if(remoteserver) remoteserver->LogPrint(newlogstring);
}

void CAdminOP :: ChangeLevel( const char *s1, const char *s2 )
{
    if(m_bChangeLevelBlock)
    {
        Msg("[SOURCEOP] Blocking ChangeLevel.\n");
        RETURN_META(MRES_SUPERCEDE);
    }
    RETURN_META(MRES_IGNORED);
}

void CAdminOP :: ReplicateCVar( const char *var, const char *value )
{
    if(!var || !value)
        return;

    char data[2048];
    bf_write msg(data, 2048);

    msg.WriteUBitLong(5, 6);
    msg.WriteByte(1);
    msg.WriteString(var);
    msg.WriteString(value);

    for(int i = 1; i <= maxClients; i++)
    {
        INetChannel *netchan = static_cast<INetChannel *>(engine->GetPlayerNetInfo(i));
        if(netchan)
        {
            netchan->SendData(msg, false);
        }
    }
}

void CAdminOP :: HookEntities()
{
    bool usefallback = false;
    bool hasQuery = false;
    LARGE_INTEGER liFrequency,liStart,liStop;

    if(QueryPerformanceFrequency(&liFrequency)) hasQuery = true;

    ColorMsg(CONCOLOR_DARKGRAY, "[SOURCEOP] Hooking entities.");
    if(hasQuery) QueryPerformanceCounter(&liStart);
    g_hookIDS.Purge();
    g_tempHookIDS.Purge();
    if(g_useOldEntityDictionary)
    {
        CEntityFactoryDictionaryOld *dict = (CEntityFactoryDictionaryOld *)EntityFactoryDictionary();
        if ( dict )
        {
            for ( int i = dict->m_Factories.First(); i != dict->m_Factories.InvalidIndex(); i = dict->m_Factories.Next( i ) )
            {
                if(i%ENT_HOOKS_PERDOT == 0) ColorMsg(CONCOLOR_DARKGRAY, ".");
                const char *pszEntName = dict->m_Factories.GetElementName( i );
                HookEntityType(pszEntName);
            }
        }
        else
        {
            ColorMsg(CONCOLOR_RED, ".");
            ColorMsg(CONCOLOR_LIGHTRED, ".Failed -- using fallback method");
            usefallback = true;
        }
    }
    else
    {
        CEntityFactoryDictionary *dict = (CEntityFactoryDictionary *)EntityFactoryDictionary();
        extern CUtlVector<int> g_serverHookIDS;
        if ( dict )
        {
            int r = SH_ADD_HOOK_MEMFUNC(IEntityFactoryDictionary, Create, dict, &pAdminOP, &CAdminOP::EntityCreated, true);
            if(r) g_serverHookIDS.AddToTail(r);

            for ( int i = dict->m_Factories.First(); i != dict->m_Factories.InvalidIndex(); i = dict->m_Factories.Next( i ) )
            {
                if(i%ENT_HOOKS_PERDOT == 0) ColorMsg(CONCOLOR_DARKGRAY, ".");
                const char *pszEntName = dict->m_Factories.GetElementName( i );
                HookEntityType(pszEntName);
            }
        }
        else
        {
            ColorMsg(CONCOLOR_RED, ".");
            ColorMsg(CONCOLOR_LIGHTRED, ".Failed -- using fallback method");
            usefallback = true;
        }
    }
    Msg("\n");

    if(usefallback)
    {
        // at a minimum, hook grenade and item_sodacan
        HookEntityType("grenade");
        HookEntityType("item_sodacan");
    }

    // remove temporary hooks that detected whether or not entities have been hooked
    for(int i = 0; i < g_tempHookIDS.Count(); i++)
    {
        SH_REMOVE_HOOK_ID(g_tempHookIDS.Element(i));
    }

    gEntList.CleanupDeleteList();

    if(hasQuery) QueryPerformanceCounter(&liStop);
    engine->LogPrint(UTIL_VarArgs("[SOURCEOP] \"Console<0><Console><Console>\" hooked entities in \"%f\" seconds\n", hasQuery ? (double)(liStop.QuadPart - liStart.QuadPart) / (double) liFrequency.QuadPart : -1));

}

void CAdminOP :: HookEntityType(const char *pszEntName)
{
    if(strcmp(pszEntName, "dynamic_prop") && strncmp(pszEntName, "npc_", 4) && strncmp(pszEntName, "phys_", 5) && strcmp(pszEntName, "player")
        && strcmp(pszEntName, "prop_door_rotating") && strcmp(pszEntName, "prop_dynamic") && strcmp(pszEntName, "prop_dynamic_ornament")
        && strcmp(pszEntName, "prop_dynamic_override") && strcmp(pszEntName, "prop_ragdoll_attached") && strncmp(pszEntName, "prop_vehicle", 12)
        && strcmp(pszEntName, "vehicle_viewcontroller") && strcmp(pszEntName, "worldspawn") && strcmp(pszEntName, "infodecal")
        // these don't work right (entindex doesn't work on keyvalue hook)
        && strcmp(pszEntName, "info_camera_link") && strcmp(pszEntName, "info_hint")
        && strcmp(pszEntName, "info_node") && strcmp(pszEntName, "info_node_air") && strcmp(pszEntName, "info_node_air_hint")
        && strcmp(pszEntName, "info_node_climb") && strcmp(pszEntName, "info_node_hint") && strcmp(pszEntName, "env_fog_controller")
        && strcmp(pszEntName, "sky_camera") && strncmp(pszEntName, "math_", 5) && strncmp(pszEntName, "logic_", 6)
        && strncmp(pszEntName, "filter_", 7) && strcmp(pszEntName, "point_template") && strcmp(pszEntName, "env_fade")
        && strcmp(pszEntName, "ai_hint") && strcmp(pszEntName, "world_items") && strcmp(pszEntName, "env_debughistory")
        // css
        && strcmp(pszEntName, "cs_bot")
        // dods
        && strcmp(pszEntName, "dod_bomb_dispenser_icon")
        // gmod9
        && strcmp(pszEntName, "weapon_physgun")
        // FF
        && strcmp(pszEntName, "env_particlelight")
        // these ones make console msgs
        && strcmp(pszEntName, "env_fire") && strcmp(pszEntName, "func_fish_pool") && strcmp(pszEntName, "proto_sniper") && strcmp(pszEntName, "_plasma")
        // FF
        && strcmp(pszEntName, "ff_grenade_napalm") && strcmp(pszEntName, "ff_grenade_napalmlet")
        // TF
        && strcmp(pszEntName, "tf_bot") && strcmp(pszEntName, "item_teamflag") && strncmp(pszEntName, "team_", 5) && strncmp(pszEntName, "game_", 5)
        && strcmp(pszEntName, "training_prop_dynamic")
        && strcmp(pszEntName, "tf_weapon_medigun") && strcmp(pszEntName, "eyeball_boss")
        //&& strcmp(pszEntName, "tank_destruction") && strcmp(pszEntName, "tank_boss")
        && strcmp(pszEntName, "tf_logic_mann_vs_machine")
        // causes crashing in ClientActive when unloading and then loading again
        && strcmp(pszEntName, "tf_objective_resource")
        && strcmp(pszEntName, "info_populator")
        // L4D
        && strcmp(pszEntName, "boomer") && strcmp(pszEntName, "hunter") && strcmp(pszEntName, "prop_car_glass") && strcmp(pszEntName, "prop_door_rotating_checkpoint")
        && strcmp(pszEntName, "prop_health_cabinet") && strcmp(pszEntName, "prop_minigun") && strcmp(pszEntName, "smoker") && strcmp(pszEntName, "survivor_bot")
        && strcmp(pszEntName, "tank") && strcmp(pszEntName, "prop_minigun") && strcmp(pszEntName, "smoker") && strcmp(pszEntName, "survivor_bot")
        && strcmp(pszEntName, "info_changelevel") && strcmp(pszEntName, "info_solo_changelevel") && strcmp(pszEntName, "trigger") && strcmp(pszEntName, "trigger_auto_crouch")
        && strcmp(pszEntName, "trigger_autosave") && strcmp(pszEntName, "trigger_cdaudio") && strncmp(pszEntName, "trigger_", 8)
        // SourceOP
        && !IsEntityOnInstalledList(pszEntName)
//TODO: Remove this test
#ifdef __linux__
#ifdef _L4D_PLUGIN
    && (!strcmp(pszEntName, "grenade") || !strcmp(pszEntName, "item_sodacan"))
#endif
#endif
        )
    {
        //ColorMsg(CONCOLOR_MAGENTA, "%s\n", pszEntName);
        if(g_useEntFactDebugPrints)
        {
            ColorMsg(CONCOLOR_MAGENTA, "%s\n", pszEntName);
        }
        CBaseEntity *pTest = CreateEntityByName(pszEntName);
        if(pTest)
        {
            if(g_useEntFactDebugPrints)
            {
                ColorMsg(CONCOLOR_MAGENTA, "%s %s\n", pszEntName, VFuncs::GetClassName(pTest));
            }
            g_hookTest = 0;
            VFuncs::KeyValue(pTest, "sophooktest", "");
            // only hook this ent type if no hook received the keyvalue call. otherwise, that means it has been hooked already.
            if(!g_hookTest)
            {
                g_tempHookIDS.AddToTail(SH_ADD_MANUALVPHOOK(CBaseEntity_KeyValue, pTest, SH_STATIC(TestKeyValue), false));
                g_hookIDS.AddToTail(SH_ADD_MANUALVPHOOK(CBaseEntity_TraceAttack, pTest, SH_STATIC(HookTraceAttack), false));
                if(VFuncs::IsBaseCombatWeapon(pTest))
                {
                    g_hookIDS.AddToTail(SH_ADD_MANUALVPHOOK(CBaseCombatWeapon_PrimaryAttack, pTest, SH_STATIC(HookPrimaryAttack), false));
                    g_hookIDS.AddToTail(SH_ADD_MANUALVPHOOK(CBaseCombatWeapon_SecondaryAttack, pTest, SH_STATIC(HookSecondaryAttack), false));
                }
            }
            else
            {
                if(g_useEntFactDebugPrints)
                {
                    ColorMsg(CONCOLOR_LIGHTRED, " - This entity has already been hooked.\n", pszEntName);
                }
            }
            UTIL_Remove(pTest);
        }
    }
    else
    {
        if(g_useEntFactDebugPrints)
        {
            ColorMsg(CONCOLOR_LIGHTRED, "%s is on the deny list\n", pszEntName);
        }
    }
}

bool CAdminOP :: IsEntityOnInstalledList(const char *pszEntName)
{
    FOR_EACH_VEC(installedFactories, i)
    {
        classinstall_t install = installedFactories[i];
        if(!strcmp(pszEntName, install.pClassName))
            return true;
    }

    return false;
}

void CAdminOP :: ServiceEnt(CBaseEntity *pEntity, CAOPEntity *pNew)
{
    //bool hasQuery = false;
    //LARGE_INTEGER liFrequency,liStart,liStop;

    //if(QueryPerformanceFrequency(&liFrequency)) hasQuery = true;
    //if(hasQuery) QueryPerformanceCounter(&liStart);
    bool fail = 0;

    if((pNew->hookID_spawn = SH_ADD_MANUALHOOK(CBaseEntity_Spawn, pEntity, SH_MEMBER(pNew, &CAOPEntity::Spawn), false)) == 0) fail = 1;
    if((pNew->hookID_precache = SH_ADD_MANUALHOOK(CBaseEntity_Precache, pEntity, SH_MEMBER(pNew, &CAOPEntity::Precache), false)) == 0) fail = 1;
    if((pNew->hookID_acceptinput = SH_ADD_MANUALHOOK(CBaseEntity_AcceptInput, pEntity, SH_MEMBER(pNew, &CAOPEntity::AcceptInput), false)) == 0) fail = 1;
    if((pNew->hookID_touch = SH_ADD_MANUALHOOK(CBaseEntity_Touch, pEntity, SH_MEMBER(pNew, &CAOPEntity::Touch), false)) == 0) fail = 1;
    //if((pNew->hookID_touchPost = SH_ADD_MANUALHOOK(CBaseEntity_Touch, pEntity, SH_MEMBER(pNew, &CAOPEntity::TouchPost), true)) == 0) fail = 1;
    if((pNew->hookID_startTouch = SH_ADD_MANUALHOOK(CBaseEntity_StartTouch, pEntity, SH_MEMBER(pNew, &CAOPEntity::StartTouch), false)) == 0) fail = 1;
    if((pNew->hookID_think = SH_ADD_MANUALHOOK(CBaseEntity_Think, pEntity, SH_MEMBER(pNew, &CAOPEntity::Think), false)) == 0) fail = 1;
    if((pNew->hookID_keyvalue = SH_ADD_MANUALHOOK(CBaseEntity_KeyValue, pEntity, SH_MEMBER(pNew, &CAOPEntity::KeyValue), false)) == 0) fail = 1;
    if((pNew->hookID_use = SH_ADD_MANUALHOOK(CBaseEntity_Use, pEntity, SH_MEMBER(pNew, &CAOPEntity::Use), false)) == 0) fail = 1;
    //Msg("Hook ent %i:%s %i/%i\n", VFuncs::entindex(pEntity), VFuncs::GetClassname(pEntity), hooks, unhooks);
    if(fail)
    {
        TimeLog("SourceOPErrors.log", "Failure to hook all hooks for entity %s\n", VFuncs::GetClassname(pEntity));
    }
    else
    {
        hooks++;
    }
    //if(hasQuery) QueryPerformanceCounter(&liStop);
    //Msg("[SOURCEOP] Hooked ent funcs in \"%f\" seconds.\n", hasQuery ? (double)(liStop.QuadPart - liStart.QuadPart) / (double) liFrequency.QuadPart : -1);
}

void CAdminOP :: UnhookEnt(CBaseEntity *pEntity, CAOPEntity *pAOP, int index)
{
    //bool hasQuery = false;
    //LARGE_INTEGER liFrequency,liStart,liStop;

    //if(QueryPerformanceFrequency(&liFrequency)) hasQuery = true;
    //if(hasQuery) QueryPerformanceCounter(&liStart);
    bool fail = 0;
    bool hashooks = 0;
    if(pAOP->hookID_spawn)
    {
        if(SH_REMOVE_HOOK_ID(pAOP->hookID_spawn) == 0) fail = 1;
        hashooks = 1;
    }
    if(pAOP->hookID_precache)
    {
        if(SH_REMOVE_HOOK_ID(pAOP->hookID_precache) == 0) fail = 1;
        hashooks = 1;
    }
    if(pAOP->hookID_acceptinput)
    {
        if(SH_REMOVE_HOOK_ID(pAOP->hookID_acceptinput) == 0) fail = 1;
        hashooks = 1;
    }
    if(pAOP->hookID_touch)
    {
        if(SH_REMOVE_HOOK_ID(pAOP->hookID_touch) == 0) fail = 1;
        hashooks = 1;
    }
    //if(pAOP->hookID_touchPost)
    //{
        //if(SH_REMOVE_HOOK_ID(pAOP->hookID_touchPost) == 0) fail = 1;
        //hashooks = 1;
    //}
    if(pAOP->hookID_startTouch)
    {
        if(SH_REMOVE_HOOK_ID(pAOP->hookID_startTouch) == 0) fail = 1;
        hashooks = 1;
    }
    if(pAOP->hookID_think)
    {
        if(SH_REMOVE_HOOK_ID(pAOP->hookID_think) == 0) fail = 1;
        hashooks = 1;
    }
    if(pAOP->hookID_keyvalue)
    {
        if(SH_REMOVE_HOOK_ID(pAOP->hookID_keyvalue) == 0) fail = 1;
        hashooks = 1;
    }
    if(pAOP->hookID_use)
    {
        if(SH_REMOVE_HOOK_ID(pAOP->hookID_use) == 0) fail = 1;
        hashooks = 1;
    }
    if(fail)
    {
        TimeLog("SourceOPErrors.log", "Failure to unhook all hooks for entity %s\n", VFuncs::GetClassname(pEntity));
    }
    else if(hashooks == 1)
    {
        unhooks++;
    }

    edict_t *pEnt = pAOP->Get();
    for(int i = 0; i < myThinkEnts.Count(); i++)
    {
        CAOPEntity *myEnt = myThinkEnts[i];
        if(pEnt == myEnt->Get())
        {
            myThinkEnts.Remove(i);
            break;
        }
    }
    if(index <= 0 || myEntList[index] == NULL || myEntList[index]->Get() != pEnt)
    {
        bool found = false;
        ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Invalid entindex passed to UnhookEnt. Attempting to find and unhook entity.\n");
        for(int i = 0; i < myEntList.Count(); i++)
        {
            CAOPEntity *myEnt = myEntList[i];
            if(myEnt)
            {
                if(pEnt == myEnt->Get())
                {
                    found = true;
                    myEntList[i] = NULL;
                    delete myEnt;
                    break;
                }
            }
        }
        if(!found)
        {
            ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP]     - But it wasn't found!!\n");
        }
    }
    else
    {
        CAOPEntity *myEnt = myEntList[index];
        myEntList[index] = NULL;
        delete myEnt;
    }
    //Msg("Unkook ent %i:%s %i/%i\n", VFuncs::entindex(pEntity), VFuncs::GetClassname(pEntity), hooks, unhooks);
    //if(hasQuery) QueryPerformanceCounter(&liStop);
    //Msg("[SOURCEOP] Unhooked ent funcs in \"%f\" seconds.\n", hasQuery ? (double)(liStop.QuadPart - liStart.QuadPart) / (double) liFrequency.QuadPart : -1);
}

#ifdef DFENTS
void CAdminOP :: RemoveDFEnt( int entindex )
{
    if(DFEntities[entindex])
    {
        delete DFEntities[entindex];
        DFEntities[entindex] = NULL;
    }
}
#endif

class CSOPEntityStringPool : public CStringPool, public CAutoGameSystem
{
    virtual char const *Name() { return "CSOPEntityStringPool"; }

    virtual void Shutdown() 
    {
        FreeAll();
    }
};

void CAdminOP :: AddEntityToInstallList(IEntityFactory *pFactory, const char *pClassName, int feature)
{
    static CSOPEntityStringPool g_EntityStringPool;
    classinstall_t install;
    install.pClassName = g_EntityStringPool.Allocate(pClassName);
    install.pFactory = pFactory;
    install.feature = feature;

    installList.AddToTail(install);
}

// the install list can only be processed when cvar is available
// cvar is required so that EntityFactoryDictionary() can work
void CAdminOP :: ProcessInstallList()
{
    for(unsigned short i = 0; i < installList.Count(); i++)
    {
        classinstall_t *install = installList.Base() + i; // fast access
        if(install->feature < 0 || (install->feature < NUM_FEATS && FeatureStatus(install->feature)))
        {
            IEntityFactoryDictionary *pDict = EntityFactoryDictionary();
            if(pDict)
            {
                pDict->InstallFactory( install->pFactory, install->pClassName );
                installedFactories.AddToTail(*install);
            }
            else
            {
                ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Cannot install factory for entitiy %s.\n", install->pClassName);
                TimeLog("SourceOPErrors.log", "Cannot install factory for entitiy %s.", install->pClassName);
            }
        }
    }
}

void CAdminOP :: PrecacheInstallList()
{
    for(unsigned short i = 0; i < installList.Count(); i++)
    {
        classinstall_t *install = installList.Base() + i; // fast access
        if(install->feature < 0 || (install->feature < NUM_FEATS && FeatureStatus(install->feature)))
        {
            UTIL_PrecacheOther(install->pClassName);
        }
    }
}

void CAdminOP :: RemoveInstalledFactories()
{
    if(!installedFactories.Count())
        return;

    ColorMsg(CONCOLOR_DARKGRAY, "[SOURCEOP] Removing installed factories.");
    if(g_useOldEntityDictionary)
    {
        CEntityFactoryDictionaryOld *dict = (CEntityFactoryDictionaryOld *)EntityFactoryDictionary();
        if ( dict )
        {
            int prevCount = dict->m_Factories.Count();
            FOR_EACH_VEC(installedFactories, i)
            {
                classinstall_t install = installedFactories[i];
                dict->m_Factories.Remove(install.pClassName);
            }
            ColorMsg(CONCOLOR_DARKGRAY, " (%i of %i removed)", prevCount - dict->m_Factories.Count(), installedFactories.Count());
        }
        else
        {
            ColorMsg(CONCOLOR_RED, ".");
            ColorMsg(CONCOLOR_LIGHTRED, ".Failed -- no dictionary");
        }
    }
    else
    {
        CEntityFactoryDictionary *dict = (CEntityFactoryDictionary *)EntityFactoryDictionary();
        extern CUtlVector<int> g_serverHookIDS;
        if ( dict )
        {
            int prevCount = dict->m_Factories.Count();
            FOR_EACH_VEC(installedFactories, i)
            {
                classinstall_t install = installedFactories[i];
                dict->m_Factories.Remove(install.pClassName);
            }
            ColorMsg(CONCOLOR_DARKGRAY, " (%i of %i removed)", prevCount - dict->m_Factories.Count(), installedFactories.Count());
        }
        else
        {
            ColorMsg(CONCOLOR_RED, ".");
            ColorMsg(CONCOLOR_LIGHTRED, ".Failed -- no dictionary");
        }
    }
    Msg("\n");
}

bool CAdminOP :: FakeClientCommand(edict_t *pEntity, bool needCheats, bool ConCmdOnly, const CCommand &args)
{
    if(needCheats && !sv_cheats)
        return 0;

    ConCommand *concmd = (ConCommand*) cvar->FindCommand( args[0] );
    if(concmd)
    {
        char svCheats[128];
        if(needCheats)
        {
            strncpy(svCheats, sv_cheats->GetString(), 128);
            svCheats[127] = '\0';
            sv_cheats->SetValue(1);
        }

        servergameclients->SetCommandClient(ENTINDEX(pEntity)-1);
        concmd->Dispatch(args);

        if(needCheats)
        {
            sv_cheats->SetValue(svCheats);
        }
        return 1;
    }
    else if(!ConCmdOnly)
    {
        char svCheats[128];
        if(needCheats)
        {
            strncpy(svCheats, sv_cheats->GetString(), 128);
            svCheats[127] = '\0';
            sv_cheats->SetValue(1);
        }

        servergameclients->SetCommandClient(ENTINDEX(pEntity)-1);
        servergameclients->ClientCommand(pEntity, args);

        if(needCheats)
        {
            sv_cheats->SetValue(svCheats);
        }
        return 1;
    }
    return 0;
}

bool CAdminOP :: FakeClientCommand(edict_t *pEntity, bool needCheats, bool ConCmdOnly, const char *szCommand, int argc, const char *szArg1, const char *szArg2, const char *szArg3)
{
    const char **argarray = (const char **)malloc(sizeof(const char *) * 4);
    argarray[0] = szCommand;
    argarray[1] = szArg1;
    argarray[2] = szArg2;
    argarray[3] = szArg3;
    CCommand args(argc, argarray);

    return FakeClientCommand(pEntity, needCheats, ConCmdOnly, args);
}

bool CAdminOP :: GetMeleeMode()
{
    return meleeonly;
}

void CAdminOP :: SetMeleeMode(bool enabled)
{
    if(isTF2 && enabled)
    {
        for(int i = 1; i <= pAdminOP.GetMaxClients(); i++)
        {
            IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+i);

            if(info)
            {
                if(info->IsConnected())
                {
                    edict_t *pPlayer = pAdminOP.GetPlayer(i);
                    if(pPlayer)
                    {
                        engine->ClientCommand(pPlayer, "destroy 2;destroy 1;destroy 0;destroy 3");
                    }
                }
            }
        }
    }
    meleeonly = enabled;
}

/*char printable_character(char c)
{
    if(c >= '\x20' && c <= '\x7E')
        return c;
    return '.';
}

void print_data(const char *data, int len)
{
    int i;
    for(i = 0; i < len; i++)
    {
        if(i > 0 && (i % 16 == 0))
        {
            printf("\t");
            for(int j = i - 16; j < i; j++)
            {
                printf("%c", printable_character(data[j]));
            }
            printf("\n");
        }
        printf("%02X ", (unsigned int)*(unsigned char *)&data[i]);
    }
    if(i > 0)
    {
        // i'm stupid, there must be a better way to do this
        // all three of these work
        //int remaining = abs((i % 16) - 16) % 16;
        //int remaining = (((i >> 4) << 4) + 16 - i) % 16;
        int remaining = ((~(i & 0xF)) + 1) & 0xF;

        for(int j = 0; j < remaining; j++)
        {
            printf("   ");
        }
        printf("\t");

        int min = i - (i % 16);
        if(min < 0) min = 0;

        for(int j = min; j < i; j++)
        {
            printf("%c", printable_character(data[j]));
        }
    }
    printf("\n");
}

void print_data_shifted(const char *data, int len)
{
    char shifted[2048];
    memset(shifted, 0, 2048);

    for(int i = 0; i < 8; i++)
    {
        bf_read readbuf(data, len);
        readbuf.ReadSBitLong(i);
        int remaining = readbuf.GetNumBytesLeft();
        printf("%i %i\n", i, remaining);

        if(remaining)
        {
            readbuf.ReadBytes(shifted, remaining);
            print_data(shifted, remaining);
        }
    }
}

bool SendDataHook(bf_write &msg, bool bReliable)
{
    Msg("SendData %i bytes.\n", msg.GetNumBytesWritten());
    print_data((char *)msg.GetData(), min(msg.GetNumBytesWritten(), 255));
    RETURN_META_VALUE(MRES_IGNORED, 0);
}

int SendDatagramHook(bf_write *msg)
{
    if(msg)
    {
        Msg("SendDatagram %i bytes.\n", msg->GetNumBytesWritten());
        print_data((char *)msg->GetData(), min(msg->GetNumBytesWritten(), 255));
    }
    else
    {
        Msg("SendDatagram null msg\n");
    }
    RETURN_META_VALUE(MRES_IGNORED, 0);
}*/

PLUGIN_RESULT CAdminOP :: ClientConnect(bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen)
{
    int userid = ENTINDEX(pEntity);

    /*static bool firstConnect = true;
    if(firstConnect)
    {
        INetChannel *netchan = static_cast<INetChannel *>(engine->GetPlayerNetInfo(userid));
        SH_ADD_HOOK(INetChannel, SendData, netchan, SH_STATIC(SendDataHook), true);
        SH_ADD_HOOK(INetChannel, SendDatagram, netchan, SH_STATIC(SendDatagramHook), true);
        firstConnect = false;
    }*/

    SOPDLog(UTIL_VarArgs("ClientConnect(%i, %s, %s)\n", userid-1, pszName, pszAddress));
    if(userid > 0 && userid <= maxClients)
    {
        char ip_addr[IP_SIZE];
        char *p;
        // Get the IP address
        memset( ip_addr, 0, sizeof( ip_addr ));
        if( pszAddress )
        {
            strncpy( ip_addr, pszAddress, IP_SIZE );

            // Remove ":port", we just want IP 
            if( p = strchr( ip_addr, ':' )) *p = '\0';
        }

        strncpy(pAOPPlayers[userid-1].IP, ip_addr, IP_SIZE);
        pAOPPlayers[userid-1].ZeroCredits();
        pAOPPlayers[userid-1].IP[IP_SIZE-1] = '\0';
        pAOPPlayers[userid-1].addr = inet_addr(ip_addr);
        
        char *token;
        char seps[]=".";
        memset(pAOPPlayers[userid-1].ipocts, 0, sizeof(pAOPPlayers[userid-1].ipocts));
        token = strtok( ip_addr, seps );
        if(token)
        {
            pAOPPlayers[userid-1].ipocts[0] = atoi(token);
            token = strtok( NULL, seps );
        }
        if(token)
        {
            pAOPPlayers[userid-1].ipocts[1] = atoi(token);
            token = strtok( NULL, seps );
        }
        if(token)
        {
            pAOPPlayers[userid-1].ipocts[2] = atoi(token);
            token = strtok( NULL, seps );
        }
        if(token)
            pAOPPlayers[userid-1].ipocts[3] = atoi(token);

        pAOPPlayers[userid-1].SetJoinName(pszName);
        pAOPPlayers[userid-1].Connect();

        char connectMsg[256];
        V_snprintf(connectMsg, sizeof(connectMsg), "%s has joined the game", pszName);
        if(remoteserver) remoteserver->GameChat(0, connectMsg);
    }

    // Be careful if you ever return anything other than PLUGIN_CONTINUE here.
    // The metamod shim currently won't handle it properly.
    return PLUGIN_CONTINUE;
}

void CAdminOP :: ClientActive(edict_t *pEntity)
{
    int userid = ENTINDEX(pEntity);

    if(userid > 0 && userid <= maxClients)
    {
        SOPDLog(UTIL_VarArgs("ClientActive(%i, %s)\n", userid-1, pAOPPlayers[userid-1].GetJoinName()));
        if(!hookedWorld)
        {
            CBaseEntity *pWorld = (CBaseEntity *)CBaseEntity::Instance(GetEntityList());
            if(pWorld)
            {
                g_hookIDS.AddToTail(SH_ADD_MANUALVPHOOK(CBaseEntity_TraceAttack, pWorld, SH_STATIC(HookTraceAttack), false));
            }

            hookedWorld = 1;
        }

        CBaseEntity *pPlayer = (CBaseEntity *)CBaseEntity::Instance(pEntity);
        if(pPlayer && !HasHookedVTable(*(void**)pPlayer))
        {
            if(!bHasPlayerDataDesc)
            {
                GrabDataDesc(pPlayer);
                bHasPlayerDataDesc = true;
                VFuncs::LoadPlayerDataDescOffsets();
            }
            g_hookIDS.AddToTail(SH_ADD_MANUALVPHOOK(CBaseEntity_TraceAttack, pPlayer, SH_STATIC(HookTraceAttack), false));
            g_hookIDS.AddToTail(SH_ADD_MANUALVPHOOK(CBaseEntity_OnTakeDamage, pPlayer, SH_STATIC(HookPlayerOnTakeDamage), false));
            g_hookIDS.AddToTail(SH_ADD_MANUALVPHOOK(CBaseEntity_FireBullets, pPlayer, SH_STATIC(HookPlayerFireBullets), false));
            g_hookIDS.AddToTail(SH_ADD_MANUALVPHOOK(CTFPlayer_Event_Killed, pPlayer, SH_STATIC(HookPlayerEventKilled), true));
            g_hookIDS.AddToTail(SH_ADD_MANUALVPHOOK(CBasePlayer_Weapon_Switch, pPlayer, SH_STATIC(HookWeaponSwitch), true));
            g_hookIDS.AddToTail(SH_ADD_MANUALVPHOOK(CBasePlayer_ItemPostFrame, pPlayer, SH_STATIC(HookItemPostFrame), false));

            if(isTF2)
            {
                g_hookIDS.AddToTail(SH_ADD_MANUALVPHOOK(CTFPlayer_GiveNamedScriptItem, pPlayer, SH_STATIC(HookGiveNamedScriptItem), false));
            }
            //TODO: L4D may be acting up with these. Test removing it.
#ifndef _L4D_PLUGIN
            g_hookIDS.AddToTail(SH_ADD_MANUALVPHOOK(CBasePlayer_CanHearAndReadChatFrom, pPlayer, SH_STATIC(HookCanHearAndReadChatFrom), true));
#endif

            g_hookedVTables.AddToTail(*(void**)pPlayer);
        }
        pAOPPlayers[userid-1].Activate();
    }
}

const objectparams_t g_PhysDefaultObjectParams =
{
    NULL,
    1.0, //mass
    1.0, // inertia
    0.1f, // damping
    0.1f, // rotdamping
    0.05f, // rotIntertiaLimit
    "DEFAULT",
    NULL,// game data
    0.f, // volume (leave 0 if you don't have one or call physcollision->CollideVolume() to compute it)
    1.0f, // drag coefficient
    true,// enable collisions?
};

PLUGIN_RESULT CAdminOP :: ClientCommand(edict_t *pEntity, const CCommand &args)
{
    char pcmd[512];

    CBasePlayer *pPlayer = NULL;
    CAdminOPPlayer *pAOPPlayer = NULL;
    IPlayerInfo *info = NULL;
    int playerindex = 0;

    PLUGIN_RESULT adminCommands = PLUGIN_CONTINUE;
    PLUGIN_RESULT playerCommands = PLUGIN_CONTINUE;

    playerindex = ENTINDEX(pEntity);
    if(!playerindex)
        return PLUGIN_CONTINUE;

    pPlayer = (CBasePlayer *)VFuncs::Instance(pEntity);
    pAOPPlayer = &pAOPPlayers[playerindex-1];
    info = playerinfomanager->GetPlayerInfo(pEList+playerindex);
    if(!info)
        return PLUGIN_CONTINUE;
    if(!info->IsConnected())
        return PLUGIN_CONTINUE;

    strncpy( pcmd, args[0], sizeof(pcmd) ); 

    PLUGIN_RESULT cmdResult = clientCommands->RunCmd(args, pEntity, pPlayer, info);
    if(cmdResult != PLUGIN_CONTINUE)
    {
        return cmdResult;
    }

#ifdef OFFICIALSERV_ONLY
    bool isTestAdmin =  !!pAOPPlayer->IsAdmin(1024, "test");

    if(FStrEq(pcmd, "testhudnotifycustom") && isTestAdmin)
    {
        bf_write *pWrite;
        CReliableBroadcastRecipientFilter filter;
        filter.AddAllPlayers();


        pWrite = engine->UserMessageBegin(&filter, GetMessageByName("HudNotifyCustom"));
        // message
        pWrite->WriteString(args[1]);
        // from mod_textures.txt
        pWrite->WriteString(args[2]);
        // team color
        pWrite->WriteByte(atoi(args[3]));
        engine->MessageEnd();

        return PLUGIN_STOP;
    }
    if(FStrEq(pcmd, "testparticle") && isTestAdmin)
    {
        if(args.ArgC() > 2)
        {
            DispatchParticleEffect(args.Arg(1), PATTACH_POINT_FOLLOW, pPlayer, atoi(args.Arg(2)));
        }
        else
        {
            DispatchParticleEffect(args.Arg(1), VFuncs::GetAbsOrigin(pPlayer), VFuncs::GetAbsAngles(pPlayer));
        }
        return PLUGIN_STOP;
    }
    if(FStrEq(pcmd, "testremoveparticles") && isTestAdmin)
    {
        StopParticleEffects(pPlayer);
        return PLUGIN_STOP;
    }
    if(FStrEq(pcmd, "testreconnect") && isTestAdmin)
    {
        VFuncs::Reconnect(pAdminOP.pAOPPlayers[playerindex-1].baseclient);

        return PLUGIN_STOP;
    }
    if(FStrEq(pcmd, "testdisconnect") && isTestAdmin)
    {
        void *pBaseClient = pAdminOP.pAOPPlayers[playerindex-1].baseclient;
        if(pBaseClient)
        {
            if(args.ArgC() > 1)
            {
                VFuncs::Disconnect(pBaseClient, "%s", args.ArgS());
            }
            else
            {
                VFuncs::Disconnect(pBaseClient, "LOL BYE!");
            }
        }
        else
        {
            pAdminOP.pAOPPlayers[playerindex-1].SayTextChatHud("It's broken.\n");
        }

        return PLUGIN_STOP;
    }
    if(FStrEq(pcmd, "testsetnumbuildings") && isTestAdmin)
    {
        int offset = pAdminOP.GetPropOffset("DT_TFPlayer", "m_PlayerClass");
        int searchval = atoi(args.Arg(1));
        for(int i = 0; i <= 80; i++)
        {
            DECLARE_VAR_RVAL(int, m_iObjectCount, pPlayer, (offset + i), PLUGIN_STOP);
            if(*m_iObjectCount == searchval)
            {
                Msg("%i\n", i);
            }
        }
        return PLUGIN_STOP;
    }

    /*if(FStrEq(pcmd, "testallinvis") && isTestAdmin)
    {
        static bool invis = true;
        CBaseEntity *pCur = NULL;
        while(pCur = gEntList.NextEnt(pCur))
        {
            if(invis)
            {
                VFuncs::SetRenderMode(pCur, kRenderTransTexture);
                VFuncs::SetRenderColorA(pCur, 0);
            }
            else
            {
                VFuncs::SetRenderMode(pCur, kRenderNormal);
                VFuncs::SetRenderColorA(pCur, 255);
            }
        }
        invis = !invis;
        return PLUGIN_STOP;
    }
    if(FStrEq(engine->Cmd_Argv(0), "makejeep_css") && isTestAdmin)
    {
        Vector vecForward;
        //AngleVectors( VFuncs::EyeAngles(pPlayer), &vecForward );
        CBaseEntity *pJeep = CreateEntityByName( "prop_vehicle_driveable" );
        //edict_t *peJeep = engine->CreateEdict();
        //CBaseEntity *pJeep = CBaseEntity::Instance(peJeep);
        if(pJeep)
        {
            //pJeep->m_iClassname = MAKE_STRING("prop_vehivle_driveable");
            Vector vecOrigin = VFuncs::GetAbsOrigin(pPlayer) + Vector(0,0,64);
            QAngle vecAngles( 0, 0, 0 );
            //VFuncs::SetAbsOrigin(pJeep,  vecOrigin );
            //VFuncs::SetAbsAngles(pJeep,  vecAngles );
            VFuncs::KeyValue(pJeep,  "model", "models/buggy.mdl" );
            VFuncs::KeyValue(pJeep,  "solid", "6" );
            VFuncs::KeyValue(//pJeep,  "targetname", "jeep" );
            VFuncs::KeyValue(pJeep,  "vehiclescript", "scripts/vehicles/jeep_test.txt" );
            VFuncs::Spawn(pJeep);
            VFuncs::Activate(pJeep);
            VFuncs::Teleport(pJeep,  &vecOrigin, &vecAngles, NULL );
        }
    }
    if(FStrEq(engine->Cmd_Argv(0), "makeboat_css") && isTestAdmin)
    {
        Vector vecForward;
        //AngleVectors( VFuncs::EyeAngles(pPlayer), &vecForward );
        CBaseEntity *pJeep = CreateEntityByName( "prop_vehicle_driveable" );
        //edict_t *peJeep = engine->CreateEdict();
        //CBaseEntity *pJeep = CBaseEntity::Instance(peJeep);
        if(pJeep)
        {
            //pJeep->m_iClassname = MAKE_STRING("prop_vehivle_driveable");
            Vector vecOrigin = VFuncs::GetAbsOrigin(pPlayer) + Vector(0,0,64);
            QAngle vecAngles( 0, 0, 0 );
            //VFuncs::SetAbsOrigin(pJeep,  vecOrigin );
            //VFuncs::SetAbsAngles(pJeep,  vecAngles );
            VFuncs::KeyValue(pJeep,  "model", "models/airboat.mdl" );
            VFuncs::KeyValue(pJeep,  "solid", "6" );
            VFuncs::KeyValue(//pJeep,  "targetname", "jeep" );
            VFuncs::KeyValue(pJeep,  "vehiclescript", "addons/SourceOP/airboat.txt" );
            VFuncs::Spawn(pJeep);
            VFuncs::Activate(pJeep);
            VFuncs::Teleport(pJeep,  &vecOrigin, &vecAngles, NULL );
        }
    }*/
    /*if(FStrEq(engine->Cmd_Argv(0), "makecannister") && isTestAdmin)
    {
        CBaseEntity *pDoll = CreateEntityByName( "physics_cannister" );
        if(pDoll)
        {
            Vector vecOrigin = VFuncs::GetAbsOrigin(pPlayer) + Vector(0,0,128);
            QAngle vecAngles( 0, 0, 0 );
            //VFuncs::SetAbsOrigin(pDoll,  vecOrigin );
            //VFuncs::SetAbsAngles(pDoll,  vecAngles );
            VFuncs::KeyValue(pDoll,  "model", "models/props_c17/canister_propane01a.mdl" );
            VFuncs::KeyValue(pDoll,  "spawnflags", "0" );
            VFuncs::KeyValue(pDoll,  "expdamage", "200" );
            VFuncs::KeyValue(pDoll,  "expradius", "250" );
            VFuncs::KeyValue(pDoll,  "health", "25" );
            VFuncs::KeyValue(pDoll,  "thrust", "3000" );
            VFuncs::KeyValue(pDoll,  "fuel", "12" );
            VFuncs::KeyValue(pDoll,  "gassound", "ambient/gas/cannister_loop.wav" );
            VFuncs::Spawn(pDoll);
            VFuncs::Activate(pDoll);
            VFuncs::Teleport(pDoll,  &vecOrigin, &vecAngles, NULL );
        }
    }
    if(FStrEq(engine->Cmd_Argv(0), "makethruster") && isTestAdmin)
    {
        CBaseEntity *pCan = CreateEntityByName( "prop_physics_multiplayer" );
        if(pCan)
        {
            Vector vecOrigin = VFuncs::GetAbsOrigin(pPlayer) + Vector(64,0,64);
            QAngle vecAngles( 0, 0, 0 );
            VFuncs::KeyValue(pCan,  "angles", "0 0 0" );
            VFuncs::KeyValue(pCan,  "origin", UTIL_VarArgs("%0.3f %0.3f %0.3f", vecOrigin.x, vecOrigin.y, vecOrigin.z));
            VFuncs::KeyValue(pCan,  "model", "models/props_c17/canister_propane01a.mdl" );
            VFuncs::KeyValue(pCan,  "spawnflags", "0" );
            VFuncs::KeyValue(pCan,  "targetname", "SOPThruster001_Can001" );
            VFuncs::KeyValue(pCan,  "OnHealthChanged", "SOPThruster001_Thruster001,Activate,,0,-1");
            VFuncs::KeyValue(pCan,  "OnHealthChanged", "SOPThruster001_Steam001,TurnOn,,0,-1");
            VFuncs::KeyValue(pCan,  "OnHealthChanged", "SOPThruster001_Sound001,PlaySound,,0,-1");
            VFuncs::Spawn(pCan);
            VFuncs::Activate(pCan);
            VFuncs::Teleport(pCan,  &vecOrigin, &vecAngles, NULL );
        }
        CBaseEntity *pThrust = CreateEntityByName( "phys_thruster" );
        if(pThrust)
        {
            Vector vecOrigin = VFuncs::GetAbsOrigin(pPlayer) + Vector(64,0,143);
            QAngle vecAngles( 90, 0, 0 );
            VFuncs::KeyValue(pThrust,  "angles", "90 0 0" );
            VFuncs::KeyValue(pThrust,  "origin", UTIL_VarArgs("%0.3f %0.3f %0.3f", vecOrigin.x, vecOrigin.y, vecOrigin.z));
            VFuncs::KeyValue(pThrust,  "spawnflags", "14" );
            VFuncs::KeyValue(pThrust,  "targetname", "SOPThruster001_Thruster001" );
            VFuncs::KeyValue(pThrust,  "attach1", "SOPThruster001_Can001" );
            VFuncs::KeyValue(pThrust,  "force", args.ArgC() > 1 ? engine->Cmd_Argv(1) : "200000" );
            VFuncs::Spawn(pThrust);
            VFuncs::Activate(pThrust);
            //VFuncs::Teleport(pThrust,  &vecOrigin, &vecAngles, NULL );
        }
        CBaseEntity *pSteam = CreateEntityByName( "env_steam" );
        if(pSteam)
        {
            Vector vecOrigin = VFuncs::GetAbsOrigin(pPlayer) + Vector(64,0,138);
            QAngle vecAngles( -90, 0, 0 );
            VFuncs::KeyValue(pSteam,  "angles", "-90 0 0" );
            VFuncs::KeyValue(pSteam,  "origin", UTIL_VarArgs("%0.3f %0.3f %0.3f", vecOrigin.x, vecOrigin.y, vecOrigin.z));
            VFuncs::KeyValue(pSteam,  "spawnflags", "0" );
            VFuncs::KeyValue(pSteam,  "InitialState", "0" );
            VFuncs::KeyValue(pSteam,  "type", "1" );
            VFuncs::KeyValue(pSteam,  "SpreadSpeed", "30" );
            VFuncs::KeyValue(pSteam,  "Speed", "180" );
            VFuncs::KeyValue(pSteam,  "StartSize", "10" );
            VFuncs::KeyValue(pSteam,  "EndSize", "25" );
            VFuncs::KeyValue(pSteam,  "Rate", "52" );
            VFuncs::KeyValue(pSteam,  "JetLength", "160" );
            VFuncs::KeyValue(pSteam,  "rendercolor", "255 255 255" );
            VFuncs::KeyValue(pSteam,  "renderamt", "255" );
            VFuncs::KeyValue(pSteam,  "targetname", "SOPThruster001_Steam001" );
            VFuncs::KeyValue(pSteam,  "parentname", "SOPThruster001_Can001" );
            VFuncs::Spawn(pSteam);
            VFuncs::Activate(pSteam);
            VFuncs::SetParent(pSteam, pCan);
            //VFuncs::Teleport(pSteam,  &vecOrigin, &vecAngles, NULL );
        }
        CBaseEntity *pSound = CreateEntityByName( "ambient_generic" );
        if(pSound)
        {
            Vector vecOrigin = VFuncs::GetAbsOrigin(pPlayer) + Vector(64,0,137);
            QAngle vecAngles( 0, 0, 0 );
            VFuncs::KeyValue(pSound,  "origin", UTIL_VarArgs("%0.3f %0.3f %0.3f", vecOrigin.x, vecOrigin.y, vecOrigin.z));
            VFuncs::KeyValue(pSound,  "health", "10" );
            VFuncs::KeyValue(pSound,  "pitch", "100" );
            VFuncs::KeyValue(pSound,  "pitchstart", "100" );
            VFuncs::KeyValue(pSound,  "radius", "750" );
            VFuncs::KeyValue(pSound,  "spawnflags", "16" );
            VFuncs::KeyValue(pSound,  "message", "ambient/gas/cannister_loop.wav" );
            VFuncs::KeyValue(//pSound,  "message", "addons/sourceop/testmusic/test_loop2.mp3" );
            VFuncs::KeyValue(pSound,  "targetname", "SOPThruster001_Sound001" );
            VFuncs::KeyValue(pSound,  "parentname", "SOPThruster001_Can001" );
            VFuncs::Spawn(pSound);
            VFuncs::Activate(pSound);
            VFuncs::SetParent(pSound, pCan);
            //VFuncs::Teleport(pSound,  &vecOrigin, &vecAngles, NULL );
        }
        //VFuncs::SetParent(pTemp, pCan);
    }
    if ( FStrEq( pcmd, "surfaceset" ) && isTestAdmin )
    {
        Vector forward, right, up;
        QAngle playerAngles = VFuncs::EyeAngles(pPlayer);

        AngleVectors( playerAngles, &forward, &right, &up );

        float distance = 8192;
        Vector start = VFuncs::EyePosition(pPlayer);
        Vector end = VFuncs::EyePosition(pPlayer) + ( forward * distance );

        trace_t tr;
        CTraceFilterSimple traceFilter( pPlayer, COLLISION_GROUP_NONE );
        Ray_t ray;
        ray.Init( start, end );
        enginetrace->TraceRay( ray, MASK_NPCSOLID, &traceFilter, &tr );

        if(tr.DidHit() && tr.m_pEnt)
        {
            if(VFuncs::entindex(tr.m_pEnt) != 0)
            {
                int matindex = VFuncs::VPhysicsGetObject(tr.m_pEnt)->GetMaterialIndex();
                VFuncs::VPhysicsGetObject(tr.m_pEnt)->SetMaterialIndex( physprops->GetSurfaceIndex( engine->Cmd_Argv(1) ) );
                Msg("SetMaterialIndex(%s) MatIndex was %i now %i\n", engine->Cmd_Argv(1), matindex, VFuncs::VPhysicsGetObject(tr.m_pEnt)->GetMaterialIndex());
            }
        }
    }
#ifdef PLUGIN_PHYSICS
    if ( FStrEq( pcmd, "setgravity" ) && isTestAdmin )
    {
        if(physenv)
        {
            engine->ClientPrintf(pEntity, "physenv exists\n");
            physenv->SetGravity(Vector(atof(engine->Cmd_Argv(1)), atof(engine->Cmd_Argv(2)), atof(engine->Cmd_Argv(3))));
        }
        else
        {
            Msg( "physenv was null...\n" );
        }
        return PLUGIN_STOP; // we handled this function
    }
    else if ( FStrEq( pcmd, "setairdensity" ) )
    {
        char msg[128];

        Q_snprintf( msg, sizeof(msg), "Previous AirDensity: %f\n", physenv->GetAirDensity() ); 
        engine->ClientPrintf(pEntity, msg);
        physenv->SetAirDensity(atof(engine->Cmd_Argv(1)));

        return PLUGIN_STOP; // we handled this function
    }
#endif*/
    /*if(FStrEq(engine->Cmd_Argv(0), "umsg") && isTestAdmin)
    {
        for(int i=0; i<USR_MSGS_MAX; i++)
        {
            Msg("%i %s\n", i, userMessages[i]);
        }
        bf_write *pWrite;
        CReliableBroadcastRecipientFilter filter;
        filter.AddAllPlayers();
    
        pWrite = engine->UserMessageBegin(&filter, 12);
            pWrite->WriteString( "info" ); // menu name
            pWrite->WriteByte( 1 ); //show
            pWrite->WriteByte( 3 ); //count
            
            // write additional data (be carefull not more than 192 bytes!)
            ///while ( subkey )
            //{
                pWrite->WriteString( "title" );
                pWrite->WriteString( "Help" );
                pWrite->WriteString( "type" );
                pWrite->WriteString( engine->Cmd_Argv(1) );
                pWrite->WriteString( engine->Cmd_Argv(2) );
                pWrite->WriteString( engine->Cmd_Argv(3) );
                //subkey = subkey->GetNextKey();
            //}
        engine->MessageEnd();
    }
    if(FStrEq(engine->Cmd_Argv(0), "lightstyle") && isTestAdmin)
    {
        engine->LightStyle(atoi(engine->Cmd_Argv(1)), engine->Cmd_Argv(2));
    }*/
    /*if(FStrEq(engine->Cmd_Argv(0), "downloadtest") && isTestAdmin)
    {
        Msg("PUF returned %i\n", ParseUserFile());
    }*/
#endif
    if(featureStatus[FEAT_CREDITS])
    {
        if(FStrEq(pcmd, "credgive"))
        {
            if(args.ArgC() >= 3)
            {
                int pindex = ENTINDEX(pEntity);
                char tmp[128];
                char pszPlayer[128];
                int playerList[128];
                int playerCount = 0;

                Q_snprintf( pszPlayer, sizeof(pszPlayer), "%s", args[1] );
                if(pszPlayer[0] == '\"' && pszPlayer[strlen(pszPlayer)-1] == '\"') // remove quotes
                {
                    pszPlayer[strlen(pszPlayer)-1] = '\0';
                    strcpy(pszPlayer, &pszPlayer[1]);
                }

                playerCount = FindPlayer(playerList, pszPlayer);
                if(playerCount)
                {
                    if(playerCount > 1) // show a menu
                    {
                        KeyValues *kv = new KeyValues( "menu" );
                        kv->SetString( "title", "Matched more than one player, hit ESC" );
                        kv->SetInt( "level", 1 );
                        kv->SetColor( "color", Color( 255, 0, 0, 128 ));
                        kv->SetInt( "time", 20 );
                        kv->SetString( "msg", "Pick a player" );

                        for( int i = 0; i < playerCount; i++ )
                        {
                            if(playerList[i] != 0)
                            {
                                IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pEList+playerList[i]);
                                if(info)
                                {
                                    char num[10], msg[128], cmd[256];
                                    Q_snprintf( num, sizeof(num), "%i", i );
                                    Q_snprintf( msg, sizeof(msg), "%s", info->GetName() );
                                    Q_snprintf( cmd, sizeof(cmd), "credgive \"%s\" %s", engine->GetPlayerNetworkIDString(pEList+playerList[i]), args[2] );

                                    KeyValues *item1 = kv->FindKey( num, true );
                                    item1->SetString( "msg", msg );
                                    item1->SetString( "command", cmd );
                                }
                            }
                        }

                        helpers->CreateMessage( pEntity, DIALOG_MENU, kv, &g_ServerPlugin );
                        kv->deleteThis();
                    }
                    else
                    {
                        if(playerList[0] != 0)
                        {
                            int amt = atoi(args[2]);
                            edict_t *pPlayer = pEList+playerList[0];
                            IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pEntity);
                            IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo(pPlayer);

                            if(info && playerinfo)
                            {
                                int donatorIsAdmin = pAOPPlayers[pindex-1].IsAdmin(2048, "credgive");
                                if(pPlayer != pEntity || donatorIsAdmin)
                                {
                                    if(pAOPPlayers[pindex-1].GetCredits() >= amt || donatorIsAdmin)
                                    {
                                        if(atoi(args[2]) > 0 || donatorIsAdmin)
                                        {
                                            if(!donatorIsAdmin)
                                            {
                                                pAOPPlayers[pindex-1].AddCredits(0-amt);
                                            }
                                            pAOPPlayers[playerList[0]-1].AddCredits(amt);
                                            Q_snprintf(tmp, sizeof(tmp), "[%s] You have just donated %i credits to %s.\n", adminname, amt, playerinfo->GetName());
                                            pAOPPlayers[pindex-1].SayText(tmp, HUD_PRINTCONSOLE);
                                            Q_snprintf(tmp, sizeof(tmp), "[%s] %s has donated %i credits to you.\n", adminname, info->GetName(), amt);
                                            pAOPPlayers[playerList[0]-1].SayTextChatHud(tmp);
                                            //TODO : LOG
                                        }
                                        else
                                        {
                                            Q_snprintf(tmp, sizeof(tmp), "[%s] You have to give something.\n", adminname);
                                            pAOPPlayers[pindex-1].SayText(tmp, HUD_PRINTCONSOLE);
                                        }
                                    }
                                    else
                                    {
                                        Q_snprintf(tmp, sizeof(tmp), "[%s] You don't have enough credits.\n", adminname);
                                        pAOPPlayers[pindex-1].SayText(tmp, HUD_PRINTCONSOLE);
                                    }
                                }
                                else
                                {
                                    Q_snprintf(tmp, sizeof(tmp), "[%s] You can't donate to yourself.\n", adminname);
                                    pAOPPlayers[pindex-1].SayText(tmp, HUD_PRINTCONSOLE);
                                }
                            }
                            else
                            {
                                Q_snprintf(tmp, sizeof(tmp), "[%s] An error occured.\n", adminname);
                                pAOPPlayers[pindex-1].SayText(tmp, HUD_PRINTCONSOLE);
                            }
                        }
                        else
                        {
                            Q_snprintf(tmp, sizeof(tmp), "[%s] Cannot find that player.\n", adminname);
                            pAOPPlayers[pindex-1].SayText(tmp, HUD_PRINTCONSOLE);
                        }
                    }
                }
                else
                {
                    Q_snprintf(tmp, sizeof(tmp), "[%s] Cannot find that player.\n", adminname);
                    pAOPPlayers[pindex-1].SayText(tmp, HUD_PRINTCONSOLE);
                }
            }
            return PLUGIN_STOP;
        }

    }
    if(featureStatus[FEAT_ADMINCOMMANDS])
    {
        adminCommands = DFAdminCommands(args, pEntity);
        if(adminCommands != PLUGIN_CONTINUE)
            return adminCommands;
    }

    playerCommands = pAOPPlayer->ClientCommand(args);
    if(playerCommands != PLUGIN_CONTINUE)
        return playerCommands;

    if(featureStatus[FEAT_LUA])
    {
        lua_getglobal(GetLuaState(), "hook");
        lua_pushliteral(GetLuaState(), "Call");
        lua_gettable(GetLuaState(), -2);
        lua_remove(GetLuaState(), -2);
        lua_pushstring(GetLuaState(), "PlayerCommand");

        // push the player
        g_entityCache.PushPlayer(playerindex);

        // push the command
        lua_pushstring(GetLuaState(), args[0]);

        // push array of arguments
        lua_newtable(GetLuaState());
        int idx = lua_gettop(GetLuaState());
        // add full args
        lua_pushstring(GetLuaState(), "args");
        lua_pushstring(GetLuaState(), args.ArgS());
        lua_settable(GetLuaState(), idx);
        // add command
        lua_pushinteger(GetLuaState(), 0);
        lua_pushstring(GetLuaState(), args[0]);
        lua_settable(GetLuaState(), idx);
        // add all arguments
        for(int i = 1; i < args.ArgC(); i++)
        {
            lua_pushinteger(GetLuaState(), i);
            lua_pushstring(GetLuaState(), args[i]);
            lua_settable(GetLuaState(), idx);
        }

        if(lua_pcall(GetLuaState(), 4, 1, 0) == 0)
        {
            /* get the result */
            int r = luaL_optbool(GetLuaState(), -1, false);
            lua_pop(GetLuaState(), 1);

            if(r) return PLUGIN_STOP;
        }
        else
        {
            CAdminOP::ColorMsg(CONCOLOR_LUA_ERR, "[SOURCEOP] Lua: error dispatching hook `PlayerCommand':\n %s\n", lua_tostring(GetLuaState(), -1));
        }
    }

    return PLUGIN_CONTINUE;
}

PLUGIN_RESULT CAdminOP :: NetworkIDValidated( const char *pszUserName, const char *pszNetworkID )
{
    int playerList[128];
    int playerCount = 0;

    //////
    // TEST
    //////
    /*char data[256];
    bf_write buffer(data, sizeof(data));
    buffer.WriteUBitLong(5, 6);
    buffer.WriteByte(1);
    buffer.WriteString("Testcvar");
    buffer.WriteString("Testvalue");

    for (int i = 1; i <= gpGlobals->maxClients; i++)
    {
        CAdminOPPlayer *pPlayer = &pAdminOP.pAOPPlayers[i-1];

        if (pPlayer->GetPlayerState() >= 1 && pPlayer->NotBot())
        {
            printf("Sending test cvar to %i\n", i);
            INetChannel *netchan = static_cast<INetChannel *>(engine->GetPlayerNetInfo(i));
            netchan->SendData(buffer);
        }
    }*/

    playerCount = pAdminOP.FindPlayerWithName(playerList, pszUserName);
    if(playerCount && playerList[0] != 0)
    {
        SOPDLog(UTIL_VarArgs("NetworkIDValidated(%i, %s, %s)\n", playerList[0]-1, pszUserName, pszNetworkID));

        pAdminOP.pAOPPlayers[playerList[0]-1].NetworkIDValidated(pszUserName, pszNetworkID);
        // TODO: seniorproj: Let every remote client know player data has changed.
    }
    SOPDLog("NetworkIDValidated finished\n");

    return PLUGIN_CONTINUE;
}

void CAdminOP :: OnQueryCvarValueFinished( QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue )
{
    if(featureStatus[FEAT_LUA] && pPlayerEntity)
    {
        int i = ENTINDEX(pPlayerEntity) - 1;
        for(int j = 0; j < pAOPPlayers[i].m_pendingCvarQueries.Count(); j++)
        {
            cvarquery_t *query = pAOPPlayers[i].m_pendingCvarQueries.Base() + j;
            if(query->cookie == iCookie)
            {
                lua_getref(GetLuaState(), query->luacallback);
                lua_pushinteger(GetLuaState(), eStatus);
                lua_pushstring(GetLuaState(), pCvarName);
                lua_pushstring(GetLuaState(), pCvarValue);
                if(lua_pcall(GetLuaState(), 3, 0, 0) != 0)
                {
                    CAdminOP::ColorMsg(CONCOLOR_LUA_ERR, "[SOURCEOP] Lua: error dispatching client convar query:\n %s\n", lua_tostring(GetLuaState(), -1));
                }

                lua_unref(GetLuaState(), query->luacallback);

                pAOPPlayers[i].m_pendingCvarQueries.Remove(j);
                break;
            }
        }
    }
}

void CAdminOP :: ClientDisconnect(edict_t *pEntity)
{
    int userid = ENTINDEX(pEntity);

    SOPDLog(UTIL_VarArgs("ClientDisconnect(%i, %s, %s)\n", userid-1, pAOPPlayers[userid-1].GetJoinName(), pAOPPlayers[userid-1].GetSteamID().Render()));

    if(featureStatus[FEAT_LUA])
    {
        lua_getglobal(GetLuaState(), "hook");
        lua_pushliteral(GetLuaState(), "Call");
        lua_gettable(GetLuaState(), -2);
        lua_remove(GetLuaState(), -2);
        lua_pushstring(GetLuaState(), "PlayerDisconnected");
        g_entityCache.PushPlayer(userid);
        lua_pcall(GetLuaState(), 2, 0, 0);
    }

    if(userid > 0 && userid <= maxClients)
    {
        pAOPPlayers[userid-1].Disconnect();
    }

    g_entityCache.PlayerDisconnected(userid);

    SOPDLog("ClientDisconnect finish.\n");
}

void CAdminOP :: RemovePlayerFromVotes(int uid)
{
    if(uid)
    {
        mapVote.RemoveChoice(uid);
        cvarVote.RemoveChoice(uid);
    }
}

void CAdminOP :: EndMapVote(void)
{
    char msg[120];
    int maxvotes = (GetPlayerCountPlaying() * vote_ratio.GetInt()) / 100;
    VoteCount_t topchoice = mapVote.GetTopChoice();
    if(topchoice.votes != 0 && topchoice.votes >= maxvotes)
    {
        if(!Q_strcasecmp(topchoice.item, "extend"))
        {
            if(GetTimeLimit() > 0)
            {
                int newTimeLimit = GetTimeLimit() + 30;
                timelimit->SetValue(newTimeLimit);
            }
            Q_snprintf(msg, 120, "[%s] Voting is now over. The map will be extended for %i more minutes.\n", adminname, vote_extendtime.GetInt());
            engine->LogPrint(UTIL_VarArgs("[SOURCEOP] \"Console<0><Console><Console>\" map vote winner is \"extend\"\n"));
            msg[119] = '\0';
            voteExtendCount++;
        }
        else
        {
            Q_snprintf(msg, 120, "[%s] Voting is now over. %s won with %i votes.\n", adminname, topchoice.item, topchoice.votes);
            engine->LogPrint(UTIL_VarArgs("[SOURCEOP] \"Console<0><Console><Console>\" map vote winner is \"%s\"\n", topchoice.item));
            msg[119] = '\0';
            changeMapTime = engine->Time() + 4;

            //show scores on all players
            for(int i = 1; i <= pAdminOP.GetMaxClients(); i++)
            {
                IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+i);

                if(info)
                {
                    if(info->IsConnected())
                    {
                        CBaseEntity *pPlayer = CBaseEntity::Instance(pAdminOP.GetEntityList()+i);
                        if(pPlayer)
                        {
                            pAOPPlayers[i-1].ShowViewPortPanel(PANEL_SCOREBOARD);
                            VFuncs::AddFlag(pPlayer, FL_FROZEN);
                        }
                    }
                }
            }

            strcpy(szChangeMap, topchoice.item);
        }
    }
    else
    {
        Q_snprintf(msg, 120, "[%s] Voting is now over. No map got enough votes to win.\n", adminname);
        engine->LogPrint(UTIL_VarArgs("[SOURCEOP] \"Console<0><Console><Console>\" map vote had no winner\n"));
        msg[119] = '\0';
    }

    SayTextAllChatHud(msg);
    mapVote.EndVote();
}

void CAdminOP :: CvarChanged(const char *cvarname, const char *cvarvalue)
{
    char datestring[32];
    char newlogstring[1024];
    tm curtime;

    g_pVCR->Hook_LocalTime( &curtime );

    Q_snprintf( datestring, sizeof( datestring ), "L %02i/%02i/%04i - %02i:%02i:%02i: ",
        curtime.tm_mon+1, curtime.tm_mday, 1900 + curtime.tm_year,
        curtime.tm_hour, curtime.tm_min, curtime.tm_sec);

    V_snprintf(newlogstring, sizeof(newlogstring), "%sserver_cvar: \"%s\" \"%s\"\n", datestring, cvarname, cvarvalue);
    if(remoteserver) remoteserver->LogPrint(newlogstring);
}

const char *CAdminOP :: DataDir(void)
{
    const char *datadir = cvar->GetCommandLineValue("DF_datadir");

    return datadir ? datadir : "addons/SourceOP";
}

CSteamID CAdminOP :: GetServerSteamID()
{
    if(isClient)
    {
            if(m_pSteamUser)
            {
                return m_pSteamUser->GetSteamID();
            }
    }
    else
    {
            if(m_pSteamGameServer)
            {
                return m_pSteamGameServer->GetSteamID();
            }
    }

    return CSteamID();
}

// The gamerules's nextmap can change midgame for example if the nextlevel cvar is changed 
void CAdminOP :: UpdateNextMap(void)
{
    if(nextlevel && nextlevel->GetString() && *nextlevel->GetString() && DFIsMapValid( nextlevel->GetString()))
    {
        V_strncpy(nextmap, nextlevel->GetString(), sizeof(nextmap));
    }
    else
    {
        if(gameRules && gameRules->IsInitialized())
            gameRules->GetNextLevelName(nextmap, sizeof(nextmap));
    }
}

CBaseEntity *CAdminOP :: GetEntity(int index)
{
    // TODO: Replace this and BaseEntityToEdict with servertools->GetBaseEntityByEntIndex
    edict_t *pEdict = INDEXENT(index);
    if(pEdict) return servergameents->EdictToBaseEntity(pEdict);
    return NULL;
}

CBaseEntity *CAdminOP :: FindPlayerResource()
{
    bPlayerResourceCached = true;
    if(pAdminOP.isTF2)
    {
        pPlayerResource = gEntList.FindEntityByClassname(NULL, "tf_player_manager");
    }
    else
    {
        pPlayerResource = NULL;
    }

    return pPlayerResource;
}

int CAdminOP :: GetPlayerCount(void)
{
    int playercount = 0;
    for(int i = 1; i <= maxClients; i++)
    {
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pEList+i);
        if(info)
        {
            playercount++;
        }
    }

    return playercount;
}

int CAdminOP :: GetPlayerCountPlaying(void)
{
    int playercount = 0;
    for(int i = 1; i <= maxClients; i++)
    {
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pEList+i);
        if(info)
        {
            if(info->IsConnected())
            {
                playercount++;
            }
        }
    }

    return playercount;
}

int CAdminOP :: GetConnectedPlayerCount(void)
{
    int players = 0;
    for(int k = 0; k < min(MAX_AOP_PLAYERS, pAdminOP.GetMaxClients()); k++)
    {
        void *baseclient = pAdminOP.pAOPPlayers[k].baseclient;
        if(baseclient)
        {
            players++;
        }
    }

    return players;
}

int CAdminOP :: GetVisibleMaxPlayers(int *pMaxPlayers)
{
    int maxplayers = pAdminOP.GetMaxClients();
    int adjustedmaxplayers = maxplayers;

    // TODO: Include sv_visiblemaxplayers
#ifdef OFFICIALSERV_ONLY
    // adjust maxplayer count as necessary
    if(serverquery_visiblemaxplayers.GetInt() > -1)
    {
        adjustedmaxplayers = (char)serverquery_visiblemaxplayers.GetInt();
    }
    else if(serverquery_addplayers.GetInt() != 0 && ((int)maxplayers) + serverquery_addplayers.GetInt() <= 255 && ((int)maxplayers) + serverquery_addplayers.GetInt() >= 0)
    {
        adjustedmaxplayers += serverquery_addplayers.GetInt();
    }
#endif

    if(pMaxPlayers)
    {
        *pMaxPlayers = maxplayers;
    }

    return adjustedmaxplayers;
}

edict_t *CAdminOP :: GetPlayer(int index)
{
    edict_t *pPlayer = NULL;
    IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo(pEList+index);

    if(playerinfo)
    {
        if(playerinfo->IsConnected())
        {
            pPlayer = pEList+index;
        }
    }

    return pPlayer;
}

bool CAdminOP :: RoundHasBegun()
{
    return inRound;
}

void CAdminOP :: OverrideMapTimeRemaining(int timeleft)
{
    overrideMapTime = timeleft;
    overrideMapTimeSetTime = effects->Time();
    overrideMapTimeLimit = GetTimeLimit();
}

int CAdminOP :: GetMapTimeRemaining()
{
    int iTimeLimit = GetTimeLimit() * 60;

    if(iTimeLimit == 0)
        return 0;

    if(overrideMapTime)
    {
        //     the override time limit - the time since it was set          + any change in timelimit since the override was set
        return overrideMapTime - (effects->Time() - overrideMapTimeSetTime) + ((GetTimeLimit() - overrideMapTimeLimit)*60);
    }
    else
    {
        return iTimeLimit - effects->Time();
    }
}

int CAdminOP :: GetTimeLimit()
{
    // if the cvar doesn't exist, we can't continue
    if(!timelimit)
        return 0;

    return timelimit->GetInt();
}

CVoteSystem *CAdminOP :: GetMapVote(void)
{
    return &mapVote;
}

const char *CAdminOP :: TeamName(int team)
{
    if(team < MIN_TEAM_NUM || team > MAX_TEAM_NUM)
    {
        return "";
    }
    return sTeamNames[team];
}

void CAdminOP :: OnGoToIntermission()
{
    if(featureStatus[FEAT_LUA])
    {
        lua_getglobal(GetLuaState(), "hook");
        lua_pushliteral(GetLuaState(), "Call");
        lua_gettable(GetLuaState(), -2);
        lua_remove(GetLuaState(), -2);
        lua_pushstring(GetLuaState(), "IntermissionStarting");
        lua_pcall(GetLuaState(), 1, 0, 0);
    }
}

void CAdminOP :: OnHandleSwitchTeams()
{
    if(featureStatus[FEAT_LUA])
    {
        lua_getglobal(GetLuaState(), "hook");
        lua_pushliteral(GetLuaState(), "Call");
        lua_gettable(GetLuaState(), -2);
        lua_remove(GetLuaState(), -2);
        lua_pushstring(GetLuaState(), "PreSwitchTeams");
        lua_pcall(GetLuaState(), 1, 0, 0);
    }
}

void CAdminOP :: OnHandleScrambleTeams()
{
    if(featureStatus[FEAT_LUA])
    {
        lua_getglobal(GetLuaState(), "hook");
        lua_pushliteral(GetLuaState(), "Call");
        lua_gettable(GetLuaState(), -2);
        lua_remove(GetLuaState(), -2);
        lua_pushstring(GetLuaState(), "PreScrambleTeams");
        lua_pcall(GetLuaState(), 1, 0, 0);
    }
}

bool CAdminOP :: AddVacAllowedPlayer(CSteamID steamid)
{
    bool isOnList = false;
    bool ret = false;

    for(int i = 0; i < vacAllowList.Count(); i++)
    {
        if(steamid == vacAllowList[i])
        {
            isOnList = true;
        }
    }

    if(!isOnList)
    {
        vacAllowList.AddToTail(steamid);
        ret = true;
    }

    return ret;
}

bool CAdminOP :: RemoveVacAllowedPlayer(CSteamID steamid)
{
    bool ret = false;

    for(int i = 0; i < vacAllowList.Count(); i++)
    {
        if(steamid == vacAllowList[i])
        {
            vacAllowList.Remove(i);
            i--;
            ret = true;
        }
    }

    return ret;
}

void CAdminOP :: PrintVacAllowedPlayerList()
{
    for(int i = 0; i < vacAllowList.Count(); i++)
    {
        Msg("%-5i %s\n", i, vacAllowList[i].Render());
    }
}

bool CAdminOP :: VacAllowPlayer(CSteamID steamid)
{
    for(int i = 0; i < vacAllowList.Count(); i++)
    {
        if(steamid == vacAllowList[i])
        {
            return true;
        }
    }

    return false;
}

ConCommandBase *CAdminOP :: GetCommands()
{
#ifdef _L4D_PLUGIN
    return cvar->FindCommandBase("anim_3wayblend");
#else
    return cvar->GetCommands();
#endif
}

int CAdminOP :: IndexOfEdict(const edict_t *pEdict)
{
    if(!pEList)
    {
        // do something to get pEList
        if(!pEList)
        {
            pAdminOP.ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Unable to determine ent index for %p.\n", pEdict);
            return 0;
        }
    }
    int ret = pEdict - pEList;
    // This was used to check to make sure the values matched
    //edict_t *pTemp = pEList + ret;
    return ret;
}

int CAdminOP :: PlayerSpeakPre(int entID, const char *text)
{
    bool retval = 0;
    IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo(pEList+entID);

    if(!playerinfo)
        return 0;
    if(!playerinfo->IsConnected())
        return 0;

    if(pAOPPlayers[entID-1].PlayerSpeakPre(text)) retval = 1;

    if(featureStatus[FEAT_LUA] && !retval)
    {
        lua_getglobal(GetLuaState(), "hook");
        lua_pushliteral(GetLuaState(), "Call");
        lua_gettable(GetLuaState(), -2);
        lua_remove(GetLuaState(), -2);
        lua_pushstring(GetLuaState(), "PlayerSay");

        // push the player
        g_entityCache.PushPlayer(entID);

        // push the text
        lua_pushstring(GetLuaState(), text);

        // push true because this isn't team chat (public = true)
        lua_pushboolean(GetLuaState(), 1);

        if(lua_pcall(GetLuaState(), 4, 1, 0) == 0)
        {
            /* get the result */
            if(lua_isboolean(GetLuaState(), -1))
            {
                // return value of false means block chat which means set retval to 1
                retval = luaL_optbool(GetLuaState(), -1, true) == 0;
                lua_pop(GetLuaState(), 1);
            }
            else if(lua_isstring(GetLuaState(), -1))
            {
                // TODO: Replace chat text when a string is returned
                CAdminOP::ColorMsg(CONCOLOR_LUA_ERR, "[SOURCEOP] Lua: TODO: Replace chat text when a string is returned.\n");
            }
            else if(!lua_isnil(GetLuaState(), -1))
            {
                CAdminOP::ColorMsg(CONCOLOR_LUA_ERR, "[SOURCEOP] Lua: invalid return type '%s' for hook `PlayerSay'\n[SOURCEOP] Expected boolean, string, or nil.\n", lua_typename(GetLuaState(), lua_type(GetLuaState(), -1)));
            }
        }
        else
        {
            CAdminOP::ColorMsg(CONCOLOR_LUA_ERR, "[SOURCEOP] Lua: error dispatching hook `PlayerSay':\n %s\n", lua_tostring(GetLuaState(), -1));
        }
    }

    // Send this chat message to all remote clients
    // Clients know player's current team, name, alive status
    // Hidden chat messages shown differently
    if(remoteserver) remoteserver->PlayerChat(entID, false, retval != 0, text);

    return retval; //0 to allow speech, 1 to stop
}

int CAdminOP :: PlayerSpeakTeamPre(int entID, const char *text)
{
    bool retval = 0;
    IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo(pEList+entID);

    if(!playerinfo)
        return 0;
    if(!playerinfo->IsConnected())
        return 0;

    if(pAOPPlayers[entID-1].PlayerSpeakTeamPre(text)) retval = 1;

    // Send this team chat message to all remote clients
    if(remoteserver) remoteserver->PlayerChat(entID, true, retval != 0, text);

    return retval; //0 to allow speech, 1 to stop
}

void CAdminOP :: PlayerSpeak(int iPlayer, int userid, const char *text)
{
    CBaseEntity *pPlayer = CBaseEntity::Instance(pEList+iPlayer);
    IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo(pEList+iPlayer);


    if(ignore_chat.GetBool())
        return;

    if(!playerinfo)
        return;
    if(!playerinfo->IsConnected())
        return;

    CAdminOPPlayer *pAOPPlayer = &pAOPPlayers[iPlayer-1];
    if(featureStatus[FEAT_ADMINSAYCOMMANDS])
    {
        if(!Q_strncasecmp("admin ", text, 6))
        {
            if(!Q_strncasecmp("admin change name to ", text, 21))
            {
                if(Q_strcasecmp("admin change name to ", text))
                {
                    if (pAOPPlayer->IsAdmin(64, "sourceopname"))
                    {
                        unsigned int i;
                        char strName[256];
                        /* we need to strip out 'admin change name to ' (21 characters */
                        for(i=21;i<strlen(text)+1;i++)
                            strName[i-21] = text[i];
                        strName[i-21] = NULL;

                        strncpy(adminname, strName, 32);
                        adminname[31] = '\0';
                        sprintf(msg, "[%s] Ok.", adminname);
                        SayTextAllChatHud(msg);
                    }
                }
            }
            else if((!Q_strcasecmp("admin god", text)) || (!Q_strcasecmp("admin make me god", text)) || (!Q_strcasecmp("admin give me god", text)) || (!Q_strcasecmp("admin god on", text)))
            {
                if(pAOPPlayer->IsAdmin(32, "godself"))
                {
                    pAOPPlayer->SetGod(1);
                    sprintf(msg, "[%s] OK admin, you have godmode.\n", adminname);
                    SayTextAllChatHud(msg);
                }
                else
                {
                    sprintf(msg, "[%s] Only admins may have godmode.\n", adminname);
                    SayTextAllChatHud(msg);
                }
            }
            else if((!Q_strcasecmp("admin god off", text)) || (!Q_strcasecmp("admin ungod", text)) || (!Q_strcasecmp("admin stop god", text)))
            {
                if(pAOPPlayer->IsAdmin(32, "godself"))
                {
                    pAOPPlayer->SetGod(0);
                    sprintf(msg, "[%s] OK admin, I turned your god off.\n", adminname);
                    SayTextAllChatHud(msg);
                }
                else
                {
                    sprintf(msg, "[%s] Only admins may turn god off.\n", adminname);
                    SayTextAllChatHud(msg);
                }
            }
            else if((!Q_strcasecmp("admin god toggle", text)) || (!Q_strcasecmp("admin toggle god", text)))
            {
                if(pAOPPlayer->IsAdmin(32, "godself"))
                {
                    pAOPPlayer->SetGod(2);
                    if (!pAOPPlayer->HasGod() )
                        sprintf(msg, "[%s] OK admin, I turned your god off.\n", adminname);
                    else
                        sprintf(msg, "[%s] OK admin, you have godmode.\n", adminname);

                    SayTextAllChatHud(msg);
                }
                else
                {
                    sprintf(msg, "[%s] Only admins may toggle god.\n", adminname);
                    SayTextAllChatHud(msg);
                }
            }
            else if((!Q_strcasecmp("admin noclip", text)) || (!Q_strcasecmp("admin make me noclip", text)) || (!Q_strcasecmp("admin give me noclip", text)) || (!Q_strcasecmp("admin noclip on", text)))
            {
                bool noclipOn = 0;
                if (pAOPPlayer->IsAdmin(32, "noclipself"))
                {
                    sprintf(msg, "[%s] OK admin, you have noclip.\n", adminname);
                    SayTextAllChatHud(msg);
                    pAOPPlayer->SetNoclip(1);
                }
                else
                {
                    if (allow_noclipping.GetInt())
                    {
                        sprintf(msg, "[%s] OK %s, you have noclip.\n", adminname, playerinfo->GetName());
                        SayTextAllChatHud(msg);
                        pAOPPlayer->SetNoclip(1);
                    }
                    else
                    {
                        sprintf(msg, "[%s] Sorry %s, noclip is not allowed.\n", adminname, playerinfo->GetName());
                        SayTextAllChatHud(msg);
                    }
                }
            }
            else if((!Q_strcasecmp("admin noclip off", text)) || (!Q_strcasecmp("admin turn noclip off", text)) || (!Q_strcasecmp("admin turn my noclip off", text)) || (!Q_strcasecmp("admin stop noclip", text)))
            {
                if (pAOPPlayer->IsAdmin(32, "noclipself"))
                {
                    sprintf(msg, "[%s] OK admin, I turned your noclip off.\n", adminname);
                    SayTextAllChatHud(msg);
                    pAOPPlayer->SetNoclip(0);
                }
                else
                {
                    if (allow_noclipping.GetInt())
                    {
                        sprintf(msg, "[%s] OK %s, I turned your noclip off.\n", adminname, playerinfo->GetName());
                        SayTextAllChatHud(msg);
                        pAOPPlayer->SetNoclip(0);
                    }
                    else
                    {
                        sprintf(msg, "[%s] Sorry %s, turning people's noclip off is not allowed.\n", adminname, playerinfo->GetName());
                        SayTextAllChatHud(msg);
                    }
                }
            }
            else if((!Q_strcasecmp("admin noclip toggle", text)) || (!Q_strcasecmp("admin toggle noclip", text)))
            {
                if (pAOPPlayer->IsAdmin(32, "noclipself"))
                {
                    if (VFuncs::GetMoveType(pPlayer) == MOVETYPE_NOCLIP)
                        sprintf(msg, "[%s] OK admin, I turned your noclip off.\n", adminname);
                    else
                        sprintf(msg, "[%s] OK admin, you have noclip.\n", adminname);

                    SayTextAllChatHud(msg);
                    pAOPPlayer->SetNoclip(2);
                }
                else
                {
                    if (allow_noclipping.GetInt())
                    {
                        if (VFuncs::GetMoveType(pPlayer) == MOVETYPE_NOCLIP)
                            sprintf(msg, "[%s] OK %s, I turned your noclip off.\n", adminname, playerinfo->GetName());
                        else
                            sprintf(msg, "[%s] OK %s, you have noclip.\n", adminname, playerinfo->GetName());

                        SayTextAllChatHud(msg);
                        pAOPPlayer->SetNoclip(2);
                    }
                    else
                    {
                        sprintf(msg, "[%s] Sorry %s, turning toggling noclip is not allowed.\n", adminname, playerinfo->GetName());
                        SayTextAllChatHud(msg);
                    }
                }
            }
        }
    }
    if(featureStatus[FEAT_PLAYERSAYCOMMANDS])
    {
        if (!Q_strcasecmp("timeleft", text))
        {
            char *colorcode = "";
            char *removecolorcode = "";
            if(isTF2)
            {
                colorcode = "\x04";
                removecolorcode = "\x01";
            }

            if(GetTimeLimit() > 0)
            {
                int TTimeleft = GetMapTimeRemaining();
                int Hours = TTimeleft;
                int Seconds = TTimeleft;
                int Minutes = TTimeleft;
                /*char SayTime[120];
                char CHours[80];
                char CMinutes[80];
                char CSeconds[80];*/

                Hours /= 3600;
                Minutes /= 60;
                Minutes -= (Hours*60);
                Seconds = TTimeleft % 60;

                /*strcpy(CHours, NumbersToWords(Hours));
                strcpy(CMinutes, NumbersToWords(Minutes));
                strcpy(CSeconds, NumbersToWords(Seconds));*/

                if(Hours == 0)
                    SayTextAllChatHud(UTIL_VarArgs("[%s] Time remaining on map: %s%i minute%s %i second%s\n", adminname, colorcode, Minutes, Minutes != 1 ? "s" : "", Seconds, Seconds != 1 ? "s" : ""));
                else
                    SayTextAllChatHud(UTIL_VarArgs("[%s] Time remaining on map: %s%i hour%s %i minute%s %i second%s\n", adminname, colorcode, Hours, Hours != 1 ? "s" : "", Minutes, Minutes != 1 ? "s" : "", Seconds, Seconds != 1 ? "s" : ""));

                /*unsigned int j;
                for(j = 0;j<strlen(CHours);j++)
                    CHours[j] = CHours[j+1];
                for(j = 0;j<strlen(CMinutes);j++)
                    CMinutes[j] = CMinutes[j+1];
                for(j = 0;j<strlen(CSeconds);j++)
                    CSeconds[j] = CSeconds[j+1];
                if(Hours>0)
                    strcat(CHours, " hours");
                else
                    strcpy(CHours, "_comma");

                if(Minutes>0)
                    strcat(CMinutes, " minutes");
                else
                    strcpy(CMinutes, "_comma");

                if(Seconds>0)
                    strcat(CSeconds, " seconds");
                else
                    strcpy(CSeconds, "_comma");

                if(Hours > 99)
                    strcpy(CHours, "fuzz");

                int i = 0;
                for ( i = 1; i <= gpGlobals->maxClients; i++ )
                {
                    edict_t *pPlayer = g_engfuncs.pfnPEntityOfEntIndex( i );
                    sprintf(SayTime, "fvox/%s %s %s remaining", CHours, CMinutes, CSeconds);
                    if((pPlayer != NULL) && (INT(pPlayer->v.netname) != 0))
                    {
                        if(EntInfo3[ENTINDEX(pPlayer)].NotBot)
                            CLIENT_COMMAND ( pPlayer, UTIL_VarArgs("speak \"%s\"\n",SayTime) );
                    }
                }*/
            }
            else
            {
                SayTextAllChatHud(UTIL_VarArgs("[%s] There is %sno time limit%s set.\n", adminname, colorcode, removecolorcode));
            }
        }
        else if (!Q_strcasecmp("currentmap", text))
        {
            char *colorcode = "";
            if(isTF2)
            {
                colorcode = "\x04";
            }
            SayTextAllChatHud(UTIL_VarArgs("[%s] The map we are playing is: %s%s\n", adminname, colorcode, szCurrentMap));
        }
        else if (!Q_strcasecmp("nextmap", text))
        {
            char mapprefix = '\"';
            char mapsuffix = '\"';
            if(isTF2)
            {
                mapprefix = '\x04';
                mapsuffix = '\x01';
            }
            SayTextAllChatHud(UTIL_VarArgs("[%s] The next map will be %c%s%c unless changed by a vote.\n", adminname, mapprefix, NextMap(), mapsuffix));
        }
        else if (!Q_strcasecmp("nextnextmap", text))
        {
            char mapprefix = '\"';
            char mapsuffix = '\"';
            if(isTF2)
            {
                mapprefix = '\x04';
                mapsuffix = '\x01';
            }

            char levelname[32];
            MapCycleTracker()->GetNextLevelName(levelname, sizeof(levelname), 1);
            SayTextAllChatHud(UTIL_VarArgs("[%s] The map after next will be %c%s%c unless changed by a vote.\n", adminname, mapprefix, levelname, mapsuffix));
        }
        else if (!Q_strcasecmp("nextmaps", text))
        {
            char mapprefix = '\"';
            char mapsuffix = '\"';
            if(isTF2)
            {
                mapprefix = '\x04';
                mapsuffix = '\x01';
            }

            char levelname1[32];
            char levelname2[32];
            char levelname3[32];
            MapCycleTracker()->GetNextLevelName(levelname1, sizeof(levelname1), 0);
            MapCycleTracker()->GetNextLevelName(levelname2, sizeof(levelname2), 1);
            MapCycleTracker()->GetNextLevelName(levelname3, sizeof(levelname3), 2);
            SayTextAllChatHud(UTIL_VarArgs("[%s] The next maps are: %c%s, %s, and %s%c.\n", adminname, mapprefix, levelname1, levelname2, levelname3, mapsuffix));
        }
        else if(!Q_strncasecmp("whois ", text, 6))
        {
            if(Q_strcasecmp("whois ", text))
            {
                int playerList[128];
                int playerCount = 0;

                unsigned int i;
                char strName[256];
                /* we need to strip out 'whois ' (6 characters */
                for(i=6;i<strlen(text)+1;i++)
                    strName[i-6] = text[i];
                strName[i-6] = NULL;

                playerCount = pAdminOP.FindPlayerByName(playerList, strName);
                if(playerCount && playerList[0] != 0)
                {
                    IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+playerList[0]);

                    if(info)
                    {
                        if(info->IsConnected())
                        {
                            // if the player is allowed to have hidden mode set and he/she does and the player requesting is not an admin
                            if(pAOPPlayers[playerList[0]-1].IsHiddenFrom(iPlayer))
                            {
                                SayTextAllChatHud(UTIL_VarArgs("[%s] %s's SteamID: [U:1:%i]  UserID: %i  Admin: No\n", adminname, info->GetName(), pAOPPlayers[playerList[0]-1].GetFakeID(), info->GetUserID()));
                            }
                            else
                            {
                                SayTextAllChatHud(UTIL_VarArgs("[%s] %s's SteamID: %s  UserID: %i  Admin: %s\n", adminname, info->GetName(), engine->GetPlayerNetworkIDString(pAdminOP.GetEntityList()+playerList[0]), info->GetUserID(), (pAOPPlayers[playerList[0]-1].IsAdmin(0) && !(pAOPPlayers[playerList[0]-1].hasSip && pAOPPlayers[playerList[0]-1].sipLogin)) ? "Yes" : "No"));
                                if(pAOPPlayers[iPlayer-1].IsAdmin(8192, "whoisprivilege"))
                                    pAOPPlayers[iPlayer-1].SayTextChatHud(UTIL_VarArgs("[%s] IP: %s\n", adminname, pAOPPlayers[playerList[0]-1].IP));                           
                                if(pAOPPlayers[playerList[0]-1].hasDon)
                                    SayTextAllChatHud(UTIL_VarArgs("[%s] He/she is a donator.\n", adminname));
                                if(pAOPPlayers[playerList[0]-1].hasVip)
                                    SayTextAllChatHud(UTIL_VarArgs("[%s] He/she is a VIP.\n", adminname));
                                if(pAOPPlayers[playerList[0]-1].hasDev)
                                    SayTextAllChatHud(UTIL_VarArgs("[%s] He/she is a developer.\n", adminname));
                                if(pAOPPlayers[playerList[0]-1].userExtraInfo[0])
                                    SayTextAllChatHud(UTIL_VarArgs("[%s] %s\n", adminname, pAOPPlayers[playerList[0]-1].userExtraInfo));
                            }
                        }
                    }
                }
            }
        }
        if(featureStatus[FEAT_CREDITS])
        {
            if(!Q_strcasecmp("info", text) && playerchat_info.GetBool())
            {
                int iCredits = pAOPPlayers[iPlayer-1].GetCredits();
                int iConnections = pAOPPlayers[iPlayer-1].GetConnections();
                SayTextAllChatHud(UTIL_VarArgs("[%s] You have connected %i time%s, currently have %i credit%s, and have the rank '%s'.\n", adminname, iConnections, iConnections!=1 ? "s" : "", iCredits, iCredits!=1 ? "s" : "", GetRank(iCredits)));
            }
            else if(!Q_strncasecmp("info ", text, 5) && playerchat_info.GetBool())
            {
                if(Q_strcasecmp("info ", text))
                {
                    int playerList[128];
                    int playerCount = 0;

                    unsigned int i;
                    char strName[256];
                    /* we need to strip out 'info ' (5 characters */
                    for(i=5;i<strlen(text)+1;i++)
                        strName[i-5] = text[i];
                    strName[i-5] = NULL;

                    playerCount = pAdminOP.FindPlayerByName(playerList, strName);
                    if(playerCount && playerList[0] != 0)
                    {
                        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+playerList[0]);

                        if(info)
                        {
                            if(info->IsConnected())
                            {
                                int iCredits = pAOPPlayers[playerList[0]-1].GetCredits();
                                int iConnections = pAOPPlayers[playerList[0]-1].GetConnections();
                                
                                // if the player is allowed to have hidden mode set and he/she does and the player requesting is not an admin
                                if(pAOPPlayers[playerList[0]-1].IsHiddenFrom(iPlayer))
                                {
                                    iCredits = 0;
                                    iConnections = 1;
                                }

                                SayTextAllChatHud(UTIL_VarArgs("[%s] %s has connected %i time%s, currently has %i credit%s, and has the rank '%s'.\n", adminname, info->GetName(), iConnections, iConnections!=1 ? "s" : "", iCredits, iCredits!=1 ? "s" : "", GetRank(iCredits)));
                            }
                        }
                    }
                }
            }
            else if(!Q_strncasecmp("names ", text, 6) && playerchat_names.GetBool())
            {
                if(Q_strcasecmp("names ", text))
                {
                    int playerList[128];
                    int playerCount = 0;

                    unsigned int i;
                    char strName[256];
                    /* we need to strip out 'names ' (6 characters */
                    for(i=6;i<strlen(text)+1;i++)
                        strName[i-6] = text[i];
                    strName[i-6] = NULL;

                    playerCount = pAdminOP.FindPlayerByName(playerList, strName);
                    if(playerCount && playerList[0] != 0)
                    {
                        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+playerList[0]);

                        if(info)
                        {
                            if(info->IsConnected())
                            {
                                CAdminOPPlayer *pFoundPlayer = &pAOPPlayers[playerList[0]-1];
                                // if the player is allowed to have hidden mode set and he/she does and the player requesting is not an admin
                                if(pAOPPlayers[playerList[0]-1].IsHiddenFrom(iPlayer))
                                {
                                    SayTextAllChatHud(UTIL_VarArgs("[%s] Last name used: (none)   First name used: %s\n", adminname, pFoundPlayer->GetJoinName()));
                                }
                                else
                                {
                                    SayTextAllChatHud(UTIL_VarArgs("[%s] Last name used: %s   First name used: %s\n", adminname, pFoundPlayer->GetLastName()[0] ? pFoundPlayer->GetLastName() : "(none)", pFoundPlayer->GetFirstName()[0] ? pFoundPlayer->GetFirstName() : pFoundPlayer->GetJoinName()));
                                }
                            }
                        }
                    }
                }
            }
            else if(!Q_strncasecmp("time ", text, 5) && playerchat_time.GetBool())
            {
                if(Q_strcasecmp("time ", text))
                {
                    int playerList[128];
                    int playerCount = 0;

                    unsigned int i;
                    char strName[256];
                    /* we need to strip out 'time ' (5 characters */
                    for(i=5;i<strlen(text)+1;i++)
                        strName[i-5] = text[i];
                    strName[i-5] = NULL;

                    playerCount = pAdminOP.FindPlayerByName(playerList, strName);
                    if(playerCount && playerList[0] != 0)
                    {
                        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+playerList[0]);

                        if(info)
                        {
                            if(info->IsConnected())
                            {
                                int PlayerTotal;

                                // if the player is allowed to have hidden mode set and he/she does and the player requesting is not an admin
                                if(pAOPPlayers[playerList[0]-1].IsHiddenFrom(iPlayer))
                                {
                                    PlayerTotal = time(NULL) - pAOPPlayers[playerList[0]-1].GetSessionStartTime();
                                }
                                else
                                {
                                    PlayerTotal = pAOPPlayers[playerList[0]-1].GetTimePlayed();
                                }
                                int PlayerDays, PlayerHours, PlayerMinutes, PlayerSeconds;
                                PlayerDays = PlayerHours = PlayerMinutes = PlayerSeconds = PlayerTotal;
                                PlayerDays /= 86400;
                                PlayerHours /= 3600;
                                PlayerHours -= (PlayerDays*24);
                                PlayerMinutes /= 60;
                                PlayerMinutes -= (PlayerHours*60);
                                PlayerMinutes -= (PlayerDays*1440);
                                PlayerSeconds = PlayerTotal % 60;
                                SayTextAllChatHud(UTIL_VarArgs("[%s] %s has been on this server for %id:%ih:%im:%is.\n", adminname, info->GetName(), PlayerDays, PlayerHours, PlayerMinutes, PlayerSeconds));
                            }
                        }
                    }
                }
            }
        }
    }
    if(featureStatus[FEAT_MAPVOTE])
    {
        if (!Q_strcasecmp("rockthevote", text) || !Q_strcasecmp("mapvote", text) || !Q_strcasecmp("votemap", text) || !Q_strcasecmp("rtv", text))
        {
            char msg[256];

            if((feature_mapvote.GetInt() || pAOPPlayers[iPlayer-1].IsAdmin(16384, "mapvoteadmin")) && !pAOPPlayers[iPlayer-1].IsDenied("mapvote"))
            {
                if(mapVote.VoteInProgress())
                {
                    pAOPPlayer->ShowVoteMenu(0, 2);
                    sprintf(msg, "[%s] A map vote is already in progress.\n", adminname);
                    SayTextAllChatHud(msg);
                }
                else
                {
                    int votefreq = vote_freq.GetInt();
                    int votefreq_min = votefreq / 60;
                    if(mapVote.lastVote + votefreq > engine->Time() && !pAOPPlayers[iPlayer-1].IsAdmin(16384, "mapvoteadmin"))
                    {
                        if(mapVote.hasVoted)
                        {
                            sprintf(msg, "[%s] A map vote is not allowed within %i minute%s of a previous vote.\n", adminname, votefreq_min, votefreq_min != 1 ? "s" : "");
                            SayTextAllChatHud(msg);
                        }
                        else
                        {
                            sprintf(msg, "[%s] A map vote is not allowed within %i minute%s of the map start.\n", adminname, votefreq_min, votefreq_min != 1 ? "s" : "");
                            SayTextAllChatHud(msg);
                        }
                    }
                    else
                    {
                        if(vote_menu.GetInt() > 0)
                        {
                            for(int i = 0; i < pAdminOP.GetMaxClients(); i++)
                                pAOPPlayers[i].ShowVoteMenu(0); // if player isn't connected or not valid, ShowVoteMenu ignores
                        }
                        sprintf(msg, "[%s] Map voting has been enabled for 3 minutes.  Say 'vote <mapname>' to vote.\n", adminname);
                        SayTextAllChatHud(msg);
                        if(voteExtendCount < vote_maxextend.GetInt())
                        {
                            sprintf(msg, "[%s] Say 'vote extend' to keep playing this map for %i more minutes.\n", adminname, vote_extendtime.GetInt());
                            SayTextAllChatHud(msg);
                        }
                        mapVote.endTime = engine->Time() + 180;
                        mapVote.StartVote();
                    }
                }
            }
            else
            {
                sprintf(msg, "[%s] You cannot start a map vote.\n", adminname);
                SayTextAllChatHud(msg);
            }
        }
        else if(!Q_strncasecmp("vote ", text, 5) && Q_strcasecmp("vote ", text))
        {
            if(!pAOPPlayers[iPlayer-1].IsDenied("mapvote"))
            {
                int i, Length;
                char strMap[256];
                char msg[256];
                Length = strlen(text);
                if (Length > 255) Length = 255;
                if(mapVote.VoteInProgress())
                {
                    bool sayWinningVote = 0;
                    /* we need to strip out 'vote ' (5 characters */
                    for(i=5;i<Length+1;i++)
                        strMap[i-5] = text[i];
                    strMap[i-5] = NULL;

                    if(!Q_strcasecmp(strMap, "extend"))
                    {
                        if(voteExtendCount < vote_maxextend.GetInt())
                        {
                            mapVote.ChangeVote(strMap, userid);
                            int iVoteCount = mapVote.GetCount(strMap);
                            Q_snprintf(msg, 119, "[%s] Vote by %s -- extend for %i minutes now has %i vote%s.\n", adminname, playerinfo->GetName(), vote_extendtime.GetInt(), iVoteCount, iVoteCount != 1 ? "s" : "");
                            msg[119] = '\0';
                            SayTextAllChatHud(msg);
                            sayWinningVote = 1;
                        }
                        else
                        {
                            Q_snprintf(msg, 119, "[%s] Sorry %s. The map cannot be extended any longer.\n", adminname, playerinfo->GetName());
                            msg[119] = '\0';
                            SayTextAllChatHud(msg);
                        }
                    }
                    else if(DFIsMapValid(strMap))
                    {
                        mapVote.ChangeVote(strMap, userid);
                        int iVoteCount = mapVote.GetCount(strMap);
                        Q_snprintf(msg, 119, "[%s] Vote by %s -- %s now has %i vote%s.\n", adminname, playerinfo->GetName(), strMap, iVoteCount, iVoteCount != 1 ? "s" : "");
                        msg[119] = '\0';
                        SayTextAllChatHud(msg);
                        sayWinningVote = 1;
                    }
                    else
                    {
                        Q_snprintf(msg, 119, "[%s] Sorry %s. That map was not found on this server.\n", adminname, playerinfo->GetName());
                        msg[119] = '\0';
                        SayTextAllChatHud(msg);
                    }
                    if(sayWinningVote)
                    {
                        int maxvotes = (GetPlayerCountPlaying() * vote_ratio.GetInt()) / 100;
                        VoteCount_t winVote;
                        winVote = mapVote.GetTopChoice();

                        if(winVote.votes != 0 && winVote.votes >= maxvotes)
                        {
                            if(!Q_strcasecmp(winVote.item, "extend"))
                            {
                                SayTextAllChatHud(UTIL_VarArgs("[%s] Extend for %i minutes is winning with %i vote%s\n", adminname, vote_extendtime.GetInt(), winVote.votes, winVote.votes != 1 ? "s" : ""));
                            }
                            else
                            {
                                SayTextAllChatHud(UTIL_VarArgs("[%s] %s is winning with %i vote%s\n", adminname, winVote.item, winVote.votes, winVote.votes != 1 ? "s" : ""));
                            }
                        }
                        else
                        {
                            SayTextAllChatHud(UTIL_VarArgs("[%s] No map has the %i votes required yet.\n", adminname, maxvotes));
                        }
                    }
                }
                else
                {
                    sprintf(msg, "[%s] There is no map vote in progress.\n", adminname);
                    SayTextAllChatHud(msg);
                }
            }
        }
        else if(!Q_strcasecmp("endthevote", text))
        {
            if(pAOPPlayers[iPlayer-1].IsAdmin(16384, "mapvoteadmin"))
            {
                EndMapVote();
            }
            else
            {
                char msg[72];
                sprintf(msg, "[%s] You cannot end a vote.\n", adminname);
                SayTextAllChatHud(msg);
            }
        }
        else if(!Q_strcasecmp("cancelvote", text))
        {
            if(pAOPPlayers[iPlayer-1].IsAdmin(16384, "mapvoteadmin"))
            {
                char msg[72];
                sprintf(msg, "[%s] Voting canceled!\n", adminname);
                SayTextAllChatHud(msg);
                mapVote.EndVote();
            }
            else
            {
                char msg[72];
                sprintf(msg, "[%s] You cannot cancel a vote.\n", adminname);
                SayTextAllChatHud(msg);
            }
        }
    }
    if(featureStatus[FEAT_CVARVOTE])
    {
        if(!Q_strcasecmp("jetpackvote", text))
        {
            if(jetpackvote_enable.GetBool())
            {
                if(jetpackVoteNextTime)
                {
                    if(jetpackVoteNextTime <= gpGlobals->curtime)
                    {
                        if(!cvarVote.VoteInProgress())
                        {
                            StartCvarVote(&jetpack_on, "Would you like jetpack on or off?", "Jetpack On", 1, "Jetpack Off", 0, &jetpackvote_ratio, &jetpackvote_duration, (CvarVoteCallback)CAdminOP::JetpackVoteFinish);
                            SayTextAllChatHud(UTIL_VarArgs("[%s] Jetpack vote started. Press Escape to choose your jetpack preference.\n", adminname));
                        }
                        else
                        {
                            SayTextAllChatHud(UTIL_VarArgs("[%s] A server CVAR vote is already in progress.\n", adminname));
                        }
                    }
                    else
                    {
                        SayTextAllChatHud(UTIL_VarArgs("[%s] You must wait %0.2f seconds before starting a jetpack vote.\n", adminname, jetpackVoteNextTime-gpGlobals->curtime));
                    }
                }
            }
            else
            {
                if(jetpackvote_show_disabledmsg.GetBool())
                {
                    SayTextAllChatHud(UTIL_VarArgs("[%s] Jetpack vote is disabled.\n", adminname));
                }
            }
        }
    }
    //char msg[256];
    //Q_snprintf(msg, sizeof(msg), "[%s] %s\n", adminname, text);
    //SayTextAllChatHud(msg);
}

void CAdminOP :: PlayerDeath(int iPlayer, int iAttacker, const char *pszWeapon)
{
    int playerList[128];
    int playerCount = 0;

    playerCount = FindPlayerByUserID(playerList, iPlayer);
    if(playerCount == 1)
    {
        int playerkilled = playerList[0];
        pAOPPlayers[playerList[0]-1].OnDeath(iAttacker, pszWeapon);

        if(iAttacker != 0)
            playerCount = FindPlayerByUserID(playerList, iAttacker);
        else
            playerCount = 1;

        if(playerCount == 1)
        {
            // TODO: seniorproj: Forward this death message to all remote clients
        }
    }

    if(iAttacker != 0)
    {
        playerCount = FindPlayerByUserID(playerList, iAttacker);
        if(playerCount == 1)
        {
            pAOPPlayers[playerList[0]-1].OnKill(iPlayer, pszWeapon);
        }
    }

    if(iPlayer != iAttacker && iAttacker != 0)
    {
        if(Kills.firstblood == 0)
        {
            if(FeatureStatus(FEAT_KILLSOUNDS) && feature_killsounds.GetBool()) PlaySoundAll("SourceOP/firstblood.wav");
            Kills.firstblood = 1;
        }
    }
}

bool CAdminOP :: SetNoclip(int iPlayer, int iType) //0-Remove, 1-Give, 2-Toggle
{
    if(iPlayer > 0)
    {
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pEList+iPlayer);
        if(info)
        {
            if(info->IsConnected())
            {
                return pAOPPlayers[iPlayer-1].SetNoclip(iType);
            }
        }
    }
    return 0;
}

bool CAdminOP :: SetGod(int iPlayer, int iType) //0-Remove, 1-Give, 2-Toggle
{
    if(iPlayer > 0)
    {
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pEList+iPlayer);
        if(info)
        {
            if(info->IsConnected())
            {
                return pAOPPlayers[iPlayer-1].SetGod(iType);
            }
        }
    }
    return 0;
}

bool CAdminOP :: SlapPlayer(int iPlayer)
{
    if(iPlayer > 0)
    {
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pEList+iPlayer);
        if(info)
        {
            if(info->IsConnected())
            {
                pAOPPlayers[iPlayer-1].Slap();
                return 1;
            }
        }
    }
    return 0;
}

bool CAdminOP :: SlayPlayer(int iPlayer)
{
    if(iPlayer > 0)
    {
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pEList+iPlayer);
        if(info && info->IsConnected())
        {
            CAdminOPPlayer *pAOPPlayer = &pAOPPlayers[iPlayer-1];
            if(pAOPPlayer->IsAlive())
            {
                CBaseEntity *pBase = CBaseEntity::Instance(pEList+iPlayer);
                effects->Sparks(VFuncs::GetAbsOrigin(pBase)-Vector(0,0,8), 10, 1);
                effects->Sparks(VFuncs::GetAbsOrigin(pBase), 10, 1);
                effects->Sparks(VFuncs::GetAbsOrigin(pBase)+Vector(0,0,20), 10, 1);
                pAOPPlayer->SayTextChatHud(UTIL_VarArgs("[%s] You were slayed.\n", adminname));
                pAOPPlayer->Kill();
                return 1;
            }
        }
    }
    return 0;
}

bool CAdminOP :: KickPlayer(IPlayerInfo *info, const char *pszReason)
{
    if(info)
    {
        if(info->IsConnected())
        {
            char msg[128];
            if(pszReason)
            {
                Q_snprintf(msg, sizeof(msg), "kickid %i %s\n", info->GetUserID(), pszReason);
            }
            else
            {
                Q_snprintf(msg, sizeof(msg), "kickid %i\n", info->GetUserID());
            }
            engine->ServerCommand(msg);
            return 1;
        }
    }
    return 0;
}

bool CAdminOP :: BanPlayer(int index, IPlayerInfo *info, CSteamID bannedID, int time, const char *pszBannerName, const char *pszBannerID, const char *pszReason, bool ip, const char *pszExtra)
{
    if(info)
    {
        if(info->IsConnected())
        {
            if(!ip)
            {
                UTIL_BanPlayerByID(info->GetName(), info->GetUserID(), pAOPPlayers[index-1].baseclient, bannedID, pAOPPlayers[index-1].GetIP(), pszBannerName, pszBannerID, szCurrentMap, time, pszReason ? pszReason : "No reason specified", pszExtra);
                return 1;
            }
            else
            {
                UTIL_BanPlayerByIP(info->GetName(), info->GetUserID(), pAOPPlayers[index-1].baseclient, bannedID, pAOPPlayers[index-1].GetIP(), pszBannerName, pszBannerID, szCurrentMap, time, pszReason ? pszReason : "No reason specified", pszExtra);
                return 1;
            }
        }
    }
    return 0;
}

int CAdminOP :: FindPlayer(int *playerList, const char *pszName)
{
    int count = 0;
    int findName = 0;
    int findID = 0;
    int findSteam = 0;
    int nameInt = atoi(pszName);
    
    findName = FindPlayerByName(playerList, pszName);
    count += findName;
    findSteam = FindPlayerBySteamID(playerList, pszName);
    count += findSteam;
    if(nameInt != 0)
    {
        findID = FindPlayerByUserID(playerList, nameInt);
        count += findID;
    }

    return count;
}

int CAdminOP :: FindPlayerByName(int *playerList, const char *pszName)
{
    int count = 0;
    for(int i = 1; i <= maxClients; i++)
    {
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pEList+i);
        if(info)
        {
            if(info->IsConnected())
            {
                const char *playerName = info->GetName();

                if(Q_strcasestr(playerName, pszName))
                {
                    bool process = 1;
                    if(count)
                    {
                        for(int j = 0; j <= count-1; j++) // make sure not already in list
                            if(playerList[j] == i) process = 0;
                    }
                    if(process)
                    {
                        playerList[count] = i;
                        count++;
                    }
                }
            }
        }
    }
    return count;
}

int CAdminOP :: FindPlayerWithName(int *playerList, const char *pszName)
{
    int count = 0;
    for(int i = 1; i <= maxClients; i++)
    {
        if(pAdminOP.pAOPPlayers[i-1].GetPlayerState())
        {
            const char *playerName = NULL;
            // try baseclient
            if(pAdminOP.pAOPPlayers[i-1].baseclient)
            {
                playerName = VFuncs::GetClientName(pAdminOP.pAOPPlayers[i-1].baseclient);
            }
            // if not available or failed, try IPlayerInfo
            if(!playerName)
            {
                IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pEList+i);
                if(info)
                {
                    playerName = info->GetName();
                }
            }

            // if we were able to get playerName from either method
            if(playerName)
            {
                if(!strcmp(playerName, pszName))
                {
                    bool process = 1;
                    if(count)
                    {
                        for(int j = 0; j <= count-1; j++) // make sure not already in list
                            if(playerList[j] == i) process = 0;
                    }
                    if(process)
                    {
                        playerList[count] = i;
                        count++;
                    }
                }
            }
        }
    }
    return count;
}

int CAdminOP :: FindPlayerByUserID(int *playerList, const int iID)
{
    int count = 0;
    for(int i = 1; i <= maxClients; i++)
    {
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pEList+i);
        if(info)
        {
            if(info->IsConnected())
            {
                int userID = info->GetUserID();

                if(userID == iID)
                {
                    bool process = 1;
                    if(count)
                    {
                        for(int j = 0; j <= count-1; j++) // make sure not already in list
                            if(playerList[j] == i) process = 0;
                    }
                    if(process)
                    {
                        playerList[count] = i;
                        count++;
                    }
                }
            }
        }
    }
    return count;
}

int CAdminOP :: FindPlayerBySteamID(int *playerList, const char *pszSteam)
{
    int count = 0;

    for(int i = 1; i <= maxClients; i++)
    {
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pEList+i);
        if(info)
        {
            if(info->IsConnected())
            {
                const char *playerName = engine->GetPlayerNetworkIDString(pEList+i);

                if(FStrEq(playerName, pszSteam))
                {
                    bool process = 1;
                    if(count)
                    {
                        for(int j = 0; j <= count-1; j++) // make sure not already in list
                            if(playerList[j] == i) process = 0;
                    }
                    if(process)
                    {
                        playerList[count] = i;
                        count++;
                    }
                }
            }
        }
    }
    return count;
}

const char *CAdminOP :: GetMessageName(int iMsg)
{
    if(iMsg >= 0 && iMsg < USR_MSGS_MAX)
    {
        return userMessages[iMsg];
    }
    return "";
}

int CAdminOP :: GetMessageByName(const char * szMsg)
{
    for(int i=0; i<USR_MSGS_MAX; i++)
    {
        if(!Q_strcasecmp(userMessages[i], szMsg))
        {
            return i;
        }
    }
    return -1;
}

void CAdminOP :: PropogateMapList()
{
    mapList.Purge();

    FileFindHandle_t findHandle;
    const char *bspFilename = filesystem->FindFirstEx( "maps/*.bsp", "MOD", &findHandle );

    while ( bspFilename )
    {
        char mapName[256];
        maplist_t newMap;
        newMap.indexNumber = mapList.Count();
        strncpy(mapName, bspFilename, sizeof(mapName));
        mapName[255] = '\0';
        mapName[strlen(mapName)-4] = '\0';
        strncpy(newMap.map, mapName, sizeof(newMap.map));
        newMap.map[sizeof(newMap.map)-1] = '\0';

        mapList.AddToTail(newMap);

        bspFilename = filesystem->FindNext( findHandle );
    }
    filesystem->FindClose( findHandle );
}

void CAdminOP :: InitCommandLineCVars()
{
#ifdef OFFICIALSERV_ONLY
    char gamedir[256];
    char filepath[512];
    FILE *fp;

    engine->GetGameDir(gamedir, sizeof(gamedir));

    const char *maxplayers_f = cvar->GetCommandLineValue("DF_maxplayers_force");
    if(maxplayers_f)
    {
        maxplayers_force.SetValue(maxplayers_f);
        commandline->RemoveParm("+maxplayers");
        commandline->AppendParm("+maxplayers", maxplayers_f);
    }

    Q_snprintf(filepath, sizeof(filepath), "%s/%s/cvar/DF_maxplayers_force.txt", gamedir, DataDir());
    V_FixSlashes(filepath);
    fp=fopen(filepath, "rt");
    if(fp)
    {
        char tmp[256];
        if(fgets(tmp,sizeof(tmp),fp))
        {
            strrtrim(tmp, "\x0D\x0A");
            maxplayers_force.SetValue(tmp);
            commandline->RemoveParm("+maxplayers");
            commandline->AppendParm("+maxplayers", tmp);
        }
        fclose(fp);
    }
#endif

    // DF_remote_port
    const char *pszRemotePort = cvar->GetCommandLineValue("DF_remote_port");
    if(pszRemotePort)
    {
        remote_port.SetValue(pszRemotePort);
    }
}

#define CHECK_FEATURE(file, index) \
    fp = fopen(UTIL_VarArgs("%s" SLASHSTRING "" file, filepath), "rb"); \
    if(fp) \
    { \
        featureStatus[index] = fgetc(fp) == '1'; \
        if(featureStatus[index]) ColorMsg(CONCOLOR_DARKGRAY, "[SOURCEOP] Feature " #index " is enabled.\n"); \
        fclose(fp); \
    }

void CAdminOP :: LoadFeatures()
{
    char gamedir[256];
    char filepath[512];
    FILE *fp;

    for(int i = 0; i < NUM_FEATS; i++)
    {
        featureStatus[i] = 0;
    }

    engine->GetGameDir(gamedir, sizeof(gamedir));
    Q_snprintf(filepath, sizeof(filepath), "%s/%s/features", gamedir, DataDir());
    V_FixSlashes(filepath);

    CHECK_FEATURE("credits.txt", FEAT_CREDITS);
    CHECK_FEATURE("admincommands.txt", FEAT_ADMINCOMMANDS);
    CHECK_FEATURE("entcommands.txt", FEAT_ENTCOMMANDS);
    CHECK_FEATURE("adminsaycommands.txt", FEAT_ADMINSAYCOMMANDS);
    CHECK_FEATURE("playersaycommands.txt", FEAT_PLAYERSAYCOMMANDS);
    CHECK_FEATURE("killsounds.txt", FEAT_KILLSOUNDS);
    CHECK_FEATURE("mapvote.txt", FEAT_MAPVOTE);
    CHECK_FEATURE("cvarvote.txt", FEAT_CVARVOTE);
    CHECK_FEATURE("remote.txt", FEAT_REMOTE);
    CHECK_FEATURE("hook.txt", FEAT_HOOK);
    CHECK_FEATURE("jetpack.txt", FEAT_JETPACK);
    CHECK_FEATURE("snark.txt", FEAT_SNARK);
    CHECK_FEATURE("radio.txt", FEAT_RADIO);
    CHECK_FEATURE("lua.txt", FEAT_LUA);
}

void CAdminOP :: LoadPrecached()
{
    char gamedir[256];
    FILE *fp;
    char filepath[512];

    precached.Purge();

    engine->GetGameDir(gamedir, sizeof(gamedir));

    Q_snprintf(filepath, sizeof(filepath), "%s/%s/DF_precache.txt", gamedir, DataDir());
    V_FixSlashes(filepath);
    fp=fopen(filepath, "rt");
    if(fp)
    {
        char tmp[256];
        while(!feof(fp))
        {
            if(fgets(tmp,sizeof(tmp),fp))
            {
                char *tmp2;
                tmp2 = strTrim(tmp);
                if((strncmp(tmp2, "#", 1)) && (strncmp(tmp2, "//", 2)) && (strncmp(tmp2, "\n", 1)) && (strlen(tmp2)))
                {
                    char *file;
                    int type;
                    if(sscanf(tmp2, "%d %255s", &type, tmp) == 2)
                    {
                        precached_t precache;

                        file = strdup(tmp);
                        //V_StrSubst(tmp, "/", SLASHSTRING, file, strlen(tmp)+1, true);
                        precache.FileName = file;
                        precache.Type = type;

                        precached.AddToTail(precache);
                    }
                }
                free(tmp2);
            }
        }
        fclose(fp);
    }
    engine->LogPrint(UTIL_VarArgs("[SOURCEOP] \"Console<0><Console><Console>\" loaded \"%i\" precache entries\n", precached.Count()));
}

void CAdminOP :: LoadForceDownload()
{
    char gamedir[256];
    FILE *fp;
    char filepath[512];

    for(unsigned short i = downloads.Head(); i != downloads.InvalidIndex(); i = downloads.Next(i))
    {
        free(downloads.Element(i));
    }
    downloads.Purge();

    engine->GetGameDir(gamedir, sizeof(gamedir));

    Q_snprintf(filepath, sizeof(filepath), "%s/%s/DF_forcedownload.txt", gamedir, DataDir());
    V_FixSlashes(filepath);
    fp=fopen(filepath, "rt");
    if(fp)
    {
        char tmp[256];
        while(!feof(fp))
        {
            if(fgets(tmp,sizeof(tmp),fp))
            {
                char *tmp2;
                tmp2 = strTrim(tmp);
                if((strncmp(tmp2, "#", 1)) && (strncmp(tmp2, "//", 2)) && (strncmp(tmp2, "\n", 1)) && (strlen(tmp2)))
                {
                    char *tmp3;
                    tmp3 = strdup(tmp2);
                    V_StrSubst(tmp2, "/", SLASHSTRING, tmp3, strlen(tmp2)+1, true);
                    downloads.AddToTail(tmp3);
                }
                free(tmp2);
            }
        }
        fclose(fp);
    }
    engine->LogPrint(UTIL_VarArgs("[SOURCEOP] \"Console<0><Console><Console>\" loaded \"%i\" force download entries\n", downloads.Count()));
}

void CAdminOP :: LoadSpawnAlias()
{
    char gamedir[256];
    FILE *fp;
    char filepath[512];

    spawnAlias.Purge();

    engine->GetGameDir(gamedir, sizeof(gamedir));

    Q_snprintf(filepath, sizeof(filepath), "%s/%s/DF_spawnalias.txt", gamedir, DataDir());
    V_FixSlashes(filepath);
    fp = fopen(filepath, "rt");

    if(fp)
    {
        int readingBlock = 0;
        int thisIndex = 0;
        char tmp[256];
        char buf[256];
        char key[256];
        char value[256];
        while(!feof(fp))
        {
            if(fgets(tmp,sizeof(tmp),fp))
            {
                if ((strncmp(tmp, "#", 1) != 0) && (strncmp(tmp, "//", 2) != 0) && (strncmp(tmp, "\n", 1) != 0) && (tmp[0] != 0x0D) && (tmp[0] != 0x0A) && (strlen(tmp) != 0))
                {
                    char firstchar;
                    sscanf(tmp, "%s", buf);
                    firstchar = buf[0];
                    if(readingBlock == 1 && firstchar != '}')
                    {
                        unsigned int eql = 0;
                        for(eql = 0; eql < strlen(tmp); eql++)
                        {
                            if(tmp[eql] == '=')
                            {
                                break;
                            }
                        }
                        if(eql)
                        {
                            spawnaliasdata_t *newBlock;
                            newBlock = &spawnAlias.Element(spawnAlias.Tail());
                            strcpy(key, tmp);
                            key[eql] = '\0';
                            sscanf(key, "%s", buf);
                            strcpy(key, buf);
                            strcpy(value, &tmp[eql+1]);
                            sscanf(value, "%s", buf);
                            strcpy(value, buf);
                            if(!stricmp(key, "model"))
                            {
                                strncpy(newBlock->model, value, sizeof(newBlock->model));
                                newBlock->model[sizeof(newBlock->model)-1] = '\0';
                            }
                            if(!stricmp(key, "spawnmode"))
                            {
                                newBlock->spawnMode = atoi(value);
                            }
                        }
                    }
                    else if(firstchar == '{')
                    {
                        readingBlock = 1;
                    }
                    else if(firstchar == '}')
                    {
                        readingBlock = 0;
                    }
                    else
                    {
                        spawnaliasdata_t newBlock;
                        strcpy(key, buf);
                        strncpy(newBlock.name, key, sizeof(newBlock.name));
                        newBlock.name[sizeof(newBlock.name)-1] = '\0';
                        newBlock.indexNumber = spawnAlias.Count();
                        newBlock.model[0] = '\0';
                        newBlock.spawnMode = 0;
                        thisIndex = newBlock.indexNumber;
                        spawnAlias.AddToTail(newBlock);
                    }
                }
            }
        }
        fclose(fp);
    }
}

const spawnaliasdata_t *CAdminOP :: FindModelFromAlias(const char * pszAlias)
{
    static spawnaliasdata_t errordata;

    for ( unsigned short i=spawnAlias.Head(); i != spawnAlias.InvalidIndex(); i = spawnAlias.Next( i ) )
    {
        spawnaliasdata_t *data = &spawnAlias.Element(i);
        if(!stricmp(data->name, pszAlias))
        {
            return data;
        }
    }

    strcpy(errordata.model, "models/error.mdl");
    strcpy(errordata.name, "error");
    errordata.spawnMode = 2;
    errordata.indexNumber = -1;

    return &errordata;
}

const spawnaliasdata_t *CAdminOP :: FindAliasFromModel(const char * pszModel)
{
    static spawnaliasdata_t errordata;

    for ( unsigned short i=spawnAlias.Head(); i != spawnAlias.InvalidIndex(); i = spawnAlias.Next( i ) )
    {
        spawnaliasdata_t *data = &spawnAlias.Element(i);
        if(!stricmp(data->model, pszModel))
        {
            return data;
        }
    }

    strcpy(errordata.model, "models/error.mdl");
    strcpy(errordata.name, "error");
    errordata.spawnMode = 2;
    errordata.indexNumber = -1;

    return &errordata;
}

void CAdminOP :: LoadAdmins()
{
    adminData.Purge();

    char gamedir[256];
    FILE *fp;
    char filepath[512];

    listDenied.Purge();
    listAdminAllow.Purge();
    listAdminDenied.Purge();
    defaultUser.listDenied.Purge();
    defaultUser.listAdminAllow.Purge();
    defaultUser.listAdminDenied.Purge();

    engine->GetGameDir(gamedir, sizeof(gamedir));

    Q_snprintf(filepath, sizeof(filepath), "%s/%s/DF_admins.txt", gamedir, DataDir());
    V_FixSlashes(filepath);
    fp = fopen(filepath, "rt");

    if(fp)
    {
        int readingAdmin = 0;
        int readingDefault = 0;
        int thisIndex = 0;
        char tmp[256];
        char buf[256];
        //char trimed[256];
        char key[256];
        char value[256];
        while(!feof(fp))
        {
            if(fgets(tmp,sizeof(tmp),fp))
            {
                if ((strncmp(tmp, "#", 1) != 0) && (strncmp(tmp, "//", 2) != 0) && (strncmp(tmp, "\n", 1) != 0) && (tmp[0] != 0x0D) && (tmp[0] != 0x0A) && (strlen(tmp) != 0))
                {
                    char firstchar;
                    sscanf(tmp, "%s", buf);
                    firstchar = buf[0];
                    if(firstchar == '\"')
                    {
                        int quotes = 0;
                        int quotepos[2];
                        for(unsigned int i = 0; i < strlen(tmp); i++)
                        {
                            if(tmp[i] == '\"')
                            {
                                quotepos[quotes] = i;
                                quotes++;
                                if(quotes == 2) break;
                            }
                        }
                        if(quotes == 2)
                        {
                            tmp[quotepos[1]] = '\0';
                            strcpy(buf, &tmp[quotepos[0]+1]);

                            unsigned int colon = 0;
                            for(colon = 0; colon < strlen(buf); colon++)
                            {
                                if(buf[colon] == ':')
                                {
                                    break;
                                }
                            }

                            if(colon)
                            {
                                admindata_t newAdmin;
                                readingAdmin = 1;
                                strcpy(key, buf);
                                key[colon] = '\0';
                                strcpy(value, &buf[colon+1]);
                                //Msg("Admin file user\n Type: \"%s\"\n Value: \"%s\"\n", key, value);
                                strncpy(newAdmin.id, value, sizeof(newAdmin.id));
                                newAdmin.id[sizeof(newAdmin.id)-1] = '\0';
                                newAdmin.indexNumber = adminData.Count();
                                newAdmin.baseLevel = 0;
                                newAdmin.password[0] = '\0';
                                newAdmin.spawnLimit = SPAWN_LIMIT_USE_CVAR;
                                newAdmin.loggedIn = 0;
                                thisIndex = newAdmin.indexNumber;
                                if(!stricmp(key, "Name"))
                                {
                                    newAdmin.type = ADMIN_TYPE_NAME;
                                    adminData.AddToTail(newAdmin);
                                }
                                else if(!stricmp(key, "SteamID"))
                                {
                                    // Convert SteamID format
                                    char pszSteamIDError[64];
                                    if ( !IsValidSteamID( newAdmin.id, pszSteamIDError, sizeof( pszSteamIDError ) ) )
                                    {
                                        CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Invalid SteamID %s in admin file: %s\n", newAdmin.id, pszSteamIDError );
                                        continue;
                                    }

                                    CSteamID steamIDCredits( newAdmin.id, k_EUniversePublic );
                                    strcpy(newAdmin.id, steamIDCredits.Render() );

                                    newAdmin.type = ADMIN_TYPE_STEAMID;
                                    adminData.AddToTail(newAdmin);
                                }
                                else if(!stricmp(key, "IP"))
                                {
                                    newAdmin.type = ADMIN_TYPE_IP;
                                    adminData.AddToTail(newAdmin);
                                }
                                else if(!stricmp(key, "Default"))
                                {
                                    readingDefault = 1;
                                    newAdmin.type = ADMIN_TYPE_DEFAULT;
                                    newAdmin.indexNumber = ADMIN_INDEX_DEFAULT;
                                    thisIndex = newAdmin.indexNumber;
                                    memcpy(&defaultUser.adminData, &newAdmin, sizeof(defaultUser.adminData));
                                }
                                else
                                {
                                    readingAdmin = 0;
                                    Msg("Unknown admin type \"%s\" for user \"%s\"\n", key, value);
                                    TimeLog("SourceOPErrors.log", "Unknown admin type \"%s\" for user \"%s\"\n", key, value);
                                }
                            }
                        }
                    }
                    else if(firstchar == '{')
                    {
                    }
                    else if(firstchar == '}')
                    {
                        readingAdmin = 0;
                        readingDefault = 0;
                    }
                    else if(readingAdmin == 1)
                    {
                        unsigned int eql = 0;
                        for(eql = 0; eql < strlen(tmp); eql++)
                        {
                            if(tmp[eql] == '=')
                            {
                                break;
                            }
                        }
                        if(eql)
                        {
                            admindata_t *newAdmin;
                            if(readingDefault)
                                newAdmin = &defaultUser.adminData;
                            else
                                newAdmin = &adminData.Element(adminData.Tail());
                            strcpy(key, tmp);
                            key[eql] = '\0';
                            sscanf(key, "%s", buf);
                            strcpy(key, buf);
                            strcpy(value, &tmp[eql+1]);
                            sscanf(value, "%s", buf);
                            strcpy(value, buf);
                            //Msg("Admin file\n Key: \"%s\"\n Value: \"%s\"\n", key, value);
                            if(!stricmp(key, "baseLevel"))
                            {
                                newAdmin->baseLevel = atoi(value);
                            }
                            else if(!stricmp(key, "password"))
                            {
                                if(!readingDefault)
                                {
                                    strncpy(newAdmin->password, value, sizeof(newAdmin->password));
                                    newAdmin->password[sizeof(newAdmin->password)-1] = '\0';
                                }
                            }
                            else if(!stricmp(key, "denyCmd"))
                            {
                                char cmd[64];
                                admindatacmd_t adminCmd;
                                strncpy(cmd, value, sizeof(cmd));
                                cmd[sizeof(cmd)-1] = '\0';
                                adminCmd.indexNumber = thisIndex;
                                strcpy(adminCmd.cmd, cmd);

                                if(readingDefault)
                                    defaultUser.listDenied.AddToTail(adminCmd);
                                else
                                    listDenied.AddToTail(adminCmd);
                            }
                            else if(!stricmp(key, "addAdminCmd"))
                            {
                                char cmd[64];
                                admindatacmd_t adminCmd;
                                strncpy(cmd, value, sizeof(cmd));
                                cmd[sizeof(cmd)-1] = '\0';
                                adminCmd.indexNumber = thisIndex;
                                strcpy(adminCmd.cmd, cmd);

                                if(readingDefault)
                                    defaultUser.listAdminAllow.AddToTail(adminCmd);
                                else
                                    listAdminAllow.AddToTail(adminCmd);
                            }
                            else if(!stricmp(key, "denyAdminCmd"))
                            {
                                char cmd[64];
                                admindatacmd_t adminCmd;
                                strncpy(cmd, value, sizeof(cmd));
                                cmd[sizeof(cmd)-1] = '\0';
                                adminCmd.indexNumber = thisIndex;
                                strcpy(adminCmd.cmd, cmd);

                                if(readingDefault)
                                    defaultUser.listAdminDenied.AddToTail(adminCmd);
                                else
                                    listAdminDenied.AddToTail(adminCmd);
                            }
                            else if(!stricmp(key, "spawnLimit"))
                            {
                                newAdmin->spawnLimit = atoi(value);
                            }
                        }
                    }
                }
            }
        }
        fclose(fp);
        /*Msg("Admin file summary:\n");
        for ( unsigned short i=adminData.Head(); i != adminData.InvalidIndex(); i = adminData.Next( i ) )
        {
            unsigned short j;
            admindata_t *admin = &adminData.Element(i);
            admindatacmd_t *adminCmd;

            Msg("Admin %s with a type of %i has the following attributes:\n baseLevel: %i\n %s%s\n", admin->id, admin->type, admin->baseLevel, admin->password[0] ? "Password: " : "Not using a password.", admin->password);
            Msg(" Blocked commands:\n");
            for ( j=listDenied.Head(); j != listDenied.InvalidIndex(); j = listDenied.Next( j ) )
            {
                adminCmd = &listDenied.Element(j);
                if(adminCmd->indexNumber == admin->indexNumber)
                {
                    Msg("  %s\n", adminCmd->cmd);
                }
            }
            Msg(" Allowed admin commands:\n");
            for ( j=listAdminAllow.Head(); j != listAdminAllow.InvalidIndex(); j = listAdminAllow.Next( j ) )
            {
                adminCmd = &listAdminAllow.Element(j);
                if(adminCmd->indexNumber == admin->indexNumber)
                {
                    Msg("  %s\n", adminCmd->cmd);
                }
            }
            Msg(" Blocked admin commands:\n");
            for ( j=listAdminDenied.Head(); j != listAdminDenied.InvalidIndex(); j = listAdminDenied.Next( j ) )
            {
                adminCmd = &listAdminDenied.Element(j);
                if(adminCmd->indexNumber == admin->indexNumber)
                {
                    Msg("  %s\n", adminCmd->cmd);
                }
            }
        }

        Msg("Straight dump:\n");
        for ( unsigned short i=adminData.Head(); i != adminData.InvalidIndex(); i = adminData.Next( i ) )
        {
            admindata_t *admin = &adminData.Element(i);
            Msg("Admin index:%i type:%i %s %s %i\n", admin->indexNumber, admin->type, admin->id, admin->password, admin->baseLevel);
        }
        for ( unsigned short i=listDenied.Head(); i != listDenied.InvalidIndex(); i = listDenied.Next( i ) )
        {
            admindatacmd_t *admin = &listDenied.Element(i);
            Msg("Command denied %i %s\n", admin->indexNumber, admin->cmd);
        }
        for ( unsigned short i=listAdminAllow.Head(); i != listAdminAllow.InvalidIndex(); i = listAdminAllow.Next( i ) )
        {
            admindatacmd_t *admin = &listAdminAllow.Element(i);
            Msg("Admin allow %i %s\n", admin->indexNumber, admin->cmd);
        }
        for ( unsigned short i=listAdminDenied.Head(); i != listAdminDenied.InvalidIndex(); i = listAdminDenied.Next( i ) )
        {
            admindatacmd_t *admin = &listAdminDenied.Element(i);
            Msg("Admin denied %i %s\n", admin->indexNumber, admin->cmd);
        }*/
    }
}

void CAdminOP :: LoadRank()
{
    char gamedir[256];
    FILE *fp;
    char filepath[512];

    memset(&ranks, 0, sizeof(ranks));
    engine->GetGameDir(gamedir, sizeof(gamedir));

    Q_snprintf(filepath, sizeof(filepath), "%s/%s/DF_ranks.txt", gamedir, DataDir());
    V_FixSlashes(filepath);
    fp = fopen(filepath, "rt");

    if(fp)
    {
        char tmp[256];
        while(!feof(fp))
        {
            if(fgets(tmp,sizeof(tmp),fp))
            {
                if ((strncmp(tmp, "#", 1) != 0) && (strncmp(tmp, "//", 2) != 0) && (strncmp(tmp, "\n", 1) != 0) && (tmp[0] != 0x0D) && (tmp[0] != 0x0A) && (strlen(tmp) != 0))
                {
                    char seps[] = ":";
                    char *token;

                    if(!strstr(tmp, ":") || ranks.ranks == MAX_RANKS)
                    {
                        if(fp) fclose(fp);
                        Q_snprintf(ranks.finalname, sizeof(ranks.finalname), tmp);
                        return;
                    }
                    token = strtok( tmp, seps );
                    ranks.creditmatch[ranks.ranks] = atoi(token);
                    token = strtok( NULL, seps );
                    if(token)
                    {
                        Q_snprintf(ranks.rankname[ranks.ranks], sizeof(ranks.rankname[0]), token);
                        for(int i = 0;i < sizeof(ranks.rankname[0]); i++)
                        {
                            if(ranks.rankname[ranks.ranks][i] == 0x0D || ranks.rankname[ranks.ranks][i] == 0x0A)
                            {
                                ranks.rankname[ranks.ranks][i] = '\0';
                                break;
                            }
                        }
                    }

                    ranks.ranks++;
                }
            }
        }
        if(fp) fclose(fp);
    }
}

const char *CAdminOP :: GetRank(int credits)
{
    for(int i = 0; i < ranks.ranks; i++)
    {
        if(credits <= ranks.creditmatch[i])
        {
            return ranks.rankname[i];
        }
    }

    return ranks.finalname;
}

void CAdminOP :: LoadRadioLoops()
{
    FILE *fp;
    char filepath[512];

    radioLoops.Purge();

    Q_snprintf(filepath, sizeof(filepath), "%s/%s/DF_radio.txt", GameDir(), DataDir());
    V_FixSlashes(filepath);
    fp = fopen(filepath, "rt");

    if(fp)
    {
        bool setFile = 0;
        bool setName = 0;
        bool readingBlock = 0;
        char tmp[256];
        char key[256];
        char value[256];
        radioloop_t newLoop;

        while(!feof(fp))
        {
            if(fgets(tmp,sizeof(tmp),fp))
            {
                if ((strncmp(tmp, "#", 1) != 0) && (strncmp(tmp, "//", 2) != 0) && (strncmp(tmp, "\n", 1) != 0) && (tmp[0] != 0x0D) && (tmp[0] != 0x0A) && (strlen(tmp) != 0))
                {
                    char *pszTmp;
                    char firstchar;

                    pszTmp = strtrim(tmp);
                    firstchar = pszTmp[0];
                    if(readingBlock == 1 && firstchar != '}')
                    {
                        unsigned int eql = 0;
                        for(eql = 0; eql < strlen(tmp); eql++)
                        {
                            if(tmp[eql] == '=')
                            {
                                break;
                            }
                        }
                        if(eql)
                        {
                            tmp[eql] = '\0';
                            pszTmp = strtrim(tmp);
                            strcpy(key, pszTmp);
                            pszTmp = strtrim(&tmp[eql+1]);
                            strcpy(value, pszTmp);
                            if(!stricmp(key, "File"))
                            {
                                strncpy(newLoop.File, value, sizeof(newLoop.File));
                                newLoop.File[sizeof(newLoop.File)-1] = '\0';
                                setFile = 1;
                            }
                            else if(!stricmp(key, "Name"))
                            {
                                strncpy(newLoop.Name, value, sizeof(newLoop.Name));
                                newLoop.Name[sizeof(newLoop.Name)-1] = '\0';
                                setName = 1;
                            }
                            else if(!stricmp(key, "ShortName"))
                            {
                                strncpy(newLoop.ShortName, value, sizeof(newLoop.ShortName));
                                newLoop.ShortName[sizeof(newLoop.ShortName)-1] = '\0';
                            }
                            else if(!stricmp(key, "Pitch")) newLoop.Pitch = clamp(atoi(value), 1, 255);
                            else if(!stricmp(key, "Volume")) newLoop.Pitch = clamp(atoi(value), 0, 10);
                            else if(!stricmp(key, "Dynamics")) newLoop.Pitch = clamp(atoi(value), 0, 27);
                            else if(!stricmp(key, "NotLooped")) newLoop.Pitch = clamp(atoi(value), 0, 1);
                            else if(!stricmp(key, "Radius")) newLoop.Pitch = clamp(atoi(value), 1, 3);
                            else
                            {
                                Msg("[SOURCEOP] Found a line with unknown key name \"%s\" inside a block while parsing DF_radio.txt.\n", key);
                                TimeLog("SourceOPErrors.log", "Found a line with unknown key name \"%s\" inside a block while parsing DF_radio.txt.\n", key);
                            }
                        }
                        else
                        {
                            Msg("[SOURCEOP] Found a line with no '=' inside a block while parsing DF_radio.txt.\n");
                            TimeLog("SourceOPErrors.log", "Found a line with no '=' inside a block while parsing DF_radio.txt.\n");
                        }
                    }
                    else if(firstchar == '{')
                    {
                        memset(&newLoop, 0, sizeof(newLoop));
                        newLoop.Pitch = 100;
                        newLoop.Volume = 10;
                        newLoop.Radius = 2;
                        sprintf(newLoop.ShortName, "%i", radioLoops.Count()+1);
                        setFile = 0;
                        setName = 0;
                        readingBlock = 1;
                    }
                    else if(firstchar == '}')
                    {
                        if(setFile)
                        {
                            if(!setName)
                            {
                                strncpy(newLoop.Name, newLoop.File, sizeof(newLoop.Name));
                                newLoop.Name[sizeof(newLoop.Name)-1] = '\0';
                            }
                            radioLoops.AddToTail(newLoop);
                        }
                        else
                        {
                            Msg("[SOURCEOP] Found a block in DF_radio.txt with no file set.\n");
                            TimeLog("SourceOPErrors.log", "Found a block in DF_radio.txt with no file set.\n");
                        }
                        readingBlock = 0;
                    }
                    else
                    {
                        Msg("[SOURCEOP] Found a string outside a block while parsing DF_radio.txt.\n");
                        TimeLog("SourceOPErrors.log", "Found a string outside a block while parsing DF_radio.txt.\n");
                    }
                }
            }
        }
        fclose(fp);
    }
}

bool CAdminOP :: MapVoteInProgress()
{
    return mapVote.VoteInProgress();
}

void CAdminOP :: ColorMsg(int color, const char *pszMsg, ...)
{
    va_list         argptr;
    static char     string[1024];
    static char     stringsafe[1024];
    
    va_start (argptr, pszMsg);
    Q_vsnprintf(string, sizeof(string), pszMsg,argptr);
    va_end (argptr);

    V_StrSubst(string, "%", "%%", stringsafe, sizeof(stringsafe), true);

#ifdef WIN32
    HANDLE h;
    if(g_IgnoreColorMessages <= 0)
    {
        h = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(h,color);
    }
 #else
    if(g_IgnoreColorMessages <= 0 && color >= 0 && color < 16)
        Msg("\033[%s", linConColors[color]);
#endif
    Msg(stringsafe);
#ifdef WIN32
    if(g_IgnoreColorMessages <= 0)
        SetConsoleTextAttribute(h,FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
#else
    if(g_IgnoreColorMessages <= 0)
        Msg("\033[0;37m");
#endif
}

void CAdminOP :: SayTextAll(const char *pText)
{
    SayTextAll(pText, HUD_PRINTTALK, 0);
}

void CAdminOP :: SayTextAll(const char *pText, int type, bool sendClient, int teamFilter)
{
    bf_write *pWrite;
    CRecipientFilter *filter = NULL;
    CReliableBroadcastRecipientFilter broadcastfilter;
    CRecipientFilter standardfilter;
    if(teamFilter == -1)
    {
        broadcastfilter.AddAllPlayers();
        filter = &broadcastfilter;
    }
    else
    {
        filter = &standardfilter;
        for(int i = 0; i < GetMaxClients(); i++)
        {
            if(pAOPPlayers[i].GetPlayerState() == 2 && pAOPPlayers[i].GetTeam() == teamFilter)
            {
                filter->AddRecipient(i+1);
            }
        }
    }

    if(type != HUD_PRINTTALK)
    {
        pWrite = engine->UserMessageBegin(filter, GetMessageByName("TextMsg"));
        pWrite->WriteByte(type);
        pWrite->WriteString(pText);
        engine->MessageEnd();
    }
    else
    {
        int messageNumber = GetMessageByName("SayText2");
        if(messageNumber == -1)
        {
            messageNumber = GetMessageByName("SayText");
            pWrite = engine->UserMessageBegin(filter, messageNumber);
                pWrite->WriteByte(0);
                pWrite->WriteString(pText);
                pWrite->WriteByte(0);
            engine->MessageEnd();
        }
        else
        {
            char realText[512];
            V_snprintf(realText, sizeof(realText), "\x01%s", pText);
            pWrite = engine->UserMessageBegin(filter, messageNumber);
                pWrite->WriteByte(m_iSayTextPlayerIndex);
                pWrite->WriteByte(1);
                pWrite->WriteString(realText);
            engine->MessageEnd();
            SetSayTextPlayerIndex(0);
        }
    }

    if(type == HUD_PRINTTALK && sendClient)
    {
        char sendText[512];
        V_strncpy(sendText, pText, sizeof(sendText));
        int sendTextLen = strlen(sendText);

        if(sendTextLen && sendText[sendTextLen-1] == '\n')
            sendText[sendTextLen-1] = '\0';

        if(remoteserver) remoteserver->GameChat(m_iSayTextPlayerIndex, sendText);
    }
}

void CAdminOP :: SayTextAllChatHud(const char *pText, bool sendClient)
{
    SayTextAll(pText, HUD_PRINTTALK, sendClient);
    //SayTextAll(pText, HUD_PRINTCONSOLE, 0);
}

void CAdminOP :: HintTextAll(const char *pText)
{
    bf_write *pWrite;
    CReliableBroadcastRecipientFilter filter;
    filter.AddAllPlayers();

    pWrite = engine->UserMessageBegin(&filter, GetMessageByName("HintText"));
        //pWrite->WriteByte(1);
        pWrite->WriteString(pText);
    engine->MessageEnd();
}

void CAdminOP :: PlaySoundAll(const char *sound)
{
    for(int i = 1; i <= maxClients; i++)
    {
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pEList+i);
        if(info)
        {
            if(info->IsConnected())
            {
                pAOPPlayers[i-1].PlaySoundLocal(sound);
            }
        }
    }
}

void CAdminOP :: TimeLog(char * File, char * Text, ...)
{
    FILE *fp;
    char gamedir[256];
    char filepath[512];

    engine->GetGameDir(gamedir, sizeof(gamedir));

    Q_snprintf(filepath, sizeof(filepath), "%s/%s/%s", gamedir, DataDir(), File);
    V_FixSlashes(filepath);
    fp = fopen(filepath, "a");

    if(fp)
    {
        va_list         argptr;
        static char             string[1024];

        va_start (argptr, Text);
        vsprintf (string, Text,argptr);
        va_end (argptr);

        struct tm timestruct;
        g_pVCR->Hook_LocalTime(&timestruct);
        int hours_ampm = timestruct.tm_hour % 12;
        const char * str_ampm = (timestruct.tm_hour >= 0 && timestruct.tm_hour < 12) ? "AM" : "PM";
        /*char hours[80];
        char minutes[80];
        strcpy(hours, NumbersToWords(hours_ampm));
        strcpy(minutes, NumbersToWords(timestruct->tm_min));*/
        if ( hours_ampm == 0 )
            hours_ampm = 12;
        /*if(timestruct->tm_min == 0)
            strcpy(minutes, " ");*/

        fputs(UTIL_VarArgs("[%02d/%02d/%02d %02d:%02d:%02d %s] %s", timestruct.tm_mon+1, timestruct.tm_mday, timestruct.tm_year % 100, hours_ampm, timestruct.tm_min, timestruct.tm_sec, str_ampm, string), fp); //The % 100 in the year is to avoid output of a year like 2003 as 103
        fclose(fp);
    }
}

void CAdminOP :: DebugLog(char *Text)
{
    FILE *fp;
    static bool filepathfilled = false;
    static char filepath[512];

    if(!filepathfilled)
    {
        Q_snprintf(filepath, sizeof(filepath), "%s/%s/debuglog.txt", gameDir, DataDir());
        V_FixSlashes(filepath);
        filepathfilled = true;
    }
    fp = fopen(filepath, "a");

    if(fp)
    {
        static char fullstring[1280];
        struct tm timestruct;

        g_pVCR->Hook_LocalTime(&timestruct);
        int hours_ampm = timestruct.tm_hour % 12;
        const char * str_ampm = (timestruct.tm_hour >= 0 && timestruct.tm_hour < 12) ? "AM" : "PM";
        if ( hours_ampm == 0 )
            hours_ampm = 12;

        V_snprintf(fullstring, sizeof(fullstring), "[%02d/%02d/%02d %02d:%02d:%02d %s] %s", timestruct.tm_mon+1, timestruct.tm_mday, timestruct.tm_year % 100, hours_ampm, timestruct.tm_min, timestruct.tm_sec, str_ampm, Text);
        fputs(fullstring, fp);
        fclose(fp);
    }
}

void CAdminOP :: LockAdminTut(void)
{
    char gamedir[256];
    char filepath[512];
    FILE *fp;

    engine->GetGameDir(gamedir, sizeof(gamedir));

    Q_snprintf(filepath, sizeof(filepath), "%s/%s/admintut_lock.txt", gamedir, DataDir());
    V_FixSlashes(filepath);
    fp = fopen(filepath, "wt");

    if(fp)
    {
        fputs("This file locks the admin tutorial from being used again. Don't delete unless you want access back to the tutorial.\n", fp);
        fclose(fp);
    }
}

CBaseEntity *CAdminOP :: CreateTurret(const char *name, unsigned int team, Vector origin, QAngle angles, CAdminOPPlayer *pAOPPlayer)
{
    Vector forward, right, up;
    AngleVectors( angles, &forward, &right, &up );

    CBaseEntity *pTurret = CreateEntityByName( "npc_turret_floor" );
    if(pTurret)
    {
        VFuncs::KeyValue(pTurret,  "origin", UTIL_VarArgs("%f %f %f", origin.x, origin.y, origin.z) );
        VFuncs::KeyValue(pTurret,  "targetname", UTIL_VarArgs("%s", name) );
        VFuncs::KeyValue(pTurret,  "spawnflags", "128" );
        VFuncs::KeyValue(pTurret,  "angles", UTIL_VarArgs("%i %i %i", (int)angles.x, (int)angles.y, (int)angles.z) );
        VFuncs::KeyValue(pTurret,  "classname", "npc_turret_floor" );
        VFuncs::KeyValue(pTurret,  "OnDeploy", UTIL_VarArgs("%sh,Enable,,0,-1", name) );
        VFuncs::KeyValue(pTurret,  "OnTipped", UTIL_VarArgs("%sh,Disable,,0,-1", name) );
        VFuncs::KeyValue(pTurret,  "OnPhysGunDrop", UTIL_VarArgs("%sr,SetValue,1,0,-1", name) );
        VFuncs::KeyValue(pTurret,  "OnPhysGunPickup", UTIL_VarArgs("%sr,SetValue,0,0,-1", name) );
        VFuncs::KeyValue(pTurret,  "OnPhysGunPickup", UTIL_VarArgs("%stm,Disable,,0,-1", name) );
        VFuncs::KeyValue(pTurret,  "OnPhysGunDrop", UTIL_VarArgs("%stm,Enable,,0,-1", name) );
        VFuncs::KeyValue(pTurret,  "OnTipped", UTIL_VarArgs("%stm,Disable,,0,-1", name) );
        VFuncs::KeyValue(pTurret,  "OnTipped", UTIL_VarArgs("%ss,HideSprite,,3,-1", name) );
        VFuncs::KeyValue(pTurret,  "OnPhysGunDrop", UTIL_VarArgs("%ss,ShowSprite,,3,-1", name) );
        VFuncs::KeyValue(pTurret,  "OnPhysGunPickup", UTIL_VarArgs("%ss,HideSprite,,3,-1", name) );
        VFuncs::KeyValue(pTurret,  "OnPhysGunPickup", UTIL_VarArgs("%sg,Disable,,0,-1", name) );
        VFuncs::KeyValue(pTurret,  "OnPhysGunPickup", UTIL_VarArgs("%sl,TurnOff,,0,-1", name) );
        VFuncs::Spawn(pTurret);
        VFuncs::Activate(pTurret);
        VFuncs::Teleport(pTurret,  &origin, &angles, NULL );
        if(pAOPPlayer)
        {
            pAOPPlayer->spawnedEnts.AddToTail(VFuncs::entindex(pTurret));
            spawnedServerEnts.AddToTail(VFuncs::entindex(pTurret));
        }
    }

    CBaseEntity *pTrigger1 = CreateEntityByName("trigger_multiple");
    if(pTrigger1)
    {
        Vector trigorigin = origin + forward * 267 + (up * 45);

        VFuncs::KeyValue(pTrigger1,  "model", "models/props_wasteland/coolingtank02.mdl" );
        VFuncs::KeyValue(pTrigger1,  "wait", "1" );
        VFuncs::KeyValue(pTrigger1,  "origin", UTIL_VarArgs("%f %f %f", trigorigin.x, trigorigin.y, trigorigin.z) );
        VFuncs::KeyValue(pTrigger1,  "angles", UTIL_VarArgs("%f %f %f", angles.x+22, angles.y+90, angles.z+270) );
        VFuncs::KeyValue(pTrigger1,  "spawnflags", "1" );
        VFuncs::KeyValue(pTrigger1,  "StartDisabled", "1" );
        VFuncs::KeyValue(pTrigger1,  "targetname", UTIL_VarArgs("%sh", name) );
        VFuncs::KeyValue(pTrigger1,  "parentname", UTIL_VarArgs("%s", name) );
        VFuncs::KeyValue(pTrigger1,  "classname", "trigger_multiple" );
        VFuncs::KeyValue(pTrigger1,  "OnEndTouch", UTIL_VarArgs("%sho,TestActivator,,0.1,-1", name) );
        VFuncs::KeyValue(pTrigger1,  "OnStartTouch", UTIL_VarArgs("%shi,TestActivator,,0,-1", name) );
        VFuncs::SetParent(pTrigger1, pTurret);

        VFuncs::Spawn(pTrigger1);
        VFuncs::Activate(pTrigger1);
        VFuncs::SetModel( pTrigger1, "models/props_wasteland/coolingtank02.mdl" );

        if(pAOPPlayer)
        {
            pAOPPlayer->spawnedEnts.AddToTail(VFuncs::entindex(pTrigger1));
            spawnedServerEnts.AddToTail(VFuncs::entindex(pTrigger1));
        }
    }

    /*CBaseEntity *pTrigger = CreateEntityByName("trigger_multiple");
    if(pTrigger)
    {
        Vector trigorigin = origin + (up * 31.5);

        VFuncs::KeyValue(//pTrigger,  "model", "models/props_c17/oildrum001_explosive.mdl" );
        VFuncs::KeyValue(pTrigger,  "model", "*2" );
        VFuncs::KeyValue(pTrigger,  "wait", "1" );
        VFuncs::KeyValue(pTrigger,  "origin", UTIL_VarArgs("%f %f %f", trigorigin.x, trigorigin.y, trigorigin.z) );
        VFuncs::KeyValue(pTrigger,  "angles", UTIL_VarArgs("%f %f %f", angles.x, angles.y-270, angles.z) );
        VFuncs::KeyValue(pTrigger,  "spawnflags", "1" );
        VFuncs::KeyValue(pTrigger,  "StartDisabled", "1" );
        VFuncs::KeyValue(pTrigger,  "targetname", UTIL_VarArgs("%sh", name) );
        VFuncs::KeyValue(pTrigger,  "parentname", UTIL_VarArgs("%s", name) );
        VFuncs::KeyValue(pTrigger,  "classname", "trigger_multiple" );
        VFuncs::KeyValue(pTrigger,  "OnEndTouch", UTIL_VarArgs("%sho,TestActivator,,0.1,-1", name) );
        VFuncs::KeyValue(pTrigger,  "OnStartTouch", UTIL_VarArgs("%shi,TestActivator,,0,-1", name) );
        VFuncs::SetParent(pTrigger, pTurret);

        VFuncs::Spawn(pTrigger);
        VFuncs::Activate(pTrigger);
        //pTrigger->SetModel( "models/props_c17/oildrum001_explosive.mdl" );
        pTrigger->SetModel( "*2" );

        //variant_t value;
        //value.SetString( MAKE_STRING(UTIL_VarArgs("OnEndTouch %sho,TestActivator,,0.1,-1", name)) );
        //VFuncs::AcceptInput(pTrigger, "AddOutput", pTrigger, pTrigger, value, 0 );
        //value.SetString( MAKE_STRING(UTIL_VarArgs("OnStartTouch %shi,TestActivator,,0,-1", name)) );
        //VFuncs::AcceptInput(pTrigger, "AddOutput", pTrigger, pTrigger, value, 0 );

        if(VFuncs::GetEFlags(pTrigger) & EFL_KILLME)
        {
            SayTextAll(UTIL_VarArgs("[%s] Spawn failed.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
        
        }
        if(pAOPPlayer)
        {
            pAOPPlayer->spawnedEnts.AddToTail(VFuncs::entindex(pTrigger));
            spawnedServerEnts.AddToTail(VFuncs::entindex(pTrigger));
        }
    }
    else
    {
        SayTextAll(UTIL_VarArgs("[%s] Spawn failed2.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
    }*/

    CBaseEntity *pGunTarget = CreateEntityByName("info_target");
    if(pGunTarget)
    {
        Vector gtorigin = origin + (forward * 48) + (up * 55);
        VFuncs::KeyValue(pGunTarget,  "origin", UTIL_VarArgs("%f %f %f", gtorigin.x, gtorigin.y, gtorigin.z) );
        VFuncs::KeyValue(pGunTarget,  "parentname", UTIL_VarArgs("%s", name) );
        VFuncs::KeyValue(pGunTarget,  "targetname", UTIL_VarArgs("%sgt", name) );
        VFuncs::KeyValue(pGunTarget,  "classname", "info_target" );
        VFuncs::SetParent(pGunTarget, pTurret);
        VFuncs::Spawn(pGunTarget);
        VFuncs::Activate(pGunTarget);
        VFuncs::SetParent(pGunTarget, pTurret);
        if(pAOPPlayer)
        {
            pAOPPlayer->spawnedEnts.AddToTail(VFuncs::entindex(pGunTarget));
            spawnedServerEnts.AddToTail(VFuncs::entindex(pGunTarget));
        }
    }

    CBaseEntity *pLaserTarget = CreateEntityByName("info_target");
    if(pLaserTarget)
    {
        Vector ltorigin = origin + (forward * /*392*/494) + (up * 55);
        VFuncs::KeyValue(pLaserTarget,  "origin", UTIL_VarArgs("%f %f %f", ltorigin.x, ltorigin.y, ltorigin.z) );
        VFuncs::KeyValue(pLaserTarget,  "parentname", UTIL_VarArgs("%s", name) );
        VFuncs::KeyValue(pLaserTarget,  "targetname", UTIL_VarArgs("%slt", name) );
        VFuncs::KeyValue(pLaserTarget,  "classname", "info_target" );
        VFuncs::SetParent(pLaserTarget, pTurret);
        VFuncs::Spawn(pLaserTarget);
        VFuncs::Activate(pLaserTarget);
        VFuncs::SetParent(pLaserTarget, pTurret);
        if(pAOPPlayer)
        {
            pAOPPlayer->spawnedEnts.AddToTail(VFuncs::entindex(pLaserTarget));
            spawnedServerEnts.AddToTail(VFuncs::entindex(pLaserTarget));
        }
    }

    CBaseEntity *pEnvGun = CreateEntityByName( "env_gunfire" );
    if(pEnvGun)
    {
        Vector envorigin = origin + (forward * 16) + (up * 55);
        VFuncs::KeyValue(pEnvGun,  "origin", UTIL_VarArgs("%f %f %f", envorigin.x, envorigin.y, envorigin.z) );
        VFuncs::KeyValue(pEnvGun,  "target", UTIL_VarArgs("%sgt", name) );
        VFuncs::KeyValue(pEnvGun,  "tracertype", "AR2TRACER" );
        VFuncs::KeyValue(pEnvGun,  "shootsound", "Weapon_AR2.NPC_Single" );
        VFuncs::KeyValue(pEnvGun,  "collisions", "1" );
        VFuncs::KeyValue(pEnvGun,  "bias", "1" );
        VFuncs::KeyValue(pEnvGun,  "spread", "10" );
        VFuncs::KeyValue(pEnvGun,  "rateoffire", "10" );
        VFuncs::KeyValue(pEnvGun,  "maxburstdelay", "0" );
        VFuncs::KeyValue(pEnvGun,  "minburstdelay", "0" );
        VFuncs::KeyValue(pEnvGun,  "maxburstsize", "10" );
        VFuncs::KeyValue(pEnvGun,  "minburstsize", "10" );
        VFuncs::KeyValue(pEnvGun,  "StartDisabled", "1" );
        VFuncs::KeyValue(pEnvGun,  "targetname", UTIL_VarArgs("%sg", name) );
        VFuncs::KeyValue(pEnvGun,  "parentname", UTIL_VarArgs("%s", name) );
        VFuncs::KeyValue(pEnvGun,  "classname", "env_gunfire" );
        VFuncs::Teleport(pEnvGun,  &envorigin, NULL, NULL );
        VFuncs::SetParent(pEnvGun, pTurret);
        VFuncs::Spawn(pEnvGun);
        VFuncs::Activate(pEnvGun);
        if(pAOPPlayer)
        {
            pAOPPlayer->spawnedEnts.AddToTail(VFuncs::entindex(pEnvGun));
            spawnedServerEnts.AddToTail(VFuncs::entindex(pEnvGun));
        }
    }

    CBaseEntity *pLogic = CreateEntityByName("logic_branch");
    if(pLogic)
    {
        VFuncs::KeyValue(pLogic,  "origin", UTIL_VarArgs("%i %i %i", (int)origin.x, (int)origin.y, (int)origin.z) );
        VFuncs::KeyValue(pLogic,  "InitialValue", "1" );
        VFuncs::KeyValue(pLogic,  "targetname", UTIL_VarArgs("%sr", name) );
        VFuncs::KeyValue(pLogic,  "classname", "logic_branch" );
        VFuncs::KeyValue(pLogic,  "OnTrue", UTIL_VarArgs("%sg,Enable,,0,-1", name) );
        VFuncs::KeyValue(pLogic,  "OnTrue", UTIL_VarArgs("%sl,TurnOn,,0,-1", name) );
        VFuncs::Spawn(pLogic);
        VFuncs::Activate(pLogic);
        VFuncs::Teleport(pLogic,  &origin, NULL, NULL );
        /*if(pAOPPlayer)
        {
            pAOPPlayer->spawnedEnts.AddToTail(VFuncs::entindex(pLogic));
            spawnedServerEnts.AddToTail(VFuncs::entindex(pLogic));
        }*/
    }

    CBaseEntity *pLaser = CreateEntityByName("env_laser");
    if(pLaser)
    {
        Vector laserorigin = origin + (forward * 48) + (up * 55);
        VFuncs::KeyValue(pLaser,  "origin", UTIL_VarArgs("%f %f %f", laserorigin.x, laserorigin.y, laserorigin.z) );
        VFuncs::KeyValue(pLaser,  "parentname", UTIL_VarArgs("%s", name) );
        VFuncs::KeyValue(pLaser,  "LaserTarget", UTIL_VarArgs("%slt", name) );
        VFuncs::KeyValue(pLaser,  "disolvetype", "-1" );
        VFuncs::KeyValue(pLaser,  "damage", "400" );
        VFuncs::KeyValue(pLaser,  "framestart", "0" );
        VFuncs::KeyValue(pLaser,  "TextureScroll", "35" );
        VFuncs::KeyValue(pLaser,  "texture", "sprites/laserbeam.spr" );
        VFuncs::KeyValue(pLaser,  "NoiseAmplitude", "0" );
        VFuncs::KeyValue(pLaser,  "width", "128" );
        VFuncs::KeyValue(pLaser,  "rendercolor", "0 0 0" );
        VFuncs::KeyValue(pLaser,  "renderamt", "1" );
        VFuncs::KeyValue(pLaser,  "renderfx", "0" );
        VFuncs::KeyValue(pLaser,  "spawnflags", "0" );
        VFuncs::KeyValue(pLaser,  "targetname", UTIL_VarArgs("%sl", name) );
        VFuncs::KeyValue(pLaser,  "classname", "env_laser" );
        VFuncs::Teleport(pLaser,  &laserorigin, NULL, NULL );
        VFuncs::SetParent(pLaser, pTurret);
        VFuncs::Spawn(pLaser);
        VFuncs::Activate(pLaser);
        //VFuncs::Teleport(pLaser,  &origin, NULL, NULL );
        if(pAOPPlayer)
        {
            pAOPPlayer->spawnedEnts.AddToTail(VFuncs::entindex(pLaser));
            spawnedServerEnts.AddToTail(VFuncs::entindex(pLaser));
        }
    }

    CBaseEntity *pSound = CreateEntityByName("ambient_generic");
    if(pSound)
    {
        VFuncs::KeyValue(pSound,  "origin", UTIL_VarArgs("%i %i %i", (int)origin.x, (int)origin.y, (int)origin.z) );
        VFuncs::KeyValue(pSound,  "SourceEntityName", UTIL_VarArgs("%s", name) );
        VFuncs::KeyValue(pSound,  "message", "npc/turret_floor/ping.wav" );
        VFuncs::KeyValue(pSound,  "targetname", UTIL_VarArgs("%ssp", name) );
        VFuncs::KeyValue(pSound,  "spawnflags", "48" );
        VFuncs::KeyValue(pSound,  "pitchstart", "100" );
        VFuncs::KeyValue(pSound,  "pitch", "100" );
        VFuncs::KeyValue(pSound,  "health", "10" );
        VFuncs::KeyValue(pSound,  "radius", "256" );
        VFuncs::KeyValue(pSound,  "classname", "ambient_generic" );
        VFuncs::Spawn(pSound);
        VFuncs::Activate(pSound);
        VFuncs::Teleport(pSound,  &origin, NULL, NULL );
        if(pAOPPlayer)
        {
            pAOPPlayer->spawnedEnts.AddToTail(VFuncs::entindex(pSound));
            spawnedServerEnts.AddToTail(VFuncs::entindex(pSound));
        }
    }

    CBaseEntity *pTimer = CreateEntityByName("logic_timer");
    if(pTimer)
    {
        VFuncs::KeyValue(pTimer,  "origin", UTIL_VarArgs("%i %i %i", (int)origin.x, (int)origin.y, (int)origin.z) );
        VFuncs::KeyValue(pTimer,  "RefireTime", "1.2" );
        VFuncs::KeyValue(pTimer,  "StartDisabled", "1" );
        VFuncs::KeyValue(pTimer,  "targetname", UTIL_VarArgs("%stm", name) );
        VFuncs::KeyValue(pTimer,  "classname", "logic_timer" );
        VFuncs::KeyValue(pTimer,  "OnTimer", UTIL_VarArgs("%ssp,StopSound,,2,-1", name) );
        VFuncs::KeyValue(pTimer,  "OnTimer", UTIL_VarArgs("%ssp,PlaySound,,2.1,-1", name) );
        VFuncs::Spawn(pTimer);
        VFuncs::Activate(pTimer);
        VFuncs::Teleport(pTimer,  &origin, NULL, NULL );
        /*if(pAOPPlayer)
        {
            pAOPPlayer->spawnedEnts.AddToTail(VFuncs::entindex(pTimer));
            spawnedServerEnts.AddToTail(VFuncs::entindex(pTimer));
        }*/
    }

    CBaseEntity *pSprite = CreateEntityByName("env_sprite");
    if(pSprite)
    {
        Vector spriteorigin = origin + (right * 2.47) + (forward * 7.63) + (up * 58.609);
        Msg("%f %f %f\n", spriteorigin.x, spriteorigin.y, spriteorigin.z);
        VFuncs::KeyValue(pSprite,  "origin", UTIL_VarArgs("%f %f %f", spriteorigin.x, spriteorigin.y, spriteorigin.z) );
        VFuncs::KeyValue(pSprite,  "HDRColorScale", "1.0" );
        VFuncs::KeyValue(pSprite,  "maxdxlevel", "0" );
        VFuncs::KeyValue(pSprite,  "mindxlevel", "0" );
        VFuncs::KeyValue(pSprite,  "disablereceiveshadows", "1" );
        VFuncs::KeyValue(pSprite,  "spawnflags", "0" );
        VFuncs::KeyValue(pSprite,  "targetname", UTIL_VarArgs("%ss", name) );
        VFuncs::KeyValue(pSprite,  "parentname", UTIL_VarArgs("%s", name) );
        VFuncs::KeyValue(pSprite,  "scale", "0.1" );
        VFuncs::KeyValue(pSprite,  "renderfx", "3" );
        VFuncs::KeyValue(pSprite,  "rendermode", "5" );
        VFuncs::KeyValue(pSprite,  "GlowProxySize", "1" );
        VFuncs::KeyValue(pSprite,  "model", "sprites/glow01.spr" );
        VFuncs::KeyValue(pSprite,  "framerate", "10.0" );
        if(team == 2)
            VFuncs::KeyValue(pSprite,  "rendercolor", "255 0 64" );
        else if(team == 3)
            VFuncs::KeyValue(pSprite,  "rendercolor", "0 192 255" );
        else
            VFuncs::KeyValue(pSprite,  "rendercolor", "255 128 64" );
        VFuncs::KeyValue(pSprite,  "renderamt", "255" );
        VFuncs::KeyValue(pSprite,  "classname", "env_sprite" );
        VFuncs::SetParent(pSprite, pTurret);
        VFuncs::Spawn(pSprite);
        VFuncs::Activate(pSprite);
        if(pAOPPlayer)
        {
            pAOPPlayer->spawnedEnts.AddToTail(VFuncs::entindex(pSprite));
            spawnedServerEnts.AddToTail(VFuncs::entindex(pSprite));
        }
    }

    CBaseEntity *pFilter1 = CreateEntityByName("filter_activator_team");
    if(pFilter1)
    {
        VFuncs::KeyValue(pFilter1,  "origin", UTIL_VarArgs("%i %i %i", (int)origin.x, (int)origin.y, (int)origin.z) );
        VFuncs::KeyValue(pFilter1,  "targetname", UTIL_VarArgs("%shi", name) );
        VFuncs::KeyValue(pFilter1,  "filterteam", UTIL_VarArgs("%i", team) );
        VFuncs::KeyValue(pFilter1,  "Negated", "Allow entities that match criteria" );
        VFuncs::KeyValue(pFilter1,  "classname", "filter_activator_team" );
        VFuncs::KeyValue(pFilter1,  "OnPass", UTIL_VarArgs("%sr,Test,,0,-1", name) );
        VFuncs::Spawn(pFilter1);
        VFuncs::Activate(pFilter1);
        VFuncs::Teleport(pFilter1,  &origin, NULL, NULL );
        /*if(pAOPPlayer)
        {
            pAOPPlayer->spawnedEnts.AddToTail(VFuncs::entindex(pFilter1));
            spawnedServerEnts.AddToTail(VFuncs::entindex(pFilter1));
        }*/
    }

    CBaseEntity *pFilter2 = CreateEntityByName("filter_activator_team");
    if(pFilter2)
    {
        VFuncs::KeyValue(pFilter2,  "origin", UTIL_VarArgs("%i %i %i", (int)origin.x, (int)origin.y, (int)origin.z) );
        VFuncs::KeyValue(pFilter2,  "targetname", UTIL_VarArgs("%sho", name) );
        VFuncs::KeyValue(pFilter2,  "filterteam", UTIL_VarArgs("%i", team) );
        VFuncs::KeyValue(pFilter2,  "Negated", "Allow entities that match criteria" );
        VFuncs::KeyValue(pFilter2,  "classname", "filter_activator_team" );
        VFuncs::KeyValue(pFilter2,  "OnPass", UTIL_VarArgs("%sl,TurnOff,,0,-1", name) );
        VFuncs::KeyValue(pFilter2,  "OnPass", UTIL_VarArgs("%sg,Disable,,0,-1", name) );
        VFuncs::Spawn(pFilter2);
        VFuncs::Activate(pFilter2);
        VFuncs::Teleport(pFilter2,  &origin, NULL, NULL );
        /*if(pAOPPlayer)
        {
            pAOPPlayer->spawnedEnts.AddToTail(VFuncs::entindex(pFilter2));
            spawnedServerEnts.AddToTail(VFuncs::entindex(pFilter2));
        }*/
    }

    CBaseEntity *pOverride = CreateEntityByName("prop_dynamic_override");
    if(pOverride)
    {
        VFuncs::KeyValue(pOverride,  "origin", UTIL_VarArgs("%f %f %f", origin.x, origin.y, origin.z) );
        VFuncs::KeyValue(pOverride,  "disableshadows", "0" );
        VFuncs::KeyValue(pOverride,  "parentname", UTIL_VarArgs("%s", name) );
        VFuncs::KeyValue(pOverride,  "spawnflags", "prop_dynamic_override" );
        VFuncs::KeyValue(pOverride,  "skin", "0" );
        VFuncs::KeyValue(pOverride,  "model", "models/combine_turrets/floor_turret.mdl" );
        VFuncs::KeyValue(pOverride,  "fadescale", "1" );
        VFuncs::KeyValue(pOverride,  "fademindist", "-1" );
        VFuncs::KeyValue(pOverride,  "MaxAnimTime", "10" );
        VFuncs::KeyValue(pOverride,  "MinAnimTime", "5" );
        VFuncs::KeyValue(pOverride,  "solid", "0" );
        VFuncs::KeyValue(pOverride,  "angles", UTIL_VarArgs("%i %i %i", (int)angles.x, (int)angles.y, (int)angles.z) );
        VFuncs::KeyValue(pOverride,  "classname", "prop_dynamic_override" );
        VFuncs::SetParent(pOverride, pTurret);
        VFuncs::Spawn(pOverride);
        VFuncs::Activate(pOverride);
        if(pAOPPlayer)
        {
            pAOPPlayer->spawnedEnts.AddToTail(VFuncs::entindex(pOverride));
            spawnedServerEnts.AddToTail(VFuncs::entindex(pOverride));
        }
    }

    return pTurret;
}

CBaseEntity *CAdminOP :: CreateSpriteTrail(const char *pSpriteName, const Vector &origin, bool animate)
{
    CBaseEntity *pTrail = CreateEntityByName("env_spritetrail");
    if(pTrail)
    {
        VFuncs::SetModelName(pTrail, MAKE_STRING(pSpriteName));
        VFuncs::SetLocalOrigin(pTrail, origin);
        VFuncs::Spawn(pTrail);

        VFuncs::SetSolid(pTrail, SOLID_NONE);
        VFuncs::SetMoveType(pTrail, MOVETYPE_NOCLIP);

        UTIL_SetSize(pTrail, vec3_origin, vec3_origin);

        if(animate)
        {
            // do stuff
        }
    }
    return pTrail;
}

bool CAdminOP :: CvarVoteInProgress()
{
    return cvarVote.VoteInProgress();
}

void CAdminOP :: StartCvarVote(ConVar *cvar, const char *pszMessage, const char *choice1, int value1, const char *choice2, int value2, ConVar *ratio, ConVar* duration, CvarVoteCallback endcallback = NULL)
{
    cvarVoteCvar = cvar;
    cvarVoteValue1 = value1;
    cvarVoteValue2 = value2;
    strncpy(cvarVoteChoice1, choice1, sizeof(cvarVoteChoice1));
    strncpy(cvarVoteChoice2, choice2, sizeof(cvarVoteChoice2));
    cvarVoteRatio = ratio;
    cvarVoteEndCallback = endcallback;
    cvarVote.StartVote();
    cvarVote.endTime = engine->Time() + duration->GetFloat();

    KeyValues *kv = new KeyValues( "menu" );
    kv->SetString( "title", "Vote menu, hit ESC" );
    kv->SetInt( "level", 2 );
    kv->SetColor( "color", Color( 0, 255, 0, 255 ));
    kv->SetInt( "time", 20 );
    kv->SetString( "msg", pszMessage );
    KeyValues *item = kv->FindKey( "1", true );
    item->SetString( "msg", choice1 );
    item->SetString( "command", UTIL_VarArgs("sop_cvarvote %i;cancelselect", value1) );
    item = kv->FindKey( "2", true );
    item->SetString( "msg", choice2 );
    item->SetString( "command", UTIL_VarArgs("sop_cvarvote %i;cancelselect", value2) );
    for(int i = 1; i <= GetMaxClients(); i++)
    {
        if(pAOPPlayers[i-1].GetPlayerState() == 2)
        {
            helpers->CreateMessage( pEList+i, DIALOG_MENU, kv, &g_ServerPlugin );
        }
    }
    kv->deleteThis();
}

void CAdminOP :: AddPlayerToCvarVote(IPlayerInfo *playerinfo, int choice)
{
    if(!cvarVote.VoteInProgress())
        return;

    if(choice == cvarVoteValue1)
    {
        cvarVote.ChangeVote(cvarVoteChoice1, playerinfo->GetUserID());
        if(jetpackvote_printvoters.GetBool())
            pAdminOP.SayTextAllChatHud(UTIL_VarArgs("[%s] %s votes for %s.\n", pAdminOP.adminname, playerinfo->GetName(), cvarVoteChoice1));
    }
    else if(choice == cvarVoteValue2)
    {
        cvarVote.ChangeVote(cvarVoteChoice2, playerinfo->GetUserID());
        if(jetpackvote_printvoters.GetBool())
            pAdminOP.SayTextAllChatHud(UTIL_VarArgs("[%s] %s votes for %s.\n", pAdminOP.adminname, playerinfo->GetName(), cvarVoteChoice2));
    }
}

void CAdminOP :: PrintCvarVoteStandings()
{
    int maxvotes = (GetPlayerCountPlaying() * cvarVoteRatio->GetInt()) / 100;
    VoteCount_t winVote;
    winVote = cvarVote.GetTopChoice();

    if(winVote.votes != 0 && winVote.votes >= maxvotes)
    {
        SayTextAllChatHud(UTIL_VarArgs("[%s] %s is winning with %i vote%s\n", adminname, winVote.item, winVote.votes, winVote.votes != 1 ? "s" : ""));
    }
    else
    {
        SayTextAllChatHud(UTIL_VarArgs("[%s] No choice has the %i votes required yet.\n", adminname, maxvotes));
    }
}

void CAdminOP :: EndCvarVote()
{
    if(!cvarVote.VoteInProgress()) return;

    char msg[120];
    int maxvotes = (GetPlayerCountPlaying() * cvarVoteRatio->GetInt()) / 100;
    VoteCount_t topchoice = cvarVote.GetTopChoice();
    if(topchoice.votes != 0 && topchoice.votes >= maxvotes)
    {
        Q_snprintf(msg, 120, "[%s] CVAR voting is now over. %s won with %i votes.\n", adminname, topchoice.item, topchoice.votes);
        msg[119] = '\0';

        if(!strcmp(topchoice.item, cvarVoteChoice1))
        {
            cvarVoteCvar->SetValue(cvarVoteValue1);
        }
        else if(!strcmp(topchoice.item, cvarVoteChoice2))
        {
            cvarVoteCvar->SetValue(cvarVoteValue2);
        }
        else // should not happen
        {
            SayTextAllChatHud(UTIL_VarArgs("[%s] An unknown error occured on CVAR vote.\n", adminname));
        }
    }
    else
    {
        Q_snprintf(msg, 120, "[%s] CVAR voting is now over. No option got enough votes to win.\n", adminname);
        msg[119] = '\0';
    }

    SayTextAllChatHud(msg);
    if(cvarVoteEndCallback) cvarVoteEndCallback(this);
    cvarVote.EndVote();
}

void CAdminOP :: JetpackVoteFinish(CAdminOP *aop)
{
    aop->jetpackVoteNextTime = gpGlobals->curtime + jetpackvote_freq.GetFloat();
}

void CAdminOP :: LoadLua()
{
    luaState = luaopen();
    if(!luaState) return;

    // reset caches
    g_entityCache.InitCache();

    // globals
    lua_pushstring(luaState, "SOPVERSION");
    lua_pushstring(luaState, SourceOPVersion);
    lua_settable(luaState, LUA_GLOBALSINDEX);

    char gamedir[256];
    char filepath[512];
    engine->GetGameDir(gamedir, sizeof(gamedir));
    Q_snprintf(filepath, sizeof(filepath), "%s/%s/lua/includes/init.lua", gamedir, DataDir());
    V_FixSlashes(filepath);
    luascript(luaState, filepath);

    int numscripts;
    Q_snprintf(filepath, sizeof(filepath), "%s/%s/lua/autorun", gamedir, DataDir());
    numscripts = LoadLuaAutorunScripts(filepath);
    ColorMsg(CONCOLOR_LUA, "[SOURCEOP] \"Console<0><Console><Console>\" Loaded %i autorun scripts.\n", numscripts);

    Q_snprintf(filepath, sizeof(filepath), "%s/%s/lua/entities", gamedir, DataDir());
    numscripts = LoadLuaEntities(filepath);
    ColorMsg(CONCOLOR_LUA, "[SOURCEOP] \"Console<0><Console><Console>\" Loaded %i Lua entities.\n", numscripts);
}

int CAdminOP :: LoadLuaAutorunScripts(const char *path)
{
    int numscripts = 0;
    char filepath[512];

    if(path[0])
        Q_snprintf(filepath, sizeof(filepath), "%s/*", path);
    else
        Q_snprintf(filepath, sizeof(filepath), "*");
    V_FixSlashes(filepath);

    FileFindHandle_t findHandle;
    const char *pFilename = filesystem->FindFirstEx( filepath, "MOD", &findHandle );
    while(pFilename != NULL)
    {
        if(pFilename[0] != '.' && !Q_stristr(pFilename, "no_autorun"))
        {
#ifdef __linux__
            struct stat statbuf;

            Q_snprintf(filepath, sizeof(filepath), "%s/%s", path, pFilename);
            V_FixSlashes(filepath);

            stat(filepath, &statbuf);
            if(S_ISDIR(statbuf.st_mode))
#else
            if(filesystem->FindIsDirectory(findHandle))
#endif
            {
                char nextdir[512];
                if(path[0])
                    Q_snprintf(nextdir, sizeof(nextdir), "%s/%s", path, pFilename);
                else
                    Q_snprintf(nextdir, sizeof(nextdir), "%s", pFilename);

                numscripts += LoadLuaAutorunScripts(nextdir);
            }
            else
            {
                char ext[10];
                Q_ExtractFileExtension(pFilename, ext, sizeof(ext));

                if(!Q_stricmp(ext, "lua"))
                {
                    Q_snprintf(filepath, sizeof(filepath), "%s/%s", path, pFilename);
                    V_FixSlashes(filepath);

                    luascript(GetLuaState(), filepath);
                    numscripts++;
                }
            }
        }
        pFilename = filesystem->FindNext( findHandle );
    }
    filesystem->FindClose(findHandle);

    return numscripts;
}

class CSOPLuaDerivedTypes : public CStringPool, public CAutoGameSystem
{
    virtual char const *Name() { return "CSOPLuaDerivedTypes"; }

    virtual void Shutdown() 
    {
        FreeAll();
    }
};

static CSOPLuaDerivedTypes g_LuaTypesStringPool;

extern void AddLuaNormalEntity(const char *pszName, int reference);
int CAdminOP :: LoadLuaEntities(const char *path)
{
    int numscripts = 0;
    char filepath[512];

    if(path[0])
        Q_snprintf(filepath, sizeof(filepath), "%s/*", path);
    else
        Q_snprintf(filepath, sizeof(filepath), "*");
    V_FixSlashes(filepath);

    FileFindHandle_t findHandle;
    const char *pFilename = filesystem->FindFirstEx( filepath, "MOD", &findHandle );
    while(pFilename != NULL)
    {
        if(pFilename[0] != '.')
        {
#ifdef __linux__
            struct stat statbuf;

            Q_snprintf(filepath, sizeof(filepath), "%s/%s", path, pFilename);
            V_FixSlashes(filepath);

            stat(filepath, &statbuf);
            if(S_ISDIR(statbuf.st_mode))
#else
            if(filesystem->FindIsDirectory(findHandle))
#endif
            {
                SOPEntity::dynamicDerivedTypes.AddToTail(g_LuaTypesStringPool.Allocate(pFilename));
                Lunar<SOPEntity>::Register(GetLuaState(), pFilename);
                luaL_dostring(GetLuaState(), UTIL_VarArgs("ENT = %s ", pFilename));

                Q_snprintf(filepath, sizeof(filepath), "%s/%s/init.lua", path, pFilename);
                V_FixSlashes(filepath);
                luascript(GetLuaState(), filepath);

                lua_getglobal(GetLuaState(), "ENT");
                int ref = lua_ref(GetLuaState(), true);
                if(ref != LUA_REFNIL)
                {
                    AddLuaNormalEntity(pFilename, ref);
                    numscripts++;
                }
                else
                {
                    ColorMsg(CONCOLOR_LUA_ERR, "[SOURCEOP] Failed to load Lua entity: %s\n", pFilename);
                }
            }
        }
        pFilename = filesystem->FindNext( findHandle );
    }
    filesystem->FindClose(findHandle);

    return numscripts;
}

void CAdminOP :: CloseLua()
{
    lua_State *L = GetLuaState();
    if(!L) return;

    char gamedir[256];
    char filepath[512];
    engine->GetGameDir(gamedir, sizeof(gamedir));
    Q_snprintf(filepath, sizeof(filepath), "%s/%s/lua/includes/cleanup.lua", gamedir, DataDir());
    V_FixSlashes(filepath);
    luascript(L, filepath);

    //lua_setgcthreshold(L, 0);  // collected garbage
    lua_close(L);
    luaState = NULL;
}

void CAdminOP :: CopyData(int size, void *pDataDest, const void *pDataSrc)
{
    memcpy(pDataDest, pDataSrc, size);
}

bool CAdminOP :: CopyBasicField(void *pDataDest, const void *pDataSrc, typedescription_t *pDest, typedescription_t *pSrc)
{
    switch( pSrc->fieldType )
    {
        case FIELD_FLOAT:
            CopyData(pSrc->fieldSizeInBytes, pDataDest, pDataSrc);
            //*(float *)pDataDest = *(float *)pDataSrc;
            break;
            
        case FIELD_STRING:
            CopyData(pSrc->fieldSizeInBytes, pDataDest, pDataSrc);
            //*(string_t *)pDataDest = *(string_t *)pDataSrc;
            break;
            
        case FIELD_VECTOR:
            VectorCopy(*(Vector *)pDataSrc, *(Vector *)pDataDest);
            break;

        case FIELD_QUATERNION:
            CopyData(pSrc->fieldSizeInBytes, pDataDest, pDataSrc);
            ((Quaternion *)pDataDest)->w = ((Quaternion *)pDataSrc)->w;
            ((Quaternion *)pDataDest)->x = ((Quaternion *)pDataSrc)->x;
            ((Quaternion *)pDataDest)->y = ((Quaternion *)pDataSrc)->y;
            ((Quaternion *)pDataDest)->z = ((Quaternion *)pDataSrc)->z;
            break;

        case FIELD_INTEGER:
            CopyData(pSrc->fieldSizeInBytes, pDataDest, pDataSrc);
            //Msg("Field: %s\n", pSrc->fieldName);
            //*(int *)pDataDest = *(int *)pDataSrc;
            break;

        case FIELD_BOOLEAN:
            CopyData(pSrc->fieldSizeInBytes, pDataDest, pDataSrc);
            break;

        case FIELD_SHORT:
            CopyData(2 * pSrc->fieldSize, pDataDest, pDataSrc);
            break;

        case FIELD_CHARACTER:
            CopyData(pSrc->fieldSize, pDataDest, pDataSrc);
            break;

        case FIELD_COLOR32:
            CopyData(4*pSrc->fieldSize, pDataDest, pDataSrc);   
            break;

        case FIELD_EMBEDDED:
        {
            AssertMsg( ( (pSrc->flags & FTYPEDESC_PTR) == 0 ) || (pSrc->fieldSize == 1), "Arrays of embedded pointer types presently unsupported by save/restore" );
            Assert( !(pSrc->flags & FTYPEDESC_PTR) || *((void **)pDataSrc) );
            int nFieldCount = pSrc->fieldSize;
            char *pFieldDataDest = (char *)( ( !(pDest->flags & FTYPEDESC_PTR) ) ? pDataDest : *((void **)pDataDest) );
            char *pFieldDataSrc = (char *)( ( !(pSrc->flags & FTYPEDESC_PTR) ) ? pDataSrc : *((void **)pDataSrc) );

            while ( --nFieldCount >= 0 )
            {
                CopyDataMap( pFieldDataDest, pFieldDataSrc, pDest->td, pSrc->td );
                pFieldDataDest += pSrc->fieldSizeInBytes;
                pFieldDataSrc += pSrc->fieldSizeInBytes;
            }

            break;
        }

        case FIELD_CUSTOM:
        {
            break;
        }

        default:
            Warning( "Bad field type\n" );
            Assert(0);
            return false;
    }

    return true;
}

#define MAX_ENTITYARRAY 1024
bool CAdminOP :: CopyGameField(void *pDataDest, const void *pDataSrc, typedescription_t *pDest, typedescription_t *pSrc)
{
    switch( pSrc->fieldType )
    {
        case FIELD_CLASSPTR:
            CopyData(pSrc->fieldSizeInBytes, pDataDest, pDataSrc);
            /*for ( int i = 0; i < pSrc->fieldSize && i < MAX_ENTITYARRAY; i++ )
            {
                ((CBaseEntity **)pDataDest)[i] = ((CBaseEntity **)pDataSrc)[i];
            }*/
            //*(CBaseEntity **)pDataDest = *(CBaseEntity **)pDataSrc;
            //WriteEntityPtr( pField->fieldName, (CBaseEntity **)pData, pField->fieldSize );
            break;
    
        case FIELD_EDICT:
            CopyData(pSrc->fieldSizeInBytes, pDataDest, pDataSrc);
            /*for ( int i = 0; i < pSrc->fieldSize && i < MAX_ENTITYARRAY; i++ )
            {
                ((edict_t **)pDataDest)[i] = ((edict_t **)pDataSrc)[i];
            }*/
            //*(edict_t **)pDataDest = *(edict_t **)pDataSrc;
            //WriteEdictPtr( pField->fieldName, (edict_t **)pData, pField->fieldSize );
            break;
    
        case FIELD_EHANDLE:
            CopyData(pSrc->fieldSizeInBytes, pDataDest, pDataSrc);
            /*for ( int i = 0; i < pSrc->fieldSize && i < MAX_ENTITYARRAY; i++ )
            {
                ((EHANDLE *)pDataDest)[i] = ((EHANDLE *)pDataSrc)[i];
            }*/
            //WriteEHandle( pField->fieldName, (EHANDLE *)pData, pField->fieldSize );
            break;
        
        case FIELD_POSITION_VECTOR:
            CopyData(pSrc->fieldSizeInBytes, pDataDest, pDataSrc);
            //WritePositionVector( pField->fieldName, (Vector *)pData, pField->fieldSize );
            break;
            
        case FIELD_TIME:
            CopyData(pSrc->fieldSizeInBytes, pDataDest, pDataSrc);
            break;

        case FIELD_TICK:
            CopyData(pSrc->fieldSizeInBytes, pDataDest, pDataSrc);
            break;
            
        case FIELD_MODELINDEX:
        case FIELD_MATERIALINDEX:
            CopyData(pSrc->fieldSizeInBytes, pDataDest, pDataSrc);
            //*(int *)pDataDest = *(int *)pDataSrc;
            break;

        case FIELD_MODELNAME:
        case FIELD_SOUNDNAME:
            CopyData(pSrc->fieldSizeInBytes, pDataDest, pDataSrc);
            //*(string_t *)pDataDest = *(string_t *)pDataSrc;
            break;

        // For now, just write the address out, we're not going to change memory while doing this yet!
        case FIELD_FUNCTION:
            CopyData(pSrc->fieldSizeInBytes, pDataDest, pDataSrc);
            break;
            
        case FIELD_VMATRIX:
            //WriteVMatrix( pField->fieldName, (VMatrix *)pData, pField->fieldSize );
            break;
        case FIELD_VMATRIX_WORLDSPACE:
            //WriteVMatrixWorldspace( pField->fieldName, (VMatrix *)pData, pField->fieldSize );
            break;

        case FIELD_MATRIX3X4_WORLDSPACE:
            //WriteMatrix3x4Worldspace( pField->fieldName, (const matrix3x4_t *)pData, pField->fieldSize );
            break;

        case FIELD_INTERVAL:
            //WriteInterval( pField->fieldName, (interval_t *)pData, pField->fieldSize );
            break;

        default:
            //Warning( "Bad field type\n" );
            //Assert(0);
            return false;
    }

    return true;
}

void CAdminOP :: CopyField(void *pDataDest, const void *pDataSrc, typedescription_t *pDest, typedescription_t *pSrc)
{
    if (pSrc->fieldType <= FIELD_CUSTOM)
    {
        if(pSrc->fieldType != FIELD_VOID)
        {
            CopyBasicField(pDataDest, pDataSrc, pDest, pSrc);
        }
    }
    CopyGameField(pDataDest, pDataSrc, pDest, pSrc);
}

void CAdminOP :: CopyDataFields(void *pBaseDataDest, const void *pBaseDataSrc, typedescription_t *pFieldsDest, typedescription_t *pFieldsSrc, int fieldCount)
{
    typedescription_t *pDest;
    typedescription_t *pSrc;
    for(int i = 0; i < fieldCount; i++)
    {
        pDest = &pFieldsDest[i];
        pSrc = &pFieldsSrc[i];

        void *pOutputDataDest = ( (char *)pBaseDataDest + pDest->fieldOffset[ TD_OFFSET_NORMAL ] );
        void *pOutputDataSrc = ( (char *)pBaseDataSrc + pSrc->fieldOffset[ TD_OFFSET_NORMAL ] );
        CopyField(pOutputDataDest, pOutputDataSrc, pDest, pSrc);
    }
}

void CAdminOP :: CopyDataMap(void *pLeafObjectDest, const void *pLeafObjectSrc, datamap_t *dest, datamap_t *src)
{
    if(!dest || !src) return;

    if(src->baseMap)
    {
        CopyDataMap(pLeafObjectDest, pLeafObjectSrc, dest->baseMap, src->baseMap);
    }

    if(dest->dataNumFields != src->dataNumFields) return;

    CopyDataFields(pLeafObjectDest, pLeafObjectSrc, dest->dataDesc, src->dataDesc, src->dataNumFields);
}

void CAdminOP :: CopyDataDesc(CBaseEntity *dest, CBaseEntity *src)
{
    datamap_t *datamapdest = VFuncs::GetDataDescMap(dest);
    datamap_t *datamapsrc = VFuncs::GetDataDescMap(src);

    if(datamapdest && datamapsrc)
    {
        CopyDataMap(dest, src, datamapdest, datamapsrc);
    }
}

int SourceOPIndexOfEdict(const edict_t *pEdict)
{
    return pAdminOP.IndexOfEdict(pEdict);
}

edict_t *SourceOPPEntityOfEntIndex(int iEntIndex)
{
    return pAdminOP.PEntityOfEntIndex(iEntIndex);
}
