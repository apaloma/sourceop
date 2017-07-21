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

#ifndef ADMINOP_H
#define ADMINOP_H

#define PLUGIN_PHYSICS

class IPhysicsConstraint;
class IPhysics;
class IPhysicsSurfaceProps;
class IPhysicsCollision;
class IPhysicsEnvironment;
class IVPhysicsDebugOverlay;
class IPhysicsObjectPairHash;
class IServerTools;

class ISteamUser;
class ISteamGameServer010;

class CTakeDamageInfo;

struct lua_State;
class ClientCommands;
class CReservedSlots;
class CSOPGameRulesProxy;
class CMapCycleTracker;

//#define OFFICIALSERV_ONLY
#ifdef OFFICIALSERV_ONLY
    #define SourceOPVersion "SourceOP Internal Test Version 0.9.19"
    #define SourceOPVerShort "0.9.19"
    #pragma message( "INTERNAL VERSION " __FILE__ )
    #define SOPDLog(s) do { if(debug_log.GetBool()) pAdminOP.DebugLog(s); } while(0)
#else
    #define SourceOPVersion "SourceOP Version 0.9.18"
    #define SourceOPVerShort "0.9.18"
    #define SOPDLog(s) ((void)0)
#endif

#ifndef __linux__
#define SLASHSTRING "\\"
#define NOTSLASHSTRING "/"
#else
#define SLASHSTRING "/"
#define NOTSLASHSTRING "\\"
#endif

#define PHYSGUN_BEAM_SPRITE     "sprites/physbeam.vmt"

#include "tier1/strtools.h"
#include "utlvector.h"
#include "mathlib/vmatrix.h"

#include "shareddefs.h"

#include "interface.h"
#include "filesystem.h"
#include "engine/iserverplugin.h"
#ifdef _L4D_PLUGIN
#include "IEngineSound_l4d.h" 
#else
#include "engine/IEngineSound.h" 
#endif
#include "vstdlib/random.h"
#include "iplayerinfo.h"
#include "itempents.h"
#ifdef _L4D_PLUGIN
#include "convar_l4d.h"
#else
#include "convar.h"
#endif
#include "eiface.h"
#include "igameevents.h"
#include "shake.h"
#include "IEffects.h"
#include "IEngineTrace.h"
#include "icvar.h"
#include "icommandline.h"
#include "Color.h"
#include "utldict.h"
#include "utldict_old.h"
#include "netadr.h"
#include "steam/isteamuser.h"
#include "steamext.h"

#include "DFBaseent.h"
//#include "DFRadio.h"

#include "adminopplayer.h"
#include "votesystem.h"

#include "sopconsts.h"

#ifdef __linux__
#include <unistd.h>
#endif

#include "tier0/memdbgon.h"

class CAOPEntity;

#define INFO_PANEL_COMMAND_NONE             0
#define INFO_PANEL_COMMAND_JOINGAME         1
#define INFO_PANEL_COMMAND_CHANGETEAM       2
#define INFO_PANEL_COMMAND_IMPULSE101       3
#define INFO_PANEL_COMMAND_MAPINFO          4
#define INFO_PANEL_COMMAND_CLOSED_HTMLPAGE  5
#define INFO_PANEL_COMMAND_CHOOSETEAM       6

#define MIN_TEAM_NUM            0
#define MAX_TEAM_NUM            5
#define UNKNOWN_TEAM_NAME       "Unknown"

#define MAX_AOP_PLAYERS         256
#define MAX_PLAYERNAME_LENGTH   32
#define MAX_STEAMID_LENGTH      24  // STEAM_0:0:2147483647

#define MAX_RANKS 32
#define USR_MSGS_MAX 64
#define DF_MAX_ENTS 2048

#define MAX_DATADESC_CLASSNAME  62
#define MAX_DATADESC_MEMBERLEN  62
#define MAX_SENDTBL_LEN         62
#define MAX_SENDPROP_LEN        62

