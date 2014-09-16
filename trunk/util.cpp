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
#ifdef _L4D_PLUGIN
#include "convar_l4d.h"
#endif
#include "cbase.h"
#include "baseentity.h"
// HACK HACK HACK : BASEENTITY ACCESS

#include <stdio.h>
#include <ctype.h>

#include "AdminOP.h"

#include "inetchannelinfo.h"
#include "vphysics_interface.h"
#include "effect_dispatch_data.h"
#include "toolframework/itoolentity.h"

#include "adminopplayer.h"
#include "recipientfilter.h"
#include "bitbuf.h"
#include "cvars.h"

#include "mempatcher.h"
#include "vfuncs.h"

#include "tier0/memdbgon.h"

IEntityFactoryDictionary *g_entityFactoryDictionary = NULL;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *filename - 
//          *pLength - 
// Output : byte
//-----------------------------------------------------------------------------
byte *UTIL_LoadFileForMe( const char *filename, int *pLength )
{
    byte *buffer;

    FileHandle_t file;
    file = filesystem->Open( filename, "rb" );
    if ( FILESYSTEM_INVALID_HANDLE == file )
    {
        if ( pLength ) *pLength = 0;
        return NULL;
    }

    int size = filesystem->Size( file );
    buffer = new byte[ size + 1 ];
    if ( !buffer )
    {
        Warning( "UTIL_LoadFileForMe:  Couldn't allocate buffer of size %i for file %s\n", size + 1, filename );
        filesystem->Close( file );
        return NULL;
    }
    filesystem->Read( buffer, size, file );
    filesystem->Close( file );

    // Ensure null terminator
    buffer[ size ] =0;

    if ( pLength )
    {
        *pLength = size;
    }

    return buffer;
}

//-----------------------------------------------------------------------------
// Simple trace filter
//-----------------------------------------------------------------------------
CTraceFilterSimple::CTraceFilterSimple( const IHandleEntity *passedict, int collisionGroup )
{
    m_pPassEnt = passedict;
    m_collisionGroup = collisionGroup;
}

//-----------------------------------------------------------------------------
// The trace filter!
//-----------------------------------------------------------------------------
bool CTraceFilterSimple::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
    if(m_pPassEnt)
    {
        // Don't test if the game code tells us we should ignore this collision...
        CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
        CBaseEntity *pPass = EntityFromEntityHandle( (CBaseEntity *)m_pPassEnt );
        if(VFuncs::entindex(pEntity) != VFuncs::entindex(pPass))
            return true;
    }

    return false;
}

//-----------------------------------------------------------------------------
// Sweep an entity from the starting to the ending position 
//-----------------------------------------------------------------------------
class CTraceFilterEntity : public CTraceFilterSimple
{
    DECLARE_CLASS( CTraceFilterEntity, CTraceFilterSimple );

public:
    CTraceFilterEntity( CBaseEntity *pEntity, int nCollisionGroup ) 
        : CTraceFilterSimple( pEntity, nCollisionGroup )
    {
        //m_pRootParent = pEntity->GetRootMoveParent();
        m_pRootParent = 0;
        m_pEntity = pEntity;
        //m_checkHash = g_EntityCollisionHash->IsObjectInHash(pEntity);
        m_checkHash = 0;
    }

    bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
    {
        Assert( dynamic_cast<CBaseEntity*>(pHandleEntity) );
        CBaseEntity *pTestEntity = static_cast<CBaseEntity*>(pHandleEntity);

        // Check parents against each other
        // NOTE: Don't let siblings/parents collide.
        //if ( UTIL_EntityHasMatchingRootParent( m_pRootParent, pTestEntity ) )
            //return false;

        /*if ( m_checkHash )
        {
            if ( g_EntityCollisionHash->IsObjectPairInHash(m_pEntity, pTestEntity) )
                return false;
        }*/

        return BaseClass::ShouldHitEntity( pHandleEntity, contentsMask );
    }

private:

    CBaseEntity *m_pRootParent;
    CBaseEntity *m_pEntity;
    bool        m_checkHash;
};

class CTraceFilterEntityIgnoreOther : public CTraceFilterEntity
{
    DECLARE_CLASS( CTraceFilterEntityIgnoreOther, CTraceFilterEntity );
public:
    CTraceFilterEntityIgnoreOther( CBaseEntity *pEntity, const IHandleEntity *pIgnore, int nCollisionGroup ) : 
        CTraceFilterEntity( pEntity, nCollisionGroup ), m_pIgnoreOther( pIgnore )
    {
    }

    bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
    {
        if ( pHandleEntity == m_pIgnoreOther )
            return false;

        return BaseClass::ShouldHitEntity( pHandleEntity, contentsMask );
    }

private:
    const IHandleEntity *m_pIgnoreOther;
};

//-----------------------------------------------------------------------------
// Sweeps a particular entity through the world 
//-----------------------------------------------------------------------------
void UTIL_TraceEntity( CBaseEntity *pEntity, const Vector &vecAbsStart, const Vector &vecAbsEnd, unsigned int mask, trace_t *ptr )
{
    ICollideable *pCollision = VFuncs::GetCollideable(pEntity);

    // Adding this assertion here so game code catches it, but really the assertion belongs in the engine
    // because one day, rotated collideables will work!
    Assert( pCollision->GetCollisionAngles() == vec3_angle );

    CTraceFilterEntity traceFilter( pEntity, pCollision->GetCollisionGroup() );
    enginetrace->SweepCollideable( pCollision, vecAbsStart, vecAbsEnd, pCollision->GetCollisionAngles(), mask, &traceFilter, ptr );
}

