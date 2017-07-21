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
#include "convar_l4d.h"
#endif
#include "cbase.h"
#include "baseentity.h"
// HACK HACK HACK : BASEENTITY ACCESS

#include "sopsteamid.h"
#include "vstdlib/jobthread.h"

#include <stdio.h>
#include <ctype.h>
#include <time.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#ifdef __linux__
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "AdminOP.h"

#include "vphysics_interface.h"
#include "object_hash.h"

#include "adminopplayer.h"
#include "sourceopadmin.h"
#include "mapcycletracker.h"
#include "bitbuf.h"
#include "bytebuf.h"
#include "packet_types.h"
#include "cvars.h"
#include "vfuncs.h"
#include "crc32.h"

#include "tier0/memdbgon.h"

CRemoteAdminServer* remoteserver = NULL;
__time64_t ltime;

int     isshutdown = 0;

CUtlLinkedList <spamcheck_t, unsigned int> spamCheck;

void SOPAdminStart()
{
    SSL_library_init();
    remoteserver = new CRemoteAdminServer();
    if(remoteserver->InitServer())
    {
        CAdminOP::ColorMsg(CONCOLOR_DARKGRAY, "[SOURCEOP] Remote admin server listening on port %i.\n", remoteserver->GetPort());
    }
    else
    {
        CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Remote admin server failed to start.\n");
        delete remoteserver;
        remoteserver = NULL;
    }
}

void SOPAdminFrame()
{
    _time64( &ltime );

    if(remoteserver)
        remoteserver->Frame();
}

void SOPAdminEnd()
{
    if(remoteserver)
    {
        delete remoteserver;
        remoteserver = NULL;
    }
}


CRemoteAdminServer::CRemoteAdminServer() : CAutoSOPGameSystem("CRemoteAdminServer")
{
    m_bShuttingDown = false;
    m_iPort = remote_port.GetInt();
    m_iSocket = 0;
    m_iUdpSocket = 0;
    m_iLastUdpSessionNumber = random->RandomInt(0, INT_MAX);
    m_iConnectedClients = 0;

    m_iBlockLogOutputToClient = -1;

    m_flNextMapUpdate = 0;

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        m_clients[i] = NULL;
    }
}

CRemoteAdminServer::~CRemoteAdminServer()
{
    m_bShuttingDown = true;
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(m_clients[i])
        {
            if(m_clients[i]->IsConnected())
            {
                m_clients[i]->Disconnect();
            }

            delete m_clients[i];
            m_clients[i] = NULL;
        }
    }

    if(m_iSocket)
        closesocket(m_iSocket);
    if(m_iUdpSocket)
        closesocket(m_iUdpSocket);
}

bool CRemoteAdminServer::InitServer()
{
    bool r;
    r = CreateSSLContext();
    if(!r) return false;

    char certpath[512];
    char pkeypath[512];
    Q_snprintf(certpath, sizeof(certpath), "%s/%s/remote.crt", pAdminOP.GameDir(), pAdminOP.DataDir());
    Q_snprintf(pkeypath, sizeof(pkeypath), "%s/%s/remote.key", pAdminOP.GameDir(), pAdminOP.DataDir());

    r = LoadCertificates(certpath, pkeypath);
    if(!r) return false;
    r = Bind();
    if(!r) return false;

    return true;
}

void CRemoteAdminServer::Frame()
{
    AcceptConnections();

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(m_clients[i])
        {
            if(m_clients[i]->IsConnected())
            {
                m_clients[i]->Frame();
            }
            else
            {
                delete m_clients[i];
                m_clients[i] = NULL;
            }
        }
    }

    while(true)
    {
        char data[2048];
        struct sockaddr_in sender;
        int senderlen = sizeof(sender);

        int r = recvfrom(m_iUdpSocket, data, sizeof(data), 0, (struct sockaddr *)&sender, &senderlen);
        if(r == -1)
        {
            break;
        }

        // session + size + type
        // don't pay attention to udp packets if no admins are connected
        if(r < 7 || !m_iConnectedClients)
        {
            continue;
        }

        ByteBufRead buf(data, r);
        int session = buf.ReadLong();
        for(int i = 0; i < MAX_CLIENTS; i++)
        {
            CRemoteAdminClient *client = m_clients[i];
            if(client && client->AuthenticatedNormal())
            {
                if(client->GetUdpSessionNumber() == session && sender.sin_addr.s_addr == client->GetIP().s_addr)
                {
                    client->IncomingUdpPacket(sender, buf);
                    break;
                }
            }
        }
    }

    // send map updates
    double enginetime = Plat_FloatTime();
    if(m_iConnectedClients && enginetime >= m_flNextMapUpdate)
    {
        SVC_MapPlayerUpdate update(enginetime);
        for(int i = 1; i <= pAdminOP.GetMaxClients(); i++)
        {
            PrepareMapPlayerUpdate(i, update);
        }
        for(int i = 0; i < MAX_CLIENTS; i++)
        {
            CRemoteAdminClient *client = m_clients[i];
            if(client && client->AuthenticatedNormal() && client->HasPlayerAccess(PLAYERS_VIEW) && client->HasPlayerAccess(PLAYERS_VIEWMAP) && client->CanSendUdpPacket())
            {
                client->SetMapUpdateChangeFlags(&update);
                client->RecordMapUpdate(&update);
                client->SendNetMessage(update, true);
            }
        }
        m_flNextMapUpdate = enginetime + remote_mapupdaterate.GetFloat();
    }

    // Disconnect clients that take too long to auth
    time_t curTime;
    time(&curTime);
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        CRemoteAdminClient *client = m_clients[i];
        if(client && client->AuthenticatingOrSelectingMode() && client->GetConnectTime() + 4 <= curTime)
        {
            client->Disconnect();
        }
    }
}

void CRemoteAdminServer::ClientDisconnect(edict_t *pEntity)
{
    int player = ENTINDEX(pEntity);
    SVC_PlayerDisconnect message(player-1);

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        CRemoteAdminClient *client = m_clients[i];
        if(client && client->AuthenticatedNormal() && client->HasPlayerAccess(PLAYERS_VIEW))
        {
            client->SendNetMessage(message);
        }
    }
}

void CRemoteAdminServer::LevelInitPostEntity()
{
    m_flNextMapUpdate = 0;

    SVC_MapChange map;
    V_strncpy(map.m_szMap, MapName(), sizeof(map.m_szMap));
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        CRemoteAdminClient *client = m_clients[i];
        if(client && client->AuthenticatedNormal())
        {
            client->SendNetMessage(map);
        }
    }
}

void CRemoteAdminServer::LogPrint(const char *msg)
{
    SVC_LogData message;
    V_strncpy(message.m_szLogMessage, msg, sizeof(message.m_szLogMessage));

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        CRemoteAdminClient *client = m_clients[i];
        if(client && i != m_iBlockLogOutputToClient && client->AuthenticatedNormal() && client->HasAdminAccess(ADMIN_RCON))
        {
            client->SendNetMessage(message);
        }
    }
}

void CRemoteAdminServer::IncomingVoiceData(int playerslot, const char *data, int size)
{
    SVC_VoiceData message;

    if(size > sizeof(message.m_szVoiceData))
    {
        Msg("[SOURCEOP] Warning: Large voice data not sent to remote clients.\n");
        return;
    }

    message.m_iPlayer = playerslot;
    message.m_iSize = size;
    memcpy(message.m_szVoiceData, data, size);

    if ( sv_use_steam_voice == NULL || !sv_use_steam_voice->GetBool() )
    {
        message.m_iCodec = SOP_VOICE_CODEC_SPEEX;
    }
    else
    {
        message.m_iCodec = SOP_VOICE_CODEC_STEAM;
    }

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        CRemoteAdminClient *client = m_clients[i];
        if(client && client->AuthenticatedNormal() && client->HasChatAccess(CHAT_VIEWPLAYER))
        {
            client->SendNetMessage(message);
        }
    }
}

