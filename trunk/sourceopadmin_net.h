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

#ifndef SOURCEOPADMIN_NET_H
#define SOURCEOPADMIN_NET_H

#include "packet_types.h"
#include "bytebuf.h"
#include "sopconsts.h"

#include "tier0/memdbgon.h"

class ByteBufRead;
class ByteBufWrite;

#define SOP_PROCESS_CLC_MESSAGE( name ) \
    virtual bool Process##name( CLC_##name *msg )
#define SOP_CLC_MESSAGE_CLASS( name )  \
    class CLC_##name
#define SOP_DECLARE_CLC_MESSAGE( classname, name )  \
    bool classname::Process##name( CLC_##name *msg )
#define SOP_REGISTER_CLC_MSG( name )            \
    CLC_##name * p##name = new CLC_##name();    \
    p##name->m_pMessageHandler = this;          \
    RegisterMessage( p##name )

#define SOP_PROCESS_SVC_MESSAGE( name ) \
    virtual bool Process##name( SVC_##name *msg )
#define SOP_SVC_MESSAGE_CLASS( name )  \
    class SVC_##name
#define SOP_DECLARE_SVC_MESSAGE( classname, name )  \
    bool classname::Process##name( SVC_##name *msg )
#define SOP_REGISTER_SVC_MSG( name )            \
    SVC_##name * p##name = new SVC_##name();    \
    p##name->m_pMessageHandler = this;          \
    RegisterMessage( p##name )

SOP_CLC_MESSAGE_CLASS(Auth);
SOP_CLC_MESSAGE_CLASS(ModeSelection);
SOP_CLC_MESSAGE_CLASS(KickPlayer);
SOP_CLC_MESSAGE_CLASS(BanPlayer);
SOP_CLC_MESSAGE_CLASS(Rcon);
SOP_CLC_MESSAGE_CLASS(Ping);
SOP_CLC_MESSAGE_CLASS(PlayerChat);
SOP_CLC_MESSAGE_CLASS(SetPlayerTeam);
SOP_CLC_MESSAGE_CLASS(GagPlayer);
SOP_CLC_MESSAGE_CLASS(AdminChat);
SOP_CLC_MESSAGE_CLASS(RequestMapList);

SOP_SVC_MESSAGE_CLASS(AuthComplete);
SOP_SVC_MESSAGE_CLASS(PlayerUpdate);
SOP_SVC_MESSAGE_CLASS(PlayerDisconnect);
SOP_SVC_MESSAGE_CLASS(ServerInfo);
SOP_SVC_MESSAGE_CLASS(LogData);
SOP_SVC_MESSAGE_CLASS(VoiceData);
SOP_SVC_MESSAGE_CLASS(StatusMessage);
SOP_SVC_MESSAGE_CLASS(PingResponse);
SOP_SVC_MESSAGE_CLASS(MapPlayerUpdate);
SOP_SVC_MESSAGE_CLASS(MapChange);
SOP_SVC_MESSAGE_CLASS(PlayerChat);
SOP_SVC_MESSAGE_CLASS(AdminChat);
SOP_SVC_MESSAGE_CLASS(AdminUpdate);
SOP_SVC_MESSAGE_CLASS(AdminDisconnect);
SOP_SVC_MESSAGE_CLASS(MapList);

class IRemoteNetMessageHandler 
{
public:
    virtual ~IRemoteNetMessageHandler( void ) {}

};

class IRemoteClientMessageHandler : public IRemoteNetMessageHandler
{
public:
    virtual ~IRemoteClientMessageHandler( void ) {}

    SOP_PROCESS_CLC_MESSAGE(Auth) = 0;
    SOP_PROCESS_CLC_MESSAGE(ModeSelection) = 0;
    SOP_PROCESS_CLC_MESSAGE(KickPlayer) = 0;
    SOP_PROCESS_CLC_MESSAGE(BanPlayer) = 0;
    SOP_PROCESS_CLC_MESSAGE(Rcon) = 0;
    SOP_PROCESS_CLC_MESSAGE(Ping) = 0;
    SOP_PROCESS_CLC_MESSAGE(PlayerChat) = 0;
    SOP_PROCESS_CLC_MESSAGE(SetPlayerTeam) = 0;
    SOP_PROCESS_CLC_MESSAGE(GagPlayer) = 0;
    SOP_PROCESS_CLC_MESSAGE(AdminChat) = 0;
    SOP_PROCESS_CLC_MESSAGE(RequestMapList) = 0;
};

class IRemoteServerMessageHandler : public IRemoteNetMessageHandler
{
public:
    virtual ~IRemoteServerMessageHandler( void ) {}

    SOP_PROCESS_SVC_MESSAGE(PlayerUpdate) = 0;
    SOP_PROCESS_SVC_MESSAGE(PlayerDisconnect) = 0;
    SOP_PROCESS_SVC_MESSAGE(ServerInfo) = 0;
    SOP_PROCESS_SVC_MESSAGE(LogData) = 0;
    SOP_PROCESS_SVC_MESSAGE(VoiceData) = 0;
    SOP_PROCESS_SVC_MESSAGE(StatusMessage) = 0;
    SOP_PROCESS_SVC_MESSAGE(PingResponse) = 0;
    SOP_PROCESS_SVC_MESSAGE(MapPlayerUpdate) = 0;
    SOP_PROCESS_SVC_MESSAGE(MapChange) = 0;
    SOP_PROCESS_SVC_MESSAGE(PlayerChat) = 0;
    SOP_PROCESS_SVC_MESSAGE(AdminChat) = 0;
    SOP_PROCESS_SVC_MESSAGE(AdminUpdate) = 0;
    SOP_PROCESS_SVC_MESSAGE(AdminDisconnect) = 0;
    SOP_PROCESS_SVC_MESSAGE(MapList) = 0;
};

class ISOPNetMessage
{
public:
    virtual	~ISOPNetMessage() {}

    virtual bool    Process( void ) = 0;

    virtual	bool    ReadFromBuffer( ByteBufRead &buffer ) = 0;
    virtual	bool    WriteToBuffer( ByteBufWrite &buffer ) = 0;

    virtual int         GetType( void ) const = 0;
    virtual const char  *GetName( void ) const = 0;
};

class ISOPNetChannel
{
public:
    virtual void SendDataUdp(ByteBufWrite &msg) = 0;
    virtual void SendData(ByteBufWrite &msg) = 0;
    virtual void SendNetMessage(ISOPNetMessage &msg, bool useUdp = false) = 0;
};

SOP_CLC_MESSAGE_CLASS(Auth) : public ISOPNetMessage
{
public:
    virtual	~CLC_Auth() {}

    virtual bool    Process( void );

    virtual	bool    ReadFromBuffer( ByteBufRead &buffer );
    virtual	bool    WriteToBuffer( ByteBufWrite &buffer );

    virtual int         GetType( void ) const { return clc_auth; }
    virtual const char  *GetName( void ) const { return "clc_auth"; }

    IRemoteNetMessageHandler *m_pMessageHandler;

    char m_szUser[MAX_REMOTEADMINNAME_LENGTH];
    char m_szPass[256];
};

SOP_CLC_MESSAGE_CLASS(ModeSelection) : public ISOPNetMessage
{
public:
    CLC_ModeSelection() { m_mode = 0; }
    CLC_ModeSelection(char mode) { m_mode = mode; }
    virtual	~CLC_ModeSelection() {}

    virtual bool    Process( void );

    virtual	bool    ReadFromBuffer( ByteBufRead &buffer );
    virtual	bool    WriteToBuffer( ByteBufWrite &buffer );

    virtual int         GetType( void ) const { return clc_modeselection; }
    virtual const char  *GetName( void ) const { return "clc_modeselection"; }

    IRemoteNetMessageHandler *m_pMessageHandler;

    char m_mode;
};

SOP_CLC_MESSAGE_CLASS(KickPlayer) : public ISOPNetMessage
{
public:
    CLC_KickPlayer() { m_iPlayer = 0; }
    CLC_KickPlayer(int player) { m_iPlayer = player; }
    virtual	~CLC_KickPlayer() {}

    virtual bool    Process( void );

    virtual	bool    ReadFromBuffer( ByteBufRead &buffer );
    virtual	bool    WriteToBuffer( ByteBufWrite &buffer );

    virtual int         GetType( void ) const { return clc_kickplayer; }
    virtual const char  *GetName( void ) const { return "clc_kickplayer"; }

    IRemoteNetMessageHandler *m_pMessageHandler;

    int m_iPlayer;
};

SOP_CLC_MESSAGE_CLASS(BanPlayer) : public ISOPNetMessage
{
public:
    CLC_BanPlayer() { m_iPlayer = 0; }
    CLC_BanPlayer(int player) { m_iPlayer = player; m_szReason[0] = '\0'; }
    virtual	~CLC_BanPlayer() {}

    virtual bool    Process( void );

    virtual	bool    ReadFromBuffer( ByteBufRead &buffer );
    virtual	bool    WriteToBuffer( ByteBufWrite &buffer );

    virtual int         GetType( void ) const { return clc_banplayer; }
    virtual const char  *GetName( void ) const { return "clc_banplayer"; }

    IRemoteNetMessageHandler *m_pMessageHandler;

    int m_iPlayer;
    unsigned char m_iType;
    int m_iDuration;
    char m_szReason[256];
};

SOP_CLC_MESSAGE_CLASS(Rcon) : public ISOPNetMessage
{
public:
    CLC_Rcon() {}
    virtual	~CLC_Rcon() {}

    virtual bool    Process( void );

    virtual	bool    ReadFromBuffer( ByteBufRead &buffer );
    virtual	bool    WriteToBuffer( ByteBufWrite &buffer );

    virtual int         GetType( void ) const { return clc_rcon; }
    virtual const char  *GetName( void ) const { return "clc_rcon"; }

    IRemoteNetMessageHandler *m_pMessageHandler;

    char m_szCommand[512];
};

SOP_CLC_MESSAGE_CLASS(Ping) : public ISOPNetMessage
{
public:
    CLC_Ping() {}
    virtual	~CLC_Ping() {}

    virtual bool    Process( void );

    virtual	bool    ReadFromBuffer( ByteBufRead & );
    virtual	bool    WriteToBuffer( ByteBufWrite & );

    virtual int         GetType( void ) const { return clc_ping; }
    virtual const char  *GetName( void ) const { return "clc_ping"; }

    IRemoteNetMessageHandler *m_pMessageHandler;
};

#define CHAT_SAYALL     0
#define CHAT_CSAY       1
#define CHAT_BSAY       2
#define CHAT_SAYTEAM    3
SOP_CLC_MESSAGE_CLASS(PlayerChat) : public ISOPNetMessage
{
public:
    CLC_PlayerChat() {}
    virtual	~CLC_PlayerChat() {}

    virtual bool    Process( void );

    virtual	bool    ReadFromBuffer( ByteBufRead &buffer );
    virtual	bool    WriteToBuffer( ByteBufWrite &buffer );

    virtual int         GetType( void ) const { return clc_playerchat; }
    virtual const char  *GetName( void ) const { return "clc_playerchat"; }

    IRemoteNetMessageHandler *m_pMessageHandler;

    unsigned char m_iType;
    unsigned short m_iDestTeam;
    char m_szMessage[256];
};

SOP_CLC_MESSAGE_CLASS(SetPlayerTeam) : public ISOPNetMessage
{
public:
    CLC_SetPlayerTeam() {}
    virtual	~CLC_SetPlayerTeam() {}

    virtual bool    Process( void );

    virtual	bool    ReadFromBuffer( ByteBufRead &buffer );
    virtual	bool    WriteToBuffer( ByteBufWrite &buffer );

    virtual int         GetType( void ) const { return clc_setplayerteam; }
    virtual const char  *GetName( void ) const { return "clc_setplayerteam"; }

    IRemoteNetMessageHandler *m_pMessageHandler;

    int m_iPlayer;
    unsigned short m_iTeam;
};

SOP_CLC_MESSAGE_CLASS(GagPlayer) : public ISOPNetMessage
{
public:
    CLC_GagPlayer() { m_iPlayer = 0; }
    CLC_GagPlayer(int player) { m_iPlayer = player; }
    virtual	~CLC_GagPlayer() {}

    virtual bool    Process( void );

    virtual	bool    ReadFromBuffer( ByteBufRead &buffer );
    virtual	bool    WriteToBuffer( ByteBufWrite &buffer );

    virtual int         GetType( void ) const { return clc_gagplayer; }
    virtual const char  *GetName( void ) const { return "clc_gagplayer"; }

    IRemoteNetMessageHandler *m_pMessageHandler;

    int m_iPlayer;
};

SOP_CLC_MESSAGE_CLASS(AdminChat) : public ISOPNetMessage
{
public:
    CLC_AdminChat() {}
    virtual	~CLC_AdminChat() {}

    virtual bool    Process( void );

    virtual	bool    ReadFromBuffer( ByteBufRead &buffer );
    virtual	bool    WriteToBuffer( ByteBufWrite &buffer );

    virtual int         GetType( void ) const { return clc_adminchat; }
    virtual const char  *GetName( void ) const { return "clc_adminchat"; }

    IRemoteNetMessageHandler *m_pMessageHandler;

    char m_szMessage[256];
};

#define MAPLIST_CYCLE   0
#define MAPLIST_ALL     1
SOP_CLC_MESSAGE_CLASS(RequestMapList) : public ISOPNetMessage
{
public:
    CLC_RequestMapList() { m_iList = 0; m_iRequestNumber = 0; }
    CLC_RequestMapList(int list, unsigned int request) { m_iList = list; m_iRequestNumber = request; }
    virtual	~CLC_RequestMapList() {}

    virtual bool    Process( void );

    virtual	bool    ReadFromBuffer( ByteBufRead &buffer );
    virtual	bool    WriteToBuffer( ByteBufWrite &buffer );

    virtual int         GetType( void ) const { return clc_requestmaplist; }
    virtual const char  *GetName( void ) const { return "clc_requestmaplist"; }

    IRemoteNetMessageHandler *m_pMessageHandler;

    int m_iList;
    unsigned int m_iRequestNumber;
};



SOP_SVC_MESSAGE_CLASS(AuthComplete) : public ISOPNetMessage
{
public:
    SVC_AuthComplete() { m_iUdpPort = 0; m_iUdpSessionNumber = 0; }
    virtual	~SVC_AuthComplete() {}

    virtual bool    Process( void );

    virtual	bool    ReadFromBuffer( ByteBufRead &buffer );
    virtual	bool    WriteToBuffer( ByteBufWrite &buffer );

    virtual int         GetType( void ) const { return svc_authcomplete; }
    virtual const char  *GetName( void ) const { return "svc_authcomplete"; }

    IRemoteNetMessageHandler *m_pMessageHandler;

    bool m_bSuccess;
    int m_iUdpPort;
    int m_iUdpSessionNumber;
};

SOP_SVC_MESSAGE_CLASS(PlayerDisconnect) : public ISOPNetMessage
{
public:
    SVC_PlayerDisconnect() { m_iPlayer = 0; }
    SVC_PlayerDisconnect(unsigned short i) { m_iPlayer = i; }
    virtual	~SVC_PlayerDisconnect() {}

    virtual bool    Process( void );

    virtual	bool    ReadFromBuffer( ByteBufRead &buffer );
    virtual	bool    WriteToBuffer( ByteBufWrite &buffer );

    virtual int         GetType( void ) const { return svc_playerdisconnect; }
    virtual const char  *GetName( void ) const { return "svc_playerdisconnect"; }

    IRemoteNetMessageHandler *m_pMessageHandler;

    unsigned short GetPlayer() { return m_iPlayer; }

private:
    unsigned short m_iPlayer;
};

#define MAX_PLAYERS_PER_UPDATE 64
SOP_SVC_MESSAGE_CLASS(PlayerUpdate) : public ISOPNetMessage
{
public:
    SVC_PlayerUpdate() { Init(); }
    virtual	~SVC_PlayerUpdate() {}

    virtual bool    Process( void );

    virtual	bool    ReadFromBuffer( ByteBufRead &buffer );
    virtual	bool    WriteToBuffer( ByteBufWrite &buffer );

    virtual int         GetType( void ) const { return svc_playerupdate; }
    virtual const char  *GetName( void ) const { return "svc_playerupdate"; }

    IRemoteNetMessageHandler *m_pMessageHandler;

    typedef struct playerinfo_s
    {
        unsigned short index;
        char name[MAX_PLAYERNAME_LENGTH];
        unsigned long long steamid;
        int frags;
        int deaths;
        unsigned short team;
        unsigned short playerclass;
        unsigned int ip;
        int credits;
        int time;
        bool gagged;
    } playerinfo_t;

    void Init();
    void AddPlayer(unsigned short index, const char *pszName, unsigned long long steamid, int frags, int deaths, unsigned short team, unsigned short playerclass, unsigned int ip, int credits, int time, bool gagged);
    int GetNumPlayers() { return m_iPlayers; }
    playerinfo_t *GetPlayer(int i) { return &m_players[i]; }

private:
    unsigned char m_iPlayers;
    playerinfo_t m_players[MAX_PLAYERS_PER_UPDATE];
};

SOP_SVC_MESSAGE_CLASS(ServerInfo) : public ISOPNetMessage
{
public:
    SVC_ServerInfo() {}
    virtual	~SVC_ServerInfo() {}

    virtual bool    Process( void );

    virtual	bool    ReadFromBuffer( ByteBufRead &buffer );
    virtual	bool    WriteToBuffer( ByteBufWrite &buffer );

    virtual int         GetType( void ) const { return svc_serverinfo; }
    virtual const char  *GetName( void ) const { return "svc_serverinfo"; }

    IRemoteNetMessageHandler *m_pMessageHandler;

    char m_szServerName[256];
    char m_szGame[256];
    unsigned short m_iMaxPlayers;
    int m_iTimeLimit;
    int m_iTimeRemaining;
    int m_iFragLimit;
    int m_iFragsRemaining;
    int m_iWinLimit;
    int m_iMaxRounds;
    bool m_bFriendlyFire;
    bool m_bBansRequireReason;
    int m_iGamePort;
};

SOP_SVC_MESSAGE_CLASS(LogData) : public ISOPNetMessage
{
public:
    SVC_LogData() {}
    virtual	~SVC_LogData() {}

    virtual bool    Process( void );

    virtual	bool    ReadFromBuffer( ByteBufRead &buffer );
    virtual	bool    WriteToBuffer( ByteBufWrite &buffer );

    virtual int         GetType( void ) const { return svc_logdata; }
    virtual const char  *GetName( void ) const { return "svc_logdata"; }

    IRemoteNetMessageHandler *m_pMessageHandler;

    char m_szLogMessage[512];
};

#define SOP_VOICE_CODEC_SPEEX 0
#define SOP_VOICE_CODEC_STEAM 1
SOP_SVC_MESSAGE_CLASS(VoiceData) : public ISOPNetMessage
{
public:
    SVC_VoiceData() {}
    virtual	~SVC_VoiceData() {}

    virtual bool    Process( void );

    virtual	bool    ReadFromBuffer( ByteBufRead &buffer );
    virtual	bool    WriteToBuffer( ByteBufWrite &buffer );

    virtual int         GetType( void ) const { return svc_voicedata; }
    virtual const char  *GetName( void ) const { return "svc_voicedata"; }

    IRemoteNetMessageHandler *m_pMessageHandler;

    unsigned short m_iPlayer;
    unsigned short m_iSize;
    char m_szVoiceData[1024];
    unsigned char m_iCodec;
};

SOP_SVC_MESSAGE_CLASS(StatusMessage) : public ISOPNetMessage
{
public:
    SVC_StatusMessage() {}
    virtual	~SVC_StatusMessage() {}

    virtual bool    Process( void );

    virtual	bool    ReadFromBuffer( ByteBufRead &buffer );
    virtual	bool    WriteToBuffer( ByteBufWrite &buffer );

    virtual int         GetType( void ) const { return svc_statusmessage; }
    virtual const char  *GetName( void ) const { return "svc_statusmessage"; }

    IRemoteNetMessageHandler *m_pMessageHandler;

    char m_szMessage[512];
};

SOP_SVC_MESSAGE_CLASS(PingResponse) : public ISOPNetMessage
{
public:
    SVC_PingResponse() {}
    virtual	~SVC_PingResponse() {}

    virtual bool    Process( void );

    virtual	bool    ReadFromBuffer( ByteBufRead &buffer );
    virtual	bool    WriteToBuffer( ByteBufWrite &buffer );

    virtual int         GetType( void ) const { return svc_pingresponse; }
    virtual const char  *GetName( void ) const { return "svc_pingresponse"; }

    IRemoteNetMessageHandler *m_pMessageHandler;

    int m_iPlayers;
    int m_iMaxPlayers;
};

#define MAX_PLAYERS_PER_MAPUPDATE 64
// these are currently written as a byte.
// change to short/int if more flags are needed
#define SVC_MAPPLAYERUPDATE_INCLUDES_ALIVE      (1<<0)
#define SVC_MAPPLAYERUPDATE_INCLUDES_POS        (1<<1)
#define SVC_MAPPLAYERUPDATE_INCLUDES_VEL        (1<<2)
#define SVC_MAPPLAYERUPDATE_INCLUDES_HEALTH     (1<<3)
#define SVC_MAPPLAYERUPDATE_INCLUDES_MAXHEALTH  (1<<4)
#define SVC_MAPPLAYERUPDATE_INCLUDES_ALL        ((1<<5)-1)

SOP_SVC_MESSAGE_CLASS(MapPlayerUpdate) : public ISOPNetMessage
{
public:
    SVC_MapPlayerUpdate() { m_flGameTime = 0; m_iPlayers = 0; }
    SVC_MapPlayerUpdate(double time) { m_flGameTime = time; m_iPlayers = 0; }
    virtual	~SVC_MapPlayerUpdate() {}

    virtual bool    Process( void );

    virtual	bool    ReadFromBuffer( ByteBufRead &buffer );
    virtual	bool    WriteToBuffer( ByteBufWrite &buffer );

    virtual int         GetType( void ) const { return svc_mapplayerupdate; }
    virtual const char  *GetName( void ) const { return "svc_mapplayerupdate"; }

    IRemoteNetMessageHandler *m_pMessageHandler;

    typedef struct playerinfo_s
    {
        unsigned short index;
        int messageFlags;
        bool alive;
        short posX;
        short posY;
        short velX;
        short velY;
        short health;
        short maxHealth;
    } playerinfo_t;

    void AddPlayer(unsigned short index, bool alive, short posX, short posY, short velX, short velY, short health, short maxhealth);
    int GetNumPlayers() { return m_iPlayers; }
    playerinfo_t *GetPlayer(int i) { return &m_players[i]; }
    double GetGameTime() { return m_flGameTime; }

private:
    double m_flGameTime;
    unsigned char m_iPlayers;
    playerinfo_t m_players[MAX_PLAYERS_PER_UPDATE];
};

SOP_SVC_MESSAGE_CLASS(MapChange) : public ISOPNetMessage
{
public:
    SVC_MapChange() { m_szMap[0] = '\0'; }
    virtual	~SVC_MapChange() {}

    virtual bool    Process( void );

    virtual	bool    ReadFromBuffer( ByteBufRead &buffer );
    virtual	bool    WriteToBuffer( ByteBufWrite &buffer );

    virtual int         GetType( void ) const { return svc_mapchange; }
    virtual const char  *GetName( void ) const { return "svc_mapchange"; }

    IRemoteNetMessageHandler *m_pMessageHandler;

    char m_szMap[256];
};

#define PLAYERCHAT_FLAG_DEADCHAT    (1<<0)
#define PLAYERCHAT_FLAG_TEAMCHAT    (1<<1)
#define PLAYERCHAT_FLAG_HIDDEN      (1<<2)
#define PLAYERCHAT_FLAG_CONSOLE     (1<<3)
#define PLAYERCHAT_FLAG_FROMGAME    (1<<4)
#define PLAYERCHAT_FLAG_FROMADMIN   (1<<5)

SOP_SVC_MESSAGE_CLASS(PlayerChat) : public ISOPNetMessage
{
public:
    SVC_PlayerChat() { m_iPlayer = 0xFFFF; m_iFlags = 0; }
    virtual	~SVC_PlayerChat() {}

    virtual bool    Process( void );

    virtual	bool    ReadFromBuffer( ByteBufRead &buffer );
    virtual	bool    WriteToBuffer( ByteBufWrite &buffer );

    virtual int         GetType( void ) const { return svc_playerchat; }
    virtual const char  *GetName( void ) const { return "svc_playerchat"; }

    IRemoteNetMessageHandler *m_pMessageHandler;

    bool IsDeadChat() { return (m_iFlags & PLAYERCHAT_FLAG_DEADCHAT) != 0; }
    bool IsTeamChat() { return (m_iFlags & PLAYERCHAT_FLAG_TEAMCHAT) != 0; }
    bool IsHidden() { return (m_iFlags & PLAYERCHAT_FLAG_HIDDEN) != 0; }
    bool IsConsoleChat() { return (m_iFlags & PLAYERCHAT_FLAG_CONSOLE) != 0; }
    bool IsFromGame() { return (m_iFlags & PLAYERCHAT_FLAG_FROMGAME) != 0; }
    bool IsFromAdmin() { return (m_iFlags & PLAYERCHAT_FLAG_FROMADMIN) != 0; }

    void SetDeadChat() { m_iFlags |= PLAYERCHAT_FLAG_DEADCHAT; }
    void SetTeamChat() { m_iFlags |= PLAYERCHAT_FLAG_TEAMCHAT; }
    void SetHidden() { m_iFlags |= PLAYERCHAT_FLAG_HIDDEN; }
    void SetConsole() { m_iFlags |= PLAYERCHAT_FLAG_CONSOLE; }
    void SetFromGame() { m_iFlags |= PLAYERCHAT_FLAG_FROMGAME; }
    void SetFromAdmin() { m_iFlags |= PLAYERCHAT_FLAG_FROMADMIN; }

    unsigned short m_iPlayer;
    int m_iFlags;
    char m_szAdminName[128];
    char m_szMessage[256];
};

SOP_SVC_MESSAGE_CLASS(AdminChat) : public ISOPNetMessage
{
public:
    SVC_AdminChat() { }
    virtual	~SVC_AdminChat() {}

    virtual bool    Process( void );

    virtual	bool    ReadFromBuffer( ByteBufRead &buffer );
    virtual	bool    WriteToBuffer( ByteBufWrite &buffer );

    virtual int         GetType( void ) const { return svc_adminchat; }
    virtual const char  *GetName( void ) const { return "svc_adminchat"; }

    IRemoteNetMessageHandler *m_pMessageHandler;

    char m_szAdminName[128];
    char m_szMessage[256];
};

#define MAX_ADMINS_PER_UPDATE 32
SOP_SVC_MESSAGE_CLASS(AdminUpdate) : public ISOPNetMessage
{
public:
    SVC_AdminUpdate() { Init(); }
    virtual	~SVC_AdminUpdate() {}

    virtual bool    Process( void );

    virtual	bool    ReadFromBuffer( ByteBufRead &buffer );
    virtual	bool    WriteToBuffer( ByteBufWrite &buffer );

    virtual int         GetType( void ) const { return svc_adminupdate; }
    virtual const char  *GetName( void ) const { return "svc_adminupdate"; }

    IRemoteNetMessageHandler *m_pMessageHandler;

    typedef struct admininfo_s
    {
        unsigned short index;
        char name[MAX_REMOTEADMINNAME_LENGTH];
        bool away;
        char awaymsg[256];
    } admininfo_t;

    void Init();
    void AddAdmin(unsigned short index, const char *pszName, bool away, const char *pszAwayMsg);
    int GetNumAdmins() { return m_iAdmins; }
    admininfo_t *GetAdmin(int i) { return &m_admins[i]; }

private:
    unsigned char m_iAdmins;
    admininfo_t m_admins[MAX_ADMINS_PER_UPDATE];
};

SOP_SVC_MESSAGE_CLASS(AdminDisconnect) : public ISOPNetMessage
{
public:
    SVC_AdminDisconnect() { m_iAdmin = 0; }
    SVC_AdminDisconnect(unsigned short i) { m_iAdmin = i; }
    virtual	~SVC_AdminDisconnect() {}

    virtual bool    Process( void );

    virtual	bool    ReadFromBuffer( ByteBufRead &buffer );
    virtual	bool    WriteToBuffer( ByteBufWrite &buffer );

    virtual int         GetType( void ) const { return svc_admindisconnect; }
    virtual const char  *GetName( void ) const { return "svc_admindisconnect"; }

    IRemoteNetMessageHandler *m_pMessageHandler;

    unsigned short GetAdmin() { return m_iAdmin; }

private:
    unsigned short m_iAdmin;
};

#define MAPLIST_MAX_MAPS    32
#define MAPLIST_MAX_MAPLEN  256
SOP_SVC_MESSAGE_CLASS(MapList) : public ISOPNetMessage
{
public:
    SVC_MapList() { m_iList = 0; m_iNumMaps = 0; }
    SVC_MapList(int list, unsigned int response) { m_iList = list; m_iResponseNumber = response; m_iNumMaps = 0; }
    virtual	~SVC_MapList() {}

    virtual bool    Process( void );

    virtual	bool    ReadFromBuffer( ByteBufRead &buffer );
    virtual	bool    WriteToBuffer( ByteBufWrite &buffer );

    virtual int         GetType( void ) const { return svc_maplist; }
    virtual const char  *GetName( void ) const { return "svc_maplist"; }

    IRemoteNetMessageHandler *m_pMessageHandler;

    int m_iList;
    unsigned int m_iResponseNumber;
    int m_iNumMaps;
    char m_szMaps[MAPLIST_MAX_MAPS][MAPLIST_MAX_MAPLEN];
};

inline bool CLC_Auth::Process( void )
{
    IRemoteClientMessageHandler *client = (IRemoteClientMessageHandler *)m_pMessageHandler;
    return client->ProcessAuth(this);
}

inline bool CLC_Auth::ReadFromBuffer( ByteBufRead &buffer )
{
    buffer.ReadString(m_szUser, sizeof(m_szUser));
    buffer.ReadString(m_szPass, sizeof(m_szPass));

    return true;
}

inline bool CLC_Auth::WriteToBuffer( ByteBufWrite &buffer )
{
    buffer.WriteString(m_szUser);
    buffer.WriteString(m_szPass);

    return true;
}

inline bool CLC_ModeSelection::Process( void )
{
    IRemoteClientMessageHandler *client = (IRemoteClientMessageHandler *)m_pMessageHandler;
    return client->ProcessModeSelection(this);
}

inline bool CLC_ModeSelection::ReadFromBuffer( ByteBufRead &buffer )
{
    m_mode = buffer.ReadByte();

    return true;
}

inline bool CLC_ModeSelection::WriteToBuffer( ByteBufWrite &buffer )
{
    buffer.WriteByte(m_mode);

    return true;
}

inline bool CLC_KickPlayer::Process( void )
{
    IRemoteClientMessageHandler *client = (IRemoteClientMessageHandler *)m_pMessageHandler;
    return client->ProcessKickPlayer(this);
}

inline bool CLC_KickPlayer::ReadFromBuffer( ByteBufRead &buffer )
{
    m_iPlayer = buffer.ReadUShort();

    return true;
}

inline bool CLC_KickPlayer::WriteToBuffer( ByteBufWrite &buffer )
{
    buffer.WriteUShort(m_iPlayer);

    return true;
}

inline bool CLC_BanPlayer::Process( void )
{
    IRemoteClientMessageHandler *client = (IRemoteClientMessageHandler *)m_pMessageHandler;
    return client->ProcessBanPlayer(this);
}

inline bool CLC_BanPlayer::ReadFromBuffer( ByteBufRead &buffer )
{
    m_iPlayer = buffer.ReadUShort();
    m_iType = buffer.ReadByte();
    m_iDuration = buffer.ReadLong();
    buffer.ReadString(m_szReason, sizeof(m_szReason));

    return true;
}

inline bool CLC_BanPlayer::WriteToBuffer( ByteBufWrite &buffer )
{
    buffer.WriteUShort(m_iPlayer);
    buffer.WriteByte(m_iType);
    buffer.WriteLong(m_iDuration);
    buffer.WriteString(m_szReason);

    return true;
}

inline bool CLC_Rcon::Process( void )
{
    IRemoteClientMessageHandler *client = (IRemoteClientMessageHandler *)m_pMessageHandler;
    return client->ProcessRcon(this);
}

inline bool CLC_Rcon::ReadFromBuffer( ByteBufRead &buffer )
{
    buffer.ReadString(m_szCommand, sizeof(m_szCommand));

    return true;
}

inline bool CLC_Rcon::WriteToBuffer( ByteBufWrite &buffer )
{
    buffer.WriteString(m_szCommand);

    return true;
}

inline bool CLC_Ping::Process( void )
{
    IRemoteClientMessageHandler *client = (IRemoteClientMessageHandler *)m_pMessageHandler;
    return client->ProcessPing(this);
}

inline bool CLC_Ping::ReadFromBuffer( ByteBufRead & )
{
    return true;
}

inline bool CLC_Ping::WriteToBuffer( ByteBufWrite & )
{
    return true;
}

inline bool CLC_PlayerChat::Process( void )
{
    IRemoteClientMessageHandler *client = (IRemoteClientMessageHandler *)m_pMessageHandler;
    return client->ProcessPlayerChat(this);
}

inline bool CLC_PlayerChat::ReadFromBuffer( ByteBufRead &buffer )
{
    m_iType = buffer.ReadByte();
    m_iDestTeam = buffer.ReadUShort();
    buffer.ReadString(m_szMessage, sizeof(m_szMessage));

    return true;
}

inline bool CLC_PlayerChat::WriteToBuffer( ByteBufWrite &buffer )
{
    buffer.WriteByte(m_iType);
    buffer.WriteUShort(m_iDestTeam);
    buffer.WriteString(m_szMessage);

    return true;
}

inline bool CLC_SetPlayerTeam::Process( void )
{
    IRemoteClientMessageHandler *client = (IRemoteClientMessageHandler *)m_pMessageHandler;
    return client->ProcessSetPlayerTeam(this);
}

inline bool CLC_SetPlayerTeam::ReadFromBuffer( ByteBufRead &buffer )
{
    m_iPlayer = buffer.ReadUShort();
    m_iTeam = buffer.ReadUShort();

    return true;
}

inline bool CLC_SetPlayerTeam::WriteToBuffer( ByteBufWrite &buffer )
{
    buffer.WriteUShort(m_iPlayer);
    buffer.WriteUShort(m_iTeam);

    return true;
}

inline bool CLC_GagPlayer::Process( void )
{
    IRemoteClientMessageHandler *client = (IRemoteClientMessageHandler *)m_pMessageHandler;
    return client->ProcessGagPlayer(this);
}

inline bool CLC_GagPlayer::ReadFromBuffer( ByteBufRead &buffer )
{
    m_iPlayer = buffer.ReadUShort();

    return true;
}

inline bool CLC_GagPlayer::WriteToBuffer( ByteBufWrite &buffer )
{
    buffer.WriteUShort(m_iPlayer);

    return true;
}

inline bool CLC_AdminChat::Process( void )
{
    IRemoteClientMessageHandler *client = (IRemoteClientMessageHandler *)m_pMessageHandler;
    return client->ProcessAdminChat(this);
}

inline bool CLC_AdminChat::ReadFromBuffer( ByteBufRead &buffer )
{
    buffer.ReadString(m_szMessage, sizeof(m_szMessage));

    return true;
}

inline bool CLC_AdminChat::WriteToBuffer( ByteBufWrite &buffer )
{
    buffer.WriteString(m_szMessage);

    return true;
}

inline bool CLC_RequestMapList::Process( void )
{
    IRemoteClientMessageHandler *client = (IRemoteClientMessageHandler *)m_pMessageHandler;
    return client->ProcessRequestMapList(this);
}

inline bool CLC_RequestMapList::ReadFromBuffer( ByteBufRead &buffer )
{
    m_iList = buffer.ReadByte();
    m_iRequestNumber = buffer.ReadULong();

    return true;
}

inline bool CLC_RequestMapList::WriteToBuffer( ByteBufWrite &buffer )
{
    buffer.WriteByte(m_iList);
    buffer.WriteULong(m_iRequestNumber);

    return true;
}



inline bool SVC_AuthComplete::Process()
{
    return true;
}

inline bool SVC_AuthComplete::ReadFromBuffer( ByteBufRead &buffer )
{
    m_bSuccess = (buffer.ReadByte() != 0);
    m_iUdpPort = buffer.ReadLong();
    m_iUdpSessionNumber = buffer.ReadLong();

    return true;
}

inline bool SVC_AuthComplete::WriteToBuffer( ByteBufWrite &buffer )
{
    buffer.WriteByte(m_bSuccess);
    buffer.WriteLong(m_iUdpPort);
    buffer.WriteLong(m_iUdpSessionNumber);

    return true;
}

inline bool SVC_PlayerDisconnect::Process()
{
    IRemoteServerMessageHandler *server = (IRemoteServerMessageHandler *)m_pMessageHandler;
    return server->ProcessPlayerDisconnect(this);
}

inline bool SVC_PlayerDisconnect::ReadFromBuffer( ByteBufRead &buffer )
{
    m_iPlayer = buffer.ReadUShort();

    return true;
}

inline bool SVC_PlayerDisconnect::WriteToBuffer( ByteBufWrite &buffer )
{
    buffer.WriteUShort(m_iPlayer);

    return true;
}

inline bool SVC_PlayerUpdate::Process()
{
    IRemoteServerMessageHandler *server = (IRemoteServerMessageHandler *)m_pMessageHandler;
    return server->ProcessPlayerUpdate(this);
}

inline bool SVC_PlayerUpdate::ReadFromBuffer( ByteBufRead &buffer )
{
    Init();

    m_iPlayers = buffer.ReadByte();
    for(int i = 0; i < m_iPlayers; i++)
    {
        playerinfo_t *curPlayer = &m_players[i];
        curPlayer->index = buffer.ReadUShort();
        buffer.ReadString(curPlayer->name, sizeof(curPlayer->name));
        curPlayer->steamid = buffer.ReadULongLong();
        curPlayer->frags = buffer.ReadLong();
        curPlayer->deaths = buffer.ReadLong();
        curPlayer->team = buffer.ReadUShort();
        curPlayer->playerclass = buffer.ReadUShort();
        curPlayer->ip = buffer.ReadULong();
        curPlayer->credits = buffer.ReadLong();
        curPlayer->time = buffer.ReadLong();
        curPlayer->gagged = buffer.ReadByte() != 0;
    }
    return true;
}

inline bool SVC_PlayerUpdate::WriteToBuffer( ByteBufWrite &buffer )
{
    buffer.WriteByte(m_iPlayers);
    for(int i = 0; i < m_iPlayers; i++)
    {
        playerinfo_t *curPlayer = &m_players[i];
        buffer.WriteUShort(curPlayer->index);
        buffer.WriteString(curPlayer->name);
        buffer.WriteULongLong(curPlayer->steamid);
        buffer.WriteLong(curPlayer->frags);
        buffer.WriteLong(curPlayer->deaths);
        buffer.WriteUShort(curPlayer->team);
        buffer.WriteUShort(curPlayer->playerclass);
        buffer.WriteULong(curPlayer->ip);
        buffer.WriteLong(curPlayer->credits);
        buffer.WriteLong(curPlayer->time);
        buffer.WriteByte(curPlayer->gagged);
    }
    return true;
}

inline void SVC_PlayerUpdate::Init()
{
    m_iPlayers = 0;
}

inline void SVC_PlayerUpdate::AddPlayer(unsigned short index, const char *pszName, unsigned long long steamid, int frags, int deaths, unsigned short team, unsigned short playerclass, unsigned int ip, int credits, int time, bool gagged)
{
    playerinfo_t *newPlayer = &m_players[m_iPlayers];

    newPlayer->index = index;
    strncpy(newPlayer->name, pszName, sizeof(newPlayer->name));
    newPlayer->name[sizeof(newPlayer->name)-1] = '\0';
    newPlayer->steamid = steamid;
    newPlayer->frags = frags;
    newPlayer->deaths = deaths;
    newPlayer->team = team;
    newPlayer->playerclass = playerclass;
    newPlayer->ip = ip;
    newPlayer->credits = credits;
    newPlayer->time = time;
    newPlayer->gagged = gagged;

    m_iPlayers++;
}

inline bool SVC_ServerInfo::Process()
{
    IRemoteServerMessageHandler *server = (IRemoteServerMessageHandler *)m_pMessageHandler;
    return server->ProcessServerInfo(this);
}

inline bool SVC_ServerInfo::ReadFromBuffer( ByteBufRead &buffer )
{
    buffer.ReadString(m_szServerName, sizeof(m_szServerName));
    buffer.ReadString(m_szGame, sizeof(m_szGame));
    m_iMaxPlayers = buffer.ReadUShort();
    m_iTimeLimit = buffer.ReadLong();
    m_iTimeRemaining = buffer.ReadLong();
    m_iFragLimit = buffer.ReadLong();
    m_iFragsRemaining = buffer.ReadLong();
    m_iWinLimit = buffer.ReadLong();
    m_iMaxRounds = buffer.ReadLong();
    m_bFriendlyFire = (buffer.ReadByte() != 0);
    m_bBansRequireReason = (buffer.ReadByte() != 0);
    m_iGamePort = buffer.ReadUShort();

    return true;
}

inline bool SVC_ServerInfo::WriteToBuffer( ByteBufWrite &buffer )
{
    buffer.WriteString(m_szServerName);
    buffer.WriteString(m_szGame);
    buffer.WriteUShort(m_iMaxPlayers);
    buffer.WriteLong(m_iTimeLimit);
    buffer.WriteLong(m_iTimeRemaining);
    buffer.WriteLong(m_iFragLimit);
    buffer.WriteLong(m_iFragsRemaining);
    buffer.WriteLong(m_iWinLimit);
    buffer.WriteLong(m_iMaxRounds);
    buffer.WriteByte(m_bFriendlyFire);
    buffer.WriteByte(m_bBansRequireReason);
    buffer.WriteUShort(m_iGamePort);

    return true;
}

inline bool SVC_LogData::Process()
{
    IRemoteServerMessageHandler *server = (IRemoteServerMessageHandler *)m_pMessageHandler;
    return server->ProcessLogData(this);
}

inline bool SVC_LogData::ReadFromBuffer( ByteBufRead &buffer )
{
    buffer.ReadString(m_szLogMessage, sizeof(m_szLogMessage));

    return true;
}

inline bool SVC_LogData::WriteToBuffer( ByteBufWrite &buffer )
{
    buffer.WriteString(m_szLogMessage);

    return true;
}

inline bool SVC_VoiceData::Process()
{
    IRemoteServerMessageHandler *server = (IRemoteServerMessageHandler *)m_pMessageHandler;
    return server->ProcessVoiceData(this);
}

inline bool SVC_VoiceData::ReadFromBuffer( ByteBufRead &buffer )
{
    m_iPlayer = buffer.ReadUShort();
    m_iSize = buffer.ReadUShort();
    if(m_iSize > sizeof(m_szVoiceData))
    {
        m_iSize = sizeof(m_szVoiceData);
    }
    buffer.ReadBytes(m_szVoiceData, m_iSize);
    m_iCodec = buffer.ReadByte();

    return true;
}

inline bool SVC_VoiceData::WriteToBuffer( ByteBufWrite &buffer )
{
    buffer.WriteUShort(m_iPlayer);
    buffer.WriteUShort(m_iSize);
    buffer.WriteBytes(m_szVoiceData, m_iSize);
    buffer.WriteByte(m_iCodec);

    return true;
}

inline bool SVC_StatusMessage::Process()
{
    IRemoteServerMessageHandler *server = (IRemoteServerMessageHandler *)m_pMessageHandler;
    return server->ProcessStatusMessage(this);
}

inline bool SVC_StatusMessage::ReadFromBuffer( ByteBufRead &buffer )
{
    buffer.ReadString(m_szMessage, sizeof(m_szMessage));

    return true;
}

inline bool SVC_StatusMessage::WriteToBuffer( ByteBufWrite &buffer )
{
    buffer.WriteString(m_szMessage);

    return true;
}

inline bool SVC_PingResponse::Process()
{
    IRemoteServerMessageHandler *server = (IRemoteServerMessageHandler *)m_pMessageHandler;
    return server->ProcessPingResponse(this);
}

inline bool SVC_PingResponse::ReadFromBuffer( ByteBufRead &buffer )
{
    m_iPlayers = buffer.ReadLong();
    m_iMaxPlayers = buffer.ReadLong();

    return true;
}

inline bool SVC_PingResponse::WriteToBuffer( ByteBufWrite &buffer )
{
    buffer.WriteLong(m_iPlayers);
    buffer.WriteLong(m_iMaxPlayers);

    return true;
}

inline bool SVC_MapPlayerUpdate::Process()
{
    IRemoteServerMessageHandler *server = (IRemoteServerMessageHandler *)m_pMessageHandler;
    return server->ProcessMapPlayerUpdate(this);
}

inline bool SVC_MapPlayerUpdate::ReadFromBuffer( ByteBufRead &buffer )
{
    m_flGameTime = buffer.ReadDouble();
    m_iPlayers = buffer.ReadByte();
    for(int i = 0; i < m_iPlayers; i++)
    {
        playerinfo_t *curPlayer = &m_players[i];
        curPlayer->index = buffer.ReadUShort();
        curPlayer->messageFlags = buffer.ReadByte();
        if(curPlayer->messageFlags & SVC_MAPPLAYERUPDATE_INCLUDES_ALIVE)
            curPlayer->alive = (buffer.ReadByte() != 0);
        if(curPlayer->messageFlags & SVC_MAPPLAYERUPDATE_INCLUDES_POS)
        {
            curPlayer->posX = buffer.ReadShort();
            curPlayer->posY = buffer.ReadShort();
        }
        if(curPlayer->messageFlags & SVC_MAPPLAYERUPDATE_INCLUDES_VEL)
        {
            curPlayer->velX = buffer.ReadShort();
            curPlayer->velY = buffer.ReadShort();
        }
        if(curPlayer->messageFlags & SVC_MAPPLAYERUPDATE_INCLUDES_HEALTH)
            curPlayer->health = buffer.ReadShort();
        if(curPlayer->messageFlags & SVC_MAPPLAYERUPDATE_INCLUDES_MAXHEALTH)
            curPlayer->maxHealth = buffer.ReadShort();
    }
    return true;
}

inline bool SVC_MapPlayerUpdate::WriteToBuffer( ByteBufWrite &buffer )
{
    buffer.WriteDouble(m_flGameTime);
    buffer.WriteByte(m_iPlayers);
    for(int i = 0; i < m_iPlayers; i++)
    {
        playerinfo_t *curPlayer = &m_players[i];
        buffer.WriteUShort(curPlayer->index);
        buffer.WriteByte(curPlayer->messageFlags);
        if(curPlayer->messageFlags & SVC_MAPPLAYERUPDATE_INCLUDES_ALIVE)
            buffer.WriteByte(curPlayer->alive);
        if(curPlayer->messageFlags & SVC_MAPPLAYERUPDATE_INCLUDES_POS)
        {
            buffer.WriteShort(curPlayer->posX);
            buffer.WriteShort(curPlayer->posY);
        }
        if(curPlayer->messageFlags & SVC_MAPPLAYERUPDATE_INCLUDES_VEL)
        {
            buffer.WriteShort(curPlayer->velX);
            buffer.WriteShort(curPlayer->velY);
        }
        if(curPlayer->messageFlags & SVC_MAPPLAYERUPDATE_INCLUDES_HEALTH)
            buffer.WriteShort(curPlayer->health);
        if(curPlayer->messageFlags & SVC_MAPPLAYERUPDATE_INCLUDES_MAXHEALTH)
            buffer.WriteShort(curPlayer->maxHealth);
    }
    return true;
}

inline void SVC_MapPlayerUpdate::AddPlayer(unsigned short index, bool alive, short posX, short posY, short velX, short velY, short health, short maxhealth)
{
    playerinfo_t *newPlayer = &m_players[m_iPlayers];

    newPlayer->index = index;
    newPlayer->alive = alive;
    newPlayer->posX = posX;
    newPlayer->posY = posY;
    newPlayer->velX = velX;
    newPlayer->velY = velY;
    newPlayer->health = health;
    newPlayer->maxHealth = maxhealth;

    m_iPlayers++;
}

inline bool SVC_MapChange::Process()
{
    IRemoteServerMessageHandler *server = (IRemoteServerMessageHandler *)m_pMessageHandler;
    return server->ProcessMapChange(this);
}

inline bool SVC_MapChange::ReadFromBuffer( ByteBufRead &buffer )
{
    buffer.ReadString(m_szMap, sizeof(m_szMap));
    return true;
}

inline bool SVC_MapChange::WriteToBuffer( ByteBufWrite &buffer )
{
    buffer.WriteString(m_szMap);
    return true;
}

inline bool SVC_PlayerChat::Process()
{
    IRemoteServerMessageHandler *server = (IRemoteServerMessageHandler *)m_pMessageHandler;
    return server->ProcessPlayerChat(this);
}

inline bool SVC_PlayerChat::ReadFromBuffer( ByteBufRead &buffer )
{
    m_iPlayer = buffer.ReadUShort();
    m_iFlags = buffer.ReadByte();
    if(this->IsFromAdmin())
        buffer.ReadString(m_szAdminName, sizeof(m_szAdminName));
    buffer.ReadString(m_szMessage, sizeof(m_szMessage));
    return true;
}

inline bool SVC_PlayerChat::WriteToBuffer( ByteBufWrite &buffer )
{
    buffer.WriteUShort(m_iPlayer);
    buffer.WriteByte(m_iFlags);
    if(this->IsFromAdmin())
        buffer.WriteString(m_szAdminName);
    buffer.WriteString(m_szMessage);
    return true;
}

inline bool SVC_AdminChat::Process()
{
    IRemoteServerMessageHandler *server = (IRemoteServerMessageHandler *)m_pMessageHandler;
    return server->ProcessAdminChat(this);
}

inline bool SVC_AdminChat::ReadFromBuffer( ByteBufRead &buffer )
{
    buffer.ReadString(m_szAdminName, sizeof(m_szAdminName));
    buffer.ReadString(m_szMessage, sizeof(m_szMessage));
    return true;
}

inline bool SVC_AdminChat::WriteToBuffer( ByteBufWrite &buffer )
{
    buffer.WriteString(m_szAdminName);
    buffer.WriteString(m_szMessage);
    return true;
}

inline bool SVC_AdminUpdate::Process()
{
    IRemoteServerMessageHandler *server = (IRemoteServerMessageHandler *)m_pMessageHandler;
    return server->ProcessAdminUpdate(this);
}

inline bool SVC_AdminUpdate::ReadFromBuffer( ByteBufRead &buffer )
{
    Init();

    m_iAdmins = buffer.ReadByte();
    for(int i = 0; i < m_iAdmins; i++)
    {
        admininfo_t *curAdmin = &m_admins[i];
        curAdmin->index = buffer.ReadByte();
        buffer.ReadString(curAdmin->name, sizeof(curAdmin->name));
        curAdmin->away = buffer.ReadByte() != 0;
        buffer.ReadString(curAdmin->awaymsg, sizeof(curAdmin->awaymsg));
    }
    return true;
}

inline bool SVC_AdminUpdate::WriteToBuffer( ByteBufWrite &buffer )
{
    buffer.WriteByte(m_iAdmins);
    for(int i = 0; i < m_iAdmins; i++)
    {
        admininfo_t *curAdmin = &m_admins[i];
        buffer.WriteByte(curAdmin->index);
        buffer.WriteString(curAdmin->name);
        buffer.WriteByte(curAdmin->away);
        buffer.WriteString(curAdmin->awaymsg);
    }
    return true;
}

inline void SVC_AdminUpdate::Init()
{
    m_iAdmins = 0;
}

inline void SVC_AdminUpdate::AddAdmin(unsigned short index, const char *pszName, bool away, const char *pszAwayMsg)
{
    admininfo_t *newAdmin = &m_admins[m_iAdmins];

    newAdmin->index = index;
    strncpy(newAdmin->name, pszName, sizeof(newAdmin->name));
    newAdmin->away = away;
    strncpy(newAdmin->awaymsg, pszAwayMsg, sizeof(newAdmin->awaymsg));

    m_iAdmins++;
}

inline bool SVC_AdminDisconnect::Process()
{
    IRemoteServerMessageHandler *server = (IRemoteServerMessageHandler *)m_pMessageHandler;
    return server->ProcessAdminDisconnect(this);
}

inline bool SVC_AdminDisconnect::ReadFromBuffer( ByteBufRead &buffer )
{
    m_iAdmin = buffer.ReadByte();

    return true;
}

inline bool SVC_AdminDisconnect::WriteToBuffer( ByteBufWrite &buffer )
{
    buffer.WriteByte(m_iAdmin);

    return true;
}

inline bool SVC_MapList::Process()
{
    IRemoteServerMessageHandler *server = (IRemoteServerMessageHandler *)m_pMessageHandler;
    return server->ProcessMapList(this);
}

inline bool SVC_MapList::ReadFromBuffer( ByteBufRead &buffer )
{
    m_iList = buffer.ReadByte();
    m_iResponseNumber = buffer.ReadULong();
    m_iNumMaps = buffer.ReadByte();
    int mapsToRead = m_iNumMaps;
    if(mapsToRead > MAPLIST_MAX_MAPS)
        mapsToRead = MAPLIST_MAX_MAPS;
    for(int i = 0; i < mapsToRead; i++)
    {
        buffer.ReadString(m_szMaps[i], MAPLIST_MAX_MAPLEN);
    }

    return true;
}

inline bool SVC_MapList::WriteToBuffer( ByteBufWrite &buffer )
{
    buffer.WriteByte(m_iList);
    buffer.WriteULong(m_iResponseNumber);
    buffer.WriteByte(m_iNumMaps);
    for(int i = 0; i < m_iNumMaps; i++)
    {
        buffer.WriteString(m_szMaps[i]);
    }

    return true;
}


#endif
