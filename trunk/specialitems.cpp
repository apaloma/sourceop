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

#define GAME_DLL
#include "fixdebug.h"
#include "cbase.h"

#include "AdminOP.h"
#include "cvars.h"
#include "specialitems.h"
#include "queuethread.h"
#include "sopmysql.h"

#include "tier0/memdbgon.h"

#define GAME_HAS_ITEMS() (pAdminOP.isTF2)

enum ItemLoadType_t
{
    SPECIALITEM_LOAD,
    SPECIALITEM_SAVE
};

typedef struct specialitemload_s
{
    ItemLoadType_t loadtype;
    CSteamID steamid;
    CThreadMutexPthread *itemmutex;
    CUtlMap< uint64, CUtlVector<CSpecialItem *>*, unsigned int > *itemlist;
} specialitemload_t;

BEGIN_DATADESC_NO_BASE( CEconItemView )
END_DATADESC()

class CSpecialItemLoaderThread : public CQueueThread<specialitemload_t>
{
public:
    CSpecialItemLoaderThread()
    {
        m_SQL = NULL;
        SetName("CSpecialItemLoaderThread");
    }

    void LoadItems(CSteamID steamid, CThreadMutexPthread *itemmutex, CUtlMap< uint64, CUtlVector<CSpecialItem *>*, unsigned int > *itemlist)
    {
        if(FStrEq(mysql_database_user.GetString(), "") || FStrEq(mysql_database_name.GetString(), ""))
        {
            return;
        }

        specialitemload_t newEntry;

        newEntry.loadtype = SPECIALITEM_LOAD;
        newEntry.steamid = steamid;
        newEntry.itemmutex = itemmutex;
        newEntry.itemlist = itemlist;

        EnqueueItem(newEntry);
    }

