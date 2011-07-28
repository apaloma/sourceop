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

#ifndef SPECIALITEMS_H
#define SPECIALITEMS_H

#include "sopsteamid.h"
#include "tf2items.h"
#include "isopgamesystem.h"

#include "tier0/memdbgon.h"

class CSpecialItemLoaderThread;
class CThreadMutexPthread;

class CSpecialItem
{
public:
    CEconItemView *m_pItem;
    int m_iIndex;
    char m_szPetName[128];
    bool m_bEquipped;
};

class CSpecialItemLoader : public CAutoSOPGameSystem
{
public:
    CSpecialItemLoader();
    ~CSpecialItemLoader();

    virtual bool Init();
    virtual void Shutdown();

    virtual void NetworkIDValidated(edict_t *pEntity, const char *pszName, const char *pszNetworkID);
    virtual void ClientSessionEnd(edict_t *pEntity);

private:
    void LoadItems(CSteamID steamid);
    void SaveItems(CSteamID steamid);

public:
    CSpecialItem *GetItemIterative(CSteamID steamid, unsigned int item);

private:
    CSpecialItemLoaderThread *m_pLoadThread;
    CUtlMap< uint64, CUtlVector<CSpecialItem *>*, unsigned int > m_allItems;
    CThreadMutexPthread *m_itemsMutex;
};

extern CSpecialItemLoader g_specialItemLoader;

#endif