void CRemoteAdminServer::PlayerChat(int player, bool teamchat, bool hidden, const char *msg)
{
    SVC_PlayerChat message;

    message.m_iPlayer = player-1;
    if(player >= 1 && player <= pAdminOP.GetMaxClients() && !pAdminOP.pAOPPlayers[player-1].IsAlive())
        message.SetDeadChat();
    if(teamchat)
        message.SetTeamChat();
    if(hidden)
        message.SetHidden();
    V_strncpy(message.m_szMessage, msg, sizeof(message.m_szMessage));

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        CRemoteAdminClient *client = m_clients[i];
        if(client && client->AuthenticatedNormal() && client->HasChatAccess(CHAT_VIEWPLAYER))
        {
            client->SendNetMessage(message);
        }
    }
}

void CRemoteAdminServer::ConsoleChat(const char *msg)
{
    SVC_PlayerChat message;

    message.SetConsole();
    V_strncpy(message.m_szMessage, msg, sizeof(message.m_szMessage));

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        CRemoteAdminClient *client = m_clients[i];
        if(client && client->AuthenticatedNormal() && client->HasChatAccess(CHAT_VIEWPLAYER))
        {
            client->SendNetMessage(message);
        }
    }
}

void CRemoteAdminServer::GameChat(int player, const char *msg)
{
    SVC_PlayerChat message;

    message.SetFromGame();
    if(player == 0)
    {
        message.SetConsole();
    }
    else
    {
        message.m_iPlayer = player - 1;
    }
    V_strncpy(message.m_szMessage, msg, sizeof(message.m_szMessage));

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        CRemoteAdminClient *client = m_clients[i];
        if(client && client->AuthenticatedNormal() && client->HasChatAccess(CHAT_VIEWPLAYER))
        {
            client->SendNetMessage(message);
        }
    }
}

void CRemoteAdminServer::AdminChat(const char *admin, const char *msg)
{
    SVC_PlayerChat message;

    message.SetFromAdmin();
    V_strncpy(message.m_szAdminName, admin, sizeof(message.m_szAdminName));
    V_strncpy(message.m_szMessage, msg, sizeof(message.m_szMessage));

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        CRemoteAdminClient *client = m_clients[i];
        if(client && client->AuthenticatedNormal() && client->HasChatAccess(CHAT_VIEWPLAYER))
        {
            client->SendNetMessage(message);
        }
    }
}

void CRemoteAdminServer::AdminChatToAdmins(const char *admin, const char *msg)
{
    SVC_AdminChat message;

    V_strncpy(message.m_szAdminName, admin, sizeof(message.m_szAdminName));
    V_strncpy(message.m_szMessage, msg, sizeof(message.m_szMessage));

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        CRemoteAdminClient *client = m_clients[i];
        if(client && client->AuthenticatedNormal() && client->HasChatAccess(CHAT_VIEWADMIN))
        {
            client->SendNetMessage(message);
        }
    }
}

void CRemoteAdminServer::SendUdpData(ByteBufWrite &buf, const sockaddr *to, int tolen)
{
    sendto(m_iUdpSocket, (char *)buf.GetData(), buf.GetNumBytesWritten(), 0, to, tolen);
}

void CRemoteAdminServer::SendPlayerUpdateToAll(int player)
{
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        CRemoteAdminClient *client = m_clients[i];
        if(client && client->AuthenticatedNormal() && client->HasPlayerAccess(PLAYERS_VIEW))
        {
            client->SendSinglePlayerUpdate(player);
        }
    }
}

void CRemoteAdminServer::PreparePlayerUpdate(int i, SVC_PlayerUpdate &update)
{
    if(i <= 0 || i > pAdminOP.GetMaxClients())
        return;

    edict_t *pEdictPlayer = pAdminOP.GetEntityList()+i;
    IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pEdictPlayer);
    if(!info)
    {
        return;
    }

    CAdminOPPlayer *aopPlayer = &pAdminOP.pAOPPlayers[i-1];

    const CSteamID *steamidptr = engine->GetClientSteamID(pEdictPlayer);
    unsigned long long steamid;
    if(steamidptr)
    {
        steamid = steamidptr->ConvertToUint64();
    }
    else
    {
        steamid = 0;
    }

    update.AddPlayer(
        i-1,
        info->GetName(),
        steamid,
        info->GetFragCount(),
        info->GetDeathCount(),
        info->GetTeamIndex(),
        aopPlayer->GetPlayerClass(),
        aopPlayer->addr,
        aopPlayer->GetCredits(),
        aopPlayer->GetTimePlayed(),
        aopPlayer->IsGagged());
}

void CRemoteAdminServer::PrepareAdminUpdate(int i, SVC_AdminUpdate &update)
{
    if(i < 0 || i >= MAX_CLIENTS)
        return;

    CRemoteAdminClient *client = m_clients[i];
    if(client == NULL || !client->AuthenticatedNormal())
        return;

    // TODO: Add away mode and away message
    update.AddAdmin(i, client->GetName(), false, "");
}

void CRemoteAdminServer::PrepareMapPlayerUpdate(int i, SVC_MapPlayerUpdate &update)
{
    if(i <= 0 || i > pAdminOP.GetMaxClients())
        return;

    edict_t *pEdictPlayer = pAdminOP.GetEntityList()+i;
    IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pEdictPlayer);
    if(!info)
    {
        return;
    }
    CBasePlayer *pPlayer = (CBasePlayer *)VFuncs::Instance(pEdictPlayer);
    CAdminOPPlayer *aopPlayer = &pAdminOP.pAOPPlayers[i-1];

    Vector position = VFuncs::GetAbsOrigin(pPlayer);
    Vector velocity = VFuncs::GetAbsVelocity(pPlayer);
    update.AddPlayer(i-1, aopPlayer->IsAlive(), position.x, position.y, velocity.x, velocity.y, clamp(info->GetHealth(), -32768, 32767), clamp(info->GetMaxHealth(), -32768, 32767));
}

void CRemoteAdminServer::NotifyAdminConnectedNormal(int index)
{
    CRemoteAdminClient *connectedClient = m_clients[index];
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(i == index)
            continue;

        CRemoteAdminClient *client = m_clients[i];
        if(client && client->AuthenticatedNormal() && !stricmp(client->GetName(), connectedClient->GetName()))
        {
            SVC_StatusMessage message;
            V_snprintf(message.m_szMessage, sizeof(message.m_szMessage), "Duplicate login detected. Disconnecting.\n");
            client->SendNetMessage(message);

            // TODO: seniorproj: Change this to a message box type message
            SVC_StatusMessage messageboxmessage;
            V_snprintf(messageboxmessage.m_szMessage, sizeof(messageboxmessage.m_szMessage), "You have been disconected because you have logged in elsewhere.\n");
            client->SendNetMessage(messageboxmessage);

            // TODO: seniorproj: Send a "no auto reconnect" message.

            client->Disconnect();
        }
    }

    // update all other admins that this admin connected
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(i == index)
            continue;

        CRemoteAdminClient *client = m_clients[i];
        if(client && client->AuthenticatedNormal())
        {
            client->SendSingleAdminUpdate(index);
        }
    }
}

