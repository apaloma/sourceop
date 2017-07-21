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

//========== Copyright © 2005-2008, SourceOP, All rights reserved. ============//
//
// Purpose: Provides a mechanism for saving data about players to a MySQL
//          database.
//
// Date: 3/3/2008
//=============================================================================//

// HACK HACK HACK : BASEENTITY ACCESS
#define GAME_DLL
#include "fixdebug.h"
#include "cbase.h"
//#include "baseentity.h"
// HACK HACK HACK : BASEENTITY ACCESS

#include <time.h>

#include "AdminOP.h"
#include "cvars.h"
#include "vfuncs.h"
#include "isopgamesystem.h"
#include "queuethread.h"
#include "sopmysql.h"

#include "tier0/memdbgon.h"

// the structure that stores information about a connecting or disconnecting
// player.
typedef struct pdb_s
{
    bool        m_connect;
    int         m_iUid;
    char        m_playername[64];
    CSteamID    m_netid;
    char        m_renderedid[32];
    char        m_ip[32];
    char        m_mapname[256];
    int64       m_connectTime;
    int64       m_disconnectTime;
} pdb_t;

/**
 * Thread class that handles MySQL connection and queries.
 */
class CPlayerSaveThread : public CQueueThread<pdb_t>
{
public:
    CPlayerSaveThread()
    {
        SOPDLog("[PLRSAVETHREAD] CPlayerSaveThread()\n");
        m_SQL = NULL;
        SetName("CPlayerSaveThread");
    }

    void SavePlayer( bool connect, int userid, const char *pszName, CSteamID networkID, const char *pszIP, const char *pszMap, int64 connectTime, int64 disconnectTime )
    {
        if(FStrEq(mysql_database_user.GetString(), "") || FStrEq(mysql_database_name.GetString(), ""))
        {
            return;
        }

        SOPDLog("[PLRSAVETHREAD] SavePlayer()\n");
        pdb_t newEntry;
        const char *renderedid = networkID.Render();
        newEntry.m_connect = connect;
        newEntry.m_iUid = userid;
        V_strncpy(newEntry.m_playername, pszName, sizeof(newEntry.m_playername));
        newEntry.m_netid = networkID;
        V_strncpy(newEntry.m_renderedid, renderedid, sizeof(newEntry.m_renderedid));
        V_strncpy(newEntry.m_ip, pszIP, sizeof(newEntry.m_ip));
        V_strncpy(newEntry.m_mapname, pszMap, sizeof(newEntry.m_mapname));
        newEntry.m_connectTime = connectTime;
        newEntry.m_disconnectTime = disconnectTime;

        SOPDLog("[PLRSAVETHREAD] SavePlayer calling EnqueueItem\n");
        EnqueueItem(newEntry);
    }

    virtual bool BeginQueue()
    {
        SOPDLog("[PLRSAVETHREAD] BeginQueue()\n");
        m_SQL = new CSOPMySQLConnection();
        SOPDLog("[PLRSAVETHREAD] m_SQL created.\n");
        if(!m_SQL)
            return false;

        bool ret;
        //Msg("CPlayerSaveThread MySQL connect.\n");
        SOPDLog("[PLRSAVETHREAD] m_SQL->connect\n");
        ret = m_SQL->Connect(mysql_database_addr.GetString(), mysql_database_user.GetString(), mysql_database_pass.GetString(), mysql_database_name.GetString());
        if(!ret)
        {
            SOPDLog("[PLRSAVETHREAD] Failed to connect\n");
            CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Failed to connect player database MySQL server.\n");
        }
        return ret;
    }

