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

//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ISOPGAMESYSTEM_H
#define ISOPGAMESYSTEM_H
#ifdef _WIN32
#pragma once
#endif

#include "sopsteamid.h"

#include "tier0/memdbgon.h"

class CBaseEntity;
class CCommand;
class CBasePlayer;
class CUserCmd;

//-----------------------------------------------------------------------------
// Game systems are singleton objects in the client + server codebase responsible for
// various tasks
// The order in which the server systems appear in this list are the
// order in which they are initialized and updated. They are shut down in
// reverse order from which they are initialized.
//-----------------------------------------------------------------------------

enum clientcmdrets
{
    COMMAND_IGNORED = 0,
    COMMAND_HANDLED,
    COMMAND_STOP
};

// UNDONE: Do these need GameInit/GameShutdown as well?
// UNDONE: Remove the Pre/Post entity semantics and rely on system ordering?
// FIXME: Remove all ifdef CLIENT_DLL if we can...
abstract_class ISOPGameSystem
{
public:
	// GameSystems are expected to implement these methods.
	virtual char const *Name() = 0;

	// Init, shutdown
	// return true on success. false to abort DLL init!
	virtual bool Init() = 0;
	virtual void PostInit() = 0;
	virtual void Shutdown() = 0;

	// Level init, shutdown
	virtual void LevelInitPreEntity() = 0;
	// entities are created / spawned / precached here
	virtual void LevelInitPostEntity() = 0;

	virtual void LevelShutdownPreEntity() = 0;
	// Entities are deleted / released here...
	virtual void LevelShutdownPostEntity() = 0;
	// end of level shutdown
	
    virtual void ClientConnectPre(bool *bAllowConnect, const char *pszName, const char *pszAddress, CSteamID steamID, const char *pszPassword, char *reject, int maxrejectlen) = 0;
    virtual void NetworkIDValidated(edict_t *pEntity, const char *pszName, const char *pszNetworkID) = 0;
    virtual void ClientDisconnect(edict_t *pEntity) = 0;
    virtual void ClientSessionEnd(edict_t *pEntity) = 0;
    virtual int ClientCommand(CBaseEntity *pEntity, const CCommand &args) = 0;

	// Called during game save
	virtual void OnSave() = 0;

	// Called during game restore, after the local player has connected and entities have been fully restored
	virtual void OnRestore() = 0;

	// Called every frame. It's safe to remove an igamesystem from within this callback.
	virtual void SafeRemoveIfDesired() = 0;

	virtual bool	IsPerFrame() = 0;

	// destructor, cleans up automagically....
	virtual ~ISOPGameSystem();

	// Client systems can use this to get at the map name
	static char const*	MapName();

	// These methods are used to add and remove server systems from the
	// main server loop. The systems are invoked in the order in which
	// they are added.
	static void Add ( ISOPGameSystem* pSys );
	static void Remove ( ISOPGameSystem* pSys );
	static void RemoveAll (  );

	// These methods are used to initialize, shutdown, etc all systems
	static bool InitAllSystems();
	static void PostInitAllSystems();
	static void ShutdownAllSystems();
	static void LevelInitPreEntityAllSystems( char const* pMapName );
	static void LevelInitPostEntityAllSystems();
	static void LevelShutdownPreEntityAllSystems();
	static void LevelShutdownPostEntityAllSystems();

    static void ClientConnectPreAllSystems(bool *bAllowConnect, const char *pszName, const char *pszAddress, CSteamID steamID, const char *pszPassword, char *reject, int maxrejectlen);
    static void NetworkIDValidatedAllSystems(edict_t *pEntity, const char *pszName, const char *pszNetworkID);
    static void ClientDisconnectAllSystems(edict_t *pEntity);
    static void ClientSessionEndAllSystems(edict_t *pEntity);
    static int ClientCommandAllSystems(CBaseEntity *pEntity, const CCommand &args);

	static void OnSaveAllSystems();
	static void OnRestoreAllSystems();

	static void SafeRemoveIfDesiredAllSystems();

#ifdef CLIENT_DLL
	static void PreRenderAllSystems();
	static void UpdateAllSystems( float frametime );
	static void PostRenderAllSystems();
#else
	static void FrameUpdatePreEntityThinkAllSystems();
	static void FrameUpdatePostEntityThinkAllSystems();
	static void PreClientUpdateAllSystems();

	// Accessors for the above function
	static CBasePlayer *RunCommandPlayer();
	static CUserCmd *RunCommandUserCmd();
#endif
};

class ISOPGameSystemPerFrame : public ISOPGameSystem
{
public:
	// destructor, cleans up automagically....
	virtual ~ISOPGameSystemPerFrame();

#ifdef CLIENT_DLL
	// Called before rendering
	virtual void PreRender() = 0;

	// Gets called each frame
	virtual void Update( float frametime ) = 0;

	// Called after rendering
	virtual void PostRender() = 0;
#else
	// Called each frame before entities think
	virtual void FrameUpdatePreEntityThink() = 0;
	// called after entities think
	virtual void FrameUpdatePostEntityThink() = 0;
	virtual void PreClientUpdate() = 0;
#endif
};