void CRemoteAdminServer::NotifyAdminDisconnected(int index)
{
    m_iConnectedClients--;

    SVC_AdminDisconnect message(index);
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(i == index)
            continue;

        CRemoteAdminClient *client = m_clients[i];
        if(client && client->AuthenticatedNormal() && client->HasAdminInfoAccess(ADMININFO_VIEW))
        {
            client->SendNetMessage(message);
        }
    }
}

bool CRemoteAdminServer::Bind()
{
    struct sockaddr_in addr;
    unsigned long set = 1;

    memset(&addr, 0, sizeof(addr));
    errno = 0;
    m_iSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(m_iSocket == INVALID_SOCKET)
    {
        Warning("Error creating remote admin TCP socket.\n");
#ifdef _WIN32
        Warning(" - %i %i %i\n", WSAGetLastError(), GetLastError(), errno);
#endif
        return false;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_iPort);
    addr.sin_addr.s_addr = INADDR_ANY;
    if(srv_ip)
    {
        if(stricmp(srv_ip->GetString(), "localhost") && stricmp(srv_ip->GetString(), "127.0.0.1"))
        {
            addr.sin_addr.s_addr = inet_addr(srv_ip->GetString());
        }
    }

    // Sets the option to re-use the address the entire run of the program
    setsockopt(m_iSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&set, sizeof(set));

    if(bind(m_iSocket, (struct sockaddr *)&addr, sizeof(addr)) != 0)
    {
        Warning("Error binding port.\n");
#ifdef _WIN32
        Warning(" - %i\n", WSAGetLastError());
#endif
        return false;
    }

    if(listen(m_iSocket, 32) != 0)
    {
        Warning("Error listening on port.\n");
        return false;
    }

    // make the server non blocking
    ioctlsocket(m_iSocket, FIONBIO, &set);

    m_iUdpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(m_iUdpSocket == INVALID_SOCKET)
    {
        Warning("Error creating remote admin UDP socket.\n");
#ifdef _WIN32
        Warning(" - %i %i %i\n", WSAGetLastError(), GetLastError(), errno);
#endif
        return false;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_iPort);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if(srv_ip)
    {
        if(stricmp(srv_ip->GetString(), "localhost") && stricmp(srv_ip->GetString(), "127.0.0.1"))
        {
            addr.sin_addr.s_addr = inet_addr(srv_ip->GetString());
        }
    }
    if(bind(m_iUdpSocket, (struct sockaddr *)&addr, sizeof(addr)) != 0)
    {
        Warning("Error binding UDP port.\n");
        return false;
    }

    // make the server non blocking
    ioctlsocket(m_iUdpSocket, FIONBIO, &set);

    return true;
}

static int ssl_print_errors(const char *str, size_t len, void *u)
{
    CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] SSL Error: %s\n", str);
    return 0;
}

// http://www.metalshell.com/source_code/108/OpenSSL_Server_Example.html
bool CRemoteAdminServer::CreateSSLContext()
{
    // The method describes which SSL protocol we will be using.
    const SSL_METHOD *method;

    // Load algorithms and error strings.
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    // Compatible with SSLv2, SSLv3 and TLSv1
    method = SSLv23_server_method();

    // Create new context from method.
    ctx = SSL_CTX_new(method);
    if(ctx == NULL) {
        ERR_print_errors_cb(&ssl_print_errors, NULL);
        return false;
    }
    return true;
}

bool CRemoteAdminServer::LoadCertificates(const char *cFile, const char *kFile)
{
    if ( SSL_CTX_use_certificate_chain_file(ctx, cFile) <= 0) {
        ERR_print_errors_cb(&ssl_print_errors, NULL);
        return false;
    }
    if ( SSL_CTX_use_PrivateKey_file(ctx, kFile, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_cb(&ssl_print_errors, NULL);
        return false;
    }

    // Verify that the two keys go together.
    if ( !SSL_CTX_check_private_key(ctx) ) {
        return false;
    }

    return true;
}

void CRemoteAdminServer::AcceptConnections()
{
    if(m_iConnectedClients >= MAX_CLIENTS)
    {
        return;
    }

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(m_clients[i] && m_clients[i]->IsConnected())
            continue;

        struct sockaddr_in addr;
        int len = sizeof(addr);
        SOCKET newSock = accept(m_iSocket, (struct sockaddr *)&addr, &len);
        if(newSock == 0 || newSock == SOCKET_ERROR)
        {
            break;
        }

        if(m_clients[i] == NULL)
            m_clients[i] = new CRemoteAdminClient(i, this);

        m_clients[i]->Accept(newSock, addr, ctx);
        m_iConnectedClients++;
        Msg("[SOURCEOP] Accepting remote admin connection into slot %i.\n", i);
    }
}

CRemoteAdminClient::CRemoteAdminClient(int index, CRemoteAdminServer *server)
{
    m_pServer = server;
    m_bConnected = false;
    m_bSSLHandshaking = false;
    m_iState = STATE_UNAUTHENTICATED;
    m_iIndex = index;
    m_iSocket = 0;
    m_ip.s_addr = 0;
    m_bHasUdpSendAddr = false;
    ssl = NULL;

    m_iBufLen = 0;

    SOP_REGISTER_CLC_MSG(Auth);
    SOP_REGISTER_CLC_MSG(ModeSelection);
    SOP_REGISTER_CLC_MSG(KickPlayer);
    SOP_REGISTER_CLC_MSG(BanPlayer);
    SOP_REGISTER_CLC_MSG(Rcon);
    SOP_REGISTER_CLC_MSG(Ping);
    SOP_REGISTER_CLC_MSG(PlayerChat);
    SOP_REGISTER_CLC_MSG(SetPlayerTeam);
    SOP_REGISTER_CLC_MSG(GagPlayer);
    SOP_REGISTER_CLC_MSG(AdminChat);
    SOP_REGISTER_CLC_MSG(RequestMapList);
}

CRemoteAdminClient::~CRemoteAdminClient()
{
    for(int i = 0; i < m_messages.Count(); i++)
    {
        ISOPNetMessage *message = m_messages[i];
        delete message;
    }

    m_messages.Purge();
}

void CRemoteAdminClient::Accept(SOCKET s, struct sockaddr_in addr, SSL_CTX *ctx)
{
    unsigned long set = 1;

    m_iSocket = s;

    time(&m_connectTime);
    m_iAccessPlayers = 0;
    m_iAccessAdminInfo = 0;
    m_iAccessChat = 0;
    m_iAccessAdmin = 0;
    m_ip = addr.sin_addr;
    m_bHasUdpSendAddr = 0;
    m_iUdpSessionNumber = 0;
    m_bConnected = true;
    ioctlsocket(m_iSocket, FIONBIO, &set);
#ifdef __linux__
    fcntl(m_iSocket, F_SETFL, fcntl(m_iSocket, F_GETFL) | O_NONBLOCK);
#endif
    int buf = 65536;
    int r = setsockopt(m_iSocket, SOL_SOCKET, SO_SNDBUF, (const char *)&buf, sizeof(int));
    if(r)
    {
        Msg("[SOURCEOP] Error %i setting buffer.\n", r);
    }

    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, m_iSocket);

    m_bSSLHandshaking = true;
}

