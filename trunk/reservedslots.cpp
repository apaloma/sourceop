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

#include <time.h>

#include "AdminOP.h"
#include "cvars.h"
#include "reservedslots.h"
#include "sopmysql.h"
#include "queuethread.h"

#include "tier0/memdbgon.h"

class CReservedSlotsThread: public CThread
{
public:
    CReservedSlotsThread(CReservedSlots *rslots)
    {
        m_pRslots = rslots;
    }

    virtual int Run()
    {
        g_pSOPMySQL->ThreadInit();

        bool hasQuery = false;
        LARGE_INTEGER liFrequency,liStart,liStop;
    
        if(QueryPerformanceFrequency(&liFrequency)) hasQuery = true;
        if(hasQuery) QueryPerformanceCounter(&liStart);
        m_pRslots->SaveReservedUsersIfLoaded();

        int count = 0;
        bool ret;
        do
        {
            if(count > 0)
            {
                Msg("[SOURCEOP] Retrying reserved slot load.\n");
            }
            ret = m_pRslots->LoadReservedUsers();
            count++;
        }
        while(ret == false && count < 3);

        if(hasQuery) QueryPerformanceCounter(&liStop);
        // TODO: Add thread-safe log print.
        //engine->LogPrint(UTIL_VarArgs("[SOURCEOP] \"Console<0><Console><Console>\" refreshed reserved slot users in \"%f\" seconds\n", hasQuery ? (double)(liStop.QuadPart - liStart.QuadPart) / (double) liFrequency.QuadPart : -1));

        g_pSOPMySQL->ThreadEnd();

        return 0;
    }

private:
    CReservedSlots *m_pRslots;
};

CReservedSlots::CReservedSlots()
{
    m_listMutex = new CThreadMutexPthread();
    m_pThread = NULL;
}

CReservedSlots::~CReservedSlots()
{
    if(m_pThread)
    {
        if(m_pThread->IsAlive())
        {
            Msg("[SOURCEOP] Waiting for reserved slot thread to complete.\n");
            m_pThread->Join(10000);
        }

        delete m_pThread;
        m_pThread = NULL;
    }

    delete m_listMutex;
}

void CReservedSlots::Reset()
{
    m_listMutex->Lock();
    rslotUsers.Purge();
    m_listMutex->Unlock();
    reservedslots = 0;
    rslotsused = 0;
    memset(usingslot, 0, sizeof(usingslot));
}

bool CReservedSlots::LoadReservedUsersInThread()
{
    if(FStrEq(mysql_database_user.GetString(), "") || FStrEq(mysql_database_name.GetString(), ""))
    {
        return true;
    }

    if(m_pThread)
    {
        if(m_pThread->IsAlive())
        {
            time_t now = time(NULL);
            // terminate the thread if it has been active for a while
            if(now > m_threadStartTime + 60)
            {
                Msg("[SOURCEOP] Forcing termination of reserved slot thread.\n");
                m_pThread->Terminate();
            }
            else
            {
                Msg("[SOURCEOP] Reserved slot thread is still alive.\n");
                return false;
            }
        }

        delete m_pThread;
        m_pThread = NULL;
    }

    m_pThread = new CReservedSlotsThread(this);
    bool ret = m_pThread->Start();
    m_threadStartTime = time(NULL);
    if(!ret)
    {
        delete m_pThread;
        m_pThread = NULL;
    }

    return ret;
}

bool CReservedSlots::LoadReservedUsers()
{
    // use this to temporarily store new rslot database so that we can lock the list for a minimal amount of time
    CUtlLinkedList <rslotip_t, unsigned int> tmpRslotUsers;

    int iDatabases = rslots_databases.GetInt();
    if(iDatabases & 1)
    {
        LoadReservedUsersFromDatabase(tmpRslotUsers, 1, mysql_database_addr.GetString(), mysql_database_user.GetString(), mysql_database_pass.GetString(), mysql_database_name.GetString());
    }

    if(iDatabases & 2)
    {
        LoadReservedUsersFromDatabase(tmpRslotUsers, 2, rslots_database2_addr.GetString(), rslots_database2_user.GetString(), rslots_database2_pass.GetString(), rslots_database2_name.GetString());
    }

    // erase loaded database and add the new one
    m_listMutex->Lock();
    rslotUsers.Purge();
    FOR_EACH_LL(tmpRslotUsers, i)
    {
        rslotUsers.AddToTail(tmpRslotUsers[i]);
    }
    m_listMutex->Unlock();

    return true;
}

