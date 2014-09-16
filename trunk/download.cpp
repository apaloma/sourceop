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
//#define GAME_DLL
#include "fixdebug.h"
#ifdef _L4D_PLUGIN
#include "convar_l4d.h"
#endif
//#include "cbase.h"
//#include "baseentity.h"
// HACK HACK HACK : BASEENTITY ACCESS

#include <stdio.h>
#include <time.h>
#ifdef _WIN32
#include <winsock.h>
#else
#include <sys/types.h>  /* for Socket data types */
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <netinet/in.h> /* for IP Socket data types */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <stdlib.h>     /* for atoi() */
#include <unistd.h>     /* for close() */
#include <asm/ioctls.h>
#include <sys/ioctl.h>
#include <netdb.h>
#define SOCKET int
#define INVALID_SOCKET 0
#define SOCKET_ERROR -1
#define ioctlsocket ioctl
#define closesocket close

#define __time64_t time_t
#define _time64 time
#endif
#include <errno.h>

#include "AdminOP.h"
#include "cvars.h"
#include "download.h"
#include "queuethread.h"
#include "isopgamesystem.h"

#include "tier0/memdbgon.h"

#define READ(fd, buf, cnt) recv ((fd), (buf), (cnt), 0)
#ifdef _WIN32
#ifdef ECONNREFUSED
#undef ECONNREFUSED
#endif
#define ECONNREFUSED    WSAECONNREFUSED
#endif

uerr_t EstablishSourceOPConnection(int &sock, const char *pszCurLevel = NULL);

typedef struct parseheader_s
{
    int endHeader;
    int contentLength;
    bool done;
} parseheader_t;

void SocketShutdown(int *s)
{
    if(s != NULL /*&& host != NULL*/)
    {
        //Sleep(1000);
        if(*s != -1)
        {
            //shutdown(*s, 1);
            closesocket(*s);
        }
        s = NULL;
    }
}

parseheader_t ParseHeaders(const char *buf)
{
    char seps[]   = "\n";
    char *token;
    char tmp[8192];
    int len = 0;
    parseheader_t retMe;

    retMe.contentLength = 0;
    retMe.endHeader = 0;
    retMe.done = 0;
    strncpy(tmp, buf, 8192);

    token = strtok( tmp, seps );
    while( token != NULL )
    {
        len += (strlen(token) + 1);
        //Msg("*\n%s\n", token);
        if(!Q_strncasecmp(token, "Content-Length: ", 16))
        {
            retMe.contentLength = atoi(&token[16]);
        }
        else if(token[0] == '\x0D')
        {
            retMe.endHeader = len;
            retMe.done = 1;
            return retMe;
        }
        token = strtok( NULL, seps );
    }
    return retMe;
}

int CountLines(const char *buf)
{
    char seps[]   = "\n";
    char *token;
    char tmp[8192];
    int lines = 0;

    strncpy(tmp, buf, 8192);

    token = strtok( tmp, seps );
    while( token != NULL )
    {
        //len += (strlen(token) + 1);
        lines++;
        token = strtok( NULL, seps );
    }
    return lines;
}

