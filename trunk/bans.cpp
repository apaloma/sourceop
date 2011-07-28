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
// Purpose: Provides a mechanism for saving data about player bans to a MySQL
//          database.
//
// Date: 6/9/2008
//=============================================================================//

#define GAME_DLL
#include "fixdebug.h"
#include "cbase.h"

#include <time.h>

#include "cvars.h"
#include "isopgamesystem.h"
#include "queuethread.h"
#include "sopmysql.h"
#include "sopsteamid.h"
#include "vfuncs.h"

#include "tier0/memdbgon.h"

enum BanType_t
{
    IP_BAN,
    STEAMID_BAN
};

#define PERM_BAN_TIME UINT_MAX

// the structure that stores information about a newly banned player.
typedef struct pban_s
{
    BanType_t       m_type;
    char            m_playername[64];
    CSteamID        m_netid;
    char            m_renderedid[32];
    char            m_ip[32];
    char            m_bannerplayername[64];
    char            m_bannernetid[32];
    char            m_mapname[256];
    time_t          m_expiration;
    char            m_reason[256];
    char            m_extra[256];
} pban_t;

// the structure that stores information about a ban.
typedef struct baninfo_s
{
    CSteamID        m_netid;
    char            m_ip[32];
    char            m_reason[256];
    time_t          m_expiration;
} baninfo_t;

/**
 * Stores ip addresses and steamids of banned players.
 */
class CBansStringPool : public CStringPool, public CAutoGameSystem
{
	virtual char const *Name() { return "CBansStringPool"; }

	virtual void Shutdown() 
	{
		FreeAll();
	}
};

static CBansStringPool g_BansStringPool;

/**
 * Thread class that handles MySQL connection and queries.
 */
class CBansThread : public CQueueThread<pban_t>
{
public:
    CBansThread()
    {
        m_SQL = NULL;
        SetName("CBansThread");
    }

    void BanPlayer( BanType_t type, const char *pszName, const char *pszBannerName, const char *pszBannerID, const char *pszMap, baninfo_t info, const char *pszExtra )
    {
        pban_t newEntry;
        char *renderedid = info.m_netid.Render();
        newEntry.m_type = type;
        V_strncpy(newEntry.m_playername, pszName, sizeof(newEntry.m_playername));
        newEntry.m_netid = info.m_netid;
        V_strncpy(newEntry.m_renderedid, renderedid, sizeof(newEntry.m_renderedid));
        V_strncpy(newEntry.m_ip, info.m_ip, sizeof(newEntry.m_ip));
        V_strncpy(newEntry.m_bannerplayername, pszBannerName, sizeof(newEntry.m_bannerplayername));
        V_strncpy(newEntry.m_bannernetid, pszBannerID, sizeof(newEntry.m_bannernetid));
        V_strncpy(newEntry.m_mapname, pszMap, sizeof(newEntry.m_mapname));
        newEntry.m_expiration = info.m_expiration;
        V_strncpy(newEntry.m_reason, info.m_reason, sizeof(newEntry.m_reason));
        if(pszExtra)
        {
            V_strncpy(newEntry.m_extra, pszExtra, sizeof(newEntry.m_extra));
        }
        else
        {
            newEntry.m_extra[0] = '\0';
        }

        EnqueueItem(newEntry);
    }

    virtual bool BeginQueue()
    {
        m_SQL = new CSOPMySQLConnection();
        if(!m_SQL)
            return false;

        bool ret;
        ret = m_SQL->Connect(mysql_database_addr.GetString(), mysql_database_user.GetString(), mysql_database_pass.GetString(), mysql_database_name.GetString());
        if(!ret)
        {
            //CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Player bans thread failed to connect MySQL server.\n");
            Msg("[SOURCEOP] Player bans thread failed to connect MySQL server.\n");
        }
        return ret;
    }