void CRemoteAdminClient::Frame()
{
    char buf[BUFFER_SIZE];
    int r, err;

    if(!m_bConnected)
        return;

    if(m_bSSLHandshaking)
    {
        r = SSL_accept(ssl);
        if(r < 0)
        {
            err = SSL_get_error(ssl, r);
            if(err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE)
            {
                //Msg("Fail %i...\n", err);
                Disconnect();
            }
        }
        else if(r == 0)
        {
            //Msg("Fail r 0...\n");
            Disconnect();
        }
        else
        {
            m_bSSLHandshaking = false;

            m_iState = STATE_UNAUTHENTICATED;
            ResetVars();
        }
        return;
    }

    r = SSL_read(ssl, buf, BUFFER_SIZE);
    if(r <= 0)
    {
        err = SSL_get_error(ssl, r);
        if(err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE)
        {
            int errcode = ERR_get_error();
            if((err != SSL_ERROR_SYSCALL && err != SSL_ERROR_ZERO_RETURN) || errcode != 0)
            {
                Msg("[SOURCEOP] Read fail %i on remote client %i.\n", err, m_iIndex);
                char szErr[512];
                ERR_error_string(errcode, szErr);
                Msg("[SOURCEOP]  - %s\n", szErr);
            }
            Disconnect();
        }
    }
    else
    {
        if(m_iBufLen + r <= RECV_SIZE)
        {
            memcpy(&m_szBuf[m_iBufLen], buf, r);
            m_iBufLen += r;
            ProcessBuffer();
        }
        else
        {
            Msg("[SOURCEOP] Buffer would have overflowed for remote client %i.\n", m_iIndex);
            Disconnect();
        }
    }

    if(AuthenticatedNormal())
    {
        float now = engine->Time();
        if(m_flNextServerInfo <= now)
        {
            SendServerInfo();
            m_flNextServerInfo = now + 3;
        }
    }
}

void CRemoteAdminClient::ResetVars()
{
    m_iBufLen = 0;
    m_flNextServerInfo = 0;
    memset(m_playerHasMapHistory, 0, sizeof(m_playerHasMapHistory));
    memset(m_playerMapHistory, 0, sizeof(m_playerMapHistory));
}

void CRemoteAdminClient::ProcessBuffer()
{
    while(true)
    {
        if(m_iBufLen < sizeof(unsigned short))
            return;

        ByteBufRead buf(m_szBuf, m_iBufLen);
        unsigned short nextPacketSize = buf.PeekUShort();
        int totalLength = nextPacketSize + sizeof(unsigned short);

        if(m_iBufLen < totalLength)
        {
            //Msg("Waiting for more data (have %i need %i)...\n", m_iBufLen, totalLength);
            return;
        }

        //Msg("Reading packet of size %i. Buf len: %i\n", nextPacketSize, m_iBufLen);

        unsigned short packetSize = buf.ReadUShort();
        int type = buf.ReadByte();
        FindPacketHandler(type, buf);

        int remainingBytes = m_iBufLen - totalLength;
        if(remainingBytes > 0)
        {
            //Msg("Moving %i bytes to front.\n", remainingBytes);
            memmove(m_szBuf, &m_szBuf[totalLength], remainingBytes);
        }
        m_iBufLen = remainingBytes;
    }
}

void CRemoteAdminClient::FindPacketHandler(int type, ByteBufRead &buf)
{
    bool handled = false;
    for(int i = 0; i < m_messages.Count(); i++)
    {
        ISOPNetMessage *message = m_messages[i];
        if(message->GetType() == type)
        {
            bool r;
            r = message->ReadFromBuffer(buf);
            if(r)
            {
                r = message->Process();
                if(!r)
                {
                    Disconnect();
                }
            }
            else
            {
                Disconnect();
            }
            handled = true;
        }
    }

    if(!handled)
    {
        Msg("[SOURCEOP] Unexpected message type %i %c.\n", type, type);
        Disconnect();
    }
}

void CRemoteAdminClient::IncomingUdpPacket(struct sockaddr_in sender, ByteBufRead &buf)
{
    m_bHasUdpSendAddr = true;
    m_UdpSendAddr = sender;

    unsigned short packetSize = buf.ReadUShort();
    int type = buf.ReadByte();

    FindPacketHandler(type, buf);
}

void CRemoteAdminClient::SendDataUdp(ByteBufWrite &msg)
{
    if(!m_bHasUdpSendAddr)
    {
        return;
    }

    char szPacket[2048];
    ByteBufWrite buf(szPacket, sizeof(szPacket));
    buf.WriteLong(m_iUdpSessionNumber);
    buf.WriteUShort(msg.GetNumBytesWritten());
    buf.WriteBytes(msg.GetData(), msg.GetNumBytesWritten());

    m_pServer->SendUdpData(buf, (struct sockaddr *)&m_UdpSendAddr, sizeof(struct sockaddr_in));
}

void CRemoteAdminClient::SendData(ByteBufWrite &msg)
{
    char szPacket[2048];
    ByteBufWrite buf(szPacket, sizeof(szPacket));

    buf.WriteUShort(msg.GetNumBytesWritten());
    buf.WriteBytes(msg.GetData(), msg.GetNumBytesWritten());

    int bytesToWrite = buf.GetNumBytesWritten();
    int r = SSL_write(ssl, szPacket, bytesToWrite);
    if(r != bytesToWrite)
    {
        int err = SSL_get_error(ssl, r);
        if(err == SSL_ERROR_WANT_WRITE)
        {
            // TODO: Maybe add a send buffer or something
            Disconnect();
            Msg("[SOURCEOP] Failed to write all data to remote client %i.\n", m_iIndex);
            Msg("[SOURCEOP]  - Wanted %i, wrote %i.\n", bytesToWrite, r);
        }
        else
        {
            char szErr[512];
            ERR_error_string(ERR_get_error(), szErr);

            Disconnect();
            Msg("[SOURCEOP] Error writing to remote client %i.\n", m_iIndex);
            Msg("[SOURCEOP]  - Tried to write %i bytes, wrote %i.\n", bytesToWrite, r);
            Msg("[SOURCEOP]  - Error: %i (%s)\n", err, szErr);
        }
    }
}

void CRemoteAdminClient::SendNetMessage(ISOPNetMessage &msg, bool useUdp)
{
    if(!m_bConnected)
        return;

    char szPacket[2048];
    ByteBufWrite buf(szPacket, sizeof(szPacket));

    buf.WriteByte(msg.GetType());
    msg.WriteToBuffer(buf);

    if(useUdp)
    {
        if(m_bHasUdpSendAddr)
        {
            SendDataUdp(buf);
        }
    }
    else
    {
        SendData(buf);
    }
}

void CRemoteAdminClient::SendFullPlayerUpdate()
{
    if(!HasPlayerAccess(PLAYERS_VIEW))
        return;

    SVC_PlayerUpdate update;

    for(int i = 1; i <= pAdminOP.GetMaxClients(); i++)
    {
        m_pServer->PreparePlayerUpdate(i, update);
    }

    SendNetMessage(update);
}

void CRemoteAdminClient::SendSinglePlayerUpdate(int player)
{
    if(player <= 0 || player > pAdminOP.GetMaxClients())
        return;
    if(!HasPlayerAccess(PLAYERS_VIEW))
        return;

    SVC_PlayerUpdate update;

    m_pServer->PreparePlayerUpdate(player, update);
    SendNetMessage(update);
}

void CRemoteAdminClient::SendFullAdminUpdate()
{
    if(!HasAdminInfoAccess(ADMININFO_VIEW))
        return;

    SVC_AdminUpdate update;

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        m_pServer->PrepareAdminUpdate(i, update);
    }

    SendNetMessage(update);
}

