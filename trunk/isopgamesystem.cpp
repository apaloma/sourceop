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
// Purpose: Deals with singleton  
//
// $Revision: $
// $NoKeywords: $
//=============================================================================//

// HACK HACK HACK : BASEENTITY ACCESS
#define GAME_DLL
#include "fixdebug.h"
#include "cbase.h"
#include "isopgamesystem.h"
#include "datacache/imdlcache.h"
#include "utlvector.h"
#include "vprof.h"
#if defined( _X360 )
#include "xbox/xbox_console.h"
#endif

#include "tier0/memdbgon.h"

// Pointer to a member method of ISOPGameSystem
typedef void (ISOPGameSystem::*GameSystemFunc_t)();

// Pointer to a command member method
typedef int (ISOPGameSystem::*GameSystemCommandFunc_t)(CBaseEntity *pEntity, const CCommand &args);
typedef void (ISOPGameSystem::*GameSystemPlayerFunc_t)(edict_t *pEntity);
typedef void (ISOPGameSystem::*GameSystemConnectFunc_t)(bool *bAllowConnect, const char *pszName, const char *pszAddress, CSteamID steamID, const char *pszPassword, char *reject, int maxrejectlen);
typedef void (ISOPGameSystem::*GameSystemNetIDFunc_t)(edict_t *pEntity, const char *pszName, const char *pszNetworkID);

// Pointer to a member method of ISOPGameSystem
typedef void (ISOPGameSystemPerFrame::*PerFrameGameSystemFunc_t)();

// Used to invoke a method of all added Game systems in order
static void InvokeMethod( GameSystemFunc_t f, char const *timed = 0 );
static int InvokeCommandMethod( GameSystemCommandFunc_t f, CBaseEntity *pEntity, const CCommand &args, char const *timed = 0 );
static void InvokePlayerMethod( GameSystemPlayerFunc_t f, edict_t *pEntity, char const *timed = 0 );
static void InvokeConnectMethod( GameSystemConnectFunc_t f, bool *bAllowConnect, const char *pszName, const char *pszAddress, CSteamID steamID, const char *pszPassword, char *reject, int maxrejectlen, char const *timed = 0 );
static void InvokeNetIDMethod( GameSystemNetIDFunc_t f, edict_t *pEntity, const char *pszName, const char *pszNetworkID, char const *timed = 0 );
// Used to invoke a method of all added Game systems in reverse order
static void InvokeMethodReverseOrder( GameSystemFunc_t f );

// Used to invoke a method of all added Game systems in order
static void InvokePerFrameMethod( PerFrameGameSystemFunc_t f, char const *timed = 0 );

static bool s_bSOPSystemsInitted = false; 

// List of all installed Game systems
static CUtlVector<ISOPGameSystem*> s_SOPGameSystems( 0, 4 );
// List of all installed Game systems
static CUtlVector<ISOPGameSystemPerFrame*> s_SOPGameSystemsPerFrame( 0, 4 );

// The map name
static char* s_pMapName = 0;

static CBasePlayer *s_pRunCommandPlayer = NULL;
static CUserCmd *s_pRunCommandUserCmd = NULL;

//-----------------------------------------------------------------------------
// Auto-registration of game systems
//-----------------------------------------------------------------------------
static	CAutoSOPGameSystem *s_pSystemList = NULL;

CAutoSOPGameSystem::CAutoSOPGameSystem( char const *name ) :
	m_pszName( name )
{
	// If s_SOPGameSystems hasn't been initted yet, then add ourselves to the global list
	// because we don't know if the constructor for s_SOPGameSystems has happened yet.
	// Otherwise, we can add ourselves right into that list.
	if ( s_bSOPSystemsInitted )
	{
		Add( this );
	}
	else
	{
		m_pNext = s_pSystemList;
		s_pSystemList = this;
	}
}

static	CAutoSOPGameSystemPerFrame *s_pPerFrameSystemList = NULL;

//-----------------------------------------------------------------------------
// Purpose: This is a CAutoSOPGameSystem which also cares about the "per frame" hooks
//-----------------------------------------------------------------------------
CAutoSOPGameSystemPerFrame::CAutoSOPGameSystemPerFrame( char const *name ) :
	m_pszName( name )
{
	// If s_SOPGameSystems hasn't been initted yet, then add ourselves to the global list
	// because we don't know if the constructor for s_SOPGameSystems has happened yet.
	// Otherwise, we can add ourselves right into that list.
	if ( s_bSOPSystemsInitted )
	{
		Add( this );
	}
	else
	{
		m_pNext = s_pPerFrameSystemList;
		s_pPerFrameSystemList = this;
	}
}

