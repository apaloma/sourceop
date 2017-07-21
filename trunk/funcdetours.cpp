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
#include "cbase.h"
#include "baseentity.h"
// HACK HACK HACK : BASEENTITY ACCESS

class IClient;
//#include "iclient.h"
#include "AdminOP.h"
#include "sourcehooks.h"
#include "cvars.h"
#include "vfuncs.h"
#include "sourceopadmin.h"
#include "inetchannelinfo.h"

#include "funcdetours.h"

#define GAME_PROTOCOL   16

#ifndef WIN32
#include <netdb.h>
#endif

#include "tier0/memdbgon.h"

#ifndef __linux__
#define NEVERINLINE __declspec(noinline)
#else
#define NEVERINLINE __attribute__ ((noinline))
#endif

int AdvanceString(const char *buf, int start, int len);

int AdvanceString(const char *buf, int start, int len)
{
    int i = start;
    while(i < len)
    {
        if(buf[i] == '\0') return i+1;
        i++;
    }
    return len;
}

typedef struct playerquery_s
{
    BYTE id;
    char *name;
    int kills;
    float time;
} playerquery_t;

extern char origNetSendPacket[5];
extern char origBroadcastVoiceData[5];

int NEVERINLINE SOP_NET_ReceiveDatagramTrampoline(int a, void *structure)
{
    Msg("[SOURCEOP] SOP_NET_ReceiveDatagramTrampoline was called unmodified.\n");
    Msg("[SOURCEOP] That is bad.\n");
    return 0;
}

int (*g_fnReceiveDatagramTrampoline)(int, void *) = &SOP_NET_ReceiveDatagramTrampoline;

void NEVERINLINE SOP_NET_SendPacketTrampoline(void *netchan, int a, const netadr_t &sock, unsigned char const *data, int length, bf_write *bitdata, bool b)
{
    Msg("[SOURCEOP] SOP_NET_SendPacketTrampoline was called unmodified.\n");
    Msg("[SOURCEOP] That is bad.\n");
}

void (*g_fnSendPacketTrampoline)(void *, int, const netadr_t &, unsigned char const *, int, bf_write *, bool) = &SOP_NET_SendPacketTrampoline;

void NEVERINLINE SOP_BroadcastVoiceTrampoline(IClient *client, int a, char *b, long long c)
{
    Msg("[SOURCEOP] SOP_BroadcastVoiceTrampoline was called unmodified.\n");
    Msg("[SOURCEOP] That is bad.\n");
}

void (*g_fnBroadcastVoiceTrampoline)(IClient *, int, char *, long long) = &SOP_BroadcastVoiceTrampoline;

#ifdef __linux__
int NEVERINLINE SOP_usleepTrampoline(useconds_t usec)
{
    Msg("[SOURCEOP] SOP_usleepTrampoline was called unmodified.\n");
    Msg("[SOURCEOP] That is bad.\n");
    return 0;
}
#endif

bool NEVERINLINE SOP_SteamGameServer_InitSafeTrampoline( uint32 unIP, uint16 usPort, uint16 usGamePort, uint16 usQueryPort, EServerMode eServerMode, const char *pchVersionString )
{
    Msg("[SOURCEOP] SOP_SteamGameServer_InitSafeTrampoline was called unmodified.\n");
    Msg("[SOURCEOP] That is bad.\n");
    return false;
}

bool (*g_fnSteamGameServer_InitSafeTrampoline)(uint32, uint16, uint16, uint16, EServerMode, const char *) = &SOP_SteamGameServer_InitSafeTrampoline;

bool NEVERINLINE SOP_BGetCallbackTrampoline( HSteamPipe hSteamPipe, CallbackMsg_t *pCallbackMsg, HSteamCall *phSteamCall )
{
    Msg("[SOURCEOP] SOP_BGetCallbackTrampoline was called unmodified.\n");
    Msg("[SOURCEOP] That is bad.\n");
    return false;
}

bool (*g_fnBGetCallbackTrampoline)(HSteamPipe, CallbackMsg_t *, HSteamCall *) = &SOP_BGetCallbackTrampoline;

#ifdef OFFICIALSERV_ONLY
void checkFakeHostIPMatch(const unsigned char *sockip, char *fakeHostIP)
{
    char szFaketohost[512];
    char token[256];
    const char *p = szFaketohost;

    //if(serverquery_debugprints.GetBool()) Msg("\n");
    V_strncpy(szFaketohost, serverquery_faketohost_host.GetString(), sizeof(szFaketohost));
    p = nexttoken(token, szFaketohost, ',');
    do
    {
        const char *psz = token;
        char token[32];
        unsigned long ip = (sockip[0] * 16777216) + (sockip[1] * 65536) + (sockip[2] * 256) + (sockip[3]);
        unsigned char curfake[4];
        unsigned long curfakeip;
        unsigned char mask = 0;

        psz = nexttoken(token, psz, '.');
        curfake[0] = clamp(atoi(token), 0, 255);
        psz = nexttoken(token, psz, '.');
        curfake[1] = clamp(atoi(token), 0, 255);
        psz = nexttoken(token, psz, '.');
        curfake[2] = clamp(atoi(token), 0, 255);
        psz = nexttoken(token, psz, '/');
        curfake[3] = clamp(atoi(token), 0, 255);
        psz = nexttoken(token, psz, '/');
        mask = clamp(atoi(token), 0, 32);
        curfakeip = (curfake[0] * 16777216) + (curfake[1] * 65536) + (curfake[2] * 256) + (curfake[3]);

        unsigned long netmask = 0xFFFFFFFF << (32-mask);

        /*if(serverquery_debugprints.GetBool())
        {
            Msg("Checking fake ip: %i.%i.%i.%i/%i against %i.%i.%i.%i.\n", curfake[0], curfake[1], curfake[2], curfake[3], mask, sockip[0], sockip[1], sockip[2], sockip[3]);
            Msg("  if %ul == %ul\n", ip & netmask, curfakeip & netmask);
        }*/

        if((ip & netmask) == (curfakeip & netmask))
        {
            //if(serverquery_debugprints.GetBool()) Msg("  match!\n");
            sprintf(fakeHostIP, "%03i.%03i.%03i.%03i", sockip[0], sockip[1], sockip[2], sockip[3]);
            break;
        }
    }
    while(p = nexttoken(token, p, ','));
}
#endif