    virtual bool ServiceItem(pban_t const &item)
    {
        if(m_SQL)
        {
            int ret;
            char ip[64];
            char mapname[256];
            char name[256];
            char steamid[64];
            char bannername[256];
            char bannerid[64];
            char clientip[64];
            char reason[1024];
            char extra[2048];
            char query[4096];

            m_SQL->EscapeString(ip, srv_ip->GetString(), strlen(srv_ip->GetString()));
            m_SQL->EscapeString(mapname, item.m_mapname, strlen(item.m_mapname));
            m_SQL->EscapeString(name, item.m_playername, strlen(item.m_playername));
            m_SQL->EscapeString(steamid, item.m_renderedid, strlen(item.m_renderedid));
            m_SQL->EscapeString(bannername, item.m_bannerplayername, strlen(item.m_bannerplayername));
            m_SQL->EscapeString(bannerid, item.m_bannernetid, strlen(item.m_bannernetid));
            m_SQL->EscapeString(clientip, item.m_ip, strlen(item.m_ip));
            m_SQL->EscapeString(reason, item.m_reason, strlen(item.m_reason));
            m_SQL->EscapeString(extra, item.m_extra, strlen(item.m_extra));

            if(item.m_type == IP_BAN)
            {
                V_snprintf(query, sizeof(query), "INSERT INTO `ipbans` (`name`, `steamid`, `profileid`, `ip`, `bannername`, `bannerid`, `serverip`, `map`, `reason`, `timestamp`, `expiration`) "
                    "VALUES ('%s', '%s', %llu, '%s', '%s', '%s', '%s:%i', '%s', '%s', UNIX_TIMESTAMP(), '%lu')", name, steamid, item.m_netid.ConvertToUint64(), clientip, bannername, bannerid, ip, srv_hostport->GetInt(), mapname, reason, (unsigned long) item.m_expiration);
                ret = m_SQL->Query(query);
            }
            else if(item.m_type == STEAMID_BAN)
            {
                V_snprintf(query, sizeof(query), "INSERT INTO `steamidbans` (`name`, `steamid`, `profileid`, `ip`, `bannername`, `bannerid`, `serverip`, `map`, `reason`, `extrainfo`, `timestamp`, `expiration`) "
                    "VALUES ('%s', '%s', %llu, '%s', '%s', '%s', '%s:%i', '%s', '%s', '%s', UNIX_TIMESTAMP(), '%lu')", name, steamid, item.m_netid.ConvertToUint64(), clientip, bannername, bannerid, ip, srv_hostport->GetInt(), mapname, reason, extra, (unsigned long) item.m_expiration);
                ret = m_SQL->Query(query);
            }

            return ret == 0;
        }
        return false;
    }

    virtual void EndQueue()
    {
        if(m_SQL)
        {
            delete m_SQL;
            m_SQL = NULL;
        }
        //Msg("CBansThread MySQL disconnect.\n");
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

    }

private:
    CSOPMySQLConnection *m_SQL;
};

/**
 * Database class that listens for clients that successfully validate their
 * SteamID and disconnecting clients.
 */
class CSOPBanDatabase : public CAutoSOPGameSystem
{
public:
    CSOPBanDatabase() : CAutoSOPGameSystem( "CSOPBanDatabase" )
	{
	}

    virtual bool Init();
    virtual void Shutdown();

    virtual void LevelInitPreEntity();
    virtual void ClientConnectPre(bool *bAllowConnect, const char *pszName, const char *pszAddress, CSteamID steamID, const char *pszPassword, char *reject, int maxrejectlen);
    virtual void NetworkIDValidated(edict_t *pEntity, const char *pszName, const char *pszNetworkID);

    void BanPlayer(BanType_t type, const char *pszName, CSteamID networkID, const char *pszIP, const char *pszBannerName, const char *pszBannerID, const char *pszMap, unsigned int duration, const char *pszReason, const char *pszExtra = NULL);
    const char *IsPlayerBanned(const char *pszAddress, CSteamID steamID, int *banTimeRemaining);

    void PrintBans();

private:
    bool PopulateBanLists();
    void ResetBans();
    static time_t GetCurrentTimestamp();
    static const char *FormatTime(int t);

    void InsertIPBan(baninfo_t &info);
    void InsertSteamIDBan(baninfo_t &info);
    void RemoveIPBan(const char *pszAddress);
    void RemoveSteamIDBan(CSteamID steamID);

private:
    bool m_bPopulated;
    CUtlMap<const char *, baninfo_t> m_IPBans;
    CUtlMap<uint64, baninfo_t> m_SteamIDBans;
    CBansThread *m_BansThread;
};

// create an instance of the player bans system
static CSOPBanDatabase g_banDatabase;

bool CSOPBanDatabase::Init()
{
    m_bPopulated = false;
    m_IPBans.SetLessFunc(StringLessThan);
    m_SteamIDBans.SetLessFunc(DefLessFunc(uint64));

    m_BansThread = new CBansThread();
    return m_BansThread != NULL;
}

