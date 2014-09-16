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

#ifndef VFUNCS_H
#define VFUNCS_H

#include "tier0/memdbgon.h"

#ifndef max
#define max MAX
#endif

#ifndef min
#define min MIN
#endif

class CEntityFactoryDictionary;
class CBaseEntity;
class CBaseAnimating;
class CBaseCombatCharacter;
class CBaseCombatWeapon;
class CCollisionProperty;
class CBaseHandle;

class IHandleEntity;
class CServerNetworkProperty;
class IPhysicsObject;

typedef struct netadr_s	netadr_t;
struct edict_t;

class VfuncEmptyClass {};

enum offsets {
    OFFSET_GETREFEHANDLE,
    OFFSET_GETCOLLIDEABLE,
    OFFSET_GETNETWORKABLE,
    OFFSET_GETMODELINDEX,
    OFFSET_GETMODELNAME,
    OFFSET_SETMODELINDEX,
    OFFSET_GETDATADESCMAP,
    OFFSET_GETCLASSNAME,
    OFFSET_UPDATETRANSMITSTATE,
    OFFSET_SPAWN,
    OFFSET_PRECACHE,
    OFFSET_SETMODEL,
    OFFSET_POSTCONSTRUCTOR,
    OFFSET_KEYVALUE,
    OFFSET_ACTIVATE,
    OFFSET_SETPARENT,
    OFFSET_ACCEPTINPUT,
    OFFSET_THINK,
    OFFSET_PASSESDAMAGEFILTER,
    OFFSET_TRACEATTACK,
    OFFSET_ONTAKEDAMAGE,
    OFFSET_ISALIVE,
    OFFSET_EVENTKILLED,
    OFFSET_ISNPC,
    OFFSET_ISPLAYER,
    OFFSET_ISBASECOMBATWEAPON,
    OFFSET_CHANGETEAM,
    OFFSET_USE,
    OFFSET_STARTTOUCH,
    OFFSET_TOUCH,
    OFFSET_UPDATEONREMOVE,
    OFFSET_TELEPORT,
    OFFSET_FIREBULLETS,
    OFFSET_SETDAMAGE,
    OFFSET_EYEPOSITION,
    OFFSET_EYEANGLES,
    OFFSET_FVISIBLE,
    OFFSET_WORLDSPACECENTER,
    OFFSET_GETSOUNDEMISSIONORIGIN,
    OFFSET_CREATEVPHYSICS,
    OFFSET_VPHYSICSDESTROYOBJECT,
    OFFSET_VPHYSICSGETOBJECTLIST,
    OFFSET_EXTINGUISH,
    OFFSET_STUDIOFRAMEADVANCE,
    OFFSET_GIVEAMMO,
    OFFSET_WEAPONEQUIP,
    OFFSET_WEAPONSWITCH,
    OFFSET_WEAPONGETSLOT,
    OFFSET_REMOVEPLAYERITEM,
    OFFSET_HOLSTER,
    OFFSET_SETDAMAGERADIUS,
    OFFSET_PRIMARYATTACK,
    OFFSET_SECONDARYATTACK,
    OFFSET_FORCERESPAWN,
    OFFSET_STARTOBSERVERMODE,
    OFFSET_STOPOBSERVERMODE,
    OFFSET_ITEMPOSTFRAME,
    OFFSET_GIVENAMEDITEM,
    OFFSET_CANHEARANDREADCHATFROM,
    OFFSET_CALCULATETEAMBALANCESCORE,
    OFFSET_GIVENAMEDSCRIPTITEM,

    OFFSET_PLAYERRELATIONSHIP,
    OFFSET_PLAYERCANHEARCHAT,
    OFFSET_GETTEAMINDEX,
    OFFSET_GETINDEXEDTEAMNAME,
    OFFSET_ISVALIDTEAM,
    OFFSET_MARKACHIEVEMENT,
    OFFSET_GETNEXTLEVELNAME,
    OFFSET_CHANGELEVEL,
    OFFSET_GOTOINTERMISSION,
    OFFSET_SETSTALEMATE,
    OFFSET_SETSWITCHTEAMS,
    OFFSET_HANDLESWITCHTEAMS,
    OFFSET_SETSCRAMBLETEAMS,
    OFFSET_HANDLESCRAMBLETEAMS,
    OFFSET_GETENTITYHANDLE,
    OFFSET_GETEDICT,
    OFFSET_RELEASE,
    OFFSET_GETBASEENTITY,
    OFFSET_CONNECT,
    OFFSET_RECONNECT,
    OFFSET_DISCONNECT,
    OFFSET_IGETPLAYERSLOT,
    OFFSET_GETPLAYERSLOT,
    OFFSET_GETNETWORKID,
    OFFSET_GETCLIENTNAME,
    OFFSET_GETNETCHANNEL,
    OFFSET_GETSERVER,
    OFFSET_FILLUSERINFO,
    OFFSET_SENDSERVERINFO,
    OFFSET_SETUSERCVAR,
    OFFSET_EXECSTRINGCMD,
    OFFSET_VTABLE2FROM3,
    OFFSET_GETNUMCLIENTS,
    OFFSET_GETMAXCLIENTS,
    OFFSET_REJECTCONNECTION,
    OFFSET_CONNECTCLIENT,
    OFFSET_GETFREECLIENT,
    NUM_OFFSETS
};

