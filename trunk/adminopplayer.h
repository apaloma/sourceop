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

#ifndef ADMINOPPLAYER_H
#define ADMINOPPLAYER_H

#include "vphysics_interface.h"
#include "pingmanager.h"
#include "sopsteamid.h"

#include "tier0/memdbgon.h"

class CBaseCombatWeapon;

#define CRED_SAVEERROR_NOUSERID     2
#define CRED_SAVEERROR_NOINFO       3
#define CRED_SAVEERROR_NOTCONNECTED 4
#define CRED_SAVEERROR_PENDINGLOAD  5
#define CRED_SAVEERROR_GETPOS       6
#define CRED_SAVEERROR_SETPOS       7
#define CRED_SAVEERROR_BADDATA      8

#define AOP_PL_GAGGED               (1<<0)
#define AOP_PL_NOCLIP               (1<<1)
#define AOP_PL_GOD                  (1<<2)

enum tf2class {
    TF2_CLASS_UNKNOWN = 0,
    TF2_CLASS_SCOUT,
    TF2_CLASS_SNIPER,
    TF2_CLASS_SOLDIER,
    TF2_CLASS_DEMOMAN,
    TF2_CLASS_MEDIC,
    TF2_CLASS_HEAVY,
    TF2_CLASS_PYRO,
    TF2_CLASS_SPY,
    TF2_CLASS_ENGINEER,
    TF2_CLASS_CIVILIAN,
};

#define IP_SIZE                     32

#define VOTEMENU_MAPS               6
#define JETPACK_MAXMESSAGELEVEL     30000

#define THRUSTGRP_LEN               32


typedef struct  thismap_s
{
    int             totalawarded;       // total number of credits awarded this map
} thismap_t;

typedef struct credits_t
{
    char            WonID[24];      //player's wonid (authid)
    int             credits;        //amount of credits
    int             timeonserver;   //the time the player has been on the server
    unsigned long   lastsave;       //time in seconds since UTC 1/1/70
    int             totalconnects;  //previously total credits
    int             iuser1;         //stores flags
    char            emptyspace[64]; //for future use
    char            FirstName[36];  //stores the player's first ever name used
    char            LastName[36];   //stores the player's last used name
    char            CurrentName[36];//stores the player's name during save
} credits_t;

typedef struct creditsram_s
{
    char            WonID[24];      //player's wonid (authid)
    CSteamID        steamid;
    int             credits;        //amount of credits
    int             timeonserver;   //the time the player has been on the server
    unsigned long   lastsave;       //time in seconds since UTC 1/1/70
    int             totalconnects;  //previously total credits
    int             iuser1;         //stores flags
    //float         fuser1;
    //char          suser1[24];
    char            FirstName[36];  //stores the player's first ever name used
    char            LastName[36];   //stores the player's last used name
    char            CurrentName[36];//stores the player's name during save
    thismap_t       thismap;
} creditsram_t;

typedef struct creditsver_s
{
    char        type[12];
    int         ver;
} creditsver_t;

typedef struct admindatacmd_s
{
    int indexNumber;
    char cmd[64];
} admindatacmd_t;

#define ADMIN_TYPE_NAME     0
#define ADMIN_TYPE_STEAMID  1
#define ADMIN_TYPE_IP       2
#define ADMIN_TYPE_DEFAULT  3

#define ADMIN_INDEX_DEFAULT 0xFFFFFFFF

#define SPAWN_LIMIT_USE_CVAR -2

struct admindata_t
{
    int indexNumber;
    int type;
    char id[128];
    int baseLevel;
    char password[24];
    int spawnLimit;
    bool loggedIn;
};

struct spawnaliasdata_t
{
    int indexNumber;
    BYTE spawnMode;
    char name[60];
    char model[191];
};

struct thruster_t
{
    bool enabled;
    char group[THRUSTGRP_LEN];
    CBaseEntity *entattached;
    CBaseEntity *entthruster;
    int force;
    IPhysicsConstraint *pConstraint;
};

typedef struct cvarquery_s
{
    int luacallback;
    int cookie;
} cvarquery_t;