void UTIL_TraceEntity( CBaseEntity *pEntity, const Vector &vecAbsStart, const Vector &vecAbsEnd, 
                      unsigned int mask, const IHandleEntity *pIgnore, int nCollisionGroup, trace_t *ptr )
{
    ICollideable *pCollision = VFuncs::GetCollideable(pEntity);

    // Adding this assertion here so game code catches it, but really the assertion belongs in the engine
    // because one day, rotated collideables will work!
    Assert( pCollision->GetCollisionAngles() == vec3_angle );

    CTraceFilterEntityIgnoreOther traceFilter( pEntity, pIgnore, nCollisionGroup );
    enginetrace->SweepCollideable( pCollision, vecAbsStart, vecAbsEnd, pCollision->GetCollisionAngles(), mask, &traceFilter, ptr );
}

void UTIL_TraceEntity( CBaseEntity *pEntity, const Vector &vecAbsStart, const Vector &vecAbsEnd, 
                      unsigned int mask, ITraceFilter *pFilter, trace_t *ptr )
{
    ICollideable *pCollision = VFuncs::GetCollideable(pEntity);

    // Adding this assertion here so game code catches it, but really the assertion belongs in the engine
    // because one day, rotated collideables will work!
    Assert( pCollision->GetCollisionAngles() == vec3_angle );

    enginetrace->SweepCollideable( pCollision, vecAbsStart, vecAbsEnd, pCollision->GetCollisionAngles(), mask, pFilter, ptr );
}