#ifdef _WIN32
// windows structure is padded
typedef struct USERID_s
{
    int one;
    int unknown1;
    short steam_x;
    short unknown2;
    int unknown3;
    int id;
    int zeroone;
} USERID_t;
#else
typedef struct USERID_s
{
    int one;
    short steam_x;
    short unused;
    int id;
    int zeroone;
} USERID_t;
#endif

extern int offs[NUM_OFFSETS];

extern int g_useOldEntityDictionary;
extern int g_useEntFactDebugPrints;
extern bool g_suppressEntindexError;

#define CREATE_VFUNC_HEAD(vtbloffset, vtablenum, thisoffset) \
    void **this_ptr = (*(void ***)&pThisPtr)+thisoffset; \
    void **vtable = *(void ***)((int *)pThisPtr+vtablenum); \
    void *func = vtable[vtbloffset]; 

#ifndef __linux__
#define CREATE_VFUNC_MID     void *addr;    } u;    u.addr = func
#else
#define CREATE_VFUNC_MID     struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
#endif

#define CREATE_VFUNC0(ifacetype, ifacefunc, vtbloffset, vtablenum, thisoffset, rettype) \
    rettype VFuncs::ifacefunc( ifacetype *pThisPtr ) \
    { \
        CREATE_VFUNC_HEAD(vtbloffset,vtablenum,thisoffset) \
        union {rettype (VfuncEmptyClass::*mfpnew)( void ); \
        CREATE_VFUNC_MID; \
        return (rettype) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)( ); \
    }
#define DECLARE_VFUNC0(ifacetype, ifacefunc, rettype) static rettype ifacefunc( ifacetype *pThisPtr)
#define CREATE_VFUNC0_void(ifacetype, ifacefunc, vtbloffset, vtablenum, thisoffset) \
    void VFuncs::ifacefunc( ifacetype *pThisPtr ) \
    { \
        if(vtbloffset == -1) return; \
        CREATE_VFUNC_HEAD(vtbloffset,vtablenum,thisoffset) \
        union {void (VfuncEmptyClass::*mfpnew)( void ); \
        CREATE_VFUNC_MID; \
        (void) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)( ); \
    }
#define DECLARE_VFUNC0_void(ifacetype, ifacefunc) static void ifacefunc( ifacetype *pThisPtr)

#define CREATE_VFUNC1(ifacetype, ifacefunc, vtbloffset, vtablenum, thisoffset, rettype, type1, name1) \
    rettype VFuncs::ifacefunc( ifacetype *pThisPtr, type1 name1 ) \
    { \
        CREATE_VFUNC_HEAD(vtbloffset,vtablenum,thisoffset) \
        union {rettype (VfuncEmptyClass::*mfpnew)( type1 ); \
        CREATE_VFUNC_MID; \
        return (rettype) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)(name1); \
    }
#define DECLARE_VFUNC1(ifacetype, ifacefunc, rettype, type1, name1) static rettype ifacefunc( ifacetype *pThisPtr, type1 name1)
#define CREATE_VFUNC1_void(ifacetype, ifacefunc, vtbloffset, vtablenum, thisoffset, type1, name1) \
    void VFuncs::ifacefunc( ifacetype *pThisPtr, type1 name1 ) \
    { \
        if(vtbloffset == -1) return; \
        CREATE_VFUNC_HEAD(vtbloffset,vtablenum,thisoffset) \
        union {void (VfuncEmptyClass::*mfpnew)( type1 ); \
        CREATE_VFUNC_MID; \
        (void) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)(name1); \
    }
#define DECLARE_VFUNC1_void(ifacetype, ifacefunc, type1, name1) static void ifacefunc( ifacetype *pThisPtr, type1 name1)