//-----------------------------------------------------------------------------
// destructor, cleans up automagically....
//-----------------------------------------------------------------------------
ISOPGameSystem::~ISOPGameSystem()
{
	Remove( this );
}

//-----------------------------------------------------------------------------
// destructor, cleans up automagically....
//-----------------------------------------------------------------------------
ISOPGameSystemPerFrame::~ISOPGameSystemPerFrame()
{
	Remove( this );
}


//-----------------------------------------------------------------------------
// Adds a system to the list of systems to run
//-----------------------------------------------------------------------------
void ISOPGameSystem::Add( ISOPGameSystem* pSys )
{
	s_SOPGameSystems.AddToTail( pSys );
	if ( dynamic_cast< ISOPGameSystemPerFrame * >( pSys ) != NULL )
	{
		s_SOPGameSystemsPerFrame.AddToTail( static_cast< ISOPGameSystemPerFrame * >( pSys ) );
	}
}


//-----------------------------------------------------------------------------
// Removes a system from the list of systems to update
//-----------------------------------------------------------------------------
void ISOPGameSystem::Remove( ISOPGameSystem* pSys )
{
    // if this is the first item in the list, remove it
    // this prevents crash when radio makes a game system and then removes the system
    // all before the systems are initialized
    if(s_pSystemList == pSys)
    {
        s_pSystemList = s_pSystemList->m_pNext;
    }

	s_SOPGameSystems.FindAndRemove( pSys );
	if ( dynamic_cast< ISOPGameSystemPerFrame * >( pSys ) != NULL )
	{
		s_SOPGameSystemsPerFrame.FindAndRemove( static_cast< ISOPGameSystemPerFrame * >( pSys ) );
	}
}

//-----------------------------------------------------------------------------
// Removes *all* systems from the list of systems to update
//-----------------------------------------------------------------------------
void ISOPGameSystem::RemoveAll(  )
{
	s_SOPGameSystems.RemoveAll();
	s_SOPGameSystemsPerFrame.RemoveAll();
}


//-----------------------------------------------------------------------------
// Client systems can use this to get at the map name
//-----------------------------------------------------------------------------
char const*	ISOPGameSystem::MapName()
{
	return s_pMapName;
}

#ifndef CLIENT_DLL
CBasePlayer *ISOPGameSystem::RunCommandPlayer()
{
	return s_pRunCommandPlayer;
}

CUserCmd *ISOPGameSystem::RunCommandUserCmd()
{
	return s_pRunCommandUserCmd;
}
#endif

//-----------------------------------------------------------------------------
// Invokes methods on all installed game systems
//-----------------------------------------------------------------------------
bool ISOPGameSystem::InitAllSystems()
{
	int i;

	{
		// first add any auto systems to the end
		CAutoSOPGameSystem *pSystem = s_pSystemList;
		while ( pSystem )
		{
			if ( s_SOPGameSystems.Find( pSystem ) == s_SOPGameSystems.InvalidIndex() )
			{
				Add( pSystem );
			}
			else
			{
				DevWarning( 1, "AutoGameSystem already added to game system list!!!\n" );
			}
			pSystem = pSystem->m_pNext;
		}
		s_pSystemList = NULL;
	}

	{
		CAutoSOPGameSystemPerFrame *pSystem = s_pPerFrameSystemList;
		while ( pSystem )
		{
			if ( s_SOPGameSystems.Find( pSystem ) == s_SOPGameSystems.InvalidIndex() )
			{
				Add( pSystem );
			}
			else
			{
				DevWarning( 1, "AutoGameSystem already added to game system list!!!\n" );
			}

			pSystem = pSystem->m_pNext;
		}
		s_pSystemList = NULL;
	}
	// Now remember that we are initted so new CAutoSOPGameSystems will add themselves automatically.
	s_bSOPSystemsInitted = true;

	for ( i = 0; i < s_SOPGameSystems.Count(); ++i )
	{
		MDLCACHE_CRITICAL_SECTION();

		ISOPGameSystem *sys = s_SOPGameSystems[i];

#if defined( _X360 )
		char sz[128];
		Q_snprintf( sz, sizeof( sz ), "%s->Init():Start", sys->Name() );
		XBX_rTimeStampLog( Plat_FloatTime(), sz );
#endif
		bool valid = sys->Init();

#if defined( _X360 )
		Q_snprintf( sz, sizeof( sz ), "%s->Init():Finish", sys->Name() );
		XBX_rTimeStampLog( Plat_FloatTime(), sz );
#endif
		if ( !valid )
			return false;
	}

	return true;
}