#define FOREGROUND_BLUE      0x0001 // text color contains blue.
#define FOREGROUND_GREEN     0x0002 // text color contains green.
#define FOREGROUND_RED       0x0004 // text color contains red.
#define FOREGROUND_INTENSITY 0x0008 // text color is intensified.
#define CONCOLOR_BLUE       FOREGROUND_BLUE
#define CONCOLOR_GREEN      FOREGROUND_GREEN
#define CONCOLOR_RED        FOREGROUND_RED
#define CONCOLOR_PURPLE     (CONCOLOR_RED | CONCOLOR_BLUE)
#define CONCOLOR_CYAN       (CONCOLOR_GREEN | CONCOLOR_BLUE)
#define CONCOLOR_BROWN      (CONCOLOR_GREEN | CONCOLOR_RED)
#define CONCOLOR_GRAY       (CONCOLOR_BLUE | CONCOLOR_GREEN | CONCOLOR_RED)
#define CONCOLOR_DARKGRAY   FOREGROUND_INTENSITY
#define CONCOLOR_LIGHTBLUE  (FOREGROUND_INTENSITY | CONCOLOR_BLUE)
#define CONCOLOR_LIGHTGREEN (FOREGROUND_INTENSITY | CONCOLOR_GREEN)
#define CONCOLOR_LIGHTRED   (FOREGROUND_INTENSITY | CONCOLOR_RED)
#define CONCOLOR_MAGENTA    (FOREGROUND_INTENSITY | CONCOLOR_RED | CONCOLOR_BLUE)
#define CONCOLOR_LIGHTCYAN  (FOREGROUND_INTENSITY | CONCOLOR_GREEN | CONCOLOR_BLUE)
#define CONCOLOR_YELLOW     (FOREGROUND_INTENSITY | CONCOLOR_GREEN | CONCOLOR_RED)
#define CONCOLOR_WHITE      (FOREGROUND_INTENSITY | CONCOLOR_BLUE | CONCOLOR_GREEN | CONCOLOR_RED)
#define CONCOLOR_LUA        CONCOLOR_YELLOW
#define CONCOLOR_LUA_ERR    CONCOLOR_BROWN
extern char linConColors[16][7];

extern char msg[2048];
extern int g_IgnoreColorMessages;

char * Q_strcasestr (const char * s1, const char * s2);

extern char * strTrim(const char * str);
extern char * strRemoveReturn(const char * str);
extern char * strRemoveQuote(const char * str);
extern char * strLeft(const char * str,int n);
extern char * strRight(const char * str, int n);
extern char *strrtrim(char *str, const char *trim = NULL);
extern char *strtrim(char *str, const char *trim = NULL);
extern bool DFIsAdminTutLocked( void );

#ifdef __linux__
typedef struct _LARGE_INTEGER {
    long LowPart;
    long HighPart;
    long long QuadPart;
} LARGE_INTEGER;

bool QueryPerformanceFrequency(LARGE_INTEGER *lpFrequency);
bool QueryPerformanceCounter(LARGE_INTEGER *lpFrequency);

#define __time64_t time_t
#define _time64 time
#define _ctime64 ctime

#endif

typedef struct precached_s
{
    int Type;
    char *FileName;
} precached_t;

typedef struct maplist_s
{
    int indexNumber;
    char map[251];
} maplist_t;

typedef struct forcedownload_s
{
    char *FileName;
} forcedownload_t;

typedef struct rankstruct_s
{
    int ranks;
    int creditmatch[MAX_RANKS];
    char rankname[MAX_RANKS][32];
    char finalname[MAX_RANKS];
} rankstruct_t;

typedef struct defaultuser_s
{
    admindata_t adminData;
    CUtlLinkedList <admindatacmd_t, unsigned short> listDenied;
    CUtlLinkedList <admindatacmd_t, unsigned short> listAdminAllow;
    CUtlLinkedList <admindatacmd_t, unsigned short> listAdminDenied;
} defaultuser_t;

typedef struct userweb_s
{
    double donAmt;
    char szSteamID[32];
    char szExtraInfo[128];
} userweb_t;

typedef struct datadesc_s
{
    char classname[MAX_DATADESC_CLASSNAME];
    char member[MAX_DATADESC_MEMBERLEN];
    int offset;
    int recursive_level;
} datadesc_t;

typedef struct sendprop_s
{
    char table[MAX_SENDTBL_LEN];
    char prop[MAX_SENDPROP_LEN];
    int offset;
    int stride;
    int type;
    int recursive_level;
    SendVarProxyFn proxyfn;
    SendTableProxyFn tableproxyfn;
    ArrayLengthSendProxyFn arraylenproxyfn;
} sendprop_t;

typedef struct radioloop_s
{
    char File[156];
    char Name[48];
    char ShortName[32];
    int Pitch;
    int Volume;
    int Dynamics;
    int NotLooped;
    int Radius;
} radioloop_t;