enum EFlagEvent {
    FLAGEVENT_PICKUP = 1,
    FLAGEVENT_CAPTURE,
    FLAGEVENT_DEFEND,
    FLAGEVENT_DROPPED
};

class CGravControllerPoint : public IMotionEvent
{
public:
    CGravControllerPoint( void );
    ~CGravControllerPoint( void );
    void AttachEntity( CBaseEntity *pEntity, IPhysicsObject *pPhys, const Vector &position );
    void DetachEntity( void );
    void SetMaxVelocity( float maxVel )
    {
        m_maxVel = maxVel;
    }
    void SetTargetPosition( const Vector &target )
    {
        m_targetPosition = target;
        if ( m_attachedEntity == NULL )
        {
            m_worldPosition = target;
        }
        m_timeToArrive = gpGlobals->frametime;
    }

    IMotionEvent::simresult_e Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular );
    Vector          m_localPosition;
    Vector          m_targetPosition;
    Vector          m_worldPosition;
    Vector          m_localAlignNormal;
    Vector          m_localAlignPosition;
    Vector          m_targetAlignNormal;
    Vector          m_targetAlignPosition;
    bool            m_align;
    float           m_saveDamping;
    float           m_maxVel;
    float           m_maxAcceleration;
    Vector          m_maxAngularAcceleration;
    CBaseEntity*    m_attachedEntity;
    QAngle          m_targetRotation;
    float           m_timeToArrive;

    IPhysicsMotionController *m_controller;
};

class CAdminOPPlayer
{
public:
    CAdminOPPlayer();
    ~CAdminOPPlayer();

    void Connect();
    void Activate();
    void NetworkIDValidated(const char *pszUserName, const char *pszNetworkID);
    PLUGIN_RESULT ClientCommand(const CCommand &args);
    int PlayerSpeakPre(const char *text);
    int PlayerSpeakTeamPre(const char *text);
    void Disconnect();
    void SessionEnd();
    void PluginUnloading();
    void Cleanup();
    void LevelShutdown();
    void FreeContainingEntity( edict_t *pEntity );
    void Think();
    void HighResolutionThink();
    void UpdateInfoString(IPlayerInfo *info);
    int GetPlayerState();
    void SetEntID(int id);
    void SetJoinName(const char *pszName);
    void NameChanged(const char *pszOldName, const char *pszNewName);
    void OnKill(int iPlayer, const char *pszWeapon);
    void OnDeath(int iAttacker, const char *pszWeapon);
    void OnSpawn();
    void OnChangeTeam(int newteam);
    void OnChangeClass(int newclass);
    void OnPostInventoryApplication();
    void RoundEnd();
    void FlagEvent(EFlagEvent eventtype);
    FORCEINLINE const char *GetIP() { return IP; }
    Vector GetPosition();
    int GetTeam();
    const char *GetTeamName();
    int GetPlayerClass();
    void GetMyUserData(const char *pszName = NULL);
    void UpdateAdminLevel();
    bool IsAlive(void);
    int IsAdmin(int level);
    int IsAdmin(int level, const char *szCmd);
    int IsDenied(const char *szCmd);
    inline bool IsGagged() { return gagged; }
    inline void GagPlayer(bool gag) { gagged = gag; }
    inline bool NotBot(void) { return notBot; }
    inline bool IsSteamIDValidated(void) { return m_bSteamIDValidated; }
    inline bool HasFlag(void) { return m_bFlagCarrier; }
    bool IsHiddenFrom(int iPlayerEntIndex);
    void Slap();
    void Kill();
    void SetHealth(int iHealth);
    void SetPlayerClass(int iPlayerClass);
    void SetTeam(int iTeam);
    void DoDamage(int attacker, int dmg, const char *pszWeapon);
    void HookOff();
    void JetpackActivate();
    bool JetpackActive() { return Jetpack.OnJetpack; }
    void JetpackOff();
    bool SetNoclip(int iType);
    bool SetGod(int iType);
    bool HasNoclip();
    bool HasGod();
    BYTE GetFlagByte();
    int GetScore();