void CSOPBanDatabase::Shutdown()
{
    m_BansThread->JoinQueue();
    m_BansThread->SaveQueue();
    delete m_BansThread;
    m_BansThread = NULL;
}

void CSOPBanDatabase::LevelInitPreEntity()
{
    int count = 0;
    bool ret;
    do
    {
        if(count > 0)
        {
            Msg("[SOURCEOP] Retrying ban list load.\n");
        }
        ret = PopulateBanLists();
        count++;
    }
    while(ret == false && count < 3);
}

void CSOPBanDatabase::ClientConnectPre(bool *bAllowConnect, const char *pszName, const char *pszAddress, CSteamID steamID, const char *pszPassword, char *reject, int maxrejectlen)
{
    int banDuration;
    const char *bannedMsg = IsPlayerBanned(pszAddress, steamID, &banDuration);

    if(bannedMsg)
    {
        Msg("[SOURCEOP] Rejecting player \"%s\" because player is banned.\n", pszName);
        *bAllowConnect = false;
        if(banDuration == PERM_BAN_TIME)
        {
            V_snprintf(reject, maxrejectlen, "Permanently banned:\n%s", bannedMsg);
        }
        else
        {
            V_snprintf(reject, maxrejectlen, "Temporarily banned:\n%s\nRemaining: %s", bannedMsg, FormatTime(banDuration));
        }
    }
}

void CSOPBanDatabase::NetworkIDValidated(edict_t *pEntity, const char *pszName, const char *pszNetworkID)
{
    const CSteamID *steamidptr = engine->GetClientSteamID(pEntity);
    CSteamID steamid;
    if(steamidptr)
    {
        steamid = CSteamID(steamidptr->ConvertToUint64());
    }
    else
    {
        steamid = CSteamID();
    }

    const char *bannedMsg = IsPlayerBanned(NULL, steamid, NULL);

    if(bannedMsg)
    {
        engine->ServerCommand(UTIL_VarArgs("kickid %s %s\n", pszNetworkID, bannedMsg));
    }
}

void CSOPBanDatabase::BanPlayer(BanType_t type, const char *pszName, CSteamID networkID, const char *pszIP, const char *pszBannerName, const char *pszBannerID, const char *pszMap, unsigned int duration, const char *pszReason, const char *pszExtra)
{
    // don't do anything if MySQL bans are disabled
    if(!bans_mysql.GetBool())
    {
        Msg("[SOURCEOP] BanPlayer called but MySQL bans disabled.\n");
        return;
    }

    baninfo_t info;
    memset(&info, 0, sizeof(info));

    V_strncpy(info.m_reason, pszReason, sizeof(info.m_reason));
    if(duration)
    {
        info.m_expiration = GetCurrentTimestamp() + (duration * 60);
    }
    else
    {
        info.m_expiration = PERM_BAN_TIME;
    }
    V_strncpy(info.m_ip, pszIP, sizeof(info.m_ip));
    info.m_netid = networkID;

    if(type == IP_BAN)
    {
        RemoveIPBan(info.m_ip);
        InsertIPBan(info);
    }
    else if(type == STEAMID_BAN)
    {
        RemoveSteamIDBan(info.m_netid);
        InsertSteamIDBan(info);
    }

    m_BansThread->BanPlayer(type, pszName, pszBannerName, pszBannerID, pszMap, info, pszExtra);
}

