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

#ifndef SOURCEOPADMIN_H
#define SOURCEOPADMIN_H

#include <stdio.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <winsock.h>
#else
#include <sys/types.h>  // for Socket data types
#include <sys/socket.h> // for socket(), connect(), send(), and recv()
#include <netinet/in.h> // for IP Socket data types
#include <arpa/inet.h>  // for sockaddr_in and inet_addr()
#include <stdlib.h>     // for atoi()
#include <unistd.h>     // for close()
#include <asm/ioctls.h>
#include <sys/ioctl.h>
#define SOCKET int
#define INVALID_SOCKET 0
#define SOCKET_ERROR -1
#define ioctlsocket ioctl
#define closesocket close

#define EAGAIN WSAEWOULDBLOCK
#define EWOULDBLOCK WSAEWOULDBLOCK
#define ENOBUFS WSAENOBUFS

#define __time64_t time_t
#define _time64 time
#endif
#include <math.h>

#include "utlvector.h"

#include "sourceopadmin_net.h"
#include "isopgamesystem.h"

#define     BUFFER_SIZE     1024    // Define the buffer size of the messages
#define     RECV_SIZE       32768   // Define the full buffer size when parsing received messages
#define     SNLEN           25      // Define the max screen name length
#define     PWLEN           17      // Define the max password length
#define     PWLEN           17      // Define the max password length
#define     AMSGLEN         128     // Define the max away message length
#define     MAXCVARSEGLEN   800     // Define the max cvar list segment data length

#define     MAX_SEARCH_RESULTS  50
#define     MAX_ENTSPAWNS       64

#define     PLAYERS_VIEW        (1<<0)
#define     PLAYERS_KICK        (1<<1)
#define     PLAYERS_BAN         (1<<2)
#define     PLAYERS_GOD         (1<<3)
#define     PLAYERS_NOCLIP      (1<<4)
#define     PLAYERS_CLOAK       (1<<5) // not done
#define     PLAYERS_CREDITS     (1<<6)
#define     PLAYERS_SLAP        (1<<7)
#define     PLAYERS_SLAY        (1<<8)
#define     PLAYERS_GUNS        (1<<9)
#define     PLAYERS_VIEWMAP     (1<<10)
#define     PLAYERS_DRAGPLAYER  (1<<11)
#define     PLAYERS_HEALTH      (1<<12)
#define     PLAYERS_SETTEAM     (1<<13)
#define     PLAYERS_GAG         (1<<14)

#define     ADMININFO_VIEW      (1<<0)
#define     ADMININFO_VIEWINFO  (1<<1)

#define     CHAT_VIEWPLAYER     (1<<0)
#define     CHAT_SENDPLAYER     (1<<1)
#define     CHAT_VIEWADMIN      (1<<2)
#define     CHAT_SENDADMIN      (1<<3)
#define     CHAT_PMPLAYER       (1<<4)
#define     CHAT_PMADMINS       (1<<5) // not done
#define     CHAT_SENDCSAY       (1<<6)
#define     CHAT_SENDBSAY       (1<<7)

#define     ADMIN_VIEWUSERLIST  (1<<0)
#define     ADMIN_VIEWUSERPASS  (1<<1)
#define     ADMIN_VIEWUSERPERM  (1<<2)
#define     ADMIN_EDITUSERPASS  (1<<3) // not done
#define     ADMIN_EDITUSERPERM  (1<<4) // not done
#define     ADMIN_DELUSER       (1<<5) // not done
#define     ADMIN_ADDUSER       (1<<6) // not done
#define     ADMIN_VIEWDB        (1<<7)
#define     ADMIN_EDITDB        (1<<8)
#define     ADMIN_RCON          (1<<9)
#define     ADMIN_SPAWNITEMS    (1<<10)
#define     ADMIN_VIEWMAPLIST   (1<<11)
#define     ADMIN_CHANGEMAP     (1<<12)
#define     ADMIN_VIEWSETTINGS  (1<<13)
#define     ADMIN_EDITSETTINGS  (1<<14)
#define     ADMIN_VIEWPASS      (1<<15)
#define     ADMIN_SETPASS       (1<<16)
#define     ADMIN_VIEWBANS      (1<<17)
#define     ADMIN_CHANGEBANS    (1<<18)
#define     ADMIN_VIEWLOGS      (1<<19)

typedef struct spamcheck_s
{
    unsigned long   lastLogin;
    char            ip[32];
} spamcheck_t;

class CRemoteAdminServer;
class CRemoteAdminClient;
class ByteBufRead;
class ByteBufWrite;

extern CRemoteAdminServer* remoteserver;

void SOPAdminStart();
void SOPAdminFrame();
void SOPAdminEnd();

typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;

class CRemoteAdminServer : public CAutoSOPGameSystem
{
public:
    CRemoteAdminServer();
    virtual ~CRemoteAdminServer();

    bool InitServer();
    void Frame();
    virtual void ClientDisconnect(edict_t *pEntity);
    virtual void LevelInitPostEntity();
    void LogPrint(const char *msg);
    void IncomingVoiceData(int playerslot, const char *data, int size);
    void PlayerChat(int player, bool teamchat, bool hidden, const char *msg);
    void ConsoleChat(const char *msg);
    void GameChat(int player, const char *msg);
    void AdminChat(const char *admin, const char *msg);
    void AdminChatToAdmins(const char *admin, const char *msg);

    int GetPort() { return m_iPort; }
    int GetNewUdpSessionNumber() { return ++m_iLastUdpSessionNumber; }

    void SendUdpData(ByteBufWrite &buf, const sockaddr *to, int tolen);