#define CREATE_VFUNC2(ifacetype, ifacefunc, vtbloffset, vtablenum, thisoffset, rettype, type1, name1, type2, name2) \
    rettype VFuncs::ifacefunc( ifacetype *pThisPtr, type1 name1, type2 name2 ) \
    { \
        CREATE_VFUNC_HEAD(vtbloffset,vtablenum,thisoffset) \
        union {rettype (VfuncEmptyClass::*mfpnew)( type1, type2 ); \
        CREATE_VFUNC_MID; \
        return (rettype) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)(name1, name2); \
    }
#define DECLARE_VFUNC2(ifacetype, ifacefunc, rettype, type1, name1, type2, name2) static rettype ifacefunc( ifacetype *pThisPtr, type1 name1, type2 name2)
#define CREATE_VFUNC2_void(ifacetype, ifacefunc, vtbloffset, vtablenum, thisoffset, type1, name1, type2, name2) \
    void VFuncs::ifacefunc( ifacetype *pThisPtr, type1 name1, type2 name2 ) \
    { \
        if(vtbloffset == -1) return; \
        CREATE_VFUNC_HEAD(vtbloffset,vtablenum,thisoffset) \
        union {void (VfuncEmptyClass::*mfpnew)( type1, type2 ); \
        CREATE_VFUNC_MID; \
        (void) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)(name1, name2); \
    }
#define DECLARE_VFUNC2_void(ifacetype, ifacefunc, type1, name1, type2, name2) static void ifacefunc( ifacetype *pThisPtr, type1 name1, type2 name2)

#define CREATE_VFUNC3(ifacetype, ifacefunc, vtbloffset, vtablenum, thisoffset, rettype, type1, name1, type2, name2, type3, name3) \
    rettype VFuncs::ifacefunc( ifacetype *pThisPtr, type1 name1, type2 name2, type3 name3 ) \
    { \
        CREATE_VFUNC_HEAD(vtbloffset,vtablenum,thisoffset) \
        union {rettype (VfuncEmptyClass::*mfpnew)( type1, type2, type3 ); \
        CREATE_VFUNC_MID; \
        return (rettype) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)(name1, name2, name3); \
    }
#define DECLARE_VFUNC3(ifacetype, ifacefunc, rettype, type1, name1, type2, name2, type3, name3) static rettype ifacefunc( ifacetype *pThisPtr, type1 name1, type2 name2, type3 name3)
#define CREATE_VFUNC3_void(ifacetype, ifacefunc, vtbloffset, vtablenum, thisoffset, type1, name1, type2, name2, type3, name3) \
    void VFuncs::ifacefunc( ifacetype *pThisPtr, type1 name1, type2 name2, type3 name3 ) \
    { \
        if(vtbloffset == -1) return; \
        CREATE_VFUNC_HEAD(vtbloffset,vtablenum,thisoffset) \
        union {void (VfuncEmptyClass::*mfpnew)( type1, type2, type3 ); \
        CREATE_VFUNC_MID; \
        (void) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)(name1, name2, name3); \
    }
#define DECLARE_VFUNC3_void(ifacetype, ifacefunc, type1, name1, type2, name2, type3, name3) static void ifacefunc( ifacetype *pThisPtr, type1 name1, type2 name2, type3 name3)

#define CREATE_VFUNC4(ifacetype, ifacefunc, vtbloffset, vtablenum, thisoffset, rettype, type1, name1, type2, name2, type3, name3, type4, name4) \
    rettype VFuncs::ifacefunc( ifacetype *pThisPtr, type1 name1, type2 name2, type3 name3, type4 name4 ) \
    { \
        CREATE_VFUNC_HEAD(vtbloffset,vtablenum,thisoffset) \
        union {rettype (VfuncEmptyClass::*mfpnew)( type1, type2, type3, type4 ); \
        CREATE_VFUNC_MID; \
        return (rettype) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)(name1, name2, name3, name4); \
    }
#define DECLARE_VFUNC4(ifacetype, ifacefunc, rettype, type1, name1, type2, name2, type3, name3, type4, name4) static rettype ifacefunc( ifacetype *pThisPtr, type1 name1, type2 name2, type3 name3, type4 name4)
#define CREATE_VFUNC4_void(ifacetype, ifacefunc, vtbloffset, vtablenum, thisoffset, type1, name1, type2, name2, type3, name3, type4, name4) \
    void VFuncs::ifacefunc( ifacetype *pThisPtr, type1 name1, type2 name2, type3 name3, type4 name4 ) \
    { \
        if(vtbloffset == -1) return; \
        CREATE_VFUNC_HEAD(vtbloffset,vtablenum,thisoffset) \
        union {void (VfuncEmptyClass::*mfpnew)( type1, type2, type3, type4 ); \
        CREATE_VFUNC_MID; \
        (void) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)(name1, name2, name3, name4); \
    }