    virtual bool ServiceItem(pdb_t const &item)
    {
        SOPDLog("[PLRSAVETHREAD] ServiceItem\n");
        if(m_SQL)
        {
            int ret;
            char ip[64];
            char mapname[256];
            char name[256];
            char steamid[64];
            char clientip[64];
            char query[1024];

            SOPDLog("[PLRSAVETHREAD] Escaping strings\n");
            m_SQL->EscapeString(ip, srv_ip->GetString(), strlen(srv_ip->GetString()));
            m_SQL->EscapeString(mapname, item.m_mapname, strlen(item.m_mapname));
            m_SQL->EscapeString(name, item.m_playername, strlen(item.m_playername));
            m_SQL->EscapeString(steamid, item.m_renderedid, strlen(item.m_renderedid));
            m_SQL->EscapeString(clientip, item.m_ip, strlen(item.m_ip));

            SOPDLog("[PLRSAVETHREAD] Query 1\n");
            // update players
            // TODO: Drop steamid when you set profileid to the unique key
            V_snprintf(query, sizeof(query), "INSERT INTO `players` (`steamid`, `profileid`, `name`, `ip`, `firstseen`, `lastseen`) "
                "VALUES ('%s', %llu, '%s', '%s', UNIX_TIMESTAMP(), UNIX_TIMESTAMP()) ON DUPLICATE KEY UPDATE "
                "`name`='%s', `ip`='%s', `lastseen`=UNIX_TIMESTAMP()", steamid, item.m_netid.ConvertToUint64(), name, clientip, name, clientip);
            ret = m_SQL->Query(query);
            if(ret != 0)
            {
                SOPDLog("[PLRSAVETHREAD] Query 1 failed, returning false.\n");
                return false;
            }

            SOPDLog("[PLRSAVETHREAD] Query 2\n");
            // update names
            // TODO: Drop steamid when you set profileid to the unique key
            V_snprintf(query, sizeof(query), "INSERT INTO `playernames` (`steamid`, `profileid`, `name`, `firstused`, `lastused`) "
                "VALUES ('%s', %llu, '%s', UNIX_TIMESTAMP(), UNIX_TIMESTAMP()) ON DUPLICATE KEY UPDATE "
                "`lastused`=UNIX_TIMESTAMP()", steamid, item.m_netid.ConvertToUint64(), name);
            ret = m_SQL->Query(query);
            if(ret != 0)
            {
                SOPDLog("[PLRSAVETHREAD] Query 2 failed, returning false.\n");
                return false;
            }

            // update 
            SOPDLog("[PLRSAVETHREAD] Query 3\n");
            V_snprintf(query, sizeof(query), "INSERT INTO `connectlog` (`connect`, `serverip`, `mapname`, `name`, `steamid`, `profileid`, `ip`, `timestamp`) "
                "VALUES ('%c', '%s:%i', '%s', '%s', '%s', %llu, '%s', UNIX_TIMESTAMP())", item.m_connect ? 'C' : 'D', ip, srv_hostport->GetInt(), mapname, name, steamid, item.m_netid.ConvertToUint64(), clientip);
            ret = m_SQL->Query(query);
            if(ret != 0)
            {
                SOPDLog("[PLRSAVETHREAD] Query 3 failed, returning false.\n");
                return false;
            }

            if(item.m_connect == false)
            {
                SOPDLog("[PLRSAVETHREAD] Query 4\n");
                V_snprintf(query, sizeof(query), "INSERT INTO `sessions` (`serverip`, `mapname`, `name`, `steamid`, `profileid`, `ip`, `connecttime`, `disconnecttime`, `sessiontime`, `timestamp`) "
                    "VALUES ('%s:%i', '%s', '%s', '%s', %llu, '%s', %lld, %lld, %lld, UNIX_TIMESTAMP())", ip, srv_hostport->GetInt(), mapname, name, steamid, item.m_netid.ConvertToUint64(), clientip, item.m_connectTime, item.m_disconnectTime, item.m_disconnectTime - item.m_connectTime);
                ret = m_SQL->Query(query);
                if(ret != 0)
                {
                    SOPDLog("[PLRSAVETHREAD] Query 4 failed, returning false.\n");
                    return false;
                }
            }
            return true;
        }
        else
        {
            SOPDLog("[PLRSAVETHREAD] m_SQL was null!\n");
        }
        return false;
    }

    virtual void EndQueue()
    {
        SOPDLog("[PLRSAVETHREAD] EndQueue().\n");
        if(m_SQL)
        {
            SOPDLog("[PLRSAVETHREAD] Deleting m_SQL.\n");
            delete m_SQL;
            SOPDLog("[PLRSAVETHREAD] Deleted m_SQL.\n");
            m_SQL = NULL;
        }
        //Msg("CPlayerSaveThread MySQL disconnect.\n");
    }

    virtual void ThreadInit()
    {
        g_pSOPMySQL->ThreadInit();
    }

    virtual void ThreadEnd()
    {
        g_pSOPMySQL->ThreadEnd();
    }