void CRemoteAdminClient::SendSingleAdminUpdate(int admin)
{
    if(admin < 0 || admin >= MAX_CLIENTS)
        return;
    if(!HasPlayerAccess(ADMININFO_VIEW))
        return;

    SVC_AdminUpdate update;

    m_pServer->PrepareAdminUpdate(admin, update);
    SendNetMessage(update);
}

void CRemoteAdminClient::SendServerInfo()
{
    SVC_ServerInfo info;

    V_strncpy(info.m_szServerName, hostname->GetString(), sizeof(info.m_szServerName));
    V_strncpy(info.m_szGame, pAdminOP.ModName(), sizeof(info.m_szGame));
    info.m_iMaxPlayers = pAdminOP.GetMaxClients();
    info.m_iTimeLimit = timelimit->GetInt();
    info.m_iTimeRemaining = pAdminOP.GetMapTimeRemaining();
    info.m_iFragLimit = mp_fraglimit ? mp_fraglimit->GetInt() : 0;
    // TODO: Fill in frags remaining
    info.m_iFragsRemaining = 0;
    info.m_iWinLimit = mp_winlimit ? mp_winlimit->GetInt() : 0;
    info.m_iMaxRounds = mp_maxrounds ? mp_maxrounds->GetInt() : 0;
    info.m_bFriendlyFire = mp_friendlyfire ? mp_friendlyfire->GetBool() : false;
    info.m_bBansRequireReason = bans_require_reason.GetBool();
    info.m_iGamePort = srv_hostport->GetInt();

    SendNetMessage(info);
}

void CRemoteAdminClient::RecordMapUpdate(SVC_MapPlayerUpdate *update)
{
    for(int i = 0; i < update->GetNumPlayers(); i++)
    {
        SVC_MapPlayerUpdate::playerinfo_t *player = update->GetPlayer(i);
        m_playerHasMapHistory[player->index] = true;
        m_playerMapHistory[player->index] = *player;
    }
}

void CRemoteAdminClient::SetMapUpdateChangeFlags(SVC_MapPlayerUpdate *update)
{
    for(int i = 0; i < update->GetNumPlayers(); i++)
    {
        SVC_MapPlayerUpdate::playerinfo_t *player = update->GetPlayer(i);
        if(!m_playerHasMapHistory[player->index])
        {
            player->messageFlags = SVC_MAPPLAYERUPDATE_INCLUDES_ALL;
            continue;
        }
        player->messageFlags = 0;
        SVC_MapPlayerUpdate::playerinfo_t *oldplayer = &m_playerMapHistory[player->index];
        if(player->alive != oldplayer->alive)
        {
            player->messageFlags |= SVC_MAPPLAYERUPDATE_INCLUDES_ALIVE;
        }
        if(player->posX != oldplayer->posX || player->posY != oldplayer->posY)
        {
            player->messageFlags |= SVC_MAPPLAYERUPDATE_INCLUDES_POS;
        }
        if(player->velX != oldplayer->velX || player->velY != oldplayer->velY)
        {
            player->messageFlags |= SVC_MAPPLAYERUPDATE_INCLUDES_VEL;
        }
        if(player->health != oldplayer->health)
        {
            player->messageFlags |= SVC_MAPPLAYERUPDATE_INCLUDES_HEALTH;
        }
        if(player->maxHealth != oldplayer->maxHealth)
        {
            player->messageFlags |= SVC_MAPPLAYERUPDATE_INCLUDES_MAXHEALTH;
        }
    }
}

void CRemoteAdminClient::SendMapCycle(unsigned int response)
{
    if(!mapcyclefile)
        return;

    SVC_MapList maplist(MAPLIST_ALL, response);
    const char *mapcfile = mapcyclefile->GetString();

    int length;
    char *pFileList;
    char *aFileList = pFileList = (char*)UTIL_LoadFileForMe( mapcfile, &length );
    if ( pFileList && length )
    {
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

            if ( DFIsMapValid( cBuf ) )
            {
                V_strncpy(maplist.m_szMaps[maplist.m_iNumMaps], cBuf, MAPLIST_MAX_MAPLEN);
                maplist.m_iNumMaps++;

                if(maplist.m_iNumMaps >= MAPLIST_MAX_MAPS)
                {
                    SendNetMessage(maplist);
                    maplist.m_iNumMaps = 0;
                }
            }

            pFileList += strlen( cBuf );
        }
        if(maplist.m_iNumMaps != '\0')
        {
            SendNetMessage(maplist);
        }
        UTIL_FreeFile( (byte *)aFileList );
    }
}

void CRemoteAdminClient::SendAllMaps(unsigned int response)
{
    SVC_MapList maplist(MAPLIST_CYCLE, response);
    FileFindHandle_t findHandle;

    const char *pFilename = filesystem->FindFirstEx( "maps\\*.bsp", "MOD", &findHandle );
    while ( pFilename != NULL )
    {
        char *mapname = maplist.m_szMaps[maplist.m_iNumMaps];
        V_strncpy(mapname, pFilename, MAPLIST_MAX_MAPLEN);
        mapname[strlen(mapname)-4] = '\0';
        maplist.m_iNumMaps++;

        if(maplist.m_iNumMaps >= MAPLIST_MAX_MAPS)
        {
            SendNetMessage(maplist);
            maplist.m_iNumMaps = 0;
        }

        pFilename = filesystem->FindNext( findHandle );
    }
    if(maplist.m_iNumMaps != '\0')
    {
        SendNetMessage(maplist);
    }
    filesystem->FindClose( findHandle );
}

void CRemoteAdminClient::Disconnect()
{
    if(m_bConnected == false)
        return;

    m_pServer->NotifyAdminDisconnected(m_iIndex);
    m_bConnected = false;
    m_bSSLHandshaking = false;
    m_iState = STATE_UNAUTHENTICATED;
    m_ip.s_addr = 0;
    m_bHasUdpSendAddr = 0;
    m_iUdpSessionNumber = 0;
    Msg("[SOURCEOP] Remote client #%i %s disconnecting.\n", m_iIndex, m_szName);
    memset(m_szName, 0, sizeof(m_szName));

    if(ssl)
    {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        ssl = NULL;
    }

    if(m_iSocket)
    {
        closesocket(m_iSocket);
        m_iSocket = 0;
    }
}

bool CRemoteAdminClient::RegisterMessage(ISOPNetMessage *msg)
{
    m_messages.AddToTail(msg);
    return true;
}