enum feature {
    FEAT_CREDITS = 0,
    FEAT_ADMINCOMMANDS,
    FEAT_ENTCOMMANDS,
    FEAT_ADMINSAYCOMMANDS,
    FEAT_PLAYERSAYCOMMANDS,
    FEAT_KILLSOUNDS,
    FEAT_MAPVOTE,
    FEAT_CVARVOTE,
    FEAT_REMOTE,
    FEAT_HOOK,
    FEAT_JETPACK,
    FEAT_SNARK,
    FEAT_RADIO,
    FEAT_LUA,
    NUM_FEATS
};

typedef struct classinstall_s
{
    IEntityFactory *pFactory;
    const char *pClassName;
    int feature;
} classinstall_t;

typedef struct trampolineinfo_s
{
    unsigned char *location;
    unsigned char *olddata;
    size_t length;
    bool insteam;
} trampolineinfo_t;


class CEmptyServerPlugin: public IServerPluginCallbacks, public IGameEventListener
{
public:
    CEmptyServerPlugin();
    ~CEmptyServerPlugin();

    // IServerPluginCallbacks methods
    virtual bool            Load( CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory );
    virtual void            Unload( void );
    virtual void            Pause( void );
    virtual void            UnPause( void );
    virtual const char     *GetPluginDescription( void );
    virtual void            LevelInit( char const *pMapName );
    virtual void            ServerActivate( edict_t *pEdictList, int edictCount, int clientMax );
    virtual void            GameFrame( bool simulating );
    virtual void            LevelShutdown( void );
    virtual void            ClientActive( edict_t *pEntity );
    virtual void            ClientDisconnect( edict_t *pEntity );
    virtual void            ClientPutInServer( edict_t *pEntity, char const *playername );
    virtual void            SetCommandClient( int index );
    virtual void            ClientSettingsChanged( edict_t *pEdict );
    virtual PLUGIN_RESULT   ClientConnect( bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen );
    virtual PLUGIN_RESULT   ClientCommand( edict_t *pEntity, const CCommand &args );
    virtual PLUGIN_RESULT   NetworkIDValidated( const char *pszUserName, const char *pszNetworkID );
    virtual void            OnQueryCvarValueFinished( QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue );

    // IGameEventListener Interface
    virtual void FireGameEvent( KeyValues * event );

    virtual int GetCommandIndex() { return m_iClientCommandIndex; }

    bool MainLoad( CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory );

private:
    bool VspLoad( CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory );

    int m_iClientCommandIndex;
};

extern CEmptyServerPlugin g_ServerPlugin;

class CGameEventListener: public IGameEventListener2
{
public:
    ~CGameEventListener( void ) {};
    virtual void FireGameEvent( IGameEvent *event );
    bool FireEventHook( IGameEvent *event, bool bDontBroadcast );
};

extern CGameEventListener g_eventListener;

abstract_class IMetamodOverrides
{
public:
    virtual ~IMetamodOverrides( void ) {}

    virtual void UnregisterConCommand(ConCommandBase *pConCmd) = 0;
};

extern IMetamodOverrides *g_metamodOverrides;

class CAdminOP
{
public:
    CAdminOP( void );

    void Load( void );
    void LevelInit( char const *pMapName );
    void ServerActivate( edict_t *pEdictList, int edictCount, int clientMax );
    void GameFrame( bool simulating );
    void RoundStart( void );
    void ArenaRoundStart( void );
    void RoundEnd( void );
    void SetupStart( void );
    void SetupEnd( void );
    void GameEvent( IGameEvent *gameEvent );
    void LevelShutdown( void );
    void PreShutdown( void );
    void Unload( void );

    void LoadCreditsFromFile( void );
    void SaveCreditsToFile( bool isUnloading = 0 );