    // send player update about a single player (index starting at 1)
    void SendPlayerUpdateToAll(int player);

    void BlockLogOutputToClient(int client) { m_iBlockLogOutputToClient = client; }

    void PreparePlayerUpdate(int i, SVC_PlayerUpdate &update);
    void PrepareAdminUpdate(int i, SVC_AdminUpdate &update);
    void PrepareMapPlayerUpdate(int i, SVC_MapPlayerUpdate &update);

    void NotifyAdminConnectedNormal(int index);
    void NotifyAdminDisconnected(int index);

private:
    bool CreateSSLContext();
    bool LoadCertificates(const char *cFile, const char *kFile);
    bool Bind();

    void AcceptConnections();

private:
    bool m_bShuttingDown;
    int m_iPort;
    SOCKET m_iSocket;
    SOCKET m_iUdpSocket;
    int m_iLastUdpSessionNumber;
    SSL_CTX *ctx;

    int m_iConnectedClients;
    CRemoteAdminClient *m_clients[MAX_CLIENTS];

    int m_iBlockLogOutputToClient;

    double m_flNextMapUpdate;
};

enum
{
    STATE_UNAUTHENTICATED,
    STATE_MODESELECTION,
    STATE_NORMALMODE,
    STATE_FILETRANSFER,
};

class CRemoteAdminClient : public IRemoteClientMessageHandler
{
public:
    CRemoteAdminClient(int index, CRemoteAdminServer *server);
    ~CRemoteAdminClient();

    CRemoteAdminServer *GetServer() { return m_pServer; }
    bool IsConnected() { return m_bConnected; }
    time_t GetConnectTime() { return m_connectTime; }
    bool AuthenticatingOrSelectingMode() { return m_bConnected && (m_iState == STATE_UNAUTHENTICATED || m_iState == STATE_MODESELECTION); }
    bool IsAwaitingModeSelection() { return m_bConnected && m_iState == STATE_MODESELECTION; }
    bool AuthenticatedNormal() { return m_bConnected && m_iState == STATE_NORMALMODE; }
    bool HasPlayerAccess(int access) { return (m_iAccessPlayers & access) != 0; }
    bool HasAdminInfoAccess(int access) { return (m_iAccessAdminInfo & access) != 0; }
    bool HasChatAccess(int access) { return (m_iAccessChat & access) != 0; }
    bool HasAdminAccess(int access) { return (m_iAccessAdmin & access) != 0; }
    bool CanSendUdpPacket() { return m_bHasUdpSendAddr; }
    int GetUdpSessionNumber() { return m_iUdpSessionNumber; }
    in_addr GetIP() { return m_ip; }
    const char *GetName() { return m_szName; }

    void Accept(SOCKET s, struct sockaddr_in addr, SSL_CTX *ctx);
    void Frame();
    void ResetVars();

    void ProcessBuffer();
    void FindPacketHandler(int type, ByteBufRead &buf);
    void IncomingUdpPacket(struct sockaddr_in sender, ByteBufRead &buf);
    void SendDataUdp(ByteBufWrite &msg);
    void SendData(ByteBufWrite &msg);
    void SendNetMessage(ISOPNetMessage &msg, bool useUdp = false);

    void SendFullPlayerUpdate();
    void SendSinglePlayerUpdate(int player);
    void SendFullAdminUpdate();
    void SendSingleAdminUpdate(int admin);
    void SendServerInfo();

    void RecordMapUpdate(SVC_MapPlayerUpdate *update);
    void SetMapUpdateChangeFlags(SVC_MapPlayerUpdate *update);

    void SendMapCycle(unsigned int response);
    void SendAllMaps(unsigned int response);

    void Disconnect();

    bool RegisterMessage(ISOPNetMessage *msg);

    SOP_PROCESS_CLC_MESSAGE(Auth);
    SOP_PROCESS_CLC_MESSAGE(ModeSelection);
    SOP_PROCESS_CLC_MESSAGE(KickPlayer);
    SOP_PROCESS_CLC_MESSAGE(BanPlayer);
    SOP_PROCESS_CLC_MESSAGE(Rcon);
    SOP_PROCESS_CLC_MESSAGE(Ping);
    SOP_PROCESS_CLC_MESSAGE(PlayerChat);
    SOP_PROCESS_CLC_MESSAGE(SetPlayerTeam);
    SOP_PROCESS_CLC_MESSAGE(GagPlayer);
    SOP_PROCESS_CLC_MESSAGE(AdminChat);
    SOP_PROCESS_CLC_MESSAGE(RequestMapList);

    int m_iState;
private:

    CRemoteAdminServer *m_pServer;
    bool m_bConnected;
    time_t m_connectTime;
    bool m_bSSLHandshaking;
    int m_iIndex;
    char m_szName[MAX_REMOTEADMINNAME_LENGTH];

    int m_iAccessPlayers;
    int m_iAccessAdminInfo;
    int m_iAccessChat;
    int m_iAccessAdmin;

    SOCKET m_iSocket;
    in_addr m_ip;
    bool m_bHasUdpSendAddr;
    struct sockaddr_in m_UdpSendAddr;
    int m_iUdpSessionNumber;
    SSL *ssl;

    int m_iBufLen;
    char m_szBuf[RECV_SIZE];

    float m_flNextServerInfo;
    bool m_playerHasMapHistory[MAX_AOP_PLAYERS];
    SVC_MapPlayerUpdate::playerinfo_t m_playerMapHistory[MAX_AOP_PLAYERS];

    CUtlVector<ISOPNetMessage *> m_messages;
};

#endif
