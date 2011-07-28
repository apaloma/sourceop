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

#ifndef RESERVEDSLOTS_H
#define RESERVEDSLOTS_H

#include "sopsteamid.h"

typedef struct rslotip_s
{
    unsigned short db;
    unsigned int id;
    bool updated;
    CSteamID steamid;
    unsigned char ip[4];
    unsigned char ip2[4];
    unsigned char ip3[4];
    char name[MAX_PLAYERNAME_LENGTH];
} rslotip_t;

enum reserved {
    RESERVED_NO = 0,
    RESERVED_POSSIBLE,
    RESERVED_PROBABLE,
    RESERVED_YES
};

class CReservedSlotsThread;
class CThreadMutexPthread;

class CReservedSlots
{
public:
    CReservedSlots();
    ~CReservedSlots();

    void Reset();
    bool LoadReservedUsersInThread();
    bool LoadReservedUsers();
    bool LoadReservedUsersFromDatabase(CUtlLinkedList <rslotip_t, unsigned int> &tmpRslotUsers, unsigned short iDatabaseId, const char *pszAddr, const char *pszUser, const char *pszPass, const char *pszName);
    void SaveReservedUsersIfLoaded();
private:
    void SaveReservedUsersInDatabase(unsigned short iDatabaseId, const char *pszAddr, const char *pszUser, const char *pszPass, const char *pszName);
public:
    void SetConnectingPlayerName(const char *pszName);
    void SetConnectingPlayerSteamID(CSteamID steamID);
    reserved UserHasReservedSlot(const netadr_t *net, const char *pszName, CSteamID *pSteamID = NULL);
    void UpdatePlayerReservedSlot(int player, const char *pszUserName, CSteamID steamid);
    int ReservedSlots();
    int ReservedSlotsRemaining();
    void SetReservedSlots(int slots);
    void DecrementUsedReservedSlots();
    void IncrementUsedReservedSlots();
    int FindPlayerToKick();

    void SetPlayerUsingReservedSlot(int player, bool isusing);
    bool PlayerUsingReservedSlot(int player);

private:
    CUtlLinkedList <rslotip_t, unsigned int> rslotUsers;
    int reservedslots;
    int rslotsused;
    CReservedSlotsThread *m_pThread;
    time_t m_threadStartTime;
    CThreadMutexPthread *m_listMutex;

    char szConnectingPlayerName[MAX_PLAYERNAME_LENGTH];
    CSteamID connectingPlayerSteamID;
    bool usingslot[MAX_AOP_PLAYERS];
};

#endif // RESERVEDSLOTS_H