bool CReservedSlots::LoadReservedUsersFromDatabase(CUtlLinkedList <rslotip_t, unsigned int> &tmpRslotUsers, unsigned short iDatabaseId, const char *pszAddr, const char *pszUser, const char *pszPass, const char *pszName)
{
    if(FStrEq(mysql_database_user.GetString(), "") || FStrEq(mysql_database_name.GetString(), ""))
    {
        return true;
    }

    CSOPMySQLConnection sql;
    if(!sql.Connect(pszAddr, pszUser, pszPass, pszName))
    {
        CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Failed to load reserved slot players from database %i.\n", iDatabaseId);
        return false;
    }

    sql.Query(UTIL_VarArgs("SELECT `id`, `steamid`, `profileid`, `name`, `ip1`, `ip2`, `ip3` FROM `rslots` WHERE `expiration` > UNIX_TIMESTAMP() AND `donated` >= %i AND `profileid` != 0", rslots_minimum_donation.GetInt()));

    CSOPMySQLRow row = sql.NextRow();
    while(!row.IsNull())
    {
        rslotip_t rslotuser;
        memset(&rslotuser, 0, sizeof(rslotuser));
        rslotuser.db = iDatabaseId;
        rslotuser.id = atoi(row.Column(0));
        rslotuser.updated = false;
        rslotuser.steamid = CSteamID(strtoull(row.Column(2), NULL, 10));
        if(rslotuser.steamid.GetAccountID() != 0)
        {
            strncpy(rslotuser.name, row.Column(3), sizeof(rslotuser.name));
            rslotuser.name[sizeof(rslotuser.name)-1] = '\0';
            memcpy(rslotuser.ip, row.Column(4), sizeof(rslotuser.ip));
            memcpy(rslotuser.ip2, row.Column(5), sizeof(rslotuser.ip2));
            memcpy(rslotuser.ip3, row.Column(6), sizeof(rslotuser.ip3));
            if(rslotuser.ip[0] == '\0') memset(rslotuser.ip, 0, sizeof(rslotuser.ip));
            if(rslotuser.ip2[0] == '\0') memset(rslotuser.ip2, 0, sizeof(rslotuser.ip2));
            if(rslotuser.ip3[0] == '\0') memset(rslotuser.ip3, 0, sizeof(rslotuser.ip3));
            tmpRslotUsers.AddToTail(rslotuser);
        }

        row = sql.NextRow();
    }

    sql.Close();

    return true;
}

void CReservedSlots::SaveReservedUsersIfLoaded()
{
    if(FStrEq(mysql_database_user.GetString(), "") || FStrEq(mysql_database_name.GetString(), ""))
    {
        return;
    }

    m_listMutex->Lock();
    if(!rslotUsers.Count())
    {
        m_listMutex->Unlock();
        return;
    }
    m_listMutex->Unlock();

    int iDatabases = rslots_databases.GetInt();
    if(iDatabases & 1)
    {
        SaveReservedUsersInDatabase(1, mysql_database_addr.GetString(), mysql_database_user.GetString(), mysql_database_pass.GetString(), mysql_database_name.GetString());
    }

    if(iDatabases & 2)
    {
        SaveReservedUsersInDatabase(2, rslots_database2_addr.GetString(), rslots_database2_user.GetString(), rslots_database2_pass.GetString(), rslots_database2_name.GetString());
    }
}

void CReservedSlots::SaveReservedUsersInDatabase(unsigned short iDatabaseId, const char *pszAddr, const char *pszUser, const char *pszPass, const char *pszName)
{
    CSOPMySQLConnection sql;
    if(!sql.Connect(pszAddr, pszUser, pszPass, pszName))
    {
        CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Failed to update reserved slot database %i.\n", iDatabaseId);
        return;
    }

    m_listMutex->Lock();
    for(unsigned int i = rslotUsers.Head(); i != rslotUsers.InvalidIndex(); i = rslotUsers.Next(i))
    {
        rslotip_t& rslotuser = rslotUsers.Element(i);
        if(rslotuser.updated && rslotuser.db == iDatabaseId)
        {
            char name[256];
            char ip[32];
            char ip2[32];
            char ip3[32];

            rslotuser.updated = false;
            sql.EscapeString(name, rslotuser.name, strlen(rslotuser.name));
            sql.EscapeString(ip, (char *)&rslotuser.ip[0], sizeof(rslotuser.ip));
            sql.EscapeString(ip2, (char *)&rslotuser.ip2[0], sizeof(rslotuser.ip2));
            sql.EscapeString(ip3, (char *)&rslotuser.ip3[0], sizeof(rslotuser.ip3));
            // update name, ips, and lastupdate
            sql.Query(UTIL_VarArgs("UPDATE `rslots` SET `name` = '%s', `ip1` = '%s', `ip2` = '%s', `ip3` = '%s', `lastupdate` = UNIX_TIMESTAMP() WHERE `profileid` = '%llu' LIMIT 1", name, ip, ip2, ip3, rslotuser.steamid.ConvertToUint64()));
        }
    }
    m_listMutex->Unlock();

    // remove expired users
    sql.Query("DELETE FROM `rslots` WHERE `expiration` <= UNIX_TIMESTAMP()");

    sql.Close();
}