bool ParseDonator(const char *buf)
{
    if(strlen(buf) >= 7) // 123 5 7
    {
        int spaceloc[2] = {0,0};
        char type[4];
        char donamt[128];
        char steamid[256];
        char extrainfo[128];
        strncpy(type, buf, 4);
        type[3] = '\0';
        for(unsigned int i = 4;i<strlen(buf);i++)
        {
            if(buf[i] == ' ')
            {
                spaceloc[0] = i;
                break;
            }
        }
        for(unsigned int i = spaceloc[0]+1;i<strlen(buf);i++)
        {
            if(buf[i] == ' ')
            {
                spaceloc[1] = i;
                break;
            }
        }
        if(spaceloc[0] > 4 && spaceloc[1] > 4 && spaceloc[0] != spaceloc[1])
        {
            strncpy(steamid, &buf[spaceloc[0]+1], 256);
            steamid[clamp(spaceloc[1] - spaceloc[0] - 1, 0, 255)] = '\0';
            strncpy(donamt, &buf[4], 128);
            donamt[clamp(spaceloc[0]-4, 0, 127)] = '\0';
            strncpy(extrainfo, &buf[spaceloc[1]+1], 128);
            extrainfo[127] = '\0';

            // Convert SteamID format
            char pszSteamIDError[64];
            if ( !IsValidSteamID( steamid, pszSteamIDError, sizeof( pszSteamIDError ) ) )
            {
                CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Invalid SteamID %s in admin file: %s\n", steamid, pszSteamIDError );
                return false;
            }

            CSteamID steamIDCredits( steamid, k_EUniversePublic );
            strcpy(steamid, steamIDCredits.Render() );

            userweb_t uWeb;
            uWeb.donAmt = atof(donamt);
            strncpy(uWeb.szSteamID, steamid, sizeof(uWeb.szSteamID));
            uWeb.szSteamID[sizeof(uWeb.szSteamID)-1] = '\0';
            strncpy(uWeb.szExtraInfo, extrainfo, sizeof(uWeb.szExtraInfo));
            uWeb.szExtraInfo[sizeof(uWeb.szExtraInfo)-1] = '\0';

            if(type[0] == 'b' && type[1] == 'a' && type[2] == 'n')
            {
                //Msg("Banned: %s\n", steamid);
                pAdminOP.banList.AddToTail(uWeb);
            }
            else if(type[0] == 'd' && type[1] == 'o' && type[2] == 'n')
            {
                //Msg("Donator: %s ($%0.2f)\n", steamid, atof(donamt)/100.0f);
                pAdminOP.donList.AddToTail(uWeb);
            }
            else if(type[0] == 'v' && type[1] == 'i' && type[2] == 'p')
            {
                //Msg("VIP: %s\n", steamid);
                pAdminOP.vipList.AddToTail(uWeb);
            }
            else if(type[0] == 's' && type[1] == 'i' && type[2] == 'p')
            {
                //Msg("SIP: %s\n", steamid);
                pAdminOP.sipList.AddToTail(uWeb);
            }
            else if(type[0] == 'd' && type[1] == 'e' && type[2] == 'v')
            {
                //Msg("DEV: %s\n", steamid);
                pAdminOP.devList.AddToTail(uWeb);
            }
            //Msg("Parse: \"%s\" \"%s\" \"%s\"\n", type, donamt, steamid);
            return 1;
        }
    }
    return 0;
}