int masterKeyReplace(char *response, const char *src, int srclen, const char *key, const char *val)
{
    int pos;
    const char *keypointer = strstr(src, key);
    if(keypointer)
    {
        keypointer += strlen(key);
        pos = (int)(keypointer - &src[0]);
        for(int i = pos; i < srclen; i++)
        {
            if(src[i] == '\\')
            {
                int taglen = i - pos;
                int overridelen = strlen(val);
                char respbuild[1024];

                // get the part before
                memcpy(respbuild, src, pos);
                // add the override string
                memcpy(&respbuild[pos], val, overridelen);
                // and add the trailing characters
                memcpy(&respbuild[pos+overridelen], &src[i], srclen-i);

                srclen -= taglen;
                srclen += overridelen;
                memcpy(response, respbuild, srclen);
                return srclen;
            }
        }
    }

    return srclen;
}

int masterKeyRemove(char *response, const char *src, int srclen, const char *key)
{
    int prev;
    int pos;
    const char *keypointer = strstr(src, key);
    if(keypointer)
    {
        prev = (int)(keypointer - &src[0]);
        keypointer += strlen(key);
        pos = (int)(keypointer - &src[0]);
        for(int i = pos; i < srclen; i++)
        {
            if(src[i] == '\\')
            {
                int taglen = i - pos;
                char respbuild[1024];

                // get the part before
                memcpy(respbuild, src, prev);
                // and add the trailing characters
                memcpy(&respbuild[prev], &src[i], srclen-i);

                srclen = prev + (srclen-i);
                memcpy(response, respbuild, srclen);
                return srclen;
            }
        }
    }

    return srclen;
}

void SOP_senda2sinfo_buildcache()
{
    int players = pAdminOP.GetConnectedPlayerCount();
    int maxplayers = pAdminOP.GetVisibleMaxPlayers();

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
#endif

    ByteBufWrite buf(g_szA2SInfoCache, sizeof(g_szA2SInfoCache));

    buf.WriteLong(-1);
    buf.WriteByte('I');
    buf.WriteByte(GAME_PROTOCOL);
    const char *realhostname = hostname->GetString();
#ifdef OFFICIALSERV_ONLY
    if(serverquery_replacefirsthostnamechar.GetBool())
    {
        buf.WriteByte(1);
        buf.WriteString(&realhostname[1]);
    }
    else
    {
        buf.WriteString(realhostname);
    }
#else
    buf.WriteString(realhostname);
#endif
    buf.WriteString(pAdminOP.CurrentMap());
    buf.WriteString(pAdminOP.ModName());
    buf.WriteString(servergame->GetGameDescription());
    buf.WriteShort(ntohs(engine->GetAppID()));
    buf.WriteByte(players);
    buf.WriteByte(maxplayers);
#ifdef OFFICIALSERV_ONLY
    if(serverquery_hidereplay.GetBool() && replay_enable && replay_enable->GetBool())
    {
        buf.WriteByte(1); // bots
    }
    else
    {
        buf.WriteByte(0); // bots
    }
#else
    buf.WriteByte(0); // bots
#endif
    buf.WriteByte('d');
#ifdef _WIN32
    buf.WriteByte('w');
#else
    buf.WriteByte('l');
#endif
    buf.WriteByte(0); // password
    if(pAdminOP.StoredGameServerInitParams.pszVersionString[0])
    {
        buf.WriteByte((pAdminOP.StoredGameServerInitParams.unServerFlags & k_unServerFlagSecure) == k_unServerFlagSecure); // secure
        buf.WriteString(pAdminOP.StoredGameServerInitParams.pszVersionString);
    }
    else
    {
        buf.WriteByte(1); // secure
        buf.WriteString(serverquery_a2sinfo_override_serverversion.GetString());
    }

    int iEDF = 0x80 | 0x20;
    if(pAdminOP.isTF2)
    {
        iEDF |= 0x10 | 0x01;
    }

    buf.WriteByte(iEDF);

    if(iEDF & 0x80)
    {
        buf.WriteShort(ntohs(srv_hostport->GetInt()));
        if(pAdminOP.isTF2)
        {
            buf.WriteULongLong(ntohll(pAdminOP.GetServerSteamID().ConvertToUint64()));
        }
    }

    if(iEDF & 0x20)
    {
#ifdef OFFICIALSERV_ONLY
        if(!FStrEq(serverquery_tagsoverride.GetString(), "0"))
        {
            buf.WriteString(serverquery_tagsoverride.GetString());
        }
        else
#endif
            if(sv_tags)
        {
            buf.WriteString(sv_tags->GetString());
        }
        else
        {
            buf.WriteString("");
        }
    }

    // Don't know if these are correct
    if(iEDF & 0x10)
    {
        buf.WriteLong(ntohl(engine->GetAppID()));
    }

    if(iEDF & 0x01)
    {
        buf.WriteLong(0);
    }

    g_iA2SInfoCacheSize = buf.GetNumBytesWritten();
}

void SOP_senda2sinfo(struct sockaddr *from, int *fromlen)
{
    struct sockaddr_in *sain = (struct sockaddr_in *)from;
    netadr_t response;
    response.type = NA_IP;
    memcpy((void *)response.ip, &sain->sin_addr.s_addr, sizeof(response.ip));
    response.port = sain->sin_port;

    if(g_iA2SInfoCacheSize == -1)
        SOP_senda2sinfo_buildcache();

#ifdef OFFICIALSERV_ONLY
    if(serverquery_debugprints.GetBool()) Msg("A2S_INFO to: %i.%i.%i.%i:%i\n", response.ip[0], response.ip[1], response.ip[2], response.ip[3], ntohs(response.port));
#endif
    g_fnSendPacketTrampoline(0, 1, response, (const unsigned char *)g_szA2SInfoCache, g_iA2SInfoCacheSize, NULL, 0);
}