    static bool UnhideDevCvars(void );
    void GrabDataDesc( CBaseEntity *pEntity );
    void RecurseDataMap( datamap_t* datamap, FILE *fp = NULL, int level = 0 );
    int GetDataDescFieldAnyClass(const char *pszMember);
    void GrabSendTables( void );
    void GrabSendProps(SendTable *pTable, int recursion = 0);
    void GrabSendTablesPrecalc( void );
    void GrabSendTablePrecalc(SendTable *pTable, FILE *fp);
    SendTableProxyFn GetPropDataTableProxyFn(const char *pszTable, const char *pszProp);
    int GetPropOffset(const char *pszTable, const char *pszProp);
    int GetPropOffsetAnyTable(const char *pszProp);
    int GetPropOffsetNotTable(const char *pszTable, const char *pszProp);
    void GetVTables( CBasePlayer *pPlayer );
    bool MemPatcher( void );
    bool HookSteamFromGameServerInit( void );
    bool HookSteam( void );
    bool HookSteamGameServer( void );
    void SetupTrampoline( BYTE *pbTarget, BYTE *pbHook, BYTE *pbTrampoline, bool bIsInSteamLib = false );
    void RemoveTrampolines();
    bool IsSteamLibLoaded();
    const char *GetGameDescription( void );
    bool ServerGameDLLLevelInit( const char *pMapName, char const *pMapEntities, char const *pOldLevel, char const *pLandmarkName, bool loadGame, bool background );
    edict_t *CreateEdictPre( int iForceEdictIndex );
    edict_t *CreateEdict( int iForceEdictIndex );
    void RemoveEdictPre( edict_t *e );
    IServerNetworkable *EntityCreated( const char *pClassName );
    void FreeEntPrivateData( void *pEntity );
    void FreeContainingEntity( edict_t *pEntity );
    void MarkEntitiesAsTouching( edict_t *e1, edict_t *e2 );
    void GetPlayerLimits( int& minplayers, int& maxplayers, int &defaultMaxPlayers );
    void EmitSound( IRecipientFilter& filter, int iEntIndex, int iChannel, const char *pSample, 
        float flVolume, soundlevel_t iSoundlevel, int iFlags = 0, int iPitch = PITCH_NORM, 
        const Vector *pOrigin = NULL, const Vector *pDirection = NULL, CUtlVector< Vector >* pUtlVecOrigins = NULL, bool bUpdatePositions = true, float soundtime = 0.0f, int speakerentity = -1 );
    bool SetClientListening(int iReceiver, int iSender, bool bListen);
    int ParmValue( const char *psz, int nDefaultVal );
    bool PrecacheSound( const char *pSample, bool bPreload = false, bool bIsUISound = false );
    void MaxPlayersDispatch( const CCommand &command );
    CCommand *MaxPlayersDispatchHandler( const CCommand &command );
    void LogPrint( const char *msg );
    void ChangeLevel( const char *s1, const char *s2 );
    void BlockNextChangeLevel( void ) { m_bChangeLevelBlock = true; }
    void UnblockNextChangeLevel( void ) { m_bChangeLevelBlock = false; }
    void ReplicateCVar( const char *var, const char *value );

private:
    void HookEntities();
    void HookEntityType(const char *pszEntName);
    bool IsEntityOnInstalledList(const char *pszEntName);
public:
    void ServiceEnt(CBaseEntity *pEntity, CAOPEntity *pNew);
    void UnhookEnt(CBaseEntity *pEntity, CAOPEntity *pNew, int index);
    void RemoveDFEnt( int entindex );
    void AddEntityToInstallList(IEntityFactory *pFactory, const char *pClassName, int feature);
    void ProcessInstallList();
    void PrecacheInstallList();
    void RemoveInstalledFactories();
    bool FakeClientCommand(edict_t *pEntity, bool needCheats, bool ConCmdOnly, const CCommand &args);
    bool FakeClientCommand(edict_t *pEntity, bool needCheats, bool ConCmdOnly, const char *szCommand, int argc = 1, const char *szArg1 = NULL, const char *szArg2 = NULL, const char *szArg3 = NULL);
    bool GetMeleeMode();
    void SetMeleeMode(bool enabled);
    PLUGIN_RESULT ClientConnect(bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen);
    void ClientActive(edict_t *pEntity);
    PLUGIN_RESULT ClientCommand(edict_t *pEntity, const CCommand &args);
    PLUGIN_RESULT NetworkIDValidated( const char *pszUserName, const char *pszNetworkID );
    void OnQueryCvarValueFinished( QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue );
    void ClientDisconnect(edict_t *pEntity);
    void RemovePlayerFromVotes(int uid);
    void EndMapVote(void);
    void CvarChanged(const char *cvarname, const char *cvarvalue);