#define DECLARE_VFUNC4_void(ifacetype, ifacefunc, type1, name1, type2, name2, type3, name3, type4, name4) static void ifacefunc( ifacetype *pThisPtr, type1 name1, type2 name2, type3 name3, type4 name4)

#define CREATE_VFUNC5(ifacetype, ifacefunc, vtbloffset, vtablenum, thisoffset, rettype, type1, name1, type2, name2, type3, name3, type4, name4, type5, name5) \
    rettype VFuncs::ifacefunc( ifacetype *pThisPtr, type1 name1, type2 name2, type3 name3, type4 name4, type5 name5 ) \
    { \
        CREATE_VFUNC_HEAD(vtbloffset,vtablenum,thisoffset) \
        union {rettype (VfuncEmptyClass::*mfpnew)( type1, type2, type3, type4, type5 ); \
        CREATE_VFUNC_MID; \
        return (rettype) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)(name1, name2, name3, name4, name5); \
    }
#define DECLARE_VFUNC5(ifacetype, ifacefunc, rettype, type1, name1, type2, name2, type3, name3, type4, name4, type5, name5) static rettype ifacefunc( ifacetype *pThisPtr, type1 name1, type2 name2, type3 name3, type4 name4, type5 name5)
#define CREATE_VFUNC5_void(ifacetype, ifacefunc, vtbloffset, vtablenum, thisoffset, type1, name1, type2, name2, type3, name3, type4, name4, type5, name5) \
    void VFuncs::ifacefunc( ifacetype *pThisPtr, type1 name1, type2 name2, type3 name3, type4 name4, type5 name5 ) \
    { \
        if(vtbloffset == -1) return; \
        CREATE_VFUNC_HEAD(vtbloffset,vtablenum,thisoffset) \
        union {void (VfuncEmptyClass::*mfpnew)( type1, type2, type3, type4, type5 ); \
        CREATE_VFUNC_MID; \
        (void) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)(name1, name2, name3, name4, name5); \
    }
#define DECLARE_VFUNC5_void(ifacetype, ifacefunc, type1, name1, type2, name2, type3, name3, type4, name4, type5, name5) static void ifacefunc( ifacetype *pThisPtr, type1 name1, type2 name2, type3 name3, type4 name4, type5 name5)


#define DECLARE_VAR(type, name, thisPtr, offset) \
    type * name; \
    if(!(offset)) return; \
    name = (type *)(((unsigned int)thisPtr)+(offset))
#define DECLARE_VAR_RVAL(type, name, thisPtr, offset, retval) \
    type * name; \
    if(!(offset)) return (retval); \
    name = (type *)(((unsigned int)thisPtr)+(offset))