    void InitPlayerData();
    void ZeroCredits();
    int LoadCredits(bool isReload = 0);
    int SaveCredits(bool isShutdown);
    inline int GetCredits() {
        return Credits.CreditsDuringGame + Credits.CreditsJoin; }
    inline int GetTimePlayed() {
        return Credits.TimeJoin + (engine->Time() - Credits.TimeJoined); }
    inline int GetConnections() {
        return Credits.totalconnects; }
    inline const char *GetLastName() { return Credits.LastName; }
    inline const char *GetFirstName() { return Credits.FirstName; }
    inline const char *GetJoinName() { return Credits.JoinName; }
    const char *GetCurrentName();
    inline CSteamID GetSteamID() { return steamid; }
    inline int GetFakeID() { return m_iFakeID; }
    inline float GetConnectTime() {
        return connecttime; }
    inline void SetConnectTime(float time) {
        connecttime = time; }
    inline int64 GetSessionStartTime() {
        return sessionstarttime; }
    int GetSpawnLimit();
    void AddCredits(int credits);
    void SendCreditCounter();
    void SayText(const char *pText, int type = HUD_PRINTTALK);
    void SayTextChatHud(const char *pText);
    void SayTextByPlayer(const char *pText, int player = -1);
    void ShowViewPortPanel( const char * name, bool bShow = true, KeyValues *data = NULL );
    void ShowVoteMenu( unsigned int startmap, int level = 3 );
    void ShowBindMenu( const char *description, const char *command, unsigned int menuopt );
    void ShowRadioMenu( const CCommand &args );
    void PlaySoundLocal(const char *sound);
    void LeavePrivateChat();
    int GetVoiceTeam() { return m_iVoiceTeam; }
    void SetVoiceTeam(int team) { m_iVoiceTeam = team; }

    Vector EyePosition();
    CBaseEntity *FindEntityForward(unsigned int mask = MASK_PLAYERSOLID, Vector *endpos = NULL);
    CBaseEntity *GiveNamedItem(const char *pszName, int iSubType = 0);
    CBaseEntity *GiveTFWeapon(const char *pszName);
    bool GiveTouchDel( const char *pszName );
    bool GiveAllWeapons();
    bool GiveFullAmmo();
    bool SetUberLevel(float uberlevel);
    bool Unstick();

    void ThrowFragGrenade(bool drop = false);
    void ThrowSecondaryGrenade();

    void ThrusterOn(thruster_t *thruster, bool reverse = 0);
    void ThrusterOff(thruster_t *thruster, bool reverse = 0);
    void DeleteThruster(thruster_t *thruster);

    void ShowHelpPage(const char *pszURL);

    void AttachGrabbedPhysObject( CBaseEntity *pObject, const Vector& start, const Vector &end, float distance );
    void DropGrabbedPhysObject();

    void *baseclient;
    float connecttime;
    int64 sessionstarttime;
    unsigned long addr;
    char IP[IP_SIZE];
    unsigned char ipocts[4];
    double nextthink;

    CBaseCombatWeapon *pActiveWeapon;

    float NoclipTime;
    float GodTime;

    bool entMoving;
    bool entPhysMoving;
    struct physmoving_t
    {
        CBaseEntity *pBeam;
        CBaseEntity *m_hObject;
        bool        m_useDown;
        float       m_distance;
        float       m_movementLength;
        float       m_lastYaw;
        Vector      m_originalObjectPosition;
        CGravControllerPoint m_gravCallback;
    };
    physmoving_t PhysMoving;
    bool entRotating;
    edict_t *pEntMove;
    RenderMode_t origRM;
    color32 origRC;
    int origRF;
    Vector origPlayerLoc;
    Vector origEntLoc;
    QAngle origPlayerAngle;
    QAngle origEntAngle;

    bool hasDon;
    double donAmt;
    bool hasVip;
    bool hasSip;
    bool sipLogin;
    bool hasDev;
    bool isAlive;
    char userExtraInfo[128];
    char infoString[18];
    char oldInfo[18];
    bool hasInfo;
    bool sendDup;

    bool m_bItemsLoaded;

