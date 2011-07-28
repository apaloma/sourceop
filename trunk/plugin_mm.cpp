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

#include "fixdebug.h"

#include "AdminOP.h"
#include "datacache/imdlcache.h"
#include "sourcehooks.h"
#include "plugin_mm.h"

CMetamodShim g_MetamodShim;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CMetamodShim, ISmmPlugin, METAMOD_PLAPI_NAME, g_MetamodShim );

ISmmAPI *g_SMAPI = NULL;
ISmmPlugin *g_PLAPI = NULL;

SH_DECL_HOOK6(IServerGameDLL, LevelInit, SH_NOATTRIB, 0, bool, char const *, char const *, char const *, char const *, bool, bool);
SH_DECL_HOOK3_void(IServerGameDLL, ServerActivate, SH_NOATTRIB, 0, edict_t *, int, int);
SH_DECL_HOOK1_void(IServerGameDLL, GameFrame, SH_NOATTRIB, 0, bool);
SH_DECL_HOOK0_void(IServerGameDLL, LevelShutdown, SH_NOATTRIB, 0);
SH_DECL_HOOK2_void(IServerGameClients, ClientActive, SH_NOATTRIB, 0, edict_t *, bool);
SH_DECL_HOOK1_void(IServerGameClients, ClientDisconnect, SH_NOATTRIB, 0, edict_t *);
SH_DECL_HOOK2_void(IServerGameClients, ClientPutInServer, SH_NOATTRIB, 0, edict_t *, char const *);
SH_DECL_HOOK1_void(IServerGameClients, SetCommandClient, SH_NOATTRIB, 0, int);
SH_DECL_HOOK1_void(IServerGameClients, ClientSettingsChanged, SH_NOATTRIB, 0, edict_t *);
SH_DECL_HOOK5(IServerGameClients, ClientConnect, SH_NOATTRIB, 0, bool, edict_t *, const char*, const char *, char *, int);

SH_DECL_HOOK2_void(IServerGameClients, NetworkIDValidated, SH_NOATTRIB, 0, const char *, const char *);
SH_DECL_HOOK2_void(IServerGameClients, ClientCommand, SH_NOATTRIB, 0, edict_t *, const CCommand &);
SH_DECL_HOOK5_void(IServerGameDLL, OnQueryCvarValueFinished, SH_NOATTRIB, 0, QueryCvarCookie_t, edict_t *, EQueryCvarValueStatus, const char *, const char *);

/** 
 * Something like this is needed to register cvars/CON_COMMANDs.
 */
class BaseAccessor : public IConCommandBaseAccessor
{
public:
    bool RegisterConCommandBase(ConCommandBase *pCommandBase)
    {
        /* Always call META_REGCVAR instead of going through the engine. */
        return META_REGCVAR(pCommandBase);
    }
} s_BaseAccessor;

class CMetamodOverrides : public IMetamodOverrides
{
    virtual void UnregisterConCommand(ConCommandBase *pConCmd)
    {
        META_UNREGCVAR(pConCmd);
    }
};

static CMetamodOverrides s_metamodOverrides;

bool CMetamodShim::Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
    CAdminOP::ColorMsg(CONCOLOR_CYAN, "[SOURCEOP] SourceOP loading via Metamod:Source...\n");

    PLUGIN_SAVEVARS();

    CreateInterfaceFn interfaceFactory = ismm->GetEngineFactory();
    CreateInterfaceFn gameServerFactory = ismm->GetServerFactory();
    if(interfaceFactory == NULL || gameServerFactory == NULL)
    {
        CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Failed to load because at least one factory was NULL.\n");
        return false;
    }

    pConCommandAccessor = &s_BaseAccessor;
    g_metamodOverrides = &s_metamodOverrides;

    GET_V_IFACE_CURRENT(GetEngineFactory, cvar, ICvar, CVAR_INTERFACE_VERSION);
    g_pCVar = cvar;
    GET_V_IFACE_CURRENT(GetEngineFactory, mdlcache, IMDLCache, MDLCACHE_INTERFACE_VERSION);

    g_ServerPlugin.MainLoad(interfaceFactory, gameServerFactory);

    SH_ADD_HOOK_MEMFUNC(IServerGameDLL, LevelInit, servergame, this, &CMetamodShim::Hook_LevelInit, false);
    SH_ADD_HOOK_MEMFUNC(IServerGameDLL, ServerActivate, servergame, this, &CMetamodShim::Hook_ServerActivate, true);
    SH_ADD_HOOK_MEMFUNC(IServerGameDLL, GameFrame, servergame, this, &CMetamodShim::Hook_GameFrame, true);
    SH_ADD_HOOK_MEMFUNC(IServerGameDLL, LevelShutdown, servergame, this, &CMetamodShim::Hook_LevelShutdown, false);
    SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientActive, servergameclients, this, &CMetamodShim::Hook_ClientActive, true);
    SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, servergameclients, this, &CMetamodShim::Hook_ClientDisconnect, true);
    SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientPutInServer, servergameclients, this, &CMetamodShim::Hook_ClientPutInServer, true);
    SH_ADD_HOOK_MEMFUNC(IServerGameClients, SetCommandClient, servergameclients, this, &CMetamodShim::Hook_SetCommandClient, true);
    SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientSettingsChanged, servergameclients, this, &CMetamodShim::Hook_ClientSettingsChanged, false);
    SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientConnect, servergameclients, this, &CMetamodShim::Hook_ClientConnect, false);
    SH_ADD_HOOK_MEMFUNC(IServerGameClients, NetworkIDValidated, servergameclients, this, &CMetamodShim::Hook_NetworkIDValidated, false);
    SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientCommand, servergameclients, this, &CMetamodShim::Hook_ClientCommand, false);
    SH_ADD_HOOK_MEMFUNC(IServerGameDLL, OnQueryCvarValueFinished, servergame, this, &CMetamodShim::Hook_OnQueryCvarValueFinished, false);

    return true;
}