class VFuncs
{
public:
    static void LoadOffsets(void);
    static void LoadDataDescOffsets(void);
    static void LoadPlayerDataDescOffsets(void);
    DECLARE_VFUNC0(CBaseEntity, GetRefEHandle, CBaseHandle&);
    DECLARE_VFUNC0(CBaseEntity, GetCollideable, ICollideable*);
    DECLARE_VFUNC0(CBaseEntity, GetNetworkable, IServerNetworkable*);
    DECLARE_VFUNC0(CBaseEntity, GetModelIndex, int);
    DECLARE_VFUNC0(CBaseEntity, GetModelName, string_t);
    DECLARE_VFUNC1_void(CBaseEntity, SetModelIndex, int, index);
    DECLARE_VFUNC0(CBaseEntity, GetDataDescMap, datamap_t *);
    DECLARE_VFUNC0(CBaseEntity, GetClassName, const char *);
    DECLARE_VFUNC0(CBaseEntity, UpdateTransmitState, int);
    DECLARE_VFUNC0_void(CBaseEntity, Spawn);
    DECLARE_VFUNC0_void(CBaseEntity, Precache);
    DECLARE_VFUNC1_void(CBaseEntity, SetModel, const char *, ModelName);
    DECLARE_VFUNC2(CBaseEntity, KeyValue, bool, const char *, szKeyName, const char *, szValue);
    DECLARE_VFUNC0_void(CBaseEntity, Activate);
    DECLARE_VFUNC5(CBaseEntity, AcceptInput, bool, const char *, szInputName, CBaseEntity *, pActivator, CBaseEntity *, pCaller, variant_t, Value, int, outputID);
    DECLARE_VFUNC1(CBaseEntity, PassesDamageFilter, bool, const CTakeDamageInfo &, info);
    DECLARE_VFUNC4_void(CBaseEntity, TraceAttack, const CTakeDamageInfo &, info, const Vector &, vecDir, trace_t *, ptr, void *, pAccumulator);
    DECLARE_VFUNC1(CBaseEntity, OnTakeDamage, int, const CTakeDamageInfo &, info);
    DECLARE_VFUNC0(CBaseEntity, IsAlive, bool);
    DECLARE_VFUNC1_void(CBaseEntity, Event_Killed, const CTakeDamageInfo &, info);
    DECLARE_VFUNC0(CBaseEntity, IsNPC, bool);
    DECLARE_VFUNC0(CBaseEntity, IsPlayer, bool);
    DECLARE_VFUNC0(CBaseEntity, IsBaseCombatWeapon, bool);
    DECLARE_VFUNC1_void(CBaseEntity, ChangeTeam, int, iTeamNum);
    DECLARE_VFUNC4_void(CBaseEntity, Use, CBaseEntity *, pActivator, CBaseEntity *, pCaller, USE_TYPE, useType, float, value);
    DECLARE_VFUNC1_void(CBaseEntity, Touch, CBaseEntity *, pOther);
    DECLARE_VFUNC0_void(CBaseEntity, UpdateOnRemove);
    DECLARE_VFUNC3_void(CBaseEntity, Teleport, const Vector *, newPosition, const QAngle *, newAngles, const Vector *, newVelocity);
    DECLARE_VFUNC1_void(CBaseEntity, FireBullets, const FireBulletsInfo_t &, info);
    DECLARE_VFUNC1_void(CBaseEntity, SetDamage, float, flDamage);
    DECLARE_VFUNC0(CBaseEntity, EyePosition, Vector);
    DECLARE_VFUNC0(CBaseEntity, EyeAngles, const QAngle &);
    static bool FVisible(CBaseEntity *pThisPtr, CBaseEntity *pEntity, int traceMask = MASK_OPAQUE, CBaseEntity **ppBlocker = NULL);
    DECLARE_VFUNC0(CBaseEntity, WorldSpaceCenter, const Vector &);
    DECLARE_VFUNC0(CBaseEntity, GetSoundEmissionOrigin, Vector);
    DECLARE_VFUNC0(CBaseEntity, CreateVPhysics, bool);
    DECLARE_VFUNC0_void(CBaseEntity, VPhysicsDestroyObject);
    DECLARE_VFUNC2(CBaseEntity, VPhysicsGetObjectList, int, IPhysicsObject **, pList, int, listMax);
    DECLARE_VFUNC0_void(CBaseAnimating, StudioFrameAdvance);
    DECLARE_VFUNC0_void(CBaseAnimating, Extinguish);
    static int GiveAmmo(CBasePlayer *pThisPtr, int iCount, int iAmmoIndex, bool bSupressSound = false);
    DECLARE_VFUNC1_void(CBasePlayer, Weapon_Equip, CBaseCombatWeapon *, pWeapon);
    static bool Weapon_Switch(CBasePlayer *pThisPtr, CBaseCombatWeapon *pWeapon, int viewmodelindex = 0);
    DECLARE_VFUNC1(CBasePlayer, Weapon_GetSlot, CBaseCombatWeapon *, int, slot);
    DECLARE_VFUNC1(CBasePlayer, RemovePlayerItem, bool, CBaseCombatWeapon *, pItem);
    static bool Holster(CBaseCombatWeapon *pThisPtr, CBaseCombatWeapon *pSwitchingTo = NULL);
    DECLARE_VFUNC1_void(CBaseEntity, SetDamageRadius, float, flDamageRadius);
    DECLARE_VFUNC0_void(CBasePlayer, ForceRespawn);
    DECLARE_VFUNC1(CBasePlayer, StartObserverMode, bool, int, mode);
    DECLARE_VFUNC0_void(CBasePlayer, StopObserverMode);
    static CBaseEntity* GiveNamedItem(CBasePlayer *pThisPtr, const char *pszName, int iSubType = 0);