    void SaveQueue()
    {
        SOPDLog("[PLRSAVETHREAD] SaveQueue called\n");
        /*int count = QueueCount();

        Msg("SAVE QUEUE %i items\n", count);
        for(int i = 0; i < count; i++)
        {
            pdb_t elem = QueueElement(i);
            Msg(" - %i %s %s\n", elem.m_iUid, elem.m_playername, elem.m_netid);
        }*/
    }

private:
    CSOPMySQLConnection *m_SQL;
};

/**
 * Database class that listens for clients that successfully validate their
 * SteamID and disconnecting clients.
 */
class CSOPPlayerDatabase : public CAutoSOPGameSystem
{
public:
    CSOPPlayerDatabase() : CAutoSOPGameSystem( "CSOPPlayerDatabase" )
	{
	}

    virtual bool Init();
    virtual void Shutdown();

    virtual void NetworkIDValidated(edict_t *pEntity, const char *pszName, const char *pszNetworkID);
    virtual void ClientSessionEnd(edict_t *pEntity);

    //virtual void Test(bool connect, int userid, const char *pszName, const char *pszNetworkID, const char *pszIP, const char *pszMap);

    time_t GetCurrentTimestamp();

private:
    CPlayerSaveThread *m_PlayerSaveThread;
};

// create an instance of the player database system
static CSOPPlayerDatabase g_playerDatabase;

bool CSOPPlayerDatabase::Init()
{
    SOPDLog("[PLRDB] Init\n");
    m_PlayerSaveThread = new CPlayerSaveThread();
    return m_PlayerSaveThread != NULL;
}

void CSOPPlayerDatabase::Shutdown()
{
    SOPDLog("[PLRDB] Shutdown\n");
    m_PlayerSaveThread->JoinQueue();
    m_PlayerSaveThread->SaveQueue();
    delete m_PlayerSaveThread;
    m_PlayerSaveThread = NULL;
}

void CSOPPlayerDatabase::NetworkIDValidated(edict_t *pEntity, const char *pszName, const char *pszNetworkID)
{
    SOPDLog("[PLRDB] NetworkIDValidated\n");
    int entindex = pEntity-pAdminOP.GetEntityList();
    AssertMsg(entindex > 0 && entindex <= gpGlobals->maxClients, "[SOURCEOP] Got invalid entindex in CSOPPlayerDatabase::NetworkIDValidated\n");

    CSteamID networkID = pAdminOP.pAOPPlayers[entindex-1].GetSteamID();
    const char *pszIP = pAdminOP.pAOPPlayers[entindex-1].IP;

    if(sql_playerdatabase.GetBool())
        m_PlayerSaveThread->SavePlayer(true, entindex, pszName, networkID, pszIP, pAdminOP.CurrentMap(), 0, 0);
}

void CSOPPlayerDatabase::ClientSessionEnd(edict_t *pEntity)
{
    SOPDLog("[PLRDB] ClientSessionEnd\n");
    int entindex = pEntity-pAdminOP.GetEntityList();
    AssertMsg(entindex > 0 && entindex <= gpGlobals->maxClients, "[SOURCEOP] Got invalid entindex in CSOPPlayerDatabase::ClientSessionEnd\n");

    const char *pszName = pAdminOP.pAOPPlayers[entindex-1].GetJoinName();
    CSteamID networkID = pAdminOP.pAOPPlayers[entindex-1].GetSteamID();
    const char *pszIP = pAdminOP.pAOPPlayers[entindex-1].IP;

    IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pEntity);
    if(info && info->IsConnected())
    {
        pszName = info->GetName();
    }

    if(sql_playerdatabase.GetBool())
        m_PlayerSaveThread->SavePlayer(false, entindex, pszName, networkID, pszIP, pAdminOP.CurrentMap(), pAdminOP.pAOPPlayers[entindex-1].GetSessionStartTime(), time(NULL));
}

/*void CSOPPlayerDatabase::Test(bool connect, int userid, const char *pszName, const char *pszNetworkID, const char *pszIP, const char *pszMap)
{
    if(sql_playerdatabase.GetBool())
        m_PlayerSaveThread->SavePlayer(connect, userid, pszName, pszNetworkID, pszIP, pszMap);
}*/

time_t CSOPPlayerDatabase::GetCurrentTimestamp()
{
    time_t curtime;

    time(&curtime);
    return curtime;
}