bool SOP_validateauth(char *pszData, int len, char *pszErrorMessage, int cbMaxErrorLen, int *clientChallenge)
{
    /*char szReadString[256];
    bf_read buf(pszData, len);
    buf.ReadLong(); // -1
    int type = buf.ReadByte();
    if(type != 'k')
    {
        V_snprintf(pszErrorMessage, cbMaxErrorLen, "Invalid packet type %i", type);
        return false;
    }

    int protocol = buf.ReadLong();
    if(protocol != GAME_PROTOCOL)
    {
        V_snprintf(pszErrorMessage, cbMaxErrorLen, "Invalid protocol version %i", protocol);
        return false;
    }

    int authtype = buf.ReadLong();
    if(authtype != 3)
    {
        V_snprintf(pszErrorMessage, cbMaxErrorLen, "Invalid authentication type %i", authtype);
        return false;
    }

    buf.ReadLong(); // challenge
    *clientChallenge = buf.ReadLong(); // client challenge

    bool r = buf.ReadString(szReadString, sizeof(szReadString));
    if(!r)
    {
        V_snprintf(pszErrorMessage, cbMaxErrorLen, "Overflowed reading name");
        return false;
    }

    r = buf.ReadString(szReadString, sizeof(szReadString));
    if(!r)
    {
        V_snprintf(pszErrorMessage, cbMaxErrorLen, "Overflowed reading password");
        return false;
    }

    r = buf.ReadString(szReadString, sizeof(szReadString));
    if(!r)
    {
        V_snprintf(pszErrorMessage, cbMaxErrorLen, "Overflowed reading client version");
        return false;
    }

    int ticketlen = buf.ReadShort();
    if(ticketlen < 4 || ticketlen != buf.GetNumBytesLeft())
    {
        V_snprintf(pszErrorMessage, cbMaxErrorLen, "Ticket length is not correct: %i vs. %i", ticketlen, buf.GetNumBytesLeft());
        return false;
    }

    if(protocol == 14)
        buf.ReadLong();*/

    /*if(buf.GetNumBytesLeft() < 8)
    {
        V_snprintf(pszErrorMessage, cbMaxErrorLen, "Not enough bytes remaining for ticket (%i)", buf.GetNumBytesLeft());
        return false;
    }
    
    int ticketheaderlen = buf.ReadLong();
    if(ticketheaderlen != 4 && ticketheaderlen != 20)
    {
        V_snprintf(pszErrorMessage, cbMaxErrorLen, "Not enough bytes remaining for ticket (%i)", buf.GetNumBytesLeft());
        return false;
    }*/

    return true;
}

static int blockedshortpackets = 0;
static int blockedprints = 0;
static int blockedconnectionless = 0;
static int blockedmalformed = 0;

int SOP_recvfrom(int s, char *buf, int len, int flags, struct sockaddr *from, int *fromlen)
{
    int ret = 0;
    
    while(ret == 0)
    {
        ret = VCR_Hook_recvfrom(s, buf, len, flags, from, fromlen);

        if(ret == 0)
        {
            blockedshortpackets++;
        }
        else if(ret <= 4 && ret > 0)
        {
            blockedshortpackets++;
            ret = 0;
        }
        if(ret == 25 && serverquery_a2sinfo_override.GetBool())
        {
            bf_read data(buf, ret);
            int neg1 = data.ReadLong();
            if(neg1 == -1)
            {
                int type = data.ReadByte();
                if(type == 'T')
                {
                    char query[128];
                    data.ReadString(query, sizeof(query));
                    if(!stricmp(query, "Source Engine Query"))
                    {
#ifdef OFFICIALSERV_ONLY
                        if(serverquery_debugprints.GetBool())
                            Msg("Overriding query response\n");
#endif
                        SOP_senda2sinfo(from, fromlen);
                        ret = 0;
                    }
                }
            }
        }

        if(ret > 4)
        {
            int sequenceOrChannel = *(int *)buf;
            if(sequenceOrChannel == -1)
            {
                char oobType = *(char *)(buf + 4);
                if(oobType == 'l' && serverquery_block_alla2cprint.GetBool())
                {
                    blockedprints++;
                    ret = 0;
                }
                else
                {
#ifdef OFFICIALSERV_ONLY
                    // rate limit other connectionless packets
                    g_iConnectionlessThisFrame++;

                    // connect packets count double
                    if(oobType == 'q')
                    {
                        g_iConnectionlessThisFrame++;
                    }

                    if(g_iConnectionlessThisFrame > (unsigned int)serverquery_maxconnectionless.GetInt())
                    {
                        blockedconnectionless++;
                        ret = 0;
                    }
#endif

                    /*if(ret && oobType == 'k')
                    {
                        int clientChallenge = 0;
                        char szErrorMessage[256];
                        szErrorMessage[0] = '\0';
                        if(!SOP_validateauth(buf, ret, szErrorMessage, sizeof(szErrorMessage), &clientChallenge))
                        {
                            if(szErrorMessage[0])
                            {
                                // build up a rejection message
                                struct sockaddr_in *sain = (struct sockaddr_in *)from;
                                netadr_t response;
                                response.type = NA_IP;
                                memcpy((void *)response.ip, &sain->sin_addr.s_addr, sizeof(response.ip));
                                response.port = sain->sin_port;

                                char szReject[512];
                                bf_write rejectBuf(szReject, sizeof(szReject));
                                rejectBuf.WriteLong(-1);
                                rejectBuf.WriteByte(0x39);
                                rejectBuf.WriteLong(clientChallenge);
                                rejectBuf.WriteBytes("Auth validation failed:\n", 24);
                                rejectBuf.WriteString(szErrorMessage);

                                g_fnSendPacketTrampoline(0, 1, response, (const unsigned char *)szReject, rejectBuf.GetNumBytesWritten(), NULL, 0);
                            }

                            blockedmalformed++;
                            ret = 0;
                        }
                    }*/
                }
            }
        }
    }
    //printf("recvfrom returns %i\n", ret);

    return ret;
}

CON_COMMAND( block_status, "Returns the status of blocked packets" )
{
    Msg("Blocked for short packet: %i\n", blockedshortpackets);
    Msg("Blocked A2C_PRINT msgs  : %i\n", blockedprints);
#ifdef OFFICIALSERV_ONLY
    Msg("Blocked connectionless  : %i\n", blockedconnectionless);
#endif
    Msg("Blocked malformed packet: %i\n", blockedmalformed);
    Msg("------------------------  -----\n");
    Msg("Total                   : %i\n", blockedshortpackets + blockedprints + blockedconnectionless + blockedmalformed);
}

int __cdecl SOP_NET_ReceiveDatagram(int a, void *structure)
{
    //Msg("Pre : a: %i\n", a);
    int ret = g_fnReceiveDatagramTrampoline(a, structure);
    /*int long1 = *(int *)structure;
    int long2 = *(((int *)structure)+1);
    int long3 = *(((int *)structure)+2);
    int long4 = *(((int *)structure)+3);
    int long5 = *(((int *)structure)+4);
    int long6 = *(((int *)structure)+5);
    int long7 = *(((int *)structure)+6);
    int long8 = *(((int *)structure)+7);
    int long9 = *(((int *)structure)+8);
    int long10 = *(((int *)structure)+9);
    int long11 = *(((int *)structure)+10);
    int long12 = *(((int *)structure)+11);

    if(ret > 0)
    {
        Msg("Notice: recvfrom is at %p\n", g_pVCR->Hook_recvfrom);
        Msg("Incoming packet %3i:\n %8x %8x %8x %8x\n", a, long1, long2, long3, long4);
        Msg(" %8x %8x %8x %8x\n", long5, long6, long7, long8);
        Msg(" %8x %8x %8x %8x\n", long9, long10, long11, long12);
        Msg(" %s\n ret: %i\n", *(char **)(((int *)structure)+6), ret);
    }*/

    return ret;
    //Msg("Post: a: %i\n", a);
}