    struct PrivateChat
    {
        bool RemotePrivChat;
        int RemotePrivChatID;
        bool LocalPrivChat;
        int LocalPrivChatID;
    };
    PrivateChat PrivateChat;

    CUtlLinkedList <unsigned int, unsigned short> spawnedEnts;

    CUtlVector <cvarquery_t> m_pendingCvarQueries;

private:

    PLUGIN_RESULT DoBuyCommand(const CCommand &args);
    PLUGIN_RESULT DoRadioCommands(const char *pcmd, const CCommand &args, IPlayerInfo *playerinfo, CBasePlayer *pPlayer);
#ifdef OFFICIALSERV_ONLY
    PLUGIN_RESULT DoTestFuncs(const char *pcmd, const CCommand &args, IPlayerInfo *playerinfo, CBasePlayer *pPlayer);
#endif

    int entID;
    int uid;
    bool m_bSteamIDValidated;
    bool m_bCustomSteamIDValidated;
    CSteamID steamid;
    int m_iFakeID;
    bool gagged;
    bool notBot;
    bool adminTutBlockSay;
    char adminTutSaidSteamID[64];
    int adminTutPage;
    bool hasAdminData;
    admindata_t adminData;
    CUtlLinkedList <admindatacmd_t, unsigned short> listDenied;
    CUtlLinkedList <admindatacmd_t, unsigned short> listAdminAllow;
    CUtlLinkedList <admindatacmd_t, unsigned short> listAdminDenied;
    DFPingManager pingManager;
    int m_iVoiceTeam;
    bool m_bFlagCarrier;

    float nextBuildTime;

    int m_iGrenades;
    int m_iSecondaryGrenades;
    float m_flGrenadeDetonateTime;
    float m_flSecondaryGrenadeDetonateTime;
    
    int playerState;

    float nextAdminUpdate;
    float nextRemoteUpdate;
    float nextSmallRemoteUpdate;

    CBaseEntity *pTemp;
    CUtlLinkedList <IPhysicsConstraint*, unsigned short> physConstraints;
    CUtlVector <thruster_t> thrusters;

    struct Credits
    {
        int CreditsDuringGame;
        int CreditsJoin;
        int PreviousCredits;
        float LastCreditCheck;
        float LastCreditGive;
        int TimeJoin;
        float TimeJoined;
        int totalconnects;
        //float AmmoAvailable;
        //bool HasSpeed;
        //bool HasNuke;
        //bool HasScuba;
        //int BoughtSlay;
        float CheatTimer;
        float CheatCount;
        int PendingLoad;
        float NextLoad;
        int CheatNum; //keep track of how many times they were caught cheating
        int uid;
        char WonID[24]; //Cause GETPLAYERAUTHID doesn't work in ClientDisconnect :S (old - hl1)
        char JoinName[36]; //name used when joined
        char LastName[36]; //name stored when loading credits for the users last name
        char FirstName[36];
        char NameTest[36];
        thismap_t thismap;
    };
    Credits Credits;
    struct Hook
    {
        int OnHook;
        float LastHook;
        float HookOff;
        float NextHitSound;
        Vector HookedAt;
        EHANDLE HookedEnt;
        EHANDLE HookEnt; //the hook entity itself
        //edict_t *BeamEnt; //the beam entity
        Vector OriginHooked; //stores the origin of the ent they hit when they hooked it
    };
    Hook Hook;

    struct Jetpack
    {
        bool OnJetpack;
        float LastUse;
        float LastThrust;
        float JetpackOff;
        int Charge;
        float NextChargeTime;
        int MessageLevel;   
        float NextMessageLevelReset;
        EHANDLE SteamEnt;
        EHANDLE FlameEnt1;
        //CBaseEntity *FlameEnt2;
    };
    Jetpack Jetpack;

    typedef struct killtrackp_s
    {
        float LastKillTime;
        int KillCount;
        int CumKillCount; // Cum as in cumulative.
        int PlayMultiSound;
        int PlaySpreeSound;
    } killtrackp_t;
    killtrackp_t Kills;

    float nextCreditCounter;
};

//extern CAdminOPPlayer pAOPPlayers[128];

#endif