    DECLARE_VFUNC2(void, PlayerRelationship, int, CBaseEntity *, pPlayer, CBaseEntity *, pTarget);
    DECLARE_VFUNC2(void, PlayerCanHearChat, bool, CBasePlayer *, pListener, CBasePlayer *, pSpeaker);
    DECLARE_VFUNC1(void, GetTeamIndex, int, const char *, pTeamName);
    DECLARE_VFUNC1(void, GetIndexedTeamName, const char *, int, teamIndex);
    DECLARE_VFUNC1(void, IsValidTeam, bool, const char *, pTeamName);
    DECLARE_VFUNC2_void(void, MarkAchievement, IRecipientFilter &, filter, char const *,pchAchievementName);
    DECLARE_VFUNC3(void, GetNextLevelName, int, char *, szName, size_t, bufsize, bool, unknown);
    DECLARE_VFUNC0_void(void, ChangeLevel);
    DECLARE_VFUNC3_void(void, SetStalemate, int, iReason, bool, bForceMapReset, bool, bUnknown);
    DECLARE_VFUNC1_void(void, SetSwitchTeams, bool, bSwitch);
    DECLARE_VFUNC0_void(void, HandleSwitchTeams);
    DECLARE_VFUNC1_void(void, SetScrambleTeams, bool, bScramble);
    DECLARE_VFUNC0_void(void, HandleScrambleTeams);
    static bool IsMannVsMachineMode( void *pGameRules );

    DECLARE_VFUNC0(IServerNetworkable, GetEntityHandle, IHandleEntity *);
    DECLARE_VFUNC0(IServerNetworkable, GetEdict, edict_t *);
    DECLARE_VFUNC0_void(IServerNetworkable, Release);
    DECLARE_VFUNC0(IServerNetworkable, GetBaseEntity, CBaseEntity *);

    DECLARE_VFUNC0_void(void, Reconnect);
    static void Disconnect( void *pThisPtr, const char *pszReason, ... );
    DECLARE_VFUNC0(void, GetPlayerSlot, short);
    DECLARE_VFUNC0(void, GetNetworkID, USERID_t);
    DECLARE_VFUNC0(void, GetClientName, const char*);
    DECLARE_VFUNC0(void, GetNetChannel, void*);
    DECLARE_VFUNC0(void, GetServer, void *);
    DECLARE_VFUNC2_void(void, SetUserCVar, const char *, cvar, const char *, value);
    DECLARE_VFUNC0(void, IGetPlayerSlot, short);

    DECLARE_VFUNC0(void, GetNumClients, int);
    DECLARE_VFUNC0(void, GetMaxClients, int);
    static void RejectConnection( void *pThisPtr, const netadr_t &net, int challenge, const char *string );

    static void SetParent( CBaseEntity *pThisPtr, CBaseEntity* pNewParent, int iAttachment = -1 );