void __cdecl SOP_NET_SendPacket( void *netchan, int a, const netadr_t &sock, unsigned char const *data, int length, bf_write *bitdata, bool b )
{
    char *buf = (char *)data;
    if(!netchan)
    {
        const static char gametype[] = "\\gametype\\";
        const static char search[] = "\\max\\32\\bots\\"; // pos 5 and 6 are skipped
        const static int searchlen = 13; //strlen(search);

        // if there is a reason to fake the response to master
        // 64 is an arbitray number that is somewhere between 12 and minumum response length (132ish)
        // 0.\protocol\#\challenge\#\players\#\max\#\bots\#\gamedir\x\map\x\password\#\os\x\lan\#\region\#\type\x\secure\#\version\x\product\x
        if(length > 64)
        {
            // if its a infostring response
            //                  0   LF  \\  p   r   o   t   o   c   o   l   bs 
            if(!strncmp(buf, "\x30\x0a\x5c\x70\x72\x6f\x74\x6f\x63\x6f\x6c\x5c", 12) && strstr(buf, "\\challenge\\") && strstr(buf, "\\secure\\") && strstr(buf, "\\version\\") && buf[length-1] == '\x0a')
            {
                char response[1024];
                int resplen = length;
                memcpy(response, data, length);

                // if fake response is enabled
                if(serverquery_fakemaster.GetBool())
                {
                    // if the response contains the search string
                    for(int i = 0; i < resplen; i++)
                    {
                        bool isMatch = true;
                        for(int j = 0; (j < searchlen) && ((i+j) < length); j++)
                        {
                            if(j == 5 || j == 6) continue;
                            if(response[i+j] != search[j])
                            {
                                isMatch = false;
                                break;
                            }
                        }
                        if(isMatch && length > i+6)
                        {
#ifdef OFFICIALSERV_ONLY
                            if(serverquery_debugprints.GetBool()) Msg("Modifying Response to Master Server to: %i.%i.%i.%i:%d\n", sock.ip[0], sock.ip[1], sock.ip[2], sock.ip[3], ntohs(sock.port));
#endif
                            response[i+5] = '2';
                            response[i+6] = '4';
                            break;
                        }
                    }
                }
#ifdef OFFICIALSERV_ONLY
                if(!FStrEq(serverquery_tagsoverride.GetString(), "0"))
                {
                    if(FStrEq(serverquery_tagsoverride.GetString(), ""))
                    {
                        resplen = masterKeyRemove(response, response, resplen, gametype);
                    }
                    else
                    {
                        resplen = masterKeyReplace(response, response, resplen, gametype, serverquery_tagsoverride.GetString());
                    }
                }

                char fakeHostIP[32];
                fakeHostIP[0] = '\0';

                // set the fakeHostIP if we should be faking responses to this host
                if(!FStrEq(serverquery_faketohost_host.GetString(), "0"))
                {
                    // set fakeHostIP if this ip should be faked
                    checkFakeHostIPMatch(sock.ip, fakeHostIP);
                }

                if(fakeHostIP[0])
                {
                    char szCurPlayers[128];
                    int curplayers;
                    const char *keypointer = strstr(response, "\\players\\");
                    keypointer += 9;
                    nexttoken(szCurPlayers, keypointer, '\\');
                    curplayers = atoi(szCurPlayers);

                    // replace players playing if its over the max
                    if(curplayers > serverquery_faketohost_maxplayers.GetInt())
                    {
                        resplen = masterKeyReplace(response, response, resplen, "\\players\\", serverquery_faketohost_maxplayers.GetString());
                        // if we start using bots, this might become necessary
                        //resplen = masterKeyReplace(response, response, resplen, "\\bots\\", "0");
                    }
                    resplen = masterKeyReplace(response, response, resplen, "\\password\\", serverquery_faketohost_passworded.GetString());
                }
#endif

                /*response[resplen] = '\0';
                if(serverquery_debugprints.GetBool()) Msg("Master server reply: %s\n\n", response);*/
                g_fnSendPacketTrampoline(netchan, a, sock, (const unsigned char*)response, resplen, bitdata, b);
                return;

            }
        }

    #ifdef OFFICIALSERV_ONLY
        // if it's an A2S_INFO packet
        if(!strncmp(buf, "\xFF\xFF\xFF\xFF\x49", 5))
        {
            bool overridetags = !FStrEq(serverquery_tagsoverride.GetString(), "0");
            // if there is a reason to modify anything
            if(serverquery_visibleplayers.GetInt() > -1 || serverquery_visibleplayers_min.GetInt() > -1 || serverquery_visiblemaxplayers.GetInt() > -1 || serverquery_replacefirsthostnamechar.GetBool() || serverquery_addplayers.GetInt() != 0 || overridetags)
            {
                char old;
                int pos = 6;
                pos = AdvanceString(buf, pos, length);
                pos = AdvanceString(buf, pos, length);
                pos = AdvanceString(buf, pos, length);
                pos = AdvanceString(buf, pos, length);
                pos += 2;
                // make sure there is enough in the buffer left for it to be possible it is a A2S_INFO packet
                // also check the Dedicated byte and OS byte to make sure it's one of the possible values
                // then check to make sure the last byte is correct
                // if any of these are false, skip this packet because it can't possibly be an A2S_INFO packet
                if(serverquery_debugprints.GetInt() >= 2)
                {
                    Msg("NET_SendPacket a: %i  b: %i  bitdata: %p\n", a, b, bitdata);
                    Msg("Possible A2S_INFO %i %i %c %c\n ", pos, length, buf[pos+3], buf[pos+4]);
                    for(int i = pos-4; i < length; i++)
                        Msg("%2x ", (unsigned int)(unsigned char)buf[i]);
                    Msg("\n ");
                    for(int i = pos-4; i < length; i++)
                        Msg("%c  ", (buf[i] >= 32 && buf[i] <= 126) ? buf[i] : '.');
                    Msg("\n ");
                    for(int i = pos-4; i < length; i++)
                        Msg("%c  ", (i == pos+3 || i == pos+4) ? '^' : ' ');
                    Msg("\n");
                }
                if((pos+4) < length && (buf[pos+3] == 'l' || buf[pos+3] == 'd' || buf[pos+3] == 'p') && (buf[pos+4] == 'l' || buf[pos+4] == 'w'))
                {
                    char fakeHostIP[32];
                    fakeHostIP[0] = '\0';

                    if(serverquery_debugprints.GetBool()) Msg("A2S_INFO to: %i.%i.%i.%i:%i\n", sock.ip[0], sock.ip[1], sock.ip[2], sock.ip[3], ntohs(sock.port));
                    if(serverquery_log.GetBool())
                    {
                        FILE *fp;
                        char filepath[512];

                        Q_snprintf(filepath, sizeof(filepath), "%s/%s/DF_querylog.txt", pAdminOP.GameDir(), pAdminOP.DataDir());
                        V_FixSlashes(filepath);
                        fp = fopen(filepath, "at");

                        if(fp)
                        {
                            fputs(UTIL_VarArgs("A2S_INFO to: %i.%i.%i.%i:%i\n", sock.ip[0], sock.ip[1], sock.ip[2], sock.ip[3], ntohs(sock.port)), fp);
                            fclose(fp);
                        }
                    }

                    // set the fakeHostIP if we should be faking responses to this host
                    if(!FStrEq(serverquery_faketohost_host.GetString(), "0"))
                    {
                        // set fakeHostIP if this ip should be faked
                        checkFakeHostIPMatch(sock.ip, fakeHostIP);
                    }

                    // Change hostname char
                    if(serverquery_replacefirsthostnamechar.GetBool()) buf[6] = '\x01';

                    // change player count
                    old = buf[pos];
                    if(serverquery_addplayers.GetInt() != 0 && ((int)old) + serverquery_addplayers.GetInt() <= 255 && ((int)old) + serverquery_addplayers.GetInt() >= 0)
                        buf[pos] += serverquery_addplayers.GetInt();
                    // set old to reflect any changes
                    old = buf[pos];

                    if(serverquery_visibleplayers_min.GetInt() > -1)
                    {
                        unsigned char newCount;
                        newCount = (unsigned char)serverquery_visibleplayers_min.GetInt();
                        if((int)newCount > (int)old)
                        {
                            buf[pos] = newCount;
                        }
                    }
                    if(serverquery_visibleplayers.GetInt() > -1)
                    {
                        buf[pos] = (char)serverquery_visibleplayers.GetInt();
                    }

                    // fake set players playing if it's over fake max players
                    if(fakeHostIP[0])
                    {
                        int fakeMaxPlayers = serverquery_faketohost_maxplayers.GetInt();
                        if(buf[pos] > fakeMaxPlayers)
                            buf[pos] = fakeMaxPlayers;
                    }

                    // change max players
                    pos++;
                    if(serverquery_visiblemaxplayers.GetInt() > -1)
                    {
                        old = buf[pos+1];
                        buf[pos] = (char)serverquery_visiblemaxplayers.GetInt();
                        buf[pos+1] = old;
                    }
                    else if(serverquery_addplayers.GetInt() != 0 && ((int)buf[pos]) + serverquery_addplayers.GetInt() <= 255 && ((int)buf[pos]) + serverquery_addplayers.GetInt() >= 0)
                    {
                        buf[pos] += serverquery_addplayers.GetInt();
                    }

                    // fake max players if this is the fake host
                    if(fakeHostIP[0])
                    {
                        buf[pos] = serverquery_faketohost_maxplayers.GetInt();
                    }

                    pos+=4; // advanced to password byte (1 - # bots, 2 - dedicated, 3 - os, 4 - password)
                    if(fakeHostIP[0])
                    {
                        buf[pos] = serverquery_faketohost_passworded.GetBool() ? 1 : 0;
                    }

                    if(overridetags)
                    {
                        pos+=2; // advance to game version string
                        pos = AdvanceString(buf, pos, length); // skip over game version string
                        // check for presence of EDF
                        if(length > pos)
                        {
                            unsigned char EDF = (unsigned char)buf[pos];
                            pos++;

                            if(EDF & 0x80)
                            {
                                // skip server game port
                                pos+=2;

                                // skip the game server steam id
                                if(pAdminOP.isTF2)
                                {
                                    pos+=8;
                                }
                            }
                            if(EDF & 0x40)
                            {
                                // skip spectator port and spectator server name
                                pos+=2;
                                pos = AdvanceString(buf, pos, length);
                            }
                            // game tag data
                            if(EDF & 0x20)
                            {
                                static char respbuild[1024];
                                int tagend = AdvanceString(buf, pos, length);
                                int overridelen = strlen(serverquery_tagsoverride.GetString()) + 1; // add one to append '\0'

                                // get the part before
                                memcpy(respbuild, buf, pos);
                                // add the override string
                                memcpy(&respbuild[pos], serverquery_tagsoverride.GetString(), overridelen);
                                // and add the trailing characters
                                memcpy(&respbuild[pos+overridelen], &buf[tagend], length-tagend);

                                buf = respbuild;
                                // remove length of old tag string
                                length -= (tagend-pos);
                                // add override
                                length += overridelen;
                            }
                        }
                    }

                    g_fnSendPacketTrampoline(netchan, a, sock, (const unsigned char*)buf, length, bitdata, b);
                    return;
                }
                else
                {
                    // Log this temporarily
                    pAdminOP.TimeLog("sopsockwarns.log", "Got a supposed A2S_INFO packet that failed secondary checks.\n");
                }
            }
        }

        // if it's an A2S_PLAYER packet
        if(!strncmp(buf, "\xFF\xFF\xFF\xFF\x44", 5))
        {
            bool fakeplayers = serverquery_fakeplayers.GetBool();
            // if faking the player list is enabled
            if(fakeplayers || serverquery_showconnecting.GetBool())
            {
                //if(port != 7130) Msg("A2S_PLAYER from %s:%i\n", ip, port);
                // if there could be a need to fake anything
                if(serverquery_visibleplayers.GetInt() > -1 || serverquery_visibleplayers_min.GetInt() > -1 || serverquery_addplayers.GetInt() != 0 || serverquery_showconnecting.GetBool())
                {
                    char mydata[3200] = "";
                    CUtlVector<playerquery_t> playerQuery;

                    if(serverquery_debugprints.GetBool()) Msg("A2S_PLAYER to: %i.%i.%i.%i:%i\n", sock.ip[0], sock.ip[1], sock.ip[2], sock.ip[3], ntohs(sock.port));
                    if(serverquery_log.GetBool())
                    {
                        FILE *fp;
                        char filepath[512];

                        Q_snprintf(filepath, sizeof(filepath), "%s/%s/DF_querylog.txt", pAdminOP.GameDir(), pAdminOP.DataDir());
                        V_FixSlashes(filepath);
                        fp = fopen(filepath, "at");

                        if(fp)
                        {
                            fputs(UTIL_VarArgs("A2S_PLAYER to: %i.%i.%i.%i:%i\n", sock.ip[0], sock.ip[1], sock.ip[2], sock.ip[3], ntohs(sock.port)), fp);
                            fclose(fp);
                        }
                    }

                    bf_write bitbuf( mydata, sizeof(mydata) );

                    char fakeHostIP[32];
                    fakeHostIP[0] = '\0';
                    // set the fakeHostIP if we should be faking responses to this host
                    if(!FStrEq(serverquery_faketohost_host.GetString(), "0"))
                    {
                        // set fakeHostIP if this ip should be faked
                        checkFakeHostIPMatch(sock.ip, fakeHostIP);
                    }

                    int visplayers;
                    int players;
                    int totalplayers;
                    int pos = 5;
                    int tmp;
                    float tmpf;
                    int i;

                    players = buf[pos];
                    totalplayers = players;
                    //Msg(" Players: %i\n", players);
                    pos++;

                    // We have to completely rebuild if showconnecting is on.
                    if(!serverquery_showconnecting.GetBool() || fakeHostIP[0])
                    {
                        while(players--)
                        {
                            playerquery_t player;
                            if(pos >= length) break;
                            tmp = buf[pos];
                            player.id = tmp;
                            //Msg(" %i:\n", tmp);
                            pos++;
                            if(pos >= length) break;
                            player.name = &buf[pos];
                            //Msg("  Name: %s\n", &buf[pos]);
                            pos = AdvanceString(buf, pos, length);
                            if(pos >= length) break;
                            memcpy(&tmp, &buf[pos], sizeof(int));
                            player.kills = tmp;
                            //Msg("  Kills: %i\n", tmp);
                            pos += 4;
                            if(pos >= length) break;
                            memcpy(&tmpf, &buf[pos], sizeof(float));
                            player.time = tmpf;
                            //Msg("  Time: %fs\n", tmpf);
                            playerQuery.AddToTail(player);
                            pos += 4;
                            if(pos >= length) break;
                        }
                    }

                    int connectedPlayers = pAdminOP.GetConnectedPlayerCount();
                    visplayers = max(players, connectedPlayers);
                    if(((fakeplayers || serverquery_fakeaddplayersonly.GetBool()) && serverquery_addplayers.GetInt() != 0) ||
                        serverquery_addplayers.GetInt() < 0)
                    {
                        visplayers += serverquery_addplayers.GetInt();
                    }
                    if(fakeplayers && !serverquery_fakeaddplayersonly.GetBool())
                    {
                        if(serverquery_visibleplayers_min.GetInt() > -1)
                        {
                            if(visplayers < serverquery_visibleplayers_min.GetInt()) visplayers = serverquery_visibleplayers_min.GetInt();
                        }
                        if(serverquery_visibleplayers.GetInt() > -1)
                        {
                            visplayers = serverquery_visibleplayers.GetInt();
                        }
                    }

                    if(strcmp(servermoved.GetString(), "0"))
                    {
                        playerquery_t player;
                        player.id = 0;
                        player.name = "Server changed IP";
                        player.kills = 9990;
                        player.time = gpGlobals->curtime + 9999;
                        playerQuery.AddToTail(player);

                        player.id = 1;
                        player.name = "The new IP is:";
                        player.kills = 9980;
                        player.time = gpGlobals->curtime + 9998;
                        playerQuery.AddToTail(player);

                        player.id = 2;
                        player.name = (char *)servermoved.GetString();
                        player.kills = 9970;
                        player.time = gpGlobals->curtime + 9997;
                        playerQuery.AddToTail(player);

                        player.id = 3;
                        player.name = "Add it to your favorites";
                        player.kills = 9960;
                        player.time = gpGlobals->curtime + 9996;
                        playerQuery.AddToTail(player);
                    }

                    if(visplayers >= 64)
                    {
                        playerquery_t player;
                        player.id = 0;
                        player.name = "SourceOP Plugin";
                        player.kills = 9990;
                        player.time = gpGlobals->curtime + 9999;
                        playerQuery.AddToTail(player);

                        player.id = 1;
                        player.name = "  By: Drunken F00l";
                        player.kills = 9980;
                        player.time = gpGlobals->curtime + 9998;
                        playerQuery.AddToTail(player);

                        player.id = 2;
                        player.name = "  URL: www.sourceop.com";
                        player.kills = 9970;
                        player.time = gpGlobals->curtime + 9997;
                        playerQuery.AddToTail(player);

                        char ip[16];
                        sprintf(ip, "%i.%i.%i.%i", sock.ip[0], sock.ip[1], sock.ip[2], sock.ip[3]);

                        player.id = 3;
                        player.name = "Your IP is:";
                        player.kills = 9960;
                        player.time = gpGlobals->curtime + 9996;
                        playerQuery.AddToTail(player);

                        player.id = 4;
                        player.name = &ip[0];
                        player.kills = 9950;
                        player.time = gpGlobals->curtime + 9995;
                        playerQuery.AddToTail(player);

                        player.id = 5;
                        player.name = "<a href=\"http://www.sourceop.com/\">SourceOP</a>";
                        player.kills = 9940;
                        player.time = gpGlobals->curtime + 9994;
                        playerQuery.AddToTail(player);
                    }

                    // rebuild the response
                    bitbuf.WriteLong(-1);
                    bitbuf.WriteByte('\x44');

                    // clamp visible players if this is the host we're faking to
                    if(fakeHostIP[0])
                    {
                        int fakeMaxPlayers = serverquery_faketohost_maxplayers.GetInt();
                        if(visplayers > fakeMaxPlayers)
                            visplayers = fakeMaxPlayers;
                    }

                    bitbuf.WriteByte(visplayers);
                    // clamp to visplayers if necessary
                    for(i = 0; i < min(playerQuery.Count(), visplayers); i++)
                    {
                        playerquery_t *player = playerQuery.Base() + i; // fast access
                        bitbuf.WriteByte(player->id);
                        bitbuf.WriteString(player->name);
                        bitbuf.WriteLong(player->kills);
                        bitbuf.WriteFloat(player->time);
                    }

                    int largestK = 0;
                    // add players who are connecting, if this isn't a host we're faking to
                    if(serverquery_showconnecting.GetBool() && !fakeHostIP[0])
                    {
                        if(i < visplayers)
                        {
                            for(int k = 0; k < min(MAX_AOP_PLAYERS, pAdminOP.GetMaxClients()); k++)
                            {
                                void *baseclient = pAdminOP.pAOPPlayers[k].baseclient;
                                if(baseclient)
                                {
                                    const char *playername = VFuncs::GetClientName(baseclient);
                                    if(playername)
                                    {
                                        largestK = k;
                                        bitbuf.WriteByte(k);
                                        bitbuf.WriteString(playername[0] != '\0' ? playername : "connecting...");

                                        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+k+1);
                                        if(info)
                                        {
                                            bitbuf.WriteLong(info->GetFragCount());
                                        }
                                        else
                                        {
                                            bitbuf.WriteLong(0);
                                        }

                                        INetChannelInfo *netinfo = (INetChannelInfo *)VFuncs::GetNetChannel(baseclient);
                                        if(netinfo)
                                        {
                                            bitbuf.WriteBitFloat(netinfo->GetTimeConnected());
                                        }
                                        else
                                        {
                                            bitbuf.WriteFloat(gpGlobals->curtime - pAdminOP.pAOPPlayers[k].GetConnectTime());
                                        }

                                        // increment the amount of players written on query
                                        i++;
                                    }
                                }
                            }
                        }
                    }
                    // make up the other players
                    int curpos = 0;
                    int j = 0;
                    for (; i<visplayers; i++)
                    {
                        curpos = largestK + j;
                        j++;
                        // use that slot
                        bitbuf.WriteByte(curpos);
                        if(curpos < 26)
                            bitbuf.WriteString(UTIL_VarArgs("%c", curpos+65));
                        else
                            bitbuf.WriteString(UTIL_VarArgs("%c%c", ((curpos/26)-1)+65, (curpos%26)+65));
                        bitbuf.WriteLong((curpos*10) + random->RandomInt(0,2));
                        bitbuf.WriteFloat(gpGlobals->curtime + (float)i + 1);
                    }

                    g_fnSendPacketTrampoline(netchan, a, sock, (const unsigned char *)bitbuf.GetData(), bitbuf.GetNumBytesWritten(), bitdata, b);
                    return;
                }
            }
        }
        // if it's an A2S_RULES packet
        if(!strncmp(buf, "\xFF\xFF\xFF\xFF\x45", 5))
        {
            if(serverquery_debugprints.GetBool()) Msg("A2S_RULES to: %i.%i.%i.%i:%i\n", sock.ip[0], sock.ip[1], sock.ip[2], sock.ip[3], ntohs(sock.port));
            char fakeHostIP[32];
            fakeHostIP[0] = '\0';
            // set the fakeHostIP if we should be faking responses to this host
            if(!FStrEq(serverquery_faketohost_host.GetString(), "0"))
            {
                // set fakeHostIP if this ip should be faked
                checkFakeHostIPMatch(sock.ip, fakeHostIP);
            }
            if(fakeHostIP[0])
            {
                int pos = 7;
                while(pos < length -1)
                {
                    if(FStrEq(&buf[pos], "sv_password"))
                    {
                        pos = AdvanceString(buf, pos, length);
                        buf[pos] = serverquery_faketohost_passworded.GetBool() ? '1' : '0';
                        pos = AdvanceString(buf, pos, length);
                        break;
                    }
                    pos = AdvanceString(buf, pos, length);
                    pos = AdvanceString(buf, pos, length);
                }
            }
        }
    #endif // OFFICIALSERV_ONLY
    }
    g_fnSendPacketTrampoline(netchan, a, sock, data, length, bitdata, b);
}