// TODO: seniorproj: Check access level for all these
SOP_DECLARE_CLC_MESSAGE(CRemoteAdminClient, Auth)
{
    if(m_iState != STATE_UNAUTHENTICATED)
    {
        Disconnect();
        return false;
    }


    SVC_AuthComplete authcomplete;
    bool loginSuccess = false;

    // don't allow any slashes in the user name
    int userlen = strlen(msg->m_szUser);
    for(unsigned int slashcheck = 0; slashcheck < userlen; slashcheck++)
    {
        if(msg->m_szUser[slashcheck] == '/' || msg->m_szUser[slashcheck] == '\\')
        {
            authcomplete.m_bSuccess = false;
            SendNetMessage(authcomplete);
            Disconnect();
            return true;
        }
    }

    // don't allow certain usernames
    if(userlen == 0 || !strcmp(msg->m_szUser, "list"))
    {
        authcomplete.m_bSuccess = false;
        SendNetMessage(authcomplete);
        Disconnect();
        return true;
    }

    char buf[1024];
    char file[1024];
    V_snprintf(file, sizeof(file), "%s/%s/users/%s.txt", pAdminOP.GameDir(), pAdminOP.DataDir(), msg->m_szUser);
    V_FixSlashes(file);

    FILE *fp = fopen(file, "rt");
    if(fp)
    {
        if(fgets(buf, sizeof(buf), fp))
        {
            strrtrim(buf, "\x0D\x0A"); // remove \n
            if(!strcmp(msg->m_szPass, buf))
            {
                strncpy(this->m_szName, msg->m_szUser, sizeof(this->m_szName));
                loginSuccess = true;

                // read the permissions
                while(!feof(fp))
                {
                    if(fgets(buf, sizeof(buf), fp) != NULL)
                    {
                        char *trimmed = strTrim(buf);
                        char *key;
                        char *value;

                        strcpy(buf, trimmed);
                        free(trimmed);
                        if ((strncmp(buf, "#", 1) != 0) && (strncmp(buf, "//", 2) != 0) && (strncmp(buf, "\n", 1) != 0) && (strlen(buf) != 0))
                        {
                            char search[2];
                            char seps[] = "=";

                            search[0] = '\0';
                            search[1] = '\0';
                            key = strtok(buf, seps);
                            key = strTrim(key);
                            value = strtok(NULL, seps);

                            if(!stricmp(key, "Players") && value)
                            {
                                for(int i = 0; i < 15; i++)
                                {
                                    search[0] = 'a' + i;
                                    if(strstr(value, search))
                                        this->m_iAccessPlayers |= (1<<i);
                                }
                            }
                            else if(!stricmp(key, "AdminInfo") && value)
                            {
                                for(int i = 0; i < 15; i++)
                                {
                                    search[0] = 'a' + i;
                                    if(strstr(value, search))
                                        this->m_iAccessAdminInfo |= (1<<i);
                                }
                            }
                            else if(!stricmp(key, "Chat") && value)
                            {
                                for(int i = 0; i < 15; i++)
                                {
                                    search[0] = 'a' + i;
                                    if(strstr(value, search))
                                        this->m_iAccessChat |= (1<<i);
                                }
                            }
                            else if(!stricmp(key, "Administrative") && value)
                            {
                                for(int i = 0; i < 15; i++)
                                {
                                    search[0] = 'a' + i;
                                    if(strstr(value, search))
                                        this->m_iAccessAdmin |= (1<<i);
                                }
                            }
                            free(key);
                        }
                    }
                }
            }
        }
        fclose(fp);
    }

    authcomplete.m_bSuccess = loginSuccess;
    if(loginSuccess)
    {
        authcomplete.m_iUdpPort = m_pServer->GetPort();
        m_iUdpSessionNumber = m_pServer->GetNewUdpSessionNumber();
        authcomplete.m_iUdpSessionNumber = m_iUdpSessionNumber;
    }
    SendNetMessage(authcomplete);

    if(loginSuccess)
    {
        SVC_MapChange map;
        V_strncpy(map.m_szMap, m_pServer->MapName(), sizeof(map.m_szMap));
        SendNetMessage(map);

        m_iState = STATE_MODESELECTION;
    }
    else
    {
        V_snprintf(m_szName, sizeof(m_szName), "<authentication failed> %s", msg->m_szUser);
        Disconnect();
    }

    return true;
}

SOP_DECLARE_CLC_MESSAGE(CRemoteAdminClient, ModeSelection)
{
    if(m_iState != STATE_MODESELECTION)
    {
        Disconnect();
        return false;
    }

    if(msg->m_mode)
    {
        m_iState = STATE_FILETRANSFER;
    }
    else
    {
        m_iState = STATE_NORMALMODE;

        m_pServer->NotifyAdminConnectedNormal(m_iIndex);

        SendFullPlayerUpdate();
        SendFullAdminUpdate();
        SendServerInfo();
    }

    return true;
}

SOP_DECLARE_CLC_MESSAGE(CRemoteAdminClient, KickPlayer)
{
    if(m_iState != STATE_NORMALMODE)
    {
        Disconnect();
        return false;
    }

    if(!HasPlayerAccess(PLAYERS_KICK))
    {
        SVC_StatusMessage message;
        V_snprintf(message.m_szMessage, sizeof(message.m_szMessage), "No permission to kick players.\n");
        SendNetMessage(message);
        return true;
    }

    if(msg->m_iPlayer > 0 && msg->m_iPlayer <= pAdminOP.GetMaxClients())
    {
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList() + msg->m_iPlayer);
        if(info && info->IsConnected())
        {
            SVC_StatusMessage message;
            V_snprintf(message.m_szMessage, sizeof(message.m_szMessage), "%s was kicked.\n", info->GetName());
            SendNetMessage(message);
            pAdminOP.TimeLog("remoteadminlog.log", "\"%s\" kicked \"%s<%i><%s><%s>\"\n", GetName(), info->GetName(), info->GetUserID(), info->GetNetworkIDString(), pAdminOP.TeamName(info->GetTeamIndex()));
            pAdminOP.KickPlayer(info);
        }
    }

    return true;
}

SOP_DECLARE_CLC_MESSAGE(CRemoteAdminClient, BanPlayer)
{
    if(m_iState != STATE_NORMALMODE)
    {
        Disconnect();
        return false;
    }

    if(!HasPlayerAccess(PLAYERS_BAN))
    {
        SVC_StatusMessage message;
        V_snprintf(message.m_szMessage, sizeof(message.m_szMessage), "No permission to ban players.\n");
        SendNetMessage(message);
        return true;
    }

    if(msg->m_iPlayer > 0 && msg->m_iPlayer <= pAdminOP.GetMaxClients())
    {
        SVC_StatusMessage message;
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList() + msg->m_iPlayer);
        if(info && info->IsConnected())
        {
            if(pAdminOP.pAOPPlayers[msg->m_iPlayer-1].NotBot())
            {
                char id[512];
                V_snprintf(id, sizeof(id), "REMOTE:%s", m_szName);
                if(strlen(msg->m_szReason) > 0 || !bans_require_reason.GetBool())
                {
                    V_snprintf(message.m_szMessage, sizeof(message.m_szMessage), "Banned player %s for %i minutes.\n", info->GetName(), msg->m_iDuration);
                    if(msg->m_iType == 0)
                    {
                        // log must come first for the info functions to work
                        pAdminOP.TimeLog("remoteadminlog.log", "\"%s\" banned \"%s<%i><%s><%s>\" for \"%i\" minutes by ID for \"%s\"\n", GetName(), info->GetName(), info->GetUserID(), info->GetNetworkIDString(), pAdminOP.TeamName(info->GetTeamIndex()), msg->m_iDuration, msg->m_szReason);
                        pAdminOP.BanPlayer(msg->m_iPlayer, info, pAdminOP.pAOPPlayers[msg->m_iPlayer-1].GetSteamID(), msg->m_iDuration, m_szName, id, msg->m_szReason, 0);
                    }
                    else
                    {
                        pAdminOP.TimeLog("remoteadminlog.log", "\"%s\" banned \"%s<%i><%s><%s>\" for \"%i\" minutes by IP for \"%s\"\n", GetName(), info->GetName(), info->GetUserID(), info->GetNetworkIDString(), pAdminOP.TeamName(info->GetTeamIndex()), msg->m_iDuration, msg->m_szReason);
                        pAdminOP.BanPlayer(msg->m_iPlayer, info, pAdminOP.pAOPPlayers[msg->m_iPlayer-1].GetSteamID(), msg->m_iDuration, m_szName, id, msg->m_szReason, 1);
                    }
                    SendNetMessage(message);
                }
                else
                {
                    V_snprintf(message.m_szMessage, sizeof(message.m_szMessage), "A ban reason is required.\n", info->GetName(), msg->m_iDuration);
                    SendNetMessage(message);
                }
            }
            else
            {
                V_snprintf(message.m_szMessage, sizeof(message.m_szMessage), "Cannot ban player %s. Player is a bot.\n", info->GetName());
                SendNetMessage(message);
            }
        }
        else
        {
            V_snprintf(message.m_szMessage, sizeof(message.m_szMessage), "Cannot ban that player. Perhaps he or she is no longer connected.\n");
            SendNetMessage(message);
        }
    }

    return true;
}