    static CBaseEntity  *Instance( edict_t *pent );
    static void         SetClassname( CBaseEntity *pThisPtr, const char *className );
    static bool         ClassMatches( CBaseEntity *pThisPtr, const char *pszClassOrWildcard );
    static char const   *GetClassname(CBaseEntity *pThisPtr);
    static bool         NameMatches( CBaseEntity *pThisPtr, const char *pszNameOrWildcard );
    static int          entindex(CBaseEntity *pThisPtr);
    static void         SetSimulationTime(CBaseEntity *pThisPtr, float st);
    static int          GetMaxHealth(CBaseEntity *pThisPtr);
    static int          GetHealth(CBaseEntity *pThisPtr);
    static void         SetHealth( CBaseEntity *pThisPtr, int amt );
    static Vector       GetAbsOrigin(const CBaseEntity *pThisPtr);
    static void         SetAbsOrigin(CBaseEntity *pThisPtr, const Vector& absOrigin);
    static void         SetAbsAngles(CBaseEntity *pThisPtr, const QAngle& absAngles);
    static Vector       GetAbsVelocity(const CBaseEntity *pThisPtr);
    static void         SetAbsVelocity(CBaseEntity *pThisPtr, const Vector &vecAbsVelocity);
    static Vector       GetLocalOrigin(CBaseEntity *pThisPtr);
    static void         SetLocalOrigin(CBaseEntity *pThisPtr, const Vector& origin);
    static QAngle       GetLocalAngles(CBaseEntity *pThisPtr);
    static void         SetLocalAngles(CBaseEntity *pThisPtr, const QAngle& angles);
    static QAngle       GetLocalAngularVelocity( CBaseEntity *pThisPtr );
    static void         SetLocalAngularVelocity( CBaseEntity *pThisPtr, const QAngle &vecAngVelocity );
    static void         SetGravity( CBaseEntity *pThisPtr, float gravity );
    static void         SetFriction( CBaseEntity *pThisPtr, float flFriction );
    static void         SetModelName( CBaseEntity *pThisPtr, string_t name );
    static int          GetFlags(CBaseEntity *pThisPtr);
    static void         AddFlag(CBaseEntity *pThisPtr, int flags);
    static void         RemoveFlag(CBaseEntity *pThisPtr, int flagsToRemove);
    static void         ToggleFlag(CBaseEntity *pThisPtr, int flagToToggle);
    static bool         IsMarkedForDeletion( CBaseEntity *pThisPtr );
    static int          GetEFlags(CBaseEntity *pThisPtr);
    static void         SetEFlags(CBaseEntity *pThisPtr, int iEFlags );
    static void         AddEFlags(CBaseEntity *pThisPtr, int nEFlagMask );
    static void         RemoveEFlags(CBaseEntity *pThisPtr, int nEFlagMask );
    static bool         IsEFlagSet(CBaseEntity *pThisPtr, int nEFlagMask );
    static CCollisionProperty *CollisionProp( CBaseEntity *pThisPtr );
    static CServerNetworkProperty *NetworkProp( CBaseEntity *pThisPtr );
    static IPhysicsObject *VPhysicsGetObject( CBaseEntity *pThisPtr );
    static void         VPhysicsSetObject( CBaseEntity *pThisPtr, IPhysicsObject *pPhysics );
    static MoveType_t   GetMoveType( CBaseEntity *pThisPtr );
    static void         SetMoveType( CBaseEntity *pThisPtr, MoveType_t val, MoveCollide_t moveCollide = MOVECOLLIDE_DEFAULT );
    static void         FollowEntity( CBaseEntity *pThisPtr, CBaseEntity *pBaseEntity, bool bBoneMerge = true );
    static void         StopFollowingEntity( CBaseEntity *pThisPtr );
    static void         CheckHasGamePhysicsSimulation( CBaseEntity *pThisPtr );
    static bool         WillSimulateGamePhysics( CBaseEntity *pThisPtr );
    static int          GetCollisionGroup( CBaseEntity *pThisPtr );
    static void         SetCollisionGroup( CBaseEntity *pThisPtr, int collisionGroup );
    static void         SetSimulatedEveryTick( CBaseEntity *pThisPtr, bool sim );
    static void         SetAnimatedEveryTick( CBaseEntity *pThisPtr, bool sim );
    static char         GetTakeDamage( CBaseEntity *pThisPtr );
    static void         SetTakeDamage( CBaseEntity *pThisPtr, char takedamage );
    static int          GetSkin( CBaseAnimating *pThisPtr );
    static void         SetSkin( CBaseAnimating *pThisPtr, int nSkin );
    static int          GetSequence( CBaseAnimating *pThisPtr );
    static void         SetSequence( CBaseAnimating *pThisPtr, int nSequence );
    static void         ResetSequence( CBaseAnimating *pThisPtr, int nSequence );
    static float        GetDeathTime( CBasePlayer *pThisPtr );
    static void         SetDeathTime( CBasePlayer *pThisPtr, float flDeathTime );
    static int          GetPlayerClass( CBasePlayer *pThisPtr );
    static void         SetPlayerClass( CBasePlayer *pThisPtr, int PlayerClass );
    static int          GetDesiredPlayerClass( CBasePlayer *pThisPtr );
    static void         SetDesiredPlayerClass( CBasePlayer *pThisPtr, int PlayerClass );
    static int          GetNumBuildings( CBasePlayer *pThisPtr );
    static void         SetNumBuildings( CBasePlayer *pThisPtr, int numBuildings );
    static int          GetPlayerState( CBasePlayer *pThisPtr );
    static void         SetPlayerState( CBasePlayer *pThisPtr, int nPlayerState );
    static int          GetPlayerCond( CBasePlayer *pThisPtr );
    static void         SetPlayerCond( CBasePlayer *pThisPtr, int nPlayerCond );
    static unsigned short GetUber( CBasePlayer *pThisPtr );
    static void         SetUber( CBasePlayer *pThisPtr, unsigned short nUber );
    static unsigned short GetKritz( CBasePlayer *pThisPtr );
    static void         SetKritz( CBasePlayer *pThisPtr, unsigned short nUber );
    static bool         GetRageDraining( CBasePlayer *pThisPtr );
    static void         SetRageDraining( CBasePlayer *pThisPtr, bool bRageDraining );
    static float        GetChargeLevel( CBaseCombatWeapon *pThisPtr );
    static void         SetChargeLevel( CBaseCombatWeapon *pThisPtr, float flChargeLevel );
    static CHandle<CBaseEntity> GetOwnerEntity( CBaseEntity *pThisPtr );
    static void         SetOwnerEntity( CBaseEntity *pThisPtr, CBaseEntity *pOwner );
    static CHandle<CBaseCombatWeapon> GetActiveWeapon( CBasePlayer *pThisPtr );
    static float        GetNextAttack( CBasePlayer *pThisPtr );
    static void         SetNextAttack( CBasePlayer *pThisPtr, float flNextAttack );
    static RenderFx_t   GetRenderFX( CBaseEntity *pThisPtr );
    static void         SetRenderFX( CBaseEntity *pThisPtr, unsigned char fx );
    static RenderMode_t GetRenderMode( CBaseEntity *pThisPtr );
    static void         SetRenderMode( CBaseEntity *pThisPtr, unsigned char mode );
    static void         SetEffects( CBaseEntity *pThisPtr, int nEffects );
    static void         AddEffects( CBaseEntity *pThisPtr, int nEffects );
    static void         RemoveEffects( CBaseEntity *pThisPtr, int nEffects );
    static color32      GetRenderColor( CBaseEntity *pThisPtr );
    static void         SetRenderColor( CBaseEntity *pThisPtr, byte r, byte g, byte b );
    static void         SetRenderColor( CBaseEntity *pThisPtr, byte r, byte g, byte b, byte a );
    static void         SetRenderColorR( CBaseEntity *pThisPtr, byte r );
    static void         SetRenderColorG( CBaseEntity *pThisPtr, byte g );
    static void         SetRenderColorB( CBaseEntity *pThisPtr, byte b );
    static void         SetRenderColorA( CBaseEntity *pThisPtr, byte a );
    static CBaseEntity *GetThrower( CBaseEntity *pThisPtr );
    static void         SetThrower( CBaseEntity *pThisPtr, CBaseEntity *pThrower );
    static void         SetSolid( CBaseEntity *pThisPtr, SolidType_t val );
    static void         CCollProp_SetSolid( CCollisionProperty *pThisPtr, SolidType_t val );
    static void         ClearSolidFlags( CBaseEntity *pThisPtr );
    static void         RemoveSolidFlags( CBaseEntity *pThisPtr, int flags );
    static void         AddSolidFlags( CBaseEntity *pThisPtr, int flags );
    static int          GetSolidFlags( CBaseEntity *pThisPtr );
    static void         SetSolidFlags( CBaseEntity *pThisPtr, int flags );
    static string_t     GetTarget( CBaseEntity *pThisPtr );
    static string_t     GetEntityName( CBaseEntity *pThisPtr );
    static void         SetName( CBaseEntity *pThisPtr, string_t newName);
    static int          GetWaterLevel( CBaseEntity *pThisPtr );
    static QAngle       GetAbsAngles( CBaseEntity *pThisPtr );
    static int          GetImpulse( CBasePlayer *pThisPtr );
    static void         SetImpulse( CBasePlayer *pThisPtr, int impulse );
    static int          GetButtons( CBasePlayer *pThisPtr );
    static void         SetCombineBallRadius( CBaseEntity *pThisPtr, float flRadius );
    static int          CCollProp_GetSolidFlags( CCollisionProperty *pThisPtr );
    static void         CCollProp_SetSolidFlags( CCollisionProperty *pThisPtr, int flags );
    static void         SetCollisionBounds( CBaseEntity *pThisPtr, const Vector& mins, const Vector &maxs );
    static void         CCollProp_SetCollisionBounds( CCollisionProperty *pThisPtr, const Vector& mins, const Vector &maxs );
    static float        CCollProp_BoundingRadius( CCollisionProperty *pThisPtr );
    static Vector       CCollProp_OBBCenter( CCollisionProperty *pThisPtr );
    static Vector       CCollProp_GetMins( CCollisionProperty *pThisPtr );
    static Vector       CCollProp_GetMaxs( CCollisionProperty *pThisPtr );
    static FnCommandCallback_t GetCommandCallback( ConCommand *pThisPtr );
    static CEntityFactoryDictionary *GetEntityDictionary( ConCommand *pDumpEntityFactoriesCmd );

    static void         QuadBeamSetTargetPos( CBaseEntity *pThisPtr, const Vector& pos );
    static void         QuadBeamSetControlPos( CBaseEntity *pThisPtr, const Vector& control );
    static void         QuadBeamSetScrollRate( CBaseEntity *pThisPtr, float scrollRate );
    static void         QuadBeamSetWidth( CBaseEntity *pThisPtr, float width );

    static void         SpriteSetAttachment( CBaseEntity *pThisPtr, CBaseEntity *pEntity, int attachment );

    static int          GetTotalScore( CBaseEntity *pPlayerResource, int player );

    static void         SetEdictStateChanged(edict_t *pEdict, unsigned short offset);
};

#endif