    const char *DataDir(void);
    FORCEINLINE const char *GameDir(void) { return gameDir; }
    FORCEINLINE const char *ModName(void) { return modName; }
    CSteamID GetServerSteamID(void);
    FORCEINLINE const char *CurrentMap(void) { return szCurrentMap; }
    void UpdateNextMap(void);
    FORCEINLINE const char *NextMap(void) { UpdateNextMap(); return nextmap; }
    FORCEINLINE edict_t *GetEntityList(void) {return pEList; }
    static CBaseEntity *GetEntity(int index);
    // GetPlayerResource used in getting a player's score
    CBaseEntity *GetPlayerResource() { if(bPlayerResourceCached) return pPlayerResource; else return FindPlayerResource(); }
    CBaseEntity *FindPlayerResource();
    ISteamGameServer010 *SteamGameServer() { return m_pSteamGameServer; }
    FORCEINLINE int GetMaxClients(void) { return maxClients; }
    int GetPlayerCount(void);
    int GetPlayerCountPlaying(void);
    int GetConnectedPlayerCount(void);
    int GetVisibleMaxPlayers(int *maxplayers = NULL);
    edict_t *GetPlayer(int index);
    bool RoundHasBegun();
    void OverrideMapTimeRemaining(int timeleft);
    int GetMapTimeRemaining();
    int GetTimeLimit();
    FORCEINLINE CSOPGameRulesProxy *GameRules() { return gameRules; }
    FORCEINLINE CMapCycleTracker *MapCycleTracker() { return mapCycleTracker; }
    CVoteSystem *GetMapVote(void);
    const char *TeamName(int team);
    void OnGoToIntermission();
    void OnHandleSwitchTeams();
    void OnHandleScrambleTeams();

    bool AddVacAllowedPlayer(CSteamID steamid);
    bool RemoveVacAllowedPlayer(CSteamID steamid);
    void ClearVacAllowedPlayerList() { vacAllowList.Purge(); }
    void PrintVacAllowedPlayerList();
    bool VacAllowPlayer(CSteamID steamid);

    ConCommandBase *GetCommands();
    int IndexOfEdict(const edict_t *pEdict);
    edict_t *PEntityOfEntIndex(int iEntIndex) {
        if(iEntIndex < 0 || iEntIndex >= gpGlobals->maxEntities)
        {
            return NULL;
        }
        if(!pEList)
        {
            return NULL;
        }
        return pEList + iEntIndex;
    }

    int PlayerSpeakPre(int entID, const char *text);
    int PlayerSpeakTeamPre(int entID, const char *text);
    void PlayerSpeak(int iPlayer, int userid, const char *text);
    void PlayerDeath(int iPlayer, int iAttacker, const char *pszWeapon);
    bool SetNoclip(int iPlayer, int iType);
    bool SetGod(int iPlayer, int iType);
    bool SlapPlayer(int iPlayer);
    bool SlayPlayer(int iPlayer);
    bool KickPlayer(IPlayerInfo *info, const char *pszReason = NULL);
    bool BanPlayer(int index, IPlayerInfo *info, CSteamID bannedID, int time, const char *pszBannerName, const char *pszBannerID, const char *pszReason, bool ip, const char *pszExtra = NULL);
    int FindPlayer(int *playerList, const char *pszName);
    int FindPlayerByName(int *playerList, const char *pszName);
    int FindPlayerWithName(int *playerList, const char *pszName);
    int FindPlayerByUserID(int *playerList, const int iID);
    int FindPlayerBySteamID(int *playerList, const char *pszSteam);

    const char *GetMessageName(int iMsg);
    int GetMessageByName(const char * szMsg);

    void PropogateMapList();
    void InitCommandLineCVars();
    void LoadFeatures();
    void LoadPrecached();
    void LoadForceDownload();
    void LoadSpawnAlias();
    const spawnaliasdata_t *FindModelFromAlias(const char * pszAlias);
    const spawnaliasdata_t *FindAliasFromModel(const char * pszModel);
    void LoadAdmins();
    void LoadRank();
    const char *GetRank(int credits);
    void LoadRadioLoops();

    bool MapVoteInProgress();

    static void DisableColorMsg() { g_IgnoreColorMessages++; }
    static void EnableColorMsg() { g_IgnoreColorMessages--; }
    static void ColorMsg(int color, const char *pszMsg, ...);
    void SayTextAll(const char *pText);
    void SayTextAll(const char *pText, int type, bool sendClient = 1, int teamFilter = -1);
    void SayTextAllChatHud(const char *pText, bool sendClient = 1);
    void SetSayTextPlayerIndex(int player) { m_iSayTextPlayerIndex = player; }
    void HintTextAll(const char *pText);
    void PlaySoundAll(const char *sound);

    void TimeLog(char * File, char * Text, ...);
    void DebugLog(char *Text);
    void LockAdminTut(void);

    CBaseEntity *CreateTurret(const char *name, unsigned int team, Vector origin, QAngle angles, CAdminOPPlayer *pAOPPlayer);
    static CBaseEntity *CreateSpriteTrail(const char *pSpriteName, const Vector &origin, bool animate);