void __cdecl SOP_BroadcastVoice( IClient *iclient, int nCount, char *data, long long c)
{
    //Msg("SV_BroadcastVoiceData()\n");
    int slot = VFuncs::IGetPlayerSlot(iclient);

    if(remoteserver) remoteserver->IncomingVoiceData(slot, data, nCount);

    /*FILE *fp = fopen(UTIL_VarArgs("voice.raw"), "ab");
    if(fp)
    {
        fwrite(data, nCount, 1, fp);
        fclose(fp);
    }*/
    g_fnBroadcastVoiceTrampoline(iclient, nCount, data, c);
}

#ifdef __linux__
int __cdecl SOP_usleep(useconds_t usec)
{
    if(usec == 1000)
    {
        int sleeptime = sleep_time.GetInt();
        if(sleeptime != -1)
        {
            return SOP_usleepTrampoline(sleeptime);
        }
        else
        {
            return 0;
        }
    }
    else
    {
        return SOP_usleepTrampoline(usec);
    }
}
#endif

bool SOP_SteamGameServer_InitSafe( uint32 unIP, uint16 usPort, uint16 usGamePort, uint16 usQueryPort, EServerMode eServerMode, const char *pchVersionString )
{
    bool ret = g_fnSteamGameServer_InitSafeTrampoline(unIP, usPort, usGamePort, usQueryPort, eServerMode, pchVersionString);
    pAdminOP.HookSteamFromGameServerInit();

    bool bLanMode = (unIP == 1);
    uint32 serverFlags = 0;
    if(eServerMode == eServerModeAuthenticationAndSecure) 
        serverFlags |= k_unServerFlagSecure;
    if(bLanMode)
        serverFlags |= k_unServerFlagPrivate;

    pAdminOP.StoredGameServerInitParams.unIP = unIP;
    pAdminOP.StoredGameServerInitParams.usGamePort = usGamePort;
    pAdminOP.StoredGameServerInitParams.usQueryPort = usQueryPort;
    pAdminOP.StoredGameServerInitParams.unServerFlags = serverFlags;
    V_strncpy(pAdminOP.StoredGameServerInitParams.pszVersionString, pchVersionString, sizeof(pAdminOP.StoredGameServerInitParams.pszVersionString));
    pAdminOP.StoredGameServerInitParams.bLanMode = bLanMode;

    return ret;
}