void CReservedSlots::SetConnectingPlayerName(const char *pszName)
{
    strncpy(szConnectingPlayerName, pszName, sizeof(szConnectingPlayerName));
}

void CReservedSlots::SetConnectingPlayerSteamID(CSteamID szSteamID)
{
    connectingPlayerSteamID = szSteamID;
}

reserved CReservedSlots::UserHasReservedSlot(const netadr_t *net, const char *pszName, CSteamID *pSteamID)
{
    reserved ret = RESERVED_NO;
    const char *name = szConnectingPlayerName;
    CSteamID *steamid;
    if(pszName && pszName[0] != '\0')
        name = pszName;
    if(pSteamID)
        steamid = pSteamID;
    else
        steamid = &connectingPlayerSteamID;

    // bots aren't reserved
    if(net->ip[0] == '\0' && net->ip[1] == '\0' && net->ip[2] == '\0' && net->ip[3] == '\0') return RESERVED_NO;

    m_listMutex->Lock();
    for(unsigned int i = rslotUsers.Head(); i != rslotUsers.InvalidIndex(); i = rslotUsers.Next(i))
    {
        rslotip_t& rslotuser = rslotUsers.Element(i);
        if(steamid && rslotuser.steamid == *steamid)
        {
            if(ret < RESERVED_YES)
            {
                ret = RESERVED_YES;
                break;
            }
        }
        // if name is contained within registered name or vice versa and either of the ips match
        if((V_stristr(name, rslotuser.name) || V_stristr(rslotuser.name, name)) && 
           (!memcmp(rslotuser.ip, net->ip, sizeof(rslotuser.ip)) || !memcmp(rslotuser.ip2, net->ip, sizeof(rslotuser.ip2)) || !memcmp(rslotuser.ip3, net->ip, sizeof(rslotuser.ip3))))
        {
            if(ret < RESERVED_PROBABLE)
                ret = RESERVED_PROBABLE;
        }
        // if name matches and first two octets of ip match
        if(!strcmp(rslotuser.name, name) &&
           ( (rslotuser.ip[0] == net->ip[0] && rslotuser.ip[1] == net->ip[1] ) || (rslotuser.ip2[0] == net->ip[0] && rslotuser.ip2[1] == net->ip[1]) || (rslotuser.ip3[0] == net->ip[0] && rslotuser.ip3[1] == net->ip[1] ) ))
        {
            if(ret < RESERVED_POSSIBLE)
                ret = RESERVED_POSSIBLE;
        }
        // if name matches and there is no ip
        // this takes care of new donators connecting
        if( (rslotuser.ip[0] == '\0' && rslotuser.ip2[0] == '\0' && rslotuser.ip3[0] == '\0') && !strcmp(rslotuser.name, name) )
        {
            if(ret < RESERVED_PROBABLE)
                ret = RESERVED_PROBABLE;
        }
    }
    m_listMutex->Unlock();

    return ret;
}