    bool CvarVoteInProgress();
    typedef void (__cdecl* CvarVoteCallback)( CAdminOP *aop );
    void StartCvarVote(ConVar *cvar, const char *pszMessage, const char *choice1, int value1, const char *choice2, int value2, ConVar *ratio, ConVar* duration, CvarVoteCallback endcallback);
    void AddPlayerToCvarVote(IPlayerInfo *playerinfo, int choice);
    void PrintCvarVoteStandings();
    void EndCvarVote();
    static void JetpackVoteFinish(CAdminOP *aop);

    void LoadLua();
    int LoadLuaAutorunScripts(const char *path);
    int LoadLuaEntities(const char *path);
    FORCEINLINE lua_State *GetLuaState() { return luaState; }
    void CloseLua();

    FORCEINLINE bool FeatureStatus(int feature) { return featureStatus[feature]; }

private:
    void CopyData(int size, void *pDataDest, const void *pDataSrc);
    bool CopyBasicField(void *pDataDest, const void *pDataSrc, typedescription_t *pDest, typedescription_t *pSrc);
    bool CopyGameField(void *pDataDest, const void *pDataSrc, typedescription_t *pDest, typedescription_t *pSrc);
    void CopyField(void *pDataDest, const void *pDataSrc, typedescription_t *pDest, typedescription_t *pSrc);
    void CopyDataFields(void *pBaseDataDest, const void *pBaseDataSrc, typedescription_t *pFieldsDest, typedescription_t *pFieldsSrc, int fieldCount);
public:
    void CopyDataMap(void *pLeafObjectDest, const void *pLeafObjectSrc, datamap_t *dest, datamap_t *src);
    void CopyDataDesc(CBaseEntity *dest, CBaseEntity *src);

    CUtlLinkedList <precached_t, unsigned short> precached;
    CUtlLinkedList <char *, unsigned short> downloads;
    CUtlLinkedList <maplist_t, unsigned short> mapList;
    CUtlVector <creditsram_t> creditList;
    CUtlMap <uint64, int, int> m_mapSteamIDToCreditEntry;
    CUtlLinkedList <edict_t *, unsigned int> entList;           // list of all entities
    CUtlVector <CAOPEntity *> myEntList;                        // hooked entities that we are managing
    CUtlVector <CAOPEntity *> myThinkEnts;                      // entities that we are managing thinks for
    CUtlVector <classinstall_t> installList;
    CUtlVector <datadesc_t> dataDesc;
    CUtlVector <sendprop_t> sendProps;
    CUtlVector <trampolineinfo_t> trampolines;
    CUtlVector <CSteamID> vacAllowList;
    CUtlLinkedList <userweb_t, unsigned int> banList;
    CUtlLinkedList <userweb_t, unsigned int> donList;
    CUtlLinkedList <userweb_t, unsigned int> vipList;
    CUtlLinkedList <userweb_t, unsigned int> sipList;
    CUtlLinkedList <userweb_t, unsigned int> devList;
    CUtlLinkedList <admindata_t, unsigned short> adminData;
    CUtlLinkedList <spawnaliasdata_t, unsigned short> spawnAlias;
    CUtlLinkedList <admindatacmd_t, unsigned short> listDenied;
    CUtlLinkedList <admindatacmd_t, unsigned short> listAdminAllow;
    CUtlLinkedList <admindatacmd_t, unsigned short> listAdminDenied;
    defaultuser_t defaultUser;
    CUtlVector <radioloop_t> radioLoops;

    void *pServer;
    char **sTeamNames;
    bool isHl2mp;
    bool isCstrike;
    bool isDod;
    bool isFF;
    bool isTF2;
    bool isClient;
    bool hasDownloadUsers;
    int serverPort;

    CAdminOPPlayer pAOPPlayers[MAX_AOP_PLAYERS];
    float TimeStarted;
    bool ShuttingDown;
    bool MapShutDown;
    bool saveCredits;
    bool attemptedCreditLoad;
    char adminname[32];
    int m_iClientCommandIndex;
    int m_iSayTextPlayerIndex;
    float nextMapPosUpdate;
    int blockLogOutputToClient;
    bool m_bEndRoundAllTalk;

    bool getCreateEdict;
    edict_t *pCreateEdict;
    CBaseEntity *pTemp;
    edict_t *pRemoveEntity;

    CUtlLinkedList <unsigned int, unsigned short> spawnedServerEnts;

    typedef struct killtrack_s
    {
        bool firstblood;
    } killtrack_t;
    killtrack_t Kills;