// Quick and dirty server system for users who don't care about precise ordering
// and usually only want to implement a few of the callbacks
class CBaseSOPGameSystem : public ISOPGameSystem
{
public:

	virtual char const *Name() { return "unnamed"; }

	// Init, shutdown
	// return true on success. false to abort DLL init!
	virtual bool Init() { return true; }
	virtual void PostInit() {}
	virtual void Shutdown() {}

	// Level init, shutdown
	virtual void LevelInitPreEntity() {}
	virtual void LevelInitPostEntity() {}
	virtual void LevelShutdownPreEntity() {}
	virtual void LevelShutdownPostEntity() {}

    virtual void ClientConnectPre(bool *bAllowConnect, const char *pszName, const char *pszAddress, CSteamID steamID, const char *pszPassword, char *reject, int maxrejectlen) {}
    virtual void NetworkIDValidated(edict_t *pEntity, const char *pszName, const char *pszNetworkID) {}
    virtual void ClientDisconnect(edict_t *pEntity) {}
    virtual void ClientSessionEnd(edict_t *pEntity) {}
    virtual int ClientCommand(CBaseEntity *pEntity, const CCommand &args) { return 0; }

	virtual void OnSave() {}
	virtual void OnRestore() {}
	virtual void SafeRemoveIfDesired() {}

	virtual bool	IsPerFrame() { return false; }
private:

	// Prevent anyone derived from CBaseGameSystem from implementing these, they need
	//  to derive from CBaseGameSystemPerFrame below!!!
#ifdef CLIENT_DLL
	// Called before rendering
	virtual void PreRender() {}

	// Gets called each frame
	virtual void Update( float frametime ) {}

	// Called after rendering
	virtual void PostRender() {}
#else
	// Called each frame before entities think
	virtual void FrameUpdatePreEntityThink() {}
	// called after entities think
	virtual void FrameUpdatePostEntityThink() {}
	virtual void PreClientUpdate() {}
#endif
};

// Quick and dirty server system for users who don't care about precise ordering
// and usually only want to implement a few of the callbacks
class CBaseSOPGameSystemPerFrame : public ISOPGameSystemPerFrame
{
public:
	virtual char const *Name() { return "unnamed"; }

	// Init, shutdown
	// return true on success. false to abort DLL init!
	virtual bool Init() { return true; }
	virtual void PostInit() {}
	virtual void Shutdown() {}

	// Level init, shutdown
	virtual void LevelInitPreEntity() {}
	virtual void LevelInitPostEntity() {}
	virtual void LevelShutdownPreEntity() {}
	virtual void LevelShutdownPostEntity() {}

    virtual void ClientConnectPre(bool *bAllowConnect, const char *pszName, const char *pszAddress, CSteamID steamID, const char *pszPassword, char *reject, int maxrejectlen) = 0;
    virtual void NetworkIDValidated(edict_t *pEntity, const char *pszName, const char *pszNetworkID) = 0;
    virtual void ClientDisconnect(edict_t *pEntity) = 0;
    virtual void ClientSessionEnd(edict_t *pEntity) = 0;
    virtual int ClientCommand(CBaseEntity *pEntity, const CCommand &args) { return 0; }

	virtual void OnSave() {}
	virtual void OnRestore() {}
	virtual void SafeRemoveIfDesired() {}

	virtual bool	IsPerFrame() { return true; }

#ifdef CLIENT_DLL
	// Called before rendering
	virtual void PreRender () { }

	// Gets called each frame
	virtual void Update( float frametime ) { }

	// Called after rendering
	virtual void PostRender () { }
#else
	// Called each frame before entities think
	virtual void FrameUpdatePreEntityThink() { }
	// called after entities think
	virtual void FrameUpdatePostEntityThink() { }
	virtual void PreClientUpdate() { }
#endif
};

// Quick and dirty server system for users who don't care about precise ordering
// and usually only want to implement a few of the callbacks
class CAutoSOPGameSystem : public CBaseSOPGameSystem
{
public:
	CAutoSOPGameSystem( char const *name = NULL );	// hooks in at startup, no need to explicitly add
	CAutoSOPGameSystem		*m_pNext;

	virtual char const *Name() { return m_pszName ? m_pszName : "unnamed"; }

private:
	char const *m_pszName;
};

//-----------------------------------------------------------------------------
// Purpose: This is a CAutoGameSystem which also cares about the "per frame" hooks
//-----------------------------------------------------------------------------
class CAutoSOPGameSystemPerFrame : public CBaseSOPGameSystemPerFrame
{
public:
	CAutoSOPGameSystemPerFrame( char const *name = NULL );
	CAutoSOPGameSystemPerFrame *m_pNext;

	virtual char const *Name() { return m_pszName ? m_pszName : "unnamed"; }
	
private:
	char const *m_pszName;
};


//-----------------------------------------------------------------------------
// Purpose: This interface is here to add more hooks than IGameSystemPerFrame exposes,
// so we don't pollute it with hooks that only the tool cares about
//-----------------------------------------------------------------------------
/*class IToolFrameworkServer
{
public:
	virtual void PreSetupVisibility() = 0;
};*/

#endif // ISOPGAMESYSTEM_H