// Update stored IP address if this player has a reserved slot.
// SteamID is not available upon connect. This function is called when it becomes available.
// Remove player's reserved slot if we incorrectly game him or her a slot.
void CReservedSlots::UpdatePlayerReservedSlot(int player, const char *pszUserName, CSteamID steamid)
{
    m_listMutex->Lock();
    for(unsigned int i = rslotUsers.Head(); i != rslotUsers.InvalidIndex(); i = rslotUsers.Next(i))
    {
        rslotip_t& rslotuser = rslotUsers.Element(i);
        if(rslotuser.steamid == steamid)
        {
            // update name
            if(stricmp(rslotuser.name, pszUserName))
            {
                strncpy(rslotuser.name, pszUserName, sizeof(rslotuser.name));
                // set user to be saved so that changes take effect
                rslotuser.updated = true;
            }
            // check if IP needs updating
            // TODO: This should probably bring older ips to the front
            if(memcmp(rslotuser.ip, pAdminOP.pAOPPlayers[player].ipocts, sizeof(rslotuser.ip)) &&
               memcmp(rslotuser.ip2, pAdminOP.pAOPPlayers[player].ipocts, sizeof(rslotuser.ip2)) &&
               memcmp(rslotuser.ip3, pAdminOP.pAOPPlayers[player].ipocts, sizeof(rslotuser.ip3)))
            {
                // update ip and push older ones back
                memcpy(rslotuser.ip3, rslotuser.ip2, sizeof(rslotuser.ip3));
                memcpy(rslotuser.ip2, rslotuser.ip, sizeof(rslotuser.ip2));
                memcpy(rslotuser.ip, pAdminOP.pAOPPlayers[player].ipocts, sizeof(rslotuser.ip));
                // set user to be saved so that changes take effect
                rslotuser.updated = true;
            }

            // adsd player slot if player should be using one but isn't
            if(!PlayerUsingReservedSlot(player))
            {
                pAdminOP.rslots->SetPlayerUsingReservedSlot(player, 1);
                pAdminOP.rslots->IncrementUsedReservedSlots();
            }

            m_listMutex->Unlock();
            return;
        }
    }

    m_listMutex->Unlock();

    // remove player reserved slot if the player shouldn't have it
    if(PlayerUsingReservedSlot(player))
    {
        SetPlayerUsingReservedSlot(player, 0);
        DecrementUsedReservedSlots();
    }
}

int CReservedSlots::ReservedSlots()
{
    return reservedslots;
}

int CReservedSlots::ReservedSlotsRemaining()
{
    return reservedslots - rslotsused;
}

void CReservedSlots::SetReservedSlots(int slots)
{
    reservedslots = slots;
}

void CReservedSlots::DecrementUsedReservedSlots()
{
    rslotsused--;
    CAdminOP::ColorMsg(CONCOLOR_DARKGRAY, "[SOURCEOP] %i reserved slots remaining.\n", pAdminOP.rslots->ReservedSlotsRemaining());
}

void CReservedSlots::IncrementUsedReservedSlots()
{
    rslotsused++;
    CAdminOP::ColorMsg(CONCOLOR_DARKGRAY, "[SOURCEOP] %i reserved slots remaining.\n", pAdminOP.rslots->ReservedSlotsRemaining());
}

int CReservedSlots::FindPlayerToKick()
{
    int player = -1;
    float maxpoints = -5000;
    for(int i = 0; i < gpGlobals->maxClients; i++)
    {
        // do not kick players with rslots
        if(PlayerUsingReservedSlot(i))
        {
            continue;
        }

        // do not kick players unconnected
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+i+1);
        if(!info || !info->IsConnected())
        {
            continue;
        }

        CAdminOPPlayer *pAOPPlayer = &(pAdminOP.pAOPPlayers[i]);

        // do not kick the SourceTV bot
        if(tv_enable->GetBool() && FStrEq(info->GetName(), tv_name->GetString()))
        {
            continue;
        }

        // likewise, do not kick the replay bot
        if(replay_enable && replay_enable->GetBool() && (FStrEq(info->GetName(), "replay") || pAOPPlayer->GetSteamID().ConvertToUint64() == 0))
        {
            continue;
        }

        float points = 0.0f;
        // more deaths per point = more likely to be kicked
        points += ((float)info->GetDeathCount() / (float)(pAOPPlayer->GetScore() > 0 ? pAOPPlayer->GetScore() : 1)) * 5;
        // give medics less chance to be kicked
        //if(pAdminOP.isTF2 && pAdminOP.pAOPPlayers[i].GetPlayerClass() == TF2_CLASS_MEDIC)
        //    points /= 3;
        if(points > maxpoints)
        {
            player = i;
            maxpoints = points;
        }
    }
    return player;
}

void CReservedSlots::SetPlayerUsingReservedSlot(int player, bool isusing)
{
    usingslot[player] = isusing;
}

bool CReservedSlots::PlayerUsingReservedSlot(int player)
{
    return usingslot[player];
}