    float jetpackVoteNextTime;

    CReservedSlots *rslots;
    float m_flNextSlotsRefresh;

    int m_iStatsResult;
    int m_iStatsRank;
    unsigned int m_iStatsTotalConnects;
    unsigned int m_iStatsTotalMinutesPlayed;

    typedef struct storedgameserverinit_s
    {
        uint32 unIP;
        uint16 usPort;
        uint16 usGamePort;
        uint16 usQueryPort;
        uint32 unServerFlags;
        char pszVersionString[256];
        bool bLanMode;
    } storedgameserverinit_t;
    storedgameserverinit_t StoredGameServerInitParams;
private:
    edict_t *pEList;
    int maxClients;
    char gameDir[256];
    char modName[256];
    char nextmap[32];
    char szCurrentMap[32];
    bool inRound;
    CSOPGameRulesProxy *gameRules;
    CUtlVector <classinstall_t> installedFactories;
    CMapCycleTracker *mapCycleTracker;
    bool m_bChangeLevelBlock;
    int voteExtendCount;
    CVoteSystem mapVote;

    ISteamUser *m_pSteamUser;
    ISteamGameServer010 *m_pSteamGameServer;

    CBaseEntity *pPlayerResource;
    bool bPlayerResourceCached;
    bool bHasDataDesc;
    bool bHasPlayerDataDesc;

    float m_flNextHeartbeat;

    int changeMapTime;
    char szChangeMap[256];

    int overrideMapTime;            // the time remaining override
    float overrideMapTimeSetTime;   // when that override was set
    int overrideMapTimeLimit;       // the timelimit when the override was set

    char userMessages[USR_MSGS_MAX][64];

    rankstruct_t ranks;

    unsigned long serverProcessID; //DWORD

    CVoteSystem cvarVote;
    ConVar      *cvarVoteCvar;
    int         cvarVoteValue1;
    int         cvarVoteValue2;
    char        cvarVoteChoice1[32];
    char        cvarVoteChoice2[32];
    ConVar      *cvarVoteRatio;
    CvarVoteCallback cvarVoteEndCallback;

    lua_State *luaState;

    ClientCommands *clientCommands;

    bool featureStatus[NUM_FEATS];

    bool meleeonly;
};

//-----------------------------------------------------------------------------
// Entity creation factory
//-----------------------------------------------------------------------------
class CEntityFactoryDictionary : public IEntityFactoryDictionary
{
public:
    CEntityFactoryDictionary();

    virtual void InstallFactory( IEntityFactory *pFactory, const char *pClassName );
    virtual IServerNetworkable *Create( const char *pClassName );
    virtual void Destroy( const char *pClassName, IServerNetworkable *pNetworkable );
    virtual const char *GetCannonicalName( const char *pClassName );
    void ReportEntitySizes();

private:
    IEntityFactory *FindFactory( const char *pClassName );
public:
    CUtlDict< IEntityFactory *, unsigned short > m_Factories;
};

class CEntityFactoryDictionaryOld : public IEntityFactoryDictionary
{
public:
    CEntityFactoryDictionaryOld();

    virtual void InstallFactory( IEntityFactory *pFactory, const char *pClassName );
    virtual IServerNetworkable *Create( const char *pClassName );
    virtual void Destroy( const char *pClassName, IServerNetworkable *pNetworkable );
    // Don't use this one
    virtual const char *GetCannonicalName( const char *pClassName );
    void ReportEntitySizes();

private:
    IEntityFactory *FindFactory( const char *pClassName );
public:
    CUtlDictOld< IEntityFactory *, unsigned short > m_Factories;
};

//-----------------------------------------------------------------------------
// Purpose: Helper for UTIL_FindClientInPVS
// Input  : check - last checked client
// Output : static int UTIL_GetNewCheckClient
//-----------------------------------------------------------------------------
// FIXME:  include bspfile.h here?
class CCheckClient /*: public CAutoGameSystem*/
{
public:
    CCheckClient( char const *name ) /*: CAutoGameSystem( name )*/
    {
    }

    void LevelInitPreEntity()
    {
        m_checkCluster = -1;
        m_lastcheck = 1;
        m_lastchecktime = -1;
        m_bClientPVSIsExpanded = false;
    }

    byte	m_checkPVS[MAX_MAP_LEAFS/8];
    byte	m_checkVisibilityPVS[MAX_MAP_LEAFS/8];
    int		m_checkCluster;
    int		m_lastcheck;
    float	m_lastchecktime;
    bool	m_bClientPVSIsExpanded;
};
extern CCheckClient g_CheckClient;