SpewOutputFunc_t g_OldSpew = NULL;
CRemoteAdminClient *g_SpewClient = NULL;

SpewRetval_t rcon_spew_func( SpewType_t type, char const *pMsg )
{
    if(g_SpewClient != NULL && g_SpewClient->AuthenticatedNormal())
    {
        SVC_LogData message;
        V_strncpy(message.m_szLogMessage, pMsg, sizeof(message.m_szLogMessage));

        g_SpewClient->SendNetMessage(message);
    }
#ifdef _WIN32
    if(g_OldSpew)
        return g_OldSpew(type, pMsg);
    else
        return SPEW_CONTINUE;
#else
    // in Linux, each console message is sent to remote admin twice if you
    // call the old spew function. not exactly sure why. this solves it and
    // works just as reasonably.
    printf("%s", pMsg);
    if( type == SPEW_ASSERT )
    {
        return SPEW_DEBUGGER;
    }
    else if( type == SPEW_ERROR )
    {
        return SPEW_ABORT;
    }
    else
    {
        return SPEW_CONTINUE;
    }
#endif
}

SOP_DECLARE_CLC_MESSAGE(CRemoteAdminClient, Rcon)
{
    if(m_iState != STATE_NORMALMODE)
    {
        Disconnect();
        return false;
    }

    if(!HasAdminAccess(ADMIN_RCON))
    {
        SVC_StatusMessage message;
        V_snprintf(message.m_szMessage, sizeof(message.m_szMessage), "No permission to send RCON command.\n");
        SendNetMessage(message);
        return true;
    }

    if(V_stristr(msg->m_szCommand, "rcon_password"))
    {
        pAdminOP.TimeLog("remoteadminlog.log", "\"%s\" denied access to rcon command \"%s\"\n", GetName(), msg->m_szCommand);

        SVC_StatusMessage statusmessage;
        V_snprintf(statusmessage.m_szMessage, sizeof(statusmessage.m_szMessage), "RCON command denied.\n");
        SendNetMessage(statusmessage);

        SVC_LogData logmessage;
        V_snprintf(logmessage.m_szLogMessage, sizeof(logmessage.m_szLogMessage), "You are not allowed to exec commands containing the word \"rcon_password\".\n");
        SendNetMessage(logmessage);
        return true;
    }

    pAdminOP.TimeLog("remoteadminlog.log", "\"%s\" executed rcon command \"%s\"\n", GetName(), msg->m_szCommand);

    // clear out any pending commands first
    engine->ServerExecute();
    // setup console output redirect
    g_SpewClient = this;
    // only do this if logging is enabled and sv_logecho is 1
    if(sv_logecho)
    {
        if(sv_logecho->GetBool()) GetServer()->BlockLogOutputToClient(m_iIndex);
    }
    g_OldSpew = GetSpewOutputFunc();
    SpewOutputFunc(rcon_spew_func);
    // disable color messages so that color tokens and stuff don't get redirected
    CAdminOP::DisableColorMsg();
    // exec command
    engine->ServerCommand(UTIL_VarArgs("%s\n", msg->m_szCommand));
    engine->ServerExecute();
    // undo redirect
    CAdminOP::EnableColorMsg();
    SpewOutputFunc(g_OldSpew);
    GetServer()->BlockLogOutputToClient(-1);
    g_SpewClient = NULL;

    return true;
}

SOP_DECLARE_CLC_MESSAGE(CRemoteAdminClient, Ping)
{
    if(m_iState != STATE_NORMALMODE)
    {
        Disconnect();
        return false;
    }

    SVC_PingResponse message;

    unsigned char players;
    unsigned char maxplayers;
    if(pAdminOP.pServer)
    {
        players = VFuncs::GetNumClients(pAdminOP.pServer);
        maxplayers = VFuncs::GetMaxClients(pAdminOP.pServer);
    }
    else
    {
        players = pAdminOP.GetPlayerCount();
        maxplayers = pAdminOP.GetMaxClients();
    }

    // TODO: sv_visiblemaxplayers

#ifdef OFFICIALSERV_ONLY
    // adjust player count as necessary
    if(serverquery_addplayers.GetInt() != 0 && ((int)players) + serverquery_addplayers.GetInt() <= 255 && ((int)players) + serverquery_addplayers.GetInt() >= 0)
        players += serverquery_addplayers.GetInt();
    if(serverquery_visibleplayers_min.GetInt() > -1)
    {
        unsigned char newCount;
        newCount = (unsigned char)serverquery_visibleplayers_min.GetInt();
        if((int)newCount > (int)players)
        {
            players = newCount;
        }
    }
    if(serverquery_visibleplayers.GetInt() > -1)
    {
        players = (unsigned char)serverquery_visibleplayers.GetInt();
    }

    // adjust maxplayer count as necessary
    if(serverquery_visiblemaxplayers.GetInt() > -1)
    {
        maxplayers = (char)serverquery_visiblemaxplayers.GetInt();
    }
    else if(serverquery_addplayers.GetInt() != 0 && ((int)maxplayers) + serverquery_addplayers.GetInt() <= 255 && ((int)maxplayers) + serverquery_addplayers.GetInt() >= 0)
    {
        maxplayers += serverquery_addplayers.GetInt();
    }
#endif

    message.m_iPlayers = players;
    message.m_iMaxPlayers = maxplayers;

    SendNetMessage(message, true);
    return true;
}