const char *CSOPBanDatabase::IsPlayerBanned(const char *pszAddress, CSteamID steamID, int *banTimeRemaining)
{
    static char bannedReason[256];
    if(bans_mysql.GetBool())
    {
        if(pszAddress)
        {
            unsigned short index = m_IPBans.Find(pszAddress);
            if(m_IPBans.IsValidIndex(index))
            {
                baninfo_t info = m_IPBans[index];
                if(info.m_expiration == PERM_BAN_TIME || info.m_expiration > GetCurrentTimestamp())
                {
                    if(banTimeRemaining)
                    {
                        if(info.m_expiration == PERM_BAN_TIME)
                            *banTimeRemaining = PERM_BAN_TIME;
                        else
                            *banTimeRemaining = info.m_expiration - GetCurrentTimestamp();
                    }
                    V_strncpy(bannedReason, info.m_reason, sizeof(bannedReason));
                    return bannedReason;
                }
            }
        }
        uint64 profileid = steamID.ConvertToUint64();
        if(profileid)
        {
            unsigned short index = m_SteamIDBans.Find(profileid);
            if(m_SteamIDBans.IsValidIndex(index))
            {
                baninfo_t info = m_SteamIDBans[index];
                if(info.m_expiration == PERM_BAN_TIME || info.m_expiration > GetCurrentTimestamp())
                {
                    if(banTimeRemaining)
                    {
                        if(info.m_expiration == PERM_BAN_TIME)
                            *banTimeRemaining = PERM_BAN_TIME;
                        else
                            *banTimeRemaining = info.m_expiration - GetCurrentTimestamp();
                    }
                    V_strncpy(bannedReason, info.m_reason, sizeof(bannedReason));
                    return bannedReason;
                }
            }
        }
    }

    return NULL;
}

void CSOPBanDatabase::PrintBans()
{
    time_t curtime = GetCurrentTimestamp();

    Msg("IP Bans:\n");
    Msg("IP              Time Remaining   Reason\n");
    Msg("------------------------------------------------------------\n");
    FOR_EACH_MAP_FAST(m_IPBans, i)
    {
        baninfo_t info = m_IPBans[i];
        if(info.m_expiration == PERM_BAN_TIME)
        {
            Msg("%-15s %-16s %s\n", info.m_ip, "Permanent ban", info.m_reason);
        }
        else if(info.m_expiration > curtime)
        {
            int remaining = info.m_expiration - curtime;

            Msg("%-15s %16s %s\n", info.m_ip, FormatTime(remaining), info.m_reason);
        }
    }
    Msg("\n");

    Msg("SteamID Bans:\n");
    Msg("SteamID              Time Remaining   Reason\n");
    Msg("------------------------------------------------------------\n");
    FOR_EACH_MAP_FAST(m_SteamIDBans, i)
    {
        baninfo_t info = m_SteamIDBans[i];
        if(info.m_expiration == PERM_BAN_TIME)
        {
            Msg("%-20s %-16s %s\n", info.m_netid.Render(), "Permanent ban", info.m_reason);
        }
        else if(info.m_expiration > curtime)
        {
            int remaining = info.m_expiration - curtime;

            Msg("%-20s %16s %s\n", info.m_netid.Render(), FormatTime(remaining), info.m_reason);
        }
    }
}

bool CSOPBanDatabase::PopulateBanLists()
{
    // empty ban list and don't load from database if mysql bans are disabled
    if(!bans_mysql.GetBool())
    {
        ResetBans();
        return true;
    }

    CSOPMySQLConnection sql;
    if(!sql.Connect(mysql_database_addr.GetString(), mysql_database_user.GetString(), mysql_database_pass.GetString(), mysql_database_name.GetString()))
    {
        Msg("[SOURCEOP] Failed to load bans.\n");
        return false;
    }

    ResetBans();

    // load ip bans
    sql.Query(UTIL_VarArgs("SELECT `ip`, `reason`, `expiration` FROM `ipbans` WHERE `expiration` > UNIX_TIMESTAMP() AND `banenable` = 'Y'"));

    CSOPMySQLRow row = sql.NextRow();
    while(!row.IsNull())
    {
        baninfo_t banned;
        memset(&banned, 0, sizeof(banned));

        V_strncpy(banned.m_ip, row.Column(0), sizeof(banned.m_ip));
        V_strncpy(banned.m_reason, row.Column(1), sizeof(banned.m_reason));
        banned.m_expiration = strtoul(row.Column(2), NULL, 10);

        InsertIPBan(banned);

        row = sql.NextRow();
    }
    sql.FreeResult();


    // load steamid bans
    sql.Query(UTIL_VarArgs("SELECT `profileid`, `reason`, `expiration` FROM `steamidbans` WHERE `expiration` > UNIX_TIMESTAMP() AND `banenable` = 'Y'"));

    row = sql.NextRow();
    while(!row.IsNull())
    {
        baninfo_t banned;
        memset(&banned, 0, sizeof(banned));

        uint64 profileid = strtoull(row.Column(0), NULL, 10);
        banned.m_netid = CSteamID(profileid);
        V_strncpy(banned.m_reason, row.Column(1), sizeof(banned.m_reason));
        banned.m_expiration = strtoul(row.Column(2), NULL, 10);

        InsertSteamIDBan(banned);

        row = sql.NextRow();
    }
    sql.FreeResult();

    sql.Close();

    m_bPopulated = true;
    return true;
}