bool SOP_BGetCallback( HSteamPipe hSteamPipe, CallbackMsg_t *pCallbackMsg, HSteamCall *phSteamCall )
{
    bool res = false;

    bool bGetAnotherCallback = false;
    do
    {
        res = g_fnBGetCallbackTrampoline(hSteamPipe, pCallbackMsg, phSteamCall);
        if(!res)
            return false;

        //CAdminOP::ColorMsg(CONCOLOR_LIGHTCYAN, "Got steam callback %-5i   size: %i.\n", pCallbackMsg->m_iCallback, pCallbackMsg->m_cubParam);

        bGetAnotherCallback = false;

        if(GSGameplayStats_t::k_iCallback == pCallbackMsg->m_iCallback)
        {
            GSGameplayStats_t callback;
            memcpy(&callback, pCallbackMsg->m_pubParam, sizeof(callback));
            Msg("[SOURCEOP] Got stats callback  %i %i %i %i.\n", callback.m_eResult,
                callback.m_nRank, callback.m_unTotalConnects, callback.m_unTotalMinutesPlayed);
            if(callback.m_eResult == 1)
            {
                pAdminOP.m_iStatsResult = callback.m_eResult;
                pAdminOP.m_iStatsRank = callback.m_nRank;
                pAdminOP.m_iStatsTotalConnects = callback.m_unTotalConnects;
                pAdminOP.m_iStatsTotalMinutesPlayed = callback.m_unTotalMinutesPlayed;
            }
        }
        else if(GSClientDeny_t::k_iCallback == pCallbackMsg->m_iCallback && unvacban_enabled.GetBool())
        {
            GSClientDeny_t callback;
            if(pCallbackMsg->m_cubParam != sizeof(callback))
            {
                Msg("[SOURCEOP] Incoming GSClientDeny_t has size %i but expected %i.\n", pCallbackMsg->m_cubParam, sizeof(callback));
                break;
            }

            memcpy(&callback, pCallbackMsg->m_pubParam, sizeof(callback));
            if(pAdminOP.VacAllowPlayer(callback.m_SteamID) && callback.m_eDenyReason == k_EDenyCheater)
            {
                // swallow this callback
                Msg("[SOURCEOP] Handling GSClientDeny_t for SteamID %llu.\n", callback.m_SteamID.ConvertToUint64());
                for(int i = 0; i < pAdminOP.GetMaxClients(); i++)
                {
                    CAdminOPPlayer *pAOPPlayer = &pAdminOP.pAOPPlayers[i];
                    if(callback.m_SteamID == pAOPPlayer->GetSteamID().ConvertToUint64() && pAOPPlayer->baseclient)
                    {
                        VFuncs::Disconnect(pAOPPlayer->baseclient, "Try again");
                    }
                }

                _FreeLastCallback(hSteamPipe);
                bGetAnotherCallback = true;
            }
#if defined(_SOPDEBUG) && defined(OFFICIALSERV_ONLY)
            else
            {
                Msg("[SOURCEOP] Ignoring GSClientDeny_t.\n");
                _FreeLastCallback(hSteamPipe);
                bGetAnotherCallback = true;
            }
#endif
        }
    }
    while(bGetAnotherCallback);

    return res;
}