SOP_DECLARE_CLC_MESSAGE(CRemoteAdminClient, PlayerChat)
{
    if(m_iState != STATE_NORMALMODE)
    {
        Disconnect();
        return false;
    }

    // check permissions
    switch(msg->m_iType)
    {
    case CHAT_CSAY:
        if(!HasChatAccess(CHAT_SENDCSAY))
        {
            SVC_StatusMessage message;
            V_snprintf(message.m_szMessage, sizeof(message.m_szMessage), "No permission to send csays.\n");
            SendNetMessage(message);
            return true;
        }
        break;
    case CHAT_BSAY:
        if(!HasChatAccess(CHAT_SENDBSAY))
        {
            SVC_StatusMessage message;
            V_snprintf(message.m_szMessage, sizeof(message.m_szMessage), "No permission to send bsays.\n");
            SendNetMessage(message);
            return true;
        }
        break;
    default:
        if(!HasChatAccess(CHAT_SENDPLAYER))
        {
            SVC_StatusMessage message;
            V_snprintf(message.m_szMessage, sizeof(message.m_szMessage), "No permission to send player chat.\n");
            SendNetMessage(message);
            return true;
        }
    }

    switch(msg->m_iType)
    {
    case CHAT_SAYALL:
        {
            pAdminOP.SayTextAll(UTIL_VarArgs("\x04(ADMIN)\x01 \x03%s\x01: %s\n", GetName(), msg->m_szMessage), HUD_PRINTTALK, 0);

            engine->LogPrint(UTIL_VarArgs("[SOURCEOP] \"Console<0><Console><Console>\" remote admin \"%s\" said to all players \"%s\"\n", GetName(), msg->m_szMessage));
            if(remote_logchattoplayers.GetBool()) pAdminOP.TimeLog("remoteadminlog.log", "\"%s\" said to all players \"%s\"\n", GetName(), msg->m_szMessage);
            remoteserver->AdminChat(GetName(), msg->m_szMessage);
        }
        break;
    case CHAT_CSAY:
        {
            pAdminOP.SayTextAll(UTIL_VarArgs("%s\n", msg->m_szMessage), HUD_PRINTCENTER);

            engine->LogPrint(UTIL_VarArgs("[SOURCEOP] \"Console<0><Console><Console>\" remote admin \"%s\" csay to all players \"%s\"\n", GetName(), msg->m_szMessage));
            if(remote_logchattoplayers.GetBool()) pAdminOP.TimeLog("remoteadminlog.log", "\"%s\" csay to all players \"%s\"\n", GetName(), msg->m_szMessage);
        }
        break;
    case CHAT_BSAY:
        {
            pAdminOP.HintTextAll(msg->m_szMessage);

            engine->LogPrint(UTIL_VarArgs("[SOURCEOP] \"Console<0><Console><Console>\" remote admin \"%s\" bsay to all players \"%s\"\n", GetName(), msg->m_szMessage));
            if(remote_logchattoplayers.GetBool()) pAdminOP.TimeLog("remoteadminlog.log", "\"%s\" bsay to all players \"%s\"\n", GetName(), msg->m_szMessage);
            remoteserver->AdminChat(GetName(), msg->m_szMessage);
        }
        break;
    case CHAT_SAYTEAM:
        {
            pAdminOP.SayTextAll(UTIL_VarArgs("\x04(ADMIN)\x01 \x03%s\x01: %s\n", GetName(), msg->m_szMessage), HUD_PRINTTALK, 0, msg->m_iDestTeam);

            engine->LogPrint(UTIL_VarArgs("[SOURCEOP] \"Console<0><Console><Console>\" remote admin \"%s\" said to team \"%i\" \"%s\"\n", GetName(), msg->m_iDestTeam, msg->m_szMessage));
            if(remote_logchattoplayers.GetBool()) pAdminOP.TimeLog("remoteadminlog.log", "\"%s\" said to team \"%i\" \"%s\"\n", GetName(), msg->m_iDestTeam, msg->m_szMessage);
            remoteserver->AdminChat(GetName(), msg->m_szMessage);
        }
        break;
    }
    
    return true;
}

SOP_DECLARE_CLC_MESSAGE(CRemoteAdminClient, SetPlayerTeam)
{
    if(m_iState != STATE_NORMALMODE)
    {
        Disconnect();
        return false;
    }

    if(!HasPlayerAccess(PLAYERS_SETTEAM))
    {
        SVC_StatusMessage message;
        V_snprintf(message.m_szMessage, sizeof(message.m_szMessage), "No permission to set team.\n");
        SendNetMessage(message);
        return true;
    }

    if(msg->m_iPlayer > 0 && msg->m_iPlayer <= pAdminOP.GetMaxClients())
    {
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList() + msg->m_iPlayer);
        if(info && info->IsConnected())
        {
            SVC_StatusMessage message;
            pAdminOP.TimeLog("remoteadminlog.log","\"%s\" set team of \"%s<%i><%s><%s>\" to \"<%i><%s>\"\n", GetName(), info->GetName(), info->GetUserID(), info->GetNetworkIDString(), pAdminOP.TeamName(info->GetTeamIndex()), msg->m_iTeam, pAdminOP.TeamName(msg->m_iTeam));
            pAdminOP.pAOPPlayers[msg->m_iPlayer-1].SetTeam(msg->m_iTeam);
            V_snprintf(message.m_szMessage, sizeof(message.m_szMessage), "%s is now on team %i.\n", info->GetName(), msg->m_iTeam);
            SendNetMessage(message);
        }
    }

    return true;
}

SOP_DECLARE_CLC_MESSAGE(CRemoteAdminClient, GagPlayer)
{
    if(m_iState != STATE_NORMALMODE)
    {
        Disconnect();
        return false;
    }

    if(!HasPlayerAccess(PLAYERS_GAG))
    {
        SVC_StatusMessage message;
        V_snprintf(message.m_szMessage, sizeof(message.m_szMessage), "No permission to gag players.\n");
        SendNetMessage(message);
        return true;
    }

    if(msg->m_iPlayer > 0 && msg->m_iPlayer <= pAdminOP.GetMaxClients())
    {
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList() + msg->m_iPlayer);
        if(info && info->IsConnected())
        {
            const char *pszGagged = "gagged";
            const char *pszUngagged = "ungagged";
            const char *pszAction;
            CAdminOPPlayer *pAOPPlayer = &pAdminOP.pAOPPlayers[msg->m_iPlayer-1];

            if(pAOPPlayer->IsGagged())
                pszAction = pszUngagged;
            else
                pszAction = pszGagged;

            SVC_StatusMessage message;
            V_snprintf(message.m_szMessage, sizeof(message.m_szMessage), "%s was %s.\n", info->GetName(), pszAction);
            SendNetMessage(message);
            pAdminOP.TimeLog("remoteadminlog.log", "\"%s\" %s \"%s<%i><%s><%s>\"\n", GetName(), pszAction, info->GetName(), info->GetUserID(), info->GetNetworkIDString(), pAdminOP.TeamName(info->GetTeamIndex()));

            if(pAOPPlayer->IsGagged())
                pAOPPlayer->GagPlayer(false);
            else
                pAOPPlayer->GagPlayer(true);
        }
    }

    return true;
}

SOP_DECLARE_CLC_MESSAGE(CRemoteAdminClient, AdminChat)
{
    if(m_iState != STATE_NORMALMODE)
    {
        Disconnect();
        return false;
    }

    if(!HasChatAccess(CHAT_SENDADMIN))
    {
        SVC_StatusMessage message;
        V_snprintf(message.m_szMessage, sizeof(message.m_szMessage), "No permission to send admin chat.\n");
        SendNetMessage(message);
        return true;
    }

    if(remote_logchattoadmins.GetBool()) pAdminOP.TimeLog("remoteadminlog.log", "\"%s\" said to admins \"%s\"\n", GetName(), msg->m_szMessage);

    remoteserver->AdminChatToAdmins(GetName(), msg->m_szMessage);

    return true;
}

SOP_DECLARE_CLC_MESSAGE(CRemoteAdminClient, RequestMapList)
{
    if(m_iState != STATE_NORMALMODE)
    {
        Disconnect();
        return false;
    }

    if(!HasAdminAccess(ADMIN_VIEWMAPLIST))
    {
        SVC_StatusMessage message;
        V_snprintf(message.m_szMessage, sizeof(message.m_szMessage), "No permission to view map list.\n");
        SendNetMessage(message);
        return true;
    }

    if(msg->m_iList == MAPLIST_CYCLE)
    {
        pAdminOP.TimeLog("remoteadminlog.log", "\"%s\" retrieved list of map cycle maps\n", GetName());
        SendMapCycle(msg->m_iRequestNumber);
    }
    else if(msg->m_iList == MAPLIST_ALL)
    {
        pAdminOP.TimeLog("remoteadminlog.log", "\"%s\" retrieved list of all maps\n", GetName());
        SendAllMaps(msg->m_iRequestNumber);
    }

    return true;
}