    void SaveItems(CSteamID steamid, CThreadMutexPthread *itemmutex, CUtlMap< uint64, CUtlVector<CSpecialItem *>*, unsigned int > *itemlist)
    {
        if(FStrEq(mysql_database_user.GetString(), "") || FStrEq(mysql_database_name.GetString(), ""))
        {
            return;
        }

        specialitemload_t newEntry;

        newEntry.loadtype = SPECIALITEM_SAVE;
        newEntry.steamid = steamid;
        newEntry.itemmutex = itemmutex;
        newEntry.itemlist = itemlist;

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
            Msg("[SOURCEOP] Special item loader thread failed to connect MySQL server.\n");
        }
        return ret;
    }

    virtual bool ServiceItem(specialitemload_t const &item)
    {
        bool ret = false;
        if(m_SQL)
        {
            if(item.loadtype == SPECIALITEM_LOAD)
            {
                ret = LoadItem(item);
            }
            else if(item.loadtype == SPECIALITEM_SAVE)
            {
                ret = SaveItem(item);
            }
        }

        return ret;
    }

    virtual void EndQueue()
    {
        if(m_SQL)
        {
            delete m_SQL;
            m_SQL = NULL;
        }
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
    bool LoadItem(specialitemload_t const &item)
    {
        unsigned long long profileid = item.steamid.ConvertToUint64();
        char query[1024];

        V_snprintf(query, sizeof(query), "SELECT `id`, `itemdef`, `petname`, `equipped`, `level`, `quality` FROM `specialitems` WHERE `owner` = %llu AND `active` = 'Y' ORDER BY `id` ASC", profileid);
        //Msg("[ITEMDBG] %s\n", query);
        m_SQL->Query(query);

        item.itemmutex->Lock();

        unsigned int index = item.itemlist->Find(profileid);
        if(item.itemlist->IsValidIndex(index))
        {
            // this should be checked first in NetworkIDValidated
            // but this could still happen if two NetworkIDValidated occur for the same ID
            // while items still loading
            Msg("[ITEMDBG] Item list already exists for %s.\n", item.steamid.Render());
            item.itemmutex->Unlock();
            return true;
        }

        item.itemmutex->Unlock();

        CUtlVector<CSpecialItem *> *items = new CUtlVector<CSpecialItem *>;

        CSOPMySQLRow row = m_SQL->NextRow();
        while(!row.IsNull())
        {
            CSpecialItem *newspecialitem = new CSpecialItem;
            newspecialitem->m_iIndex = atoi(row.Column(0));
            V_strncpy(newspecialitem->m_szPetName, row.Column(2), sizeof(newspecialitem->m_szPetName));
            newspecialitem->m_bEquipped = atoi(row.Column(3)) != 0;
            CEconItemView *newitem = new CEconItemView;
            //Msg("[ITEMDBG] Test: %i %i   %i %i\n", sizeof(CScriptCreatedItem), sizeof(CScriptCreatedAttribute), ((char *)&newitem->m_attributes) - ((char *)newitem), ((char *)&newitem->m_bInitialized) - ((char *)newitem));
            newspecialitem->m_pItem = newitem;
            newitem->m_bInitialized = false;
            newitem->m_iEntityLevel = atoi(row.Column(4));
            newitem->m_iEntityQuality = atoi(row.Column(5));
            newitem->m_iGlobalIndex = 0;
            newitem->m_iGlobalIndexHigh = 0;
            newitem->m_iGlobalIndexLow = 0;
            newitem->m_iAccountID = item.steamid.GetAccountID();
            newitem->m_iItemDefinitionIndex = atoi(row.Column(1));
            newitem->m_iPosition = 0;
            memset(newitem->m_szAttributeDescription, 0, sizeof(newitem->m_szAttributeDescription));
            newitem->m_attributes.EnsureCapacity(16);

            //Msg("[ITEMDBG] Adding item %i  type %i level %i quality %i\n", newitem->m_iGlobalIndex, newitem->m_iItemDefinitionIndex, newitem->m_iEntityLevel, newitem->m_iEntityQuality);
            items->AddToTail(newspecialitem);

            row = m_SQL->NextRow();
        }

        m_SQL->FreeResult();

        // now go and get all the attributes
        for(int i = 0; i < items->Count(); i++)
        {
            CSpecialItem *curspecialitem = items->Element(i);
            CEconItemView *curitem = curspecialitem->m_pItem;

            V_snprintf(query, sizeof(query), "SELECT `attribdef`, `value` FROM `specialitems_attributes` WHERE `itemid` = %i", (int)curspecialitem->m_iIndex);
            //Msg("[ITEMDBG] %s\n", query);
            m_SQL->Query(query);

            CSOPMySQLRow row = m_SQL->NextRow();
            while(!row.IsNull())
            {
                CEconItemAttribute newAttrib;
                newAttrib.m_iAttribDef = atoi(row.Column(0));
                newAttrib.m_flVal = atof(row.Column(1));

                //Msg("[ITEMDBG] Adding attribute to item %i  def %i  val %f\n", (int)curitem->m_iGlobalIndex, newAttrib.m_iAttribDef, newAttrib.m_flVal);
                curitem->m_attributes.AddToTail(newAttrib);

                row = m_SQL->NextRow();
            }

            m_SQL->FreeResult();
        }

        item.itemmutex->Lock();
        index = item.itemlist->Find(profileid);
        if(item.itemlist->IsValidIndex(index))
        {
            // this should be checked first in NetworkIDValidated
            // but this could still happen if two NetworkIDValidated occur for the same ID
            // while items still loading
            Msg("[ITEMDBG] Item list already exists when inserting for %s.\n", item.steamid.Render());

            // free list
            for(int i = 0; i < items->Count(); i++)
            {
                delete items->Element(i)->m_pItem;
                delete items->Element(i);
            }
            delete items;
        }
        else
        {
            item.itemlist->Insert(profileid, items);
        }
        item.itemmutex->Unlock();
        
        return true;
    }

    bool SaveItem(specialitemload_t const &item)
    {
        unsigned long long profileid = item.steamid.ConvertToUint64();

        item.itemmutex->Lock();
        unsigned int index = item.itemlist->Find(profileid);
        if(item.itemlist->IsValidIndex(index))
        {
            char query[1024];
            char petname[512];

            CUtlVector<CSpecialItem *> *items = item.itemlist->Element(index);
            item.itemmutex->Unlock();

            //Msg("[ITEMDBG] Saving/deleting old item list.\n");
            for(int i = 0; i < items->Count(); i++)
            {
                //Msg("[ITEMDBG]  - Saving/deleting item %i\n", i);
                item.itemmutex->Lock();
                CSpecialItem *olditem = items->Element(i);

                m_SQL->EscapeString(petname, olditem->m_szPetName, strlen(olditem->m_szPetName));
                
                V_snprintf(query, sizeof(query), "UPDATE `specialitems` SET `petname` = '%s', `equipped` = %i WHERE `id` = %i LIMIT 1", petname, olditem->m_bEquipped, olditem->m_iIndex);
                item.itemmutex->Unlock();
                //Msg("[ITEMDBG] %s\n", query);
                m_SQL->Query(query);
                
                delete olditem->m_pItem;
                delete olditem;
            }

            item.itemmutex->Lock();
            item.itemlist->RemoveAt(index);
            item.itemmutex->Unlock();
            delete items;
        }
        else
        {
            item.itemmutex->Unlock();
        }

        return true;
    }
    CSOPMySQLConnection *m_SQL;
};

