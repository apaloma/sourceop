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

#ifndef PLUGIN_MM
#define PLUGIN_MM

#include "mmsource/ISmmPlugin.h"

class CMetamodShim : public ISmmPlugin
{
public:
    bool Load(PluginId id, ISmmAPI *ismm, char *error, size_t maxlen, bool late);
    bool Unload(char *error, size_t maxlen);
    bool Pause(char *error, size_t maxlen);
    bool Unpause(char *error, size_t maxlen);
    void AllPluginsLoaded();
public:
    const char *GetAuthor();
    const char *GetName();
    const char *GetDescription();
    const char *GetURL();
    const char *GetLicense();
    const char *GetVersion();
    const char *GetDate();
    const char *GetLogTag();

public: //hooks
    void Hook_ServerActivate(edict_t *pEdictList, int edictCount, int clientMax);
    bool Hook_LevelInit(const char *pMapName,
        char const *pMapEntities,
        char const *pOldLevel,
        char const *pLandmarkName,
        bool loadGame,
        bool background);
    void Hook_GameFrame(bool simulating);
    void Hook_LevelShutdown(void);
    void Hook_ClientActive(edict_t *pEntity, bool bLoadGame);
    void Hook_ClientDisconnect(edict_t *pEntity);
    void Hook_ClientPutInServer(edict_t *pEntity, char const *playername);
    void Hook_SetCommandClient(int index);
    void Hook_ClientSettingsChanged(edict_t *pEdict);
    bool Hook_ClientConnect(edict_t *pEntity, 
        const char *pszName,
        const char *pszAddress,
        char *reject,
        int maxrejectlen);
    void Hook_NetworkIDValidated( const char *pszUserName, const char *pszNetworkID );
    void Hook_ClientCommand(edict_t *pEntity, const CCommand &args);
    void Hook_OnQueryCvarValueFinished( QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue );
};

extern CMetamodShim g_MetamodShim;

#endif // PLUGIN_MM