void ISOPGameSystem::PostInitAllSystems( void )
{
	InvokeMethod( &ISOPGameSystem::PostInit, "PostInit" );
}

void ISOPGameSystem::ShutdownAllSystems()
{
	InvokeMethodReverseOrder( &ISOPGameSystem::Shutdown );
}

void ISOPGameSystem::LevelInitPreEntityAllSystems( char const* pMapName )
{
	// Store off the map name
	if ( s_pMapName )
	{
		delete[] s_pMapName;
	}

	int len = Q_strlen(pMapName) + 1;
	s_pMapName = new char [ len ];
	Q_strncpy( s_pMapName, pMapName, len );

	InvokeMethod( &ISOPGameSystem::LevelInitPreEntity, "LevelInitPreEntity" );
}

void ISOPGameSystem::LevelInitPostEntityAllSystems( void )
{
	InvokeMethod( &ISOPGameSystem::LevelInitPostEntity, "LevelInitPostEntity" );
}

void ISOPGameSystem::LevelShutdownPreEntityAllSystems()
{
	InvokeMethodReverseOrder( &ISOPGameSystem::LevelShutdownPreEntity );
}

void ISOPGameSystem::LevelShutdownPostEntityAllSystems()
{
	InvokeMethodReverseOrder( &ISOPGameSystem::LevelShutdownPostEntity );

	if ( s_pMapName )
	{
		delete[] s_pMapName;
		s_pMapName = 0;
	}
}

void ISOPGameSystem::ClientConnectPreAllSystems(bool *bAllowConnect, const char *pszName, const char *pszAddress, CSteamID steamID, const char *pszPassword, char *reject, int maxrejectlen)
{
    InvokeConnectMethod( &ISOPGameSystem::ClientConnectPre, bAllowConnect, pszName, pszAddress, steamID, pszPassword, reject, maxrejectlen );
}

void ISOPGameSystem::NetworkIDValidatedAllSystems(edict_t *pEntity, const char *pszName, const char *pszNetworkID)
{
    InvokeNetIDMethod( &ISOPGameSystem::NetworkIDValidated, pEntity, pszName, pszNetworkID );
}

void ISOPGameSystem::ClientDisconnectAllSystems(edict_t *pEntity)
{
    InvokePlayerMethod( &ISOPGameSystem::ClientDisconnect, pEntity );
}

void ISOPGameSystem::ClientSessionEndAllSystems(edict_t *pEntity)
{
    InvokePlayerMethod( &ISOPGameSystem::ClientSessionEnd, pEntity );
}

int ISOPGameSystem::ClientCommandAllSystems(CBaseEntity *pEntity, const CCommand &args)
{
    return InvokeCommandMethod( &ISOPGameSystem::ClientCommand, pEntity, args );
}

void ISOPGameSystem::OnSaveAllSystems()
{
	InvokeMethod( &ISOPGameSystem::OnSave );
}

void ISOPGameSystem::OnRestoreAllSystems()
{
	InvokeMethod( &ISOPGameSystem::OnRestore );
}

void ISOPGameSystem::SafeRemoveIfDesiredAllSystems()
{
	InvokeMethodReverseOrder( &ISOPGameSystem::SafeRemoveIfDesired );
}

#ifdef CLIENT_DLL

void ISOPGameSystem::PreRenderAllSystems()
{
	VPROF("ISOPGameSystem::PreRenderAllSystems");
	InvokePerFrameMethod( &ISOPGameSystemPerFrame::PreRender );
}

void ISOPGameSystem::UpdateAllSystems( float frametime )
{
	SafeRemoveIfDesiredAllSystems();

	int i;
	int c = s_SOPGameSystemsPerFrame.Count();
	for ( i = 0; i < c; ++i )
	{
		ISOPGameSystemPerFrame *sys = s_SOPGameSystemsPerFrame[i];
		MDLCACHE_CRITICAL_SECTION();
		sys->Update( frametime );
	}
}

void ISOPGameSystem::PostRenderAllSystems()
{
	InvokePerFrameMethod( &ISOPGameSystemPerFrame::PostRender );
}

#else

void ISOPGameSystem::FrameUpdatePreEntityThinkAllSystems()
{
	InvokePerFrameMethod( &ISOPGameSystemPerFrame::FrameUpdatePreEntityThink );
}

void ISOPGameSystem::FrameUpdatePostEntityThinkAllSystems()
{
	SafeRemoveIfDesiredAllSystems();

	InvokePerFrameMethod( &ISOPGameSystemPerFrame::FrameUpdatePostEntityThink );
}