// Interfaces from the engine
#include "interfaces.h"
extern CreateInterfaceFn physicsFactory;

extern short g_sModelIndexSmoke;

extern CAdminOP pAdminOP;

extern CBaseEntity *CreateCombineBall( const Vector &origin, const Vector &velocity, float radius, float mass, float lifetime, CBaseEntity *pOwner );
typedef void            (__cdecl* PlantBombFunc)( void );
typedef CBaseEntity*    (__cdecl* _CreateEntityByNameFunc)( const char *className, int iForceEdictIndex );
typedef CBaseEntity*    (__cdecl* _CreateCombineBallFunc)( const Vector &origin, const Vector &velocity, float radius, float mass, float lifetime, CBaseEntity *pOwner );
typedef void            (__cdecl* _ClearMultiDamageFunc)( void );
typedef void            (__cdecl* _ApplyMultiDamageFunc)( void );
typedef void            (__cdecl* _RadiusDamageFunc)( const CTakeDamageInfo &info, const Vector &vecSrc, float flRadius, int iClassIgnore, CBaseEntity *pEntityIgnore );
typedef void*           _SetMoveTypeFunc;
typedef void*           _ResetSequenceFunc;
typedef int             (__cdecl* _ReceiveDatagramFunc)( int a, void *structure );
typedef void            (__cdecl* _SendPacketFunc)( void *netchan, int a, const netadr_t &sock, unsigned char const *data, int length, bf_write *bitdata, bool b );
typedef void            (__cdecl* _SV_BVDFunc)(void *, int, char*, long long);
#ifdef __linux__
typedef int             (__cdecl* _usleepfunc)(useconds_t);
#endif

typedef bool            (__cdecl* PFNSteam_GameServer_InitSafe)( uint32 unIP, uint16 usPort, uint16 usGamePort, uint16 usSpectatorPort, uint16 usQueryPort, EServerMode eServerMode, const char *pchGameDir, const char *pchVersionString );

extern PlantBombFunc PlantBomb;
extern _CreateEntityByNameFunc _CreateEntityByName;
extern _CreateCombineBallFunc _CreateCombineBall;
extern _ClearMultiDamageFunc _ClearMultiDamage;
extern _ApplyMultiDamageFunc _ApplyMultiDamage;
extern _RadiusDamageFunc _RadiusDamage;
extern _SetMoveTypeFunc _SetMoveType;
extern _ResetSequenceFunc _ResetSequence;
extern _ReceiveDatagramFunc _NET_ReceiveDatagram;
extern int (*VCR_Hook_recvfrom)(int s, char *buf, int len, int flags, struct sockaddr *from, int *fromlen);
extern _SendPacketFunc _NET_SendPacket;
extern _SV_BVDFunc _SV_BroadcastVoiceData;
#ifdef __linux__
extern _usleepfunc _dedicated_usleep;
#endif
extern PFNSteam_BGetCallback _BGetCallback;
extern PFNSteam_FreeLastCallback _FreeLastCallback;
extern PFNSteam_GameServer_InitSafe _SteamGameServer_InitSafe;

extern int g_nServerToolsVersion;

extern bool g_bIsVsp;
extern IConCommandBaseAccessor *pConCommandAccessor;

extern char g_szA2SInfoCache[1024];
extern int g_iA2SInfoCacheSize;
extern unsigned int g_iConnectionlessThisFrame;
extern unsigned int g_iConsecutiveConnectionlessOverLimit;
extern bool g_bShouldWriteOverLimitLog;

extern void UTIL_ForceRemove( IServerNetworkable *oldObj );
extern int FindPassableSpace( CBaseEntity *pPlayer, const Vector& direction, float step, Vector& oldorigin );

extern void UTIL_BanPlayerByID(const char *pszName, int playeruserid, void *pBaseClient, CSteamID networkID, const char *pszIP, const char *pszBannerName, const char *pszBannerID, const char *pszMap, unsigned int duration, const char *pszReason, const char *pszExtra = NULL);
extern void UTIL_BanPlayerByIP(const char *pszName, int playeruserid, void *pBaseClient, CSteamID networkID, const char *pszIP, const char *pszBannerName, const char *pszBannerID, const char *pszMap, unsigned int duration, const char *pszReason, const char *pszExtra = NULL);

#ifdef __linux__
extern void *ResolveSymbol(void *handle, const char *symbol);
#endif

#endif