//------------------------------------------------------------------------------
// Searches along the direction ray in steps of "step" to see if 
// the entity position is passible.
// Used for putting the player in valid space when toggling off noclip mode.
//------------------------------------------------------------------------------
int FindPassableSpace( CBaseEntity *pPlayer, const Vector& direction, float step, Vector& oldorigin )
{
    int i;
    for ( i = 0; i < 100; i++ )
    {
        Vector origin = VFuncs::GetAbsOrigin(pPlayer);
        VectorMA( origin, step, direction, origin );
        VFuncs::SetAbsOrigin(pPlayer,  origin );

        trace_t trace;
        UTIL_TraceEntity( pPlayer, VFuncs::GetAbsOrigin(pPlayer), VFuncs::GetAbsOrigin(pPlayer), MASK_PLAYERSOLID, &trace );
        if ( trace.startsolid == 0)
        {
            VectorCopy( VFuncs::GetAbsOrigin(pPlayer), oldorigin );
            return 1;
        }
    }
    return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *buffer - 
//-----------------------------------------------------------------------------
void UTIL_FreeFile( byte *buffer )
{
    delete[] buffer;
}

char *UTIL_VarArgs( const char *format, ... )
{
    va_list     argptr;
    static char     string[1024];
    
    va_start (argptr, format);
    Q_vsnprintf(string, sizeof(string), format,argptr);
    va_end (argptr);

    return string;
}

// Old style parameters
char *UTIL_VarArgs( char *format, ... )
{
    va_list     argptr;
    static char     string[1024];
    
    va_start (argptr, format);
    Q_vsnprintf(string, sizeof(string), format,argptr);
    va_end (argptr);

    return string;
}

/*
==============
UTIL_PlayerDecalTrace

A player is trying to apply his custom decal for the spray can.
Tell connected clients to display it, or use the default spray can decal
if the custom can't be loaded.
==============
*/
void UTIL_PlayerDecalTrace( trace_t *pTrace, int playernum )
{
    if (pTrace->fraction == 1.0)
        return;

    CBroadcastRecipientFilter filter;

    te->PlayerDecal( filter, 0.0,
        &pTrace->endpos, playernum, pTrace->m_pEnt->entindex() );
}

#define PRECACHE_OTHER_ONCE
// UNDONE: Do we need this to avoid doing too much of this?  Measure startup times and see
#if defined( PRECACHE_OTHER_ONCE )

#include "utlsymbol.h"
class CPrecacheOtherList : public CAutoGameSystem
{
public:
    CPrecacheOtherList( char const *name ) : CAutoGameSystem( name )
    {
    }
    virtual void LevelInitPreEntity();
    virtual void LevelShutdownPostEntity();

    bool AddOrMarkPrecached( const char *pClassname );

private:
    CUtlSymbolTable		m_list;
};

void CPrecacheOtherList::LevelInitPreEntity()
{
    m_list.RemoveAll();
}

void CPrecacheOtherList::LevelShutdownPostEntity()
{
    m_list.RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: mark or add
// Input  : *pEntity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CPrecacheOtherList::AddOrMarkPrecached( const char *pClassname )
{
    CUtlSymbol sym = m_list.Find( pClassname );
    if ( sym.IsValid() )
        return false;

    m_list.AddString( pClassname );
    return true;
}

CPrecacheOtherList g_PrecacheOtherList( "CPrecacheOtherList" );
#endif

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *szClassname - 
//			*modelName - 
//-----------------------------------------------------------------------------
void UTIL_PrecacheOther( const char *szClassname, const char *modelName )
{
#if defined( PRECACHE_OTHER_ONCE )
    // already done this one?, if not, mark as done
    if ( !g_PrecacheOtherList.AddOrMarkPrecached( szClassname ) )
        return;
#endif

    CBaseEntity	*pEntity = CreateEntityByName( szClassname );
    if ( !pEntity )
    {
        Warning( "NULL Ent in UTIL_PrecacheOther for %s\n", szClassname );
        return;
    }

    // If we have a specified model, set it before calling precache
    if ( modelName && modelName[0] )
    {
        VFuncs::SetModelName(pEntity, AllocPooledString( modelName ) );
    }
    
    if (pEntity)
        VFuncs::Precache( pEntity );

    UTIL_RemoveImmediate( pEntity );
}

//-----------------------------------------------------------------------------
// Purpose: deletes an entity, without any delay.  WARNING! Only use this when sure
//			no pointers rely on this entity.
// Input  : *oldObj - the entity to delete
//-----------------------------------------------------------------------------
void UTIL_RemoveImmediate( CBaseEntity *oldObj )
{
    if( g_nServerToolsVersion >= 2 )
    {
        servertools->RemoveEntityImmediate( oldObj );
        return;
    }

    // valid pointer or already removed?
    if ( !oldObj || (VFuncs::GetEFlags(oldObj) & EFL_KILLME) )
        return;

    // mark it for deletion 
    VFuncs::AddEFlags( oldObj, EFL_KILLME );

    if ( oldObj )
    {
        VFuncs::UpdateOnRemove(oldObj);
        VFuncs::SetName(oldObj, NULL_STRING);
    }

    if ( physenv )
    {
        physenv->CleanupDeleteList();
    }

    IServerNetworkable *pNet = VFuncs::NetworkProp(oldObj);
    if(pNet)
    {
        VFuncs::Release(pNet);
    }
}

//-----------------------------------------------------------------------------
// Purpose: Sets the entity up for deletion.  Entity will not actually be deleted
//          until the next frame, so there can be no pointer errors.
// Input  : *oldObj - object to delete
//-----------------------------------------------------------------------------
void UTIL_Remove( IServerNetworkable *oldObj, CBaseEntity *pBaseEnt )
{
    if ( !oldObj || (VFuncs::GetEFlags(pBaseEnt) & EFL_KILLME) )
        return;

    // mark it for deletion 
    VFuncs::AddEFlags( pBaseEnt, EFL_KILLME );

    //CBaseEntity *pBaseEnt = oldObj->GetBaseEntity();
    if ( pBaseEnt )
    {
        //g_bReceivedChainedUpdateOnRemove = false;
        VFuncs::UpdateOnRemove(pBaseEnt);

        //Assert( g_bReceivedChainedUpdateOnRemove );

        // clear oldObj targetname / other flags now
        VFuncs::SetName(pBaseEnt, NULL_STRING);
    }

    gEntList.AddToDeleteList( oldObj );
    /*if ( physenv )
    {
        physenv->CleanupDeleteList();
    }
    VFuncs::Release(oldObj);*/
    //oldObj->Release();
    //g_entityFactoryDictionary->Destroy(VFuncs::GetClassname(pBaseEnt), oldObj);
}

void UTIL_Remove( CBaseEntity *oldObj )
{
    if( g_nServerToolsVersion >= 2 )
    {
        servertools->RemoveEntity( oldObj );
        return;
    }

    if ( !oldObj )
        return;
    UTIL_Remove( VFuncs::NetworkProp(oldObj), oldObj );
}

/*void UTIL_ForceRemove( IServerNetworkable *oldObj )
{
    if ( !oldObj )
        return;

    // mark it for deletion
    if(!(VFuncs::GetEFlags(oldObj) & EFL_KILLME))
        VFuncs::AddEFlags( oldObj, EFL_KILLME );

    CBaseEntity *pBaseEnt = oldObj->GetBaseEntity();
    if ( pBaseEnt )
    {
        //g_bReceivedChainedUpdateOnRemove = false;
        pBaseEnt->UpdateOnRemove();

        //Assert( g_bReceivedChainedUpdateOnRemove );

        // clear oldObj targetname / other flags now
        pBaseEnt->SetName( NULL_STRING );
    }

    //gEntList.AddToDeleteList( oldObj );
    if ( physenv )
    {
        physenv->CleanupDeleteList();
    }
    oldObj->Release();
}*/

//-----------------------------------------------------------------------------
// Drops an entity onto the floor
//-----------------------------------------------------------------------------
int UTIL_DropToFloor( CBaseEntity *pEntity, unsigned int mask, CBaseEntity *pIgnore)
{
    // Assume no ground
    //pEntity->SetGroundEntity( NULL );

    Assert( pEntity );

    trace_t	trace;
    // HACK: is this really the only sure way to detect crossing a terrain boundry?
    UTIL_TraceEntity( pEntity, VFuncs::GetAbsOrigin(pEntity), VFuncs::GetAbsOrigin(pEntity), mask, pIgnore, VFuncs::GetCollisionGroup(pEntity), &trace );
    if (trace.fraction == 0.0)
        return -1;

    UTIL_TraceEntity( pEntity, VFuncs::GetAbsOrigin(pEntity), VFuncs::GetAbsOrigin(pEntity) - Vector(0,0,256), mask, pIgnore, VFuncs::GetCollisionGroup(pEntity), &trace );

    if (trace.allsolid)
        return -1;

    if (trace.fraction == 1)
        return 0;

    VFuncs::Teleport(pEntity, &trace.endpos, NULL, NULL);
    //pEntity->SetAbsOrigin( trace.endpos );
    //pEntity->SetGroundEntity( trace.m_pEnt );

    return 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : playerIndex - 
//          ping - 
//          packetloss - 
//-----------------------------------------------------------------------------
void UTIL_GetPlayerConnectionInfo( int playerIndex, int& ping, int &packetloss )
{
    edict_t *pEntity = pAdminOP.GetEntityList()+playerIndex;
    CBasePlayer *player =  (CBasePlayer *)VFuncs::Instance(pEntity);

    INetChannelInfo *nci = engine->GetPlayerNetInfo(playerIndex);

    if ( nci && player && pAdminOP.pAOPPlayers[playerIndex-1].NotBot() )
    {
        float latency = nci->GetAvgLatency( FLOW_OUTGOING ); // in seconds
        
        // that should be the correct latency, we assume that cmdrate is higher 
        // then updaterate, what is the case for default settings
        const char * szCmdRate = engine->GetClientConVarValue( playerIndex, "cl_cmdrate" );
        
        int nCmdRate = max( 1, Q_atoi( szCmdRate ) );
        latency -= (0.5f/nCmdRate) + TICKS_TO_TIME( 1.0f ); // correct latency

        // in GoldSrc we had a different, not fixed tickrate. so we have to adjust
        // Source pings by half a tick to match the old GoldSrc pings.
        latency -= TICKS_TO_TIME( 0.5f );

        ping = latency * 1000.0f; // as msecs
        ping = clamp( ping, 5, 1000 ); // set bounds, dont show pings under 5 msecs
        
        packetloss = 100.0f * nci->GetAvgLoss( FLOW_INCOMING ); // loss in percentage
        packetloss = clamp( packetloss, 0, 100 );
    }
    else
    {
        ping = 0;
        packetloss = 0;
    }
}

// So we always return a valid surface
static csurface_t   g_NullSurface = { "**empty**", 0 };

void UTIL_ClearTrace( trace_t &trace )
{
    memset( &trace, 0, sizeof(trace));
    trace.fraction = 1.f;
    trace.fractionleftsolid = 0;
    trace.surface = g_NullSurface;
}

//-----------------------------------------------------------------------------
// Sets the entity size
//-----------------------------------------------------------------------------
static void SetMinMaxSize (CBaseEntity *pEnt, const Vector& mins, const Vector& maxs )
{
    for ( int i=0 ; i<3 ; i++ )
    {
        if ( mins[i] > maxs[i] )
        {
            Error( "%s: backwards mins/maxs", ( pEnt ) ? VFuncs::GetClassname(pEnt) : "<NULL>" );
        }
    }

    Assert( pEnt );

    VFuncs::SetCollisionBounds( pEnt, mins, maxs );
}

//-----------------------------------------------------------------------------
// Sets the model size
//-----------------------------------------------------------------------------
void UTIL_SetSize( CBaseEntity *pEnt, const Vector &vecMin, const Vector &vecMax )
{
    SetMinMaxSize (pEnt, vecMin, vecMax);
}

//-----------------------------------------------------------------------------
// Sets the model to be associated with an entity
//-----------------------------------------------------------------------------
void UTIL_SetModel( CBaseEntity *pEntity, const char *pModelName )
{
    // check to see if model was properly precached
    int i = modelinfo->GetModelIndex( pModelName );
    if ( i < 0 )
    {
        Error("%i - %s:  UTIL_SetModel:  not precached: %s\n", VFuncs::entindex(pEntity),
            VFuncs::GetClassname(pEntity), pModelName);
    }

    VFuncs::SetModelIndex(pEntity, i);
    VFuncs::SetModelName( pEntity, AllocPooledString( pModelName ) );
    //VFuncs::SetModelName( pEntity, MAKE_STRING( pModelName ) );

    // brush model
    const model_t *mod = modelinfo->GetModel( i );
    if ( mod )
    {
        Vector mins, maxs;
        modelinfo->GetModelBounds( mod, mins, maxs );
        SetMinMaxSize (pEntity, mins, maxs);
    }
    else
    {
        SetMinMaxSize (pEntity, vec3_origin, vec3_origin);
    }
}

extern ConVar *sv_gravity;
// computes gravity scale for an absolute gravity.  Pass the result into CBaseEntity::SetGravity()
float UTIL_ScaleForGravity( float desiredGravity )
{
    float worldGravity = sv_gravity->GetFloat();
    return worldGravity > 0 ? desiredGravity / worldGravity : 0;
}

//-----------------------------------------------------------------------------
// Purpose: Slightly modified strtok. Does not modify the input string. Does
//			not skip over more than one separator at a time. This allows parsing
//			strings where tokens between separators may or may not be present:
//
//			Door01,,,0 would be parsed as "Door01"  ""  ""  "0"
//			Door01,Open,,0 would be parsed as "Door01"  "Open"  ""  "0"
//
// Input  : token - Returns with a token, or zero length if the token was missing.
//			str - String to parse.
//			sep - Character to use as separator. UNDONE: allow multiple separator chars
// Output : Returns a pointer to the next token to be parsed.
//-----------------------------------------------------------------------------
const char *nexttoken(char *token, const char *str, char sep)
{
    if ((str == NULL) || (*str == '\0'))
    {
        *token = '\0';
        return(NULL);
    }

    //
    // Copy everything up to the first separator into the return buffer.
    // Do not include separators in the return buffer.
    //
    while ((*str != sep) && (*str != '\0'))
    {
        *token++ = *str++;
    }
    *token = '\0';

    //
    // Advance the pointer unless we hit the end of the input string.
    //
    if (*str == '\0')
    {
        return(str);
    }

    return(++str);
}

// Client version of dispatch effect, for predicted weapons
void DispatchEffect( const char *pName, const CEffectData &data )
{
    if(!te) return;

    CPASFilter filter( data.m_vOrigin );
    te->DispatchEffect( filter, 0.0, data.m_vOrigin, pName, data );
}

void DispatchEffectAll( const char *pName, const CEffectData &data )
{
    if(!te) return;

    CReliableBroadcastRecipientFilter filter;
    te->DispatchEffect( filter, 0.0, data.m_vOrigin, pName, data );
}

void UTIL_BloodImpact( const Vector &pos, const Vector &dir, int color, int amount )
{
    CEffectData data;

    data.m_vOrigin = pos;
    data.m_vNormal = dir;
    data.m_flScale = (float)amount;
    data.m_nColor = (unsigned char)color;

    DispatchEffect( "bloodimpact", data );
}

void UTIL_BloodDrips( const Vector &origin, const Vector &direction, int color, int amount )
{
    //if ( !UTIL_ShouldShowBlood( color ) )
    //  return;

    if ( color == DONT_BLEED || amount == 0 )
        return;

    //if ( g_Language.GetInt() == LANGUAGE_GERMAN && color == BLOOD_COLOR_RED )
    //  color = 0;

    //if ( g_pGameRules->IsMultiplayer() )
    //{
        // scale up blood effect in multiplayer for better visibility
        amount *= 5;
    //}

    if ( amount > 255 )
        amount = 255;

    if (color == BLOOD_COLOR_MECH)
    {
        effects->Sparks(origin);
        if (random->RandomFloat(0, 2) >= 1)
        {
            UTIL_Smoke(origin, random->RandomInt(10, 15), 10);
        }
    }
    else
    {
        // Normal blood impact
        UTIL_BloodImpact( origin, direction, color, amount );
    }
}

void UTIL_Smoke( const Vector &origin, const float scale, const float framerate )
{
    effects->Smoke( origin, g_sModelIndexSmoke, scale, framerate );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UTIL_BloodSpray( const Vector &pos, const Vector &dir, int color, int amount, int flags )
{
    if( color == DONT_BLEED )
        return;
    
    CEffectData data;

    data.m_vOrigin = pos;
    data.m_vNormal = dir;
    data.m_flScale = (float)amount;
    data.m_fFlags = flags;
    data.m_nColor = (unsigned char)color;

    DispatchEffect( "bloodspray", data );
}

//--------------------------------------------------------------------------------------------------------------
/**
 * Returns true if the command was issued by the listenserver host, or by the dedicated server, via rcon or the server console.
 * This is valid during ConCommand execution.
 */
bool UTIL_IsCommandIssuedByServerAdmin( void )
{
    int issuingPlayerIndex = UTIL_GetCommandClientIndex();

    if ( engine->IsDedicatedServer() && issuingPlayerIndex > 0 )
        return false;

    if ( issuingPlayerIndex > 1 )
        return false;

    return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int UTIL_GetCommandClientIndex( void )
{
    // -1 == unknown,dedicated server console
    // 0  == player 1

    // Convert to 1 based offset
    return (pAdminOP.m_iClientCommandIndex+1);
}

CCheckClient g_CheckClient( "CCheckClient" );

static int UTIL_GetNewCheckClient( int check )
{
    int		i;
    edict_t	*ent;
    Vector	org;

// cycle to the next one

    if (check < 1)
        check = 1;
    if (check > gpGlobals->maxClients)
        check = gpGlobals->maxClients;

    if (check == gpGlobals->maxClients)
        i = 1;
    else
        i = check + 1;

    for ( ;  ; i++)
    {
        if ( i > gpGlobals->maxClients )
        {
            i = 1;
        }

        ent = INDEXENT( i );
        if ( !ent )
            continue;

        // Looped but didn't find anything else
        if ( i == check )
            break;	

        if ( !ent->GetUnknown() )
            continue;

        //CBaseEntity *entity = GetContainingEntity( ent );
        CBaseEntity *entity = VFuncs::GetBaseEntity(ent->GetNetworkable());
        if ( !entity )
            continue;

        if ( VFuncs::GetFlags(entity) & FL_NOTARGET )
            continue;

        // anything that is a client, or has a client as an enemy
        break;
    }

    if ( i != check )
    {
        memset( g_CheckClient.m_checkVisibilityPVS, 0, sizeof(g_CheckClient.m_checkVisibilityPVS) );
        g_CheckClient.m_bClientPVSIsExpanded = false;
    }

    if ( ent )
    {
        // get the PVS for the entity
        //CBaseEntity *pce = GetContainingEntity( ent );
        CBaseEntity *pce = VFuncs::GetBaseEntity(ent->GetNetworkable());
        if ( !pce )
            return i;

        //org = pce->EyePosition();
        org = VFuncs::EyePosition(pce);

        int clusterIndex = engine->GetClusterForOrigin( org );
        if ( clusterIndex != g_CheckClient.m_checkCluster )
        {
            g_CheckClient.m_checkCluster = clusterIndex;
            engine->GetPVSForCluster( clusterIndex, sizeof(g_CheckClient.m_checkPVS), g_CheckClient.m_checkPVS );
        }
    }
    
    return i;
}

// returns a CBaseEntity pointer to a player by index.  Only returns if the player is spawned and connected
// otherwise returns NULL
// Index is 1 based
CBasePlayer	*UTIL_PlayerByIndex( int playerIndex )
{
    CBasePlayer *pPlayer = NULL;

    if ( playerIndex > 0 && playerIndex <= gpGlobals->maxClients )
    {
        edict_t *pPlayerEdict = INDEXENT( playerIndex );
        if ( pPlayerEdict && !pPlayerEdict->IsFree() )
        {
            pPlayer = (CBasePlayer*)servergameents->EdictToBaseEntity( pPlayerEdict );
        }
    }
    
    return pPlayer;
}

//-----------------------------------------------------------------------------
// Gets the current check client....
//-----------------------------------------------------------------------------
static edict_t *UTIL_GetCurrentCheckClient()
{
    edict_t	*ent;

    // find a new check if on a new frame
    float delta = gpGlobals->curtime - g_CheckClient.m_lastchecktime;
    if ( delta >= 0.1 || delta < 0 )
    {
        g_CheckClient.m_lastcheck = UTIL_GetNewCheckClient( g_CheckClient.m_lastcheck );
        g_CheckClient.m_lastchecktime = gpGlobals->curtime;
    }

    // return check if it might be visible	
    ent = INDEXENT( g_CheckClient.m_lastcheck );

    // Allow dead clients -- JAY
    // Our monsters know the difference, and this function gates alot of behavior
    // It's annoying to die and see monsters stop thinking because you're no longer
    // "in" their PVS
    if ( !ent || ent->IsFree() || !ent->GetUnknown())
    {
        return NULL;
    }

    return ent;
}

static edict_t *UTIL_FindClientInPVSGuts(edict_t *pEdict, unsigned char *pvs, unsigned pvssize )
{
    Vector	view;

    edict_t	*ent = UTIL_GetCurrentCheckClient();
    if ( !ent )
    {
        return NULL;
    }

    //CBaseEntity *pPlayerEntity = GetContainingEntity( ent );
    CBaseEntity *pPlayerEntity = VFuncs::GetBaseEntity(ent->GetNetworkable());
    /*if( (!pPlayerEntity || (pPlayerEntity->GetFlags() & FL_NOTARGET)) && sv_strict_notarget.GetBool() )
    {
        return NULL;
    }*/
    // if current entity can't possibly see the check entity, return 0
    // UNDONE: Build a box for this and do it over that box
    // UNDONE: Use CM_BoxLeafnums()
    //CBaseEntity *pe = GetContainingEntity( pEdict );
    CBaseEntity *pe = VFuncs::GetBaseEntity(pEdict->GetNetworkable());
    if ( pe )
    {
        //view = pe->EyePosition();
        view = VFuncs::EyePosition(pe);
        
        if ( !engine->CheckOriginInPVS( view, pvs, pvssize ) )
        {
            return NULL;
        }
    }

    // might be able to see it
    return ent;
}

//-----------------------------------------------------------------------------
// Purpose: Returns a client that could see the entity directly
//-----------------------------------------------------------------------------

edict_t *UTIL_FindClientInPVS(edict_t *pEdict)
{
    return UTIL_FindClientInPVSGuts( pEdict, g_CheckClient.m_checkPVS, sizeof( g_CheckClient.m_checkPVS ) );
}

//-----------------------------------------------------------------------------
// Purpose: Returns a client that could see the entity, including through a camera
//-----------------------------------------------------------------------------
edict_t *UTIL_FindClientInVisibilityPVS( edict_t *pEdict )
{
    return UTIL_FindClientInPVSGuts( pEdict, g_CheckClient.m_checkVisibilityPVS, sizeof( g_CheckClient.m_checkVisibilityPVS ) );
}

void UTIL_StringToFloatArray( float *pVector, int count, const char *pString )
{
    char *pstr, *pfront, tempString[128];
    int	j;

    Q_strncpy( tempString, pString, sizeof(tempString) );
    pstr = pfront = tempString;

    for ( j = 0; j < count; j++ )			// lifted from pr_edict.c
    {
        pVector[j] = atof( pfront );

        // skip any leading whitespace
        while ( *pstr && *pstr <= ' ' )
            pstr++;

        // skip to next whitespace
        while ( *pstr && *pstr > ' ' )
            pstr++;

        if (!*pstr)
            break;

        pstr++;
        pfront = pstr;
    }
    for ( j++; j < count; j++ )
    {
        pVector[j] = 0;
    }
}

void UTIL_StringToVector( float *pVector, const char *pString )
{
    UTIL_StringToFloatArray( pVector, 3, pString );
}

void UTIL_StringToIntArray( int *pVector, int count, const char *pString )
{
    char *pstr, *pfront, tempString[128];
    int	j;

    Q_strncpy( tempString, pString, sizeof(tempString) );
    pstr = pfront = tempString;

    for ( j = 0; j < count; j++ )			// lifted from pr_edict.c
    {
        pVector[j] = atoi( pfront );

        while ( *pstr && *pstr != ' ' )
            pstr++;
        if (!*pstr)
            break;
        pstr++;
        pfront = pstr;
    }

    for ( j++; j < count; j++ )
    {
        pVector[j] = 0;
    }
}

void UTIL_StringToColor32( color32 *color, const char *pString )
{
    int tmp[4];
    UTIL_StringToIntArray( tmp, 4, pString );
    color->r = tmp[0];
    color->g = tmp[1];
    color->b = tmp[2];
    color->a = tmp[3];
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
AngularImpulse WorldToLocalRotation( const VMatrix &localToWorld, const Vector &worldAxis, float rotation )
{
    // fix axes of rotation to match axes of vector
    Vector rot = worldAxis * rotation;
    // since the matrix maps local to world, do a transpose rotation to get world to local
    AngularImpulse ang = localToWorld.VMul3x3Transpose( rot );

    return ang;
}

IEntityFactoryDictionary *EntityFactoryDictionary()
{
    if(g_entityFactoryDictionary)
        return g_entityFactoryDictionary;

    if ( g_nServerToolsVersion >= 2 )
    {
        g_entityFactoryDictionary = servertools->GetEntityFactoryDictionary();
        return g_entityFactoryDictionary;
    }

#ifdef WIN32
    ConCommandBase *pPtr = pAdminOP.GetCommands();
    while (pPtr)
    {
        if (pPtr->IsCommand() && strcmp(pPtr->GetName(), "dumpentityfactories") == 0)
        {
            ConCommand *ptr = (ConCommand*)pPtr;

            //CEntityFactoryDictionary *dict = ( CEntityFactoryDictionary * )*(int *)( ((char*)VFuncs::GetCommandCallback(ptr))+0x14 );
            CEntityFactoryDictionary *dict = VFuncs::GetEntityDictionary(ptr);
            if ( dict )
            {
                g_entityFactoryDictionary = dict;
                return dict;
            }
        }
        pPtr = const_cast<ConCommandBase*>(pPtr->GetNext());
    }
#else
    g_entityFactoryDictionary = VFuncs::GetEntityDictionary(NULL);
    return g_entityFactoryDictionary;
#endif
    return NULL;
}

CBaseEntity *CreateEntityByName( const char *className, int iForceEdictIndex )
{
    if(servertools != NULL)
    {
        return (CBaseEntity *)servertools->CreateEntityByName(className);
    }
    else if(_CreateEntityByName != NULL)
    {
        return _CreateEntityByName(className, iForceEdictIndex);
    }

    return NULL;
}

CBaseEntity *CreateCombineBall( const Vector &origin, const Vector &velocity, float radius, float mass, float lifetime, CBaseEntity *pOwner )
{
    if(_CreateCombineBall != NULL)
    {
        CBaseEntity *pReturn = NULL;

        pReturn = _CreateCombineBall(origin, velocity, radius, mass, lifetime, pOwner);

        // If the radius is greater than the max, let's do some hacks to make it work
        if ( radius > 12.0f )
        {
            VFuncs::SetCombineBallRadius( pReturn, radius );

            // After changing the radius, we need to recreate the VPhysics object
            VFuncs::VPhysicsDestroyObject( pReturn );
            VFuncs::CreateVPhysics( pReturn );

            // We need to set the same things on the new physics object that Spawn and CreateCombineBall set
            VFuncs::VPhysicsGetObject( pReturn )->SetVelocity( &velocity, NULL );
            PhysSetGameFlags( VFuncs::VPhysicsGetObject( pReturn ), FVPHYSICS_WAS_THROWN );
        }

        return pReturn;
    }
    return NULL;
}

char * sopstrdup(const char * str)
{
    char *ret = (char *)malloc(strlen(str)+1);
    strcpy(ret, str);
    return ret;
}

char * strTrim(const char * str)
{
    int start,end;
    char * tmp;
    tmp = sopstrdup(str);
    if(strlen(tmp) > 0)
    {
        char *tmp2;

        start = 0;
        while(isspace(tmp[start]) && tmp[start] != '\0')
        {
            start++;
        }
        tmp2 = sopstrdup(tmp+start);
        free(tmp);
        tmp = tmp2;
        
        /*//REMOVE LEADING SPACES
        while (strcmp(strLeft(tmp,1)," ") == 0 || strcmp(strLeft(tmp,1),"\t") == 0)
        {
            tmp = strRight(tmp,strlen(tmp) - 1);
        }*/
    }
    if(strlen(tmp) > 0)
    {
        end = strlen(tmp)-1;
        while(end > 0 && isspace(tmp[end]))
        {
            end--;
        }

        tmp[end+1] = '\0';
        /*//REMOVE TRAILING SPACES
        while (strcmp(strRight(tmp,1)," ") == 0 || strcmp(strRight(tmp,1),"\t") == 0)
        {
            tmp = strLeft(tmp,strlen(tmp) - 1);
        }*/
    }
    return tmp;
}

char * strRemoveReturn(const char * str)
{
    int start,end;
    char * tmp;
    tmp = sopstrdup(str);
    if(strlen(tmp) > 0)
    {
        char *tmp2;

        start = 0;
        while((tmp[start] == '\r' || tmp[start] == '\n') && tmp[start] != '\0')
        {
            start++;
        }
        tmp2 = sopstrdup(tmp+start);
        free(tmp);
        tmp = tmp2;

        /*//REMOVE LEADING SPACES
        while (strcmp(strLeft(tmp,1),"\r") == 0 || strcmp(strLeft(tmp,1),"\n") == 0)
        {
            tmp = strRight(tmp,strlen(tmp) - 1);
        }*/
    }
    if(strlen(tmp) > 0)
    {
        end = strlen(tmp)-1;
        while(end > 0 && (tmp[start] == '\r' || tmp[start] == '\n'))
        {
            end--;
        }

        tmp[end+1] = '\0';
        /*//REMOVE TRAILING SPACES
        while (strcmp(strRight(tmp,1),"\r") == 0 || strcmp(strRight(tmp,1),"\n") == 0)
        {
            tmp = strLeft(tmp,strlen(tmp) - 1);
        }*/
    }
    return tmp;
}

// FIXME: strRemoveQuote, strLeft, strRight functions have a leak
char * strRemoveQuote(const char * str)
{
    char * tmp;
    tmp = sopstrdup(str);
    if(strlen(tmp) > 0)
    {
        //REMOVE LEADING QUOTE
        while (strcmp(strLeft(tmp,1),"\"") == 0)
        {
            tmp = strRight(tmp,strlen(tmp) - 1);
        }
    }
    if(strlen(tmp) > 0)
    {
        //REMOVE TRAILING QUOTE
        while (strcmp(strRight(tmp,1),"\"") == 0)
        {
            tmp = strLeft(tmp,strlen(tmp) - 1);
        }
    }
    return tmp;
}
char * strLeft(const char * str,int n)
{
    char * tmp;
    tmp = sopstrdup(str);
    strncpy(tmp + n,"",abs((int)strlen(tmp) - n));
        
    return tmp;
}
#ifdef __linux__
char * strrev ( char * string )
{
    char *start = string;
    char *left = string;
    char ch;

    while (*string++)                 /* find end of string */
        ;
    string -= 2;

    while (left < string)
    {
        ch = *left;
        *left++ = *string;
        *string-- = ch;
    }

    return(start);
}
#endif
char * strRight(const char * str, int n)
{
    char * tmp;
    tmp = sopstrdup(str);
    strrev(tmp);
    strncpy(tmp + n,"",strlen(tmp) - n);
    strrev(tmp);
    return tmp;
}

/* Version of strtrim that works in place */
/*
** Copyright (C) 1999-2000 Open System Telecom Corporation.
**  
*/
char    *strrtrim(char *str, const char *trim)
{
    char    *end;
    
    if(!str)
        return NULL;

    if(!trim)
        trim = " \t\n\r";
        
    end = str + strlen(str);

    while(end-- > str)
    {
        if(!strchr(trim, *end))
            return str;
        *end = 0;
    }
    return str;
}

char    *strltrim(char *str, const char *trim)
{
    if(!str)
        return NULL;
    
    if(!trim)
        trim = " \t\r\n";
    
    while(*str)
    {
        if(!strchr(trim, *str))
            return str;
        ++str;
    }
    return str;
}

char    *strtrim(char *str, const char *trim)
{
    return strltrim(strrtrim(str, trim), trim);
}

bool DFIsAdminTutLocked( void )
{
    char gamedir[256];
    char filepath[512];
    FILE *fp;

    engine->GetGameDir(gamedir, sizeof(gamedir));

    Q_snprintf(filepath, sizeof(filepath), "%s/%s/admintut_lock.txt", gamedir, pAdminOP.DataDir());
    V_FixSlashes(filepath);
    fp = fopen(filepath, "rt");
    if(fp)
    {
        fclose(fp);
        return true;
    }
    return false;
}

#ifdef __linux__

#include <stdarg.h>
#include <fcntl.h>
#include <link.h>
#include <sys/mman.h>

#include "sm_symtable.h"

struct DynLibInfo
{
    void *baseAddress;
    size_t memorySize;
};

typedef uint32_t Elf32_Addr;

struct LibSymbolTable
{
    SymbolTable table;
    Elf32_Addr lib_base;
    uint32_t last_pos;
};

CUtlVector<LibSymbolTable *> m_SymTables;

void *ResolveSymbol(void *handle, const char *symbol)
{
    struct link_map *dlmap;
    struct stat dlstat;
    int dlfile;
    uintptr_t map_base;
    Elf32_Ehdr *file_hdr;
    Elf32_Shdr *sections, *shstrtab_hdr, *symtab_hdr, *strtab_hdr;
    Elf32_Sym *symtab;
    const char *shstrtab, *strtab;
    uint16_t section_count;
    uint32_t symbol_count;
    LibSymbolTable *libtable;
    SymbolTable *table;
    Symbol *symbol_entry;

    dlmap = (struct link_map *)handle;
    symtab_hdr = NULL;
    strtab_hdr = NULL;
    table = NULL;

    /* See if we already have a symbol table for this library */
    for (size_t i = 0; i < m_SymTables.Count(); i++)
    {
        libtable = m_SymTables[i];
        if (libtable->lib_base == dlmap->l_addr)
        {
            table = &libtable->table;
            break;
        }
    }

    /* If we don't have a symbol table for this library, then create one */
    if (table == NULL)
    {
        libtable = new LibSymbolTable();
        libtable->table.Initialize();
        libtable->lib_base = dlmap->l_addr;
        libtable->last_pos = 0;
        table = &libtable->table;
        m_SymTables.AddToTail(libtable);
    }

    /* See if the symbol is already cached in our table */
    symbol_entry = table->FindSymbol(symbol, strlen(symbol));
    if (symbol_entry != NULL)
    {
        return symbol_entry->address;
    }

    /* If symbol isn't in our table, then we have open the actual library */
    dlfile = open(dlmap->l_name, O_RDONLY);
    if (dlfile == -1 || fstat(dlfile, &dlstat) == -1)
    {
        close(dlfile);
        return NULL;
    }

    /* Map library file into memory */
    file_hdr = (Elf32_Ehdr *)mmap(NULL, dlstat.st_size, PROT_READ, MAP_PRIVATE, dlfile, 0);
    map_base = (uintptr_t)file_hdr;
    if (file_hdr == MAP_FAILED)
    {
        close(dlfile);
        return NULL;
    }
    close(dlfile);

    if (file_hdr->e_shoff == 0 || file_hdr->e_shstrndx == SHN_UNDEF)
    {
        munmap(file_hdr, dlstat.st_size);
        return NULL;
    }

    sections = (Elf32_Shdr *)(map_base + file_hdr->e_shoff);
    section_count = file_hdr->e_shnum;
    /* Get ELF section header string table */
    shstrtab_hdr = &sections[file_hdr->e_shstrndx];
    shstrtab = (const char *)(map_base + shstrtab_hdr->sh_offset);

    /* Iterate sections while looking for ELF symbol table and string table */
    for (uint16_t i = 0; i < section_count; i++)
    {
        Elf32_Shdr &hdr = sections[i];
        const char *section_name = shstrtab + hdr.sh_name;

        if (strcmp(section_name, ".symtab") == 0)
        {
            symtab_hdr = &hdr;
        }
        else if (strcmp(section_name, ".strtab") == 0)
        {
            strtab_hdr = &hdr;
        }
    }

    /* Uh oh, we don't have a symbol table or a string table */
    if (symtab_hdr == NULL || strtab_hdr == NULL)
    {
        munmap(file_hdr, dlstat.st_size);
        return NULL;
    }

    symtab = (Elf32_Sym *)(map_base + symtab_hdr->sh_offset);
    strtab = (const char *)(map_base + strtab_hdr->sh_offset);
    symbol_count = symtab_hdr->sh_size / symtab_hdr->sh_entsize;

    /* Iterate symbol table starting from the position we were at last time */
    for (uint32_t i = libtable->last_pos; i < symbol_count; i++)
    {
        Elf32_Sym &sym = symtab[i];
        unsigned char sym_type = ELF32_ST_TYPE(sym.st_info);
        const char *sym_name = strtab + sym.st_name;
        Symbol *cur_sym;

        /* Skip symbols that are undefined or do not refer to functions or objects */
        if (sym.st_shndx == SHN_UNDEF || (sym_type != STT_FUNC && sym_type != STT_OBJECT))
        {
            continue;
        }

        /* Caching symbols as we go along */
        cur_sym = table->InternSymbol(sym_name, strlen(sym_name), (void *)(dlmap->l_addr + sym.st_value));
        if (strcmp(symbol, sym_name) == 0)
        {
            symbol_entry = cur_sym;
            libtable->last_pos = ++i;
            break;
        }
    }

    munmap(file_hdr, dlstat.st_size);
    return symbol_entry ? symbol_entry->address : NULL;
}
#endif