static uerr_t make_connection (int *sock, char *hostname, unsigned short port)
{
  struct sockaddr_in sock_name;
  struct hostent *hptr;

  if (!(hptr = gethostbyname(hostname)))
    return HOSTERR;
  /* Copy the address of the host to socket description.  */
  memcpy(&sock_name.sin_addr, hptr->h_addr, hptr->h_length);

  /* Set port and protocol */
  sock_name.sin_family = AF_INET;
  sock_name.sin_port = htons (port);

  /* Make an internet socket, stream type.  */
  if ((*sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    return CONSOCKERR;

  /* Connect the socket to the remote host.  */
  if (connect(*sock, (struct sockaddr *) &sock_name, sizeof (sock_name)))
    {
      if (errno == ECONNREFUSED)
    return CONREFUSED;
      else
    return CONERROR;
    }
  //DEBUGP (("Created fd %d.\n", *sock));
  return NOCONERROR;
}

/* Read at most LEN bytes from FD, storing them to BUF.  This is
   virtually the same as read(), but takes care of EINTR braindamage
   and uses select() to timeout the stale connections (a connection is
   stale if more than OPT.TIMEOUT time is spent in select() or
   read()).  */
int
iread (int fd, char *buf, int len)
{
  int res;

  do
    {
#ifdef HAVE_SELECT
      if (opt.timeout)
    {
      do
        {
          res = select_fd (fd, opt.timeout, 0);
        }
      while (res == -1 && errno == EINTR);
      if (res <= 0)
        {
          /* Set errno to ETIMEDOUT on timeout.  */
          if (res == 0)
        /* #### Potentially evil!  */
        errno = ETIMEDOUT;
          return -1;
        }
    }
#endif
      res = READ (fd, buf, len);
    }
  while (res == -1 && errno == EINTR);

  return res;
}

uerr_t EstablishSourceOPConnection(int &sock, const char *pszCurLevel)
{
    uerr_t err;
    int i;

    char con_serv[128];

    char port[32];
    ConVar *ip = NULL;
    char modname[32];
    char features[NUM_FEATS+1];

    //recvtotal = 0;
    //filelength = 0;
    //iRecievedContentType = 0;

    int x_server_len = 16;
    unsigned int x_server[] = {
    'w'^0x69, 'w'^0x96, 'w'^0x69, '.'^0x96, 's'^0x69, 'o'^0x96, 'u'^0x69, 'r'^0x96,
    'c'^0x69, 'e'^0x96, 'o'^0x69, 'p'^0x96, '.'^0x69, 'c'^0x96, 'o'^0x69, 'm'^0x96};

    for (i=0; i < x_server_len; i++)
    {
        if ((i % 2) == 0)
            con_serv[i] = x_server[i]^0x69;
        else
            con_serv[i] = x_server[i]^0x96;
    }
    con_serv[x_server_len] = 0;


    //GET /aobans.txt HTTP/1.1\nHost: aopdata.coolgamescentral.com\nConnection: close\n
    //User-Agent: AdminOP 3.0\nAuthorization: Basic YXV0aDphb3BoaXNoaW5n\n\n
    char szGetData[1024];
    char szGetData1[512];
    char szGetData2[192];

    memset(szGetData1, 0, sizeof(szGetData1));

    int x_get1_len = 24;
    unsigned int x_get1[] = {
    'G'^0x69, 'E'^0x96, 'T'^0x69, ' '^0x96, '/'^0x69, 'g'^0x96, 'a'^0x69, 'm'^0x96,
    'e'^0x69, '_'^0x96, 'd'^0x69, 'a'^0x96, 't'^0x69, 'a'^0x96, '/'^0x69, 'u'^0x96,
    's'^0x69, 'e'^0x96, 'r'^0x69, 's'^0x96, '.'^0x69, 'p'^0x96, 'h'^0x69, 'p'^0x96};

    int x_get2_len = 131;
    unsigned int x_get2[] = {
    ' '^0x69, 'H'^0x96, 'T'^0x69, 'T'^0x96, 'P'^0x69, '/'^0x96, '1'^0x69, '.'^0x96,
    '0'^0x69, '\n'^0x96, 'H'^0x69, 'o'^0x96, 's'^0x69, 't'^0x96, ':'^0x69, ' '^0x96,
    'w'^0x69, 'w'^0x96, 'w'^0x69, '.'^0x96, 's'^0x69, 'o'^0x96, 'u'^0x69, 'r'^0x96,
    'c'^0x69, 'e'^0x96, 'o'^0x69, 'p'^0x96, '.'^0x69, 'c'^0x96, 'o'^0x69, 'm'^0x96,
    '\n'^0x69, 'C'^0x96, 'o'^0x69, 'n'^0x96, 'n'^0x69, 'e'^0x96, 'c'^0x69, 't'^0x96,
    'i'^0x69, 'o'^0x96, 'n'^0x69, ':'^0x96, ' '^0x69, 'c'^0x96, 'l'^0x69, 'o'^0x96,
    's'^0x69, 'e'^0x96, '\n'^0x69, 'U'^0x96, 's'^0x69, 'e'^0x96, 'r'^0x69, '-'^0x96,
    'A'^0x69, 'g'^0x96, 'e'^0x69, 'n'^0x96, 't'^0x69, ':'^0x96, ' '^0x69, 'S'^0x96,
    'o'^0x69, 'u'^0x96, 'r'^0x69, 'c'^0x96, 'e'^0x69, 'O'^0x96, 'P'^0x69, '/'^0x96,
    '0'^0x69, '.'^0x96, '9'^0x69, '\n'^0x96, 'A'^0x69, 'u'^0x96, 't'^0x69, 'h'^0x96, // SourceOP Version
    'o'^0x69, 'r'^0x96, 'i'^0x69, 'z'^0x96, 'a'^0x69, 't'^0x96, 'i'^0x69, 'o'^0x96,
    'n'^0x69, ':'^0x96, ' '^0x69, 'B'^0x96, 'a'^0x69, 's'^0x96, 'i'^0x69, 'c'^0x96,
    ' '^0x69, 'c'^0x96, '2'^0x69, '9'^0x96, 'w'^0x69, 'Z'^0x96, 'D'^0x69, 'R'^0x96,
    '0'^0x69, 'Y'^0x96, 'T'^0x69, 'p'^0x96, 'x'^0x69, 'd'^0x96, 'W'^0x69, 'l'^0x96,
    '0'^0x69, 'a'^0x96, 'G'^0x69, 'F'^0x96, 'j'^0x69, 'a'^0x96, '2'^0x69, 'l'^0x96,
    'u'^0x69, 'Z'^0x96, '2'^0x69, 'g'^0x96, '0'^0x69, 'e'^0x96, 'D'^0x69, 'B'^0x96,
    'y'^0x69, '\n'^0x96, '\n'^0x69};

    //HTTP/1.0\nHost: www.sourceop.com\nConnection: close\nUser-Agent: SourceOP/0.8\nAuthorization: Basic c29wZGF0YTpxdWl0aGFja2luZ2g0eDBy\n\n
    //HTTP/1.0\nHost: www.sourceop.com\nConnection: close\nUser-Agent: SourceOP/0.8\nAuthorization: Basic c29wZDR0YTpxdWl0aGFja2luZ2g0eDBy\n\n

    for (i=0; i < x_get1_len; i++)
    {
        if ((i % 2) == 0)
            szGetData1[i] = x_get1[i]^0x69;
        else
            szGetData1[i] = x_get1[i]^0x96;
    }
    for (i=0; i < x_get2_len; i++)
    {
        if ((i % 2) == 0)
            szGetData2[i] = x_get2[i]^0x69;
        else
            szGetData2[i] = x_get2[i]^0x96;
    }
    szGetData2[x_get2_len] = '\0';

    szGetData1[x_get1_len] = '?';
    szGetData1[x_get1_len+1] = 'i';
    szGetData1[x_get1_len+2] = '=';
    szGetData1[x_get1_len+3] = '\0';

    ip = cvar->FindVar("ip");

    if(ip != NULL)
    {
        strncat(szGetData1, ip->GetString(), sizeof(szGetData1)-strlen(szGetData1));
    }
    sprintf(port, "%i", pAdminOP.serverPort);
    strncpy(modname, pAdminOP.ModName(), sizeof(modname));
    //strncpy(modname, "wtf", sizeof(modname));
    modname[sizeof(modname)-1] = '\0';

    for(i = 0; i < NUM_FEATS; i++)
    {
        if(pAdminOP.FeatureStatus(i))
        {
            features[i] = '1';
        }
        else
        {
            features[i] = '0';
        }
    }
    features[NUM_FEATS] = '\0';

    strncat(szGetData1, "&p=", sizeof(szGetData1)-strlen(szGetData1));
    strncat(szGetData1, port, sizeof(szGetData1)-strlen(szGetData1));
    strncat(szGetData1, "&v=", sizeof(szGetData1)-strlen(szGetData1));
    strncat(szGetData1, SourceOPVerShort, sizeof(szGetData1)-strlen(szGetData1));
    strncat(szGetData1, "&f=", sizeof(szGetData1)-strlen(szGetData1));
    strncat(szGetData1, features, sizeof(szGetData1)-strlen(szGetData1));
    strncat(szGetData1, "&m=", sizeof(szGetData1)-strlen(szGetData1));
    strncat(szGetData1, modname, sizeof(szGetData1)-strlen(szGetData1));
    if(pszCurLevel != NULL)
    {
        strncat(szGetData1, "&l=", sizeof(szGetData1)-strlen(szGetData1));
        strncat(szGetData1, pszCurLevel, sizeof(szGetData1)-strlen(szGetData1));
    }
    strncat(szGetData1, "&h=", sizeof(szGetData1)-strlen(szGetData1));
    if(hostname != NULL)
    {
        char szHostname[256];
        char buf[256];
        strncpy(szHostname, hostname->GetString(), sizeof(szHostname));
        szHostname[255] = '\0';

        for(int i = 0;i<sizeof(szHostname)-4;i++)
        {
            // Replace ' ' with %20
            if(szHostname[i] == ' ')
            {
                strcpy(buf, szHostname);
                buf[i] = '\0';
                _snprintf(szHostname, sizeof(szHostname), "%s%%20%s", buf, &buf[i+1]);
            }
            // Replace '#' with %23
            if(szHostname[i] == '#')
            {
                strcpy(buf, szHostname);
                buf[i] = '\0';
                _snprintf(szHostname, sizeof(szHostname), "%s%%23%s", buf, &buf[i+1]);
            }
            // Replace '&' with %26
            if(szHostname[i] == '&')
            {
                strcpy(buf, szHostname);
                buf[i] = '\0';
                _snprintf(szHostname, sizeof(szHostname), "%s%%26%s", buf, &buf[i+1]);
            }
            // Replace '?' with %3F
            if(szHostname[i] == '?')
            {
                strcpy(buf, szHostname);
                buf[i] = '\0';
                _snprintf(szHostname, sizeof(szHostname), "%s%%3F%s", buf, &buf[i+1]);
            }
            // Replace '+' with %2B
            if(szHostname[i] == '+')
            {
                strcpy(buf, szHostname);
                buf[i] = '\0';
                _snprintf(szHostname, sizeof(szHostname), "%s%%2B%s", buf, &buf[i+1]);
            }
        }
        //strncat(szGetData1, hostname->GetString(), sizeof(szGetData1)-strlen(szGetData1));
        strncat(szGetData1, szHostname, sizeof(szGetData1)-strlen(szGetData1));
    }

    strcpy(szGetData, szGetData1);
    strncat(szGetData, szGetData2, sizeof(szGetData)-strlen(szGetData));

    /*
C:\Program Files\Apache Group\coolgam\bin>htpasswd -bc \htpasswd\coolgam\sopdata
 sopdata this!is#a@good%p4ss
Automatically using MD5 format.
Adding password for user sopdata
    */

    //sprintf(szGetData, "GET /game_data/users.php?i=67.19.104.220&p=27015&h=lol HTTP/1.0\nHost: www.sourceop.com\nConnection: close\nUser-Agent: SourceOP/0.7\nAuthorization: Basic c29wZGF0YTpxdWl0aGFja2luZ2g0eDBy\n\n");
    //strcpy(szGetData, "GET /game_data/users.php HTTP/1.0\nHost: www.sourceop.com\nConnection: close\nUser-Agent: SourceOP/0.7\nAuthorization: Digest username=\"sopdata\", realm=\"SourceOPData\", qop=\"auth\", algorithm=\"MD5\", uri=\"/game_data/\", nonce=\"o9uZbwL+AwA=a6fea63dda8f0cf241ebfa216d31165d6375ae6a\", response=\"3642170aede8591d0404009620753de2\"\n\n");
    //strcpy(szGetData, "GET /game_data/ HTTP/1.1\nHost: www.sourceop.com\nConnection: close\nUser-Agent: SourceOP/0.7\nAuthorization: Digest username=\"sopdata\", realm=\"SourceOPData\", qop=\"auth\", algorithm=\"MD5\", uri=\"/game_data/\", nonce=\"o9uZbwL+AwA=a6fea63dda8f0cf241ebfa216d31165d6375ae6a\", cnonce=\"2ee8943b614aa0225b3ce3105e733e42\", response=\"2aa970b4e948fa9e1bbebb88d1d7112d\"\n\n");

    err = make_connection(&sock, con_serv, 80);

    switch (err)
    {
    case HOSTERR:
        //logputs (LOG_VERBOSE, "\n");
        //logprintf (LOG_NOTQUIET, "%s: %s.\n", u->host, herrmsg (h_errno));
        return HOSTERR;
        break;
    case CONSOCKERR:
        //logputs (LOG_VERBOSE, "\n");
        //logprintf (LOG_NOTQUIET, "socket: %s\n", strerror (errno));
        return CONSOCKERR;
        break;
    case CONREFUSED:
        //logputs (LOG_VERBOSE, "\n");
        //logprintf (LOG_NOTQUIET, _("Connection to %s:%hu refused.\n"), u->host, u->port);
        SocketShutdown (&sock);
        return CONREFUSED;
    case CONERROR:
        //logputs (LOG_VERBOSE, "\n");
        //logprintf (LOG_NOTQUIET, "connect: %s\n", strerror (errno));
        SocketShutdown (&sock);
        return CONERROR;
        break;
    case NOCONERROR:
        /* Everything is fine!  */
        //logputs (LOG_VERBOSE, _("connected!\n"));
        break;
    default:
        //abort ();
        break;
    } /* switch */

    //Msg("%s\n", szGetData);
    send(sock, szGetData, strlen(szGetData), 0);

    return err;
}

uerr_t ParseUserFile(void)
{
    int sock, res;
    uerr_t r;
    char buf[4096];

    pAdminOP.banList.Purge();
    pAdminOP.donList.Purge();
    pAdminOP.vipList.Purge();
    pAdminOP.sipList.Purge();
    pAdminOP.devList.Purge();

    r = EstablishSourceOPConnection(sock, "Server loading");

    if(r != NOCONERROR)
    {
        return r;
    }

    if(sock)
    {
        int packet = 0;
        char realbuf[8192];

        parseheader_t headerData;
        headerData.contentLength = 0;
        headerData.endHeader = 0;
        headerData.done = 0;
        do
        {
            packet++;
            memset(buf, 0, sizeof(buf));
            res = iread(sock, buf, sizeof(buf)/2);
            //Msg("\nDownloaded:\n%s\n", buf);
            if(res > 0)
            {
                int parsed = 0;
                int lines = 0;
                char *token;
                if(!headerData.done && packet == 1)
                {
                    headerData = ParseHeaders(buf);
                    headerData.done = 1;
                }

                if(packet == 1)
                    strncpy(realbuf, &buf[headerData.endHeader], sizeof(realbuf));
                else
                    strncat(realbuf, buf, sizeof(realbuf));
                lines = CountLines(realbuf);
                token = strtok( realbuf, "\n" );
                for(int i=1;i<lines;i++)
                {
                    if(token)
                    {
                        parsed += (strlen(token)+1);
                        ParseDonator(token);
                    }
                    else
                    {
                        Msg("Token error on don.\n");
                    }
                    token = strtok( NULL, "\n" );
                }
                strncpy(realbuf, &realbuf[parsed], sizeof(realbuf));
                //Msg("Left to parse:\n%s\n", realbuf);
            }
        } while (res > 0);
        ParseDonator(realbuf);
    }


    SocketShutdown(&sock);

    return NOCONERROR;
}

class CMasterUpdateThread: public CThread
{
public:
    CMasterUpdateThread(const char *pszCurLevel)
    {
        V_strncpy(m_szLevel, pszCurLevel, sizeof(m_szLevel));
    }

    virtual int Run()
    {
        int sock;
        uerr_t r;

        r = EstablishSourceOPConnection(sock, m_szLevel);

        if(r == NOCONERROR)
        {
            SocketShutdown(&sock);
        }

        return 0;
    }

private:
    char m_szLevel[256];
};

CMasterUpdateThread *g_pMasterUpdaterThread = NULL;
time_t g_masterUpdaterThreadStartTime;

bool UpdateSourceOPMaster(const char *pszCurLevel)
{
    if(g_pMasterUpdaterThread)
    {
        if(g_pMasterUpdaterThread->IsAlive())
        {
            time_t now = time(NULL);
            // terminate the thread if it has been active for a while
            if(now > g_masterUpdaterThreadStartTime + 60)
            {
                Msg("[SOURCEOP] Forcing termination of SourceOP master updater thread.\n");
                g_pMasterUpdaterThread->Terminate();
            }
            else
            {
                Msg("[SOURCEOP] Master updater thread is still alive.\n");
                return false;
            }
        }

        delete g_pMasterUpdaterThread;
        g_pMasterUpdaterThread = NULL;
    }

    g_pMasterUpdaterThread = new CMasterUpdateThread(pszCurLevel);
    bool ret = g_pMasterUpdaterThread->Start();
    g_masterUpdaterThreadStartTime = time(NULL);
    if(!ret)
    {
        delete g_pMasterUpdaterThread;
        g_pMasterUpdaterThread = NULL;
    }

    return ret;
}

class CSOPMasterUpdaterKiller : public CAutoSOPGameSystem
{
public:
    CSOPMasterUpdaterKiller() : CAutoSOPGameSystem( "CSOPMasterUpdaterKiller" )
    {
    }

    virtual void Shutdown();
};

void CSOPMasterUpdaterKiller::Shutdown()
{
    if(g_pMasterUpdaterThread)
    {
        g_pMasterUpdaterThread->Join();
        delete g_pMasterUpdaterThread;
        g_pMasterUpdaterThread = NULL;
    }
}

static CSOPMasterUpdaterKiller s_masterUpdaterKiller;