void CSOPBanDatabase::ResetBans()
{
    m_bPopulated = false;

    m_IPBans.RemoveAll();
    m_SteamIDBans.RemoveAll();
}

time_t CSOPBanDatabase::GetCurrentTimestamp()
{
    time_t curtime;

    time(&curtime);
    return curtime;
}

const char *CSOPBanDatabase::FormatTime(int t)
{
    static char ret[256];
    int days = t;
    int hours = t;
    int minutes = t;
    int seconds = t;

    days /= 86400;
    hours /= 3600;
    hours -= (days*24);
    minutes /= 60;
    minutes -= (hours*60);
    minutes -= (days*1440);
    seconds %= 60;

    V_snprintf(ret, sizeof(ret), "%dd:%02dh:%02dm:%02ds", days, hours, minutes, seconds);
    return ret;
}

void CSOPBanDatabase::InsertIPBan(baninfo_t &info)
{
    m_IPBans.Insert(g_BansStringPool.Allocate(info.m_ip), info);
}

void CSOPBanDatabase::InsertSteamIDBan(baninfo_t &info)
{
    m_SteamIDBans.Insert(info.m_netid.ConvertToUint64(), info);
}

void CSOPBanDatabase::RemoveIPBan(const char *pszAddress)
{
    m_IPBans.Remove(pszAddress);
}

void CSOPBanDatabase::RemoveSteamIDBan(CSteamID steamID)
{
    m_SteamIDBans.Remove(steamID.ConvertToUint64());
}

void UTIL_BanPlayerByID(const char *pszName, int playeruserid, void *pBaseClient, CSteamID networkID, const char *pszIP, const char *pszBannerName, const char *pszBannerID, const char *pszMap, unsigned int duration, const char *pszReason, const char *pszExtra)
{
    if(bans_mysql.GetBool())
    {
        g_banDatabase.BanPlayer(STEAMID_BAN, pszName, networkID, pszIP, pszBannerName, pszBannerID, pszMap, duration, pszReason, pszExtra);
    }
    if(bans_bancfg.GetBool())
    {
        engine->ServerCommand(UTIL_VarArgs("banid %i %s\n", time, networkID.Render()));
        engine->ServerCommand("writeid\n");
    }
    if(pBaseClient)
    {
        if(duration)
        {
            VFuncs::Disconnect(pBaseClient, "Temporarily banned: %s", pszReason);
        }
        else
        {
            VFuncs::Disconnect(pBaseClient, "Permanently banned: %s", pszReason);
        }
    }
    else
    {
        engine->ServerCommand(UTIL_VarArgs("kickid %i Banned: %s\n", playeruserid, pszReason));
    }
}

void UTIL_BanPlayerByIP(const char *pszName, int playeruserid, void *pBaseClient, CSteamID networkID, const char *pszIP, const char *pszBannerName, const char *pszBannerID, const char *pszMap, unsigned int duration, const char *pszReason, const char *pszExtra)
{
    if(bans_mysql.GetBool())
    {
        g_banDatabase.BanPlayer(IP_BAN, pszName, networkID, pszIP, pszBannerName, pszBannerID, pszMap, duration, pszReason, pszExtra);
    }
    if(bans_bancfg.GetBool())
    {
        engine->ServerCommand(UTIL_VarArgs("addip %i %s\n", time, pszIP));
        engine->ServerCommand("writeip\n");
    }
    if(pBaseClient)
    {
        if(duration)
        {
            VFuncs::Disconnect(pBaseClient, "Temporarily banned: %s", pszReason);
        }
        else
        {
            VFuncs::Disconnect(pBaseClient, "Permanently banned: %s", pszReason);
        }
    }
    else
    {
        engine->ServerCommand(UTIL_VarArgs("kickid %i Banned: %s\n", playeruserid, pszReason));
    }
}

CON_COMMAND( DF_bans_list, "Prints out all bans." )
{
    if(UTIL_IsCommandIssuedByServerAdmin())
    {
        if(bans_mysql.GetBool())
        {
            g_banDatabase.PrintBans();
        }
        else
        {
            Msg("[SOURCEOP] MySQL bans are disabled.\n");
        }
    }
}