CSpecialItemLoader g_specialItemLoader;

CSpecialItemLoader::CSpecialItemLoader()
{
}

CSpecialItemLoader::~CSpecialItemLoader()
{
}

bool CSpecialItemLoader::Init()
{
    m_allItems.SetLessFunc(DefLessFunc(uint64));

    if(GAME_HAS_ITEMS())
    {
        m_itemsMutex = new CThreadMutexPthread();
        m_pLoadThread = new CSpecialItemLoaderThread();
        return m_pLoadThread != NULL;
    }
    else
    {
        return true;
    }
}

void CSpecialItemLoader::Shutdown()
{
    if(!GAME_HAS_ITEMS())
    {
        return;
    }

    m_pLoadThread->JoinQueue();
    m_pLoadThread->SaveQueue();

    FOR_EACH_MAP_FAST(m_allItems, list)
    {
        CUtlVector<CSpecialItem *> *itemlist = m_allItems[list];
        for(int i = 0; i < itemlist->Count(); i++)
        {
            CSpecialItem *curitem = itemlist->Element(i);
            delete curitem->m_pItem;
            delete curitem;
        }
        delete itemlist;
    }

    m_allItems.RemoveAll();
    
    delete m_itemsMutex;
    m_itemsMutex = NULL;
    delete m_pLoadThread;
    m_pLoadThread = NULL;
}

void CSpecialItemLoader::NetworkIDValidated(edict_t *pEntity, const char *pszName, const char *pszNetworkID)
{
    if(!GAME_HAS_ITEMS())
    {
        return;
    }

    int entindex = pEntity-pAdminOP.GetEntityList();
    AssertMsg(entindex > 0 && entindex <= gpGlobals->maxClients, "[SOURCEOP] Got invalid entindex in CSpecialItemLoader::NetworkIDValidated\n");

    CAdminOPPlayer *pAOPPlayer = &pAdminOP.pAOPPlayers[entindex-1];
    if(!pAOPPlayer->m_bItemsLoaded)
    {
        pAOPPlayer->m_bItemsLoaded = true;
        CSteamID steamid = pAOPPlayer->GetSteamID();
        this->LoadItems(steamid);
    }
}

void CSpecialItemLoader::ClientSessionEnd(edict_t *pEntity)
{
    if(!GAME_HAS_ITEMS())
    {
        return;
    }

    int entindex = pEntity-pAdminOP.GetEntityList();
    AssertMsg(entindex > 0 && entindex <= gpGlobals->maxClients, "[SOURCEOP] Got invalid entindex in CSpecialItemLoader::ClientSessionEnd\n");

    CAdminOPPlayer *pAOPPlayer = &pAdminOP.pAOPPlayers[entindex-1];
    CSteamID steamid = pAOPPlayer->GetSteamID();

    SaveItems(steamid);
}

void CSpecialItemLoader::LoadItems(CSteamID steamid)
{
    m_pLoadThread->LoadItems(steamid, m_itemsMutex, &m_allItems);
}

void CSpecialItemLoader::SaveItems(CSteamID steamid)
{
    m_itemsMutex->Lock();
    unsigned int index = m_allItems.Find(steamid.ConvertToUint64());
    if(m_allItems.IsValidIndex(index))
    {
        CUtlVector<CSpecialItem *> *itemlist = m_allItems.Element(index);
        if(itemlist->Count())
        {
            m_pLoadThread->SaveItems(steamid, m_itemsMutex, &m_allItems);
        }
        else
        {
            m_allItems.RemoveAt(index);
            delete itemlist;
        }
    }
    m_itemsMutex->Unlock();
}

CSpecialItem *CSpecialItemLoader::GetItemIterative(CSteamID steamid, unsigned int item)
{
    CSpecialItem *ret = NULL;

    if(GAME_HAS_ITEMS())
    {
        m_itemsMutex->Lock();
        unsigned int index = m_allItems.Find(steamid.ConvertToUint64());
        if(m_allItems.IsValidIndex(index))
        {
            CUtlVector<CSpecialItem *> *itemlist = m_allItems.Element(index);
            if(item < itemlist->Count())
            {
                ret = itemlist->Element(item);
            }
        }
        m_itemsMutex->Unlock();
    }

    return ret;
}