bool CMetamodShim::Unload(char *error, size_t maxlen)
{
    SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, LevelInit, servergame, this, &CMetamodShim::Hook_LevelInit, false);
    SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, ServerActivate, servergame, this, &CMetamodShim::Hook_ServerActivate, true);
    SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, GameFrame, servergame, this, &CMetamodShim::Hook_GameFrame, true);
    SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, LevelShutdown, servergame, this, &CMetamodShim::Hook_LevelShutdown, false);
    SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientActive, servergameclients, this, &CMetamodShim::Hook_ClientActive, true);
    SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, servergameclients, this, &CMetamodShim::Hook_ClientDisconnect, true);
    SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientPutInServer, servergameclients, this, &CMetamodShim::Hook_ClientPutInServer, true);
    SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, SetCommandClient, servergameclients, this, &CMetamodShim::Hook_SetCommandClient, true);
    SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientSettingsChanged, servergameclients, this, &CMetamodShim::Hook_ClientSettingsChanged, false);
    SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientConnect, servergameclients, this, &CMetamodShim::Hook_ClientConnect, false);
    SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, NetworkIDValidated, servergameclients, this, &CMetamodShim::Hook_NetworkIDValidated, false);
    SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientCommand, servergameclients, this, &CMetamodShim::Hook_ClientCommand, false);
    SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, OnQueryCvarValueFinished, servergame, this, &CMetamodShim::Hook_OnQueryCvarValueFinished, false);

    g_ServerPlugin.Unload();
    return true;
}

void CMetamodShim::AllPluginsLoaded()
{
    /* This is where we'd do stuff that relies on the mod or other plugins 
     * being initialized (for example, cvars added and events registered).
     */
}

bool CMetamodShim::Pause(char *error, size_t maxlen)
{
    ////g_ServerPlugin.Pause();

    V_snprintf(error, maxlen, "SourceOP does not support pausing");
    return false;
}

bool CMetamodShim::Unpause(char *error, size_t maxlen)
{
    ////g_ServerPlugin.UnPause();
    return true;
}

void CMetamodShim::Hook_ServerActivate(edict_t *pEdictList, int edictCount, int clientMax)
{
    g_ServerPlugin.ServerActivate(pEdictList, edictCount, clientMax);
}

bool CMetamodShim::Hook_LevelInit(const char *pMapName, char const *pMapEntities, char const *pOldLevel, char const *pLandmarkName, bool loadGame, bool background)
{
    g_ServerPlugin.LevelInit(pMapName);

    return true;
}

void CMetamodShim::Hook_GameFrame(bool simulating)
{
    g_ServerPlugin.GameFrame(simulating);
}

void CMetamodShim::Hook_LevelShutdown()
{
    g_ServerPlugin.LevelShutdown();
}

void CMetamodShim::Hook_ClientActive(edict_t *pEntity, bool bLoadGame)
{
    g_ServerPlugin.ClientActive(pEntity);
}

void CMetamodShim::Hook_ClientDisconnect(edict_t *pEntity)
{
    g_ServerPlugin.ClientDisconnect(pEntity);
}

void CMetamodShim::Hook_ClientPutInServer(edict_t *pEntity, char const *playername)
{
    g_ServerPlugin.ClientPutInServer(pEntity, playername);
}

void CMetamodShim::Hook_SetCommandClient(int index)
{
    g_ServerPlugin.SetCommandClient(index);
}

void CMetamodShim::Hook_ClientSettingsChanged(edict_t *pEdict)
{
    g_ServerPlugin.ClientSettingsChanged(pEdict);
}

bool CMetamodShim::Hook_ClientConnect(edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen)
{
    bool bAllowConnect = true;
    g_ServerPlugin.ClientConnect(&bAllowConnect, pEntity, pszName, pszAddress, reject, maxrejectlen);

    return true;
}

void CMetamodShim::Hook_NetworkIDValidated( const char *pszUserName, const char *pszNetworkID )
{
    g_ServerPlugin.NetworkIDValidated(pszUserName, pszNetworkID);
}

void CMetamodShim::Hook_ClientCommand(edict_t *pEntity, const CCommand &args)
{
    PLUGIN_RESULT ret = g_ServerPlugin.ClientCommand(pEntity, args);
    if(ret == PLUGIN_STOP)
        RETURN_META(MRES_SUPERCEDE);
}

void CMetamodShim::Hook_OnQueryCvarValueFinished( QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue )
{
    g_ServerPlugin.OnQueryCvarValueFinished(iCookie, pPlayerEntity, eStatus, pCvarName, pCvarValue);
}

const char *CMetamodShim::GetLicense()
{
    return "GNU General Public License. Copyright © 2005-2011 Tony Paloma, Drunken F00l, SourceOP.com. Portions Copyright © Valve Corporation.";
}

const char *CMetamodShim::GetVersion()
{
    return SourceOPVerShort;
}

const char *CMetamodShim::GetDate()
{
    return __DATE__;
}

const char *CMetamodShim::GetLogTag()
{
    return "SOURCEOP";
}

const char *CMetamodShim::GetAuthor()
{
    return "SourceOP.com, Tony \"Drunken F00l\" Paloma";
}

const char *CMetamodShim::GetDescription()
{
    return "SourceOP plugin";
}

const char *CMetamodShim::GetName()
{
    return "SourceOP";
}

const char *CMetamodShim::GetURL()
{
    return "http://www.sourceop.com/";
}