bool HookSetServerType(unsigned int unServerFlags, unsigned int unGameIP, unsigned short unGamePort, unsigned short unSpectatorPort, unsigned short usQueryPort, char const *pchGameDir, char const *pchVersion, bool bLANMode)
{
    RETURN_META_VALUE(MRES_IGNORED, 0);
}

int HookClientGCSendMessage(int unAppID, unsigned int messageId, const void *data, unsigned int cbData)
{
    return HookGCSendMessage(messageId, data, cbData);
}

int HookGCSendMessage(unsigned int messageId, const void *data, unsigned int cbData)
{
    //Msg("HookGCSendMessage: %i, %p, %i\n", messageId, data, cbData);

    /*FILE *fp=fopen("gcmessages.txt", "ab");
    if(fp)
    {
        fprintf(fp, "HookGCSendMessage: %i, %p, %i\n", messageId, data, cbData);
        fwrite(data, cbData, 1, fp);
    fprintf(fp, "\n");
        fclose(fp);
    }*/

    RETURN_META_VALUE(MRES_IGNORED, 0);
}

bool HookIsMessageAvailable(unsigned int *cbData)
{
    RETURN_META_VALUE(MRES_IGNORED, 0);
}

SOCKET g_iItemUdpSocket = 0;
struct sockaddr_in g_itemDest;
int g_iItemAppId = 0;