void ISOPGameSystem::PreClientUpdateAllSystems() 
{
	InvokePerFrameMethod( &ISOPGameSystemPerFrame::PreClientUpdate );
}

#endif


//-----------------------------------------------------------------------------
// Invokes a method on all installed game systems in proper order
//-----------------------------------------------------------------------------
void InvokeMethod( GameSystemFunc_t f, char const *timed /*=0*/ )
{
	NOTE_UNUSED( timed );

	int i;
	int c = s_SOPGameSystems.Count();
	for ( i = 0; i < c ; ++i )
	{
		ISOPGameSystem *sys = s_SOPGameSystems[i];

		MDLCACHE_CRITICAL_SECTION();

		(sys->*f)();
	}
}

//-----------------------------------------------------------------------------
// Invokes a method on all installed game systems in proper order
//-----------------------------------------------------------------------------
int InvokeCommandMethod( GameSystemCommandFunc_t f, CBaseEntity *pEntity, const CCommand &args, char const *timed /*= 0*/ )
{
	NOTE_UNUSED( timed );

	int i, r = 0, ret = COMMAND_IGNORED;
	int c = s_SOPGameSystems.Count();
	for ( i = 0; i < c ; ++i )
	{
		ISOPGameSystem *sys = s_SOPGameSystems[i];

		MDLCACHE_CRITICAL_SECTION();

        r = (sys->*f)(pEntity, args);
        if(r > ret)
            ret = r;
		if(r == COMMAND_STOP)
            break;
	}
    return ret;
}

//-----------------------------------------------------------------------------
// Invokes a method on all installed game systems in proper order
//-----------------------------------------------------------------------------
void InvokePlayerMethod( GameSystemPlayerFunc_t f, edict_t *pEntity, char const *timed /*= 0*/ )
{
	NOTE_UNUSED( timed );

	int i;
	int c = s_SOPGameSystems.Count();
	for ( i = 0; i < c ; ++i )
	{
		ISOPGameSystem *sys = s_SOPGameSystems[i];

		MDLCACHE_CRITICAL_SECTION();

        (sys->*f)(pEntity);
	}
}

//-----------------------------------------------------------------------------
// Invokes a method on all installed game systems in proper order
//-----------------------------------------------------------------------------
void InvokeConnectMethod( GameSystemConnectFunc_t f, bool *bAllowConnect, const char *pszName, const char *pszAddress, CSteamID steamID, const char *pszPassword, char *reject, int maxrejectlen, char const *timed /*= 0*/ )
{
	NOTE_UNUSED( timed );

	int i;
	int c = s_SOPGameSystems.Count();
	for ( i = 0; i < c ; ++i )
	{
		ISOPGameSystem *sys = s_SOPGameSystems[i];

		MDLCACHE_CRITICAL_SECTION();

        (sys->*f)(bAllowConnect, pszName, pszAddress, steamID, pszPassword, reject, maxrejectlen);
	}
}

//-----------------------------------------------------------------------------
// Invokes a method on all installed game systems in proper order
//-----------------------------------------------------------------------------
void InvokeNetIDMethod( GameSystemNetIDFunc_t f, edict_t *pEntity, const char *pszName, const char *pszNetworkID, char const *timed /*= 0*/ )
{
	NOTE_UNUSED( timed );

	int i;
	int c = s_SOPGameSystems.Count();
	for ( i = 0; i < c ; ++i )
	{
		ISOPGameSystem *sys = s_SOPGameSystems[i];

		MDLCACHE_CRITICAL_SECTION();

        (sys->*f)(pEntity, pszName, pszNetworkID);
	}
}

//-----------------------------------------------------------------------------
// Invokes a method on all installed game systems in proper order
//-----------------------------------------------------------------------------
void InvokePerFrameMethod( PerFrameGameSystemFunc_t f, char const *timed /*=0*/ )
{
	NOTE_UNUSED( timed );

	int i;
	int c = s_SOPGameSystemsPerFrame.Count();
	for ( i = 0; i < c ; ++i )
	{
		ISOPGameSystemPerFrame *sys  = s_SOPGameSystemsPerFrame[i];
		MDLCACHE_CRITICAL_SECTION();
		(sys->*f)();
	}
}

//-----------------------------------------------------------------------------
// Invokes a method on all installed game systems in reverse order
//-----------------------------------------------------------------------------
void InvokeMethodReverseOrder( GameSystemFunc_t f )
{
	int i;
	int c = s_SOPGameSystems.Count();
	for ( i = c; --i >= 0; )
	{
		ISOPGameSystem *sys = s_SOPGameSystems[i];
		MDLCACHE_CRITICAL_SECTION();
		(sys->*f)();
	}
}