int HookClientGCRetrieveMessage(int unAppID, unsigned int *messageId, void *data, unsigned int cbData, unsigned int *cbDataActual)
{
    return HookGCRetrieveMessage(messageId, data, cbData, cbDataActual);
}

int HookGCRetrieveMessage(unsigned int *messageId, void *data, unsigned int cbData, unsigned int *cbDataActual)
{
    //Msg("HookGCRetrieveMessage: %i, %p, %i, %i\n", *messageId, data, cbData, *cbDataActual);

    // k_ESOMsg_Create          21
    // k_ESOMsg_Update          22
    // k_ESOMsg_Destroy         23
    // k_ESOMsg_CacheSubscribed 24

#ifdef OFFICIALSERV_ONLY
    int message = (*messageId) & 0x7FFFFFFF;
    unsigned char isProtobuf = *messageId >> 31;
    if(message >= 21 && message <= 24)
    {
        static bool bSocketCreated = false;

        if(!bSocketCreated)
        {
            bSocketCreated = true;

            g_iItemAppId = engine->GetAppID();
            g_iItemUdpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

            // make the socket non-blocking
            unsigned long set = 1;
            ioctlsocket(g_iItemUdpSocket, FIONBIO, &set);

            struct hostent *hptr;
            if ((hptr = gethostbyname("www.tf2items.com")))
            {
                memcpy (&g_itemDest.sin_addr, hptr->h_addr, hptr->h_length);

                g_itemDest.sin_family = AF_INET;
                g_itemDest.sin_port = htons(27612);
            }
            else
            {
                closesocket(g_iItemUdpSocket);
                g_iItemUdpSocket = 0;
            }
        }

        // fire off a udp packet
        if(g_iItemUdpSocket != 0 && g_iItemAppId != 0)
        {
            char *pszBuf;

            int size = *cbDataActual;
            if(size >= 0 && size < 131072)
            {
                pszBuf = (char *)malloc(size + 9);
                memcpy(pszBuf, &message, 4);
                pszBuf[4] = isProtobuf;
                memcpy(&pszBuf[5], &g_iItemAppId, 4);
                memcpy(&pszBuf[9], data, size);

                sendto(g_iItemUdpSocket, pszBuf, size + 9, 0, (const sockaddr *)&g_itemDest, sizeof(g_itemDest));
                free(pszBuf);
            }
        }
    }
#endif

    /*FILE *fp=fopen("gcmessages.txt", "ab");
    if(fp)
    {
        Msg("HookGCRetrieveMessage: %i, %p, %i, %i\n", message, data, cbData, *cbDataActual);
        fprintf(fp, "HookGCRetrieveMessage: %i, %p, %i, %i\n", message, data, cbData, *cbDataActual);
        fwrite(data, *cbDataActual, 1, fp);
        fprintf(fp, "\n");
        fclose(fp);
    }*/

    RETURN_META_VALUE(MRES_IGNORED, 0);
}
