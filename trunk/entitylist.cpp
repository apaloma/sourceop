//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

// HACK HACK HACK : BASEENTITY ACCESS
#define GAME_DLL
#include "fixdebug.h"
#include "cbase.h"
#include "entitylist.h"
#include "utlvector.h"
#include "igamesystem.h"
#include "collisionutils.h"
#include "UtlSortVector.h"
#include "tier0/vprof.h"
#include "mapentities.h"
#include "globalstate.h"
#include "datacache/imdlcache.h"
#include "toolframework/itoolentity.h"

#include "AdminOP.h"
#include "vfuncs.h"

extern IServerTools *servertools;

#ifdef HL2_DLL
#include "npc_playercompanion.h"
#endif // HL2_DLL

#include "tier0/memdbgon.h"

CBaseEntity *FindPickerEntity( CBasePlayer *pPlayer );
void SceneManager_ClientActive( CBasePlayer *player );

static CUtlVector<IServerNetworkable*> g_DeleteList;

CGlobalEntityList gEntList;
CBaseEntityList *g_pEntityList = &gEntList;

CGlobalEntityList::CGlobalEntityList()
{
    m_iHighestEnt = m_iNumEnts = m_iNumEdicts = 0;
    m_bClearingEntities = false;
}


// removes the entity from the global list
// only called from with the CBaseEntity destructor
static bool g_fInCleanupDelete;


// mark an entity as deleted
void CGlobalEntityList::AddToDeleteList( IServerNetworkable *ent )
{
    //if ( ent && ent->GetEntityHandle()->GetRefEHandle() != INVALID_EHANDLE_INDEX )
    if ( ent && VFuncs::GetRefEHandle(VFuncs::GetBaseEntity(ent)) != INVALID_EHANDLE_INDEX )
    {
        g_DeleteList.AddToTail( ent );
    }
}

//extern bool g_bDisableEhandleAccess;
// call this before and after each frame to delete all of the marked entities.
void CGlobalEntityList::CleanupDeleteList( void )
{
    //VPROF( "CGlobalEntityList::CleanupDeleteList" );
    g_fInCleanupDelete = true;
    // clean up the vphysics delete list as well
    if(physenv)
    {
        physenv->CleanupDeleteList();
    }
    //PhysOnCleanupDeleteList();

    //g_bDisableEhandleAccess = true;
    for ( int i = 0; i < g_DeleteList.Count(); i++ )
    {
        VFuncs::Release(g_DeleteList[i]);
    }
    //g_bDisableEhandleAccess = false;
    g_DeleteList.RemoveAll();

    //g_fInCleanupDelete = false;
}

int CGlobalEntityList::ResetDeleteList( void )
{
    int result = g_DeleteList.Count();
    g_DeleteList.RemoveAll();
    return result;
}


    // add a class that gets notified of entity events
void CGlobalEntityList::AddListenerEntity( IEntityListener *pListener )
{
    if ( m_entityListeners.Find( pListener ) >= 0 )
    {
        AssertMsg( 0, "Can't add listeners multiple times\n" );
        return;
    }
    m_entityListeners.AddToTail( pListener );
}

void CGlobalEntityList::RemoveListenerEntity( IEntityListener *pListener )
{
    m_entityListeners.FindAndRemove( pListener );
}

void CGlobalEntityList::Clear( void )
{
    m_bClearingEntities = true;

    CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] SourceOP's CGlobalEntityList::Clear should not be called!\n");
    // Add all remaining entities in the game to the delete list and call appropriate UpdateOnRemove
    CBaseHandle hCur = FirstHandle();
    /*while ( hCur != InvalidHandle() )
    {
        IServerNetworkable *ent = GetServerNetworkable( hCur );
        if ( ent )
        {
            MDLCACHE_CRITICAL_SECTION();
            // Force UpdateOnRemove to be called
            UTIL_Remove( ent );
        }
        hCur = NextHandle( hCur );
    }*/
        
    CleanupDeleteList();
    // free the memory
    g_DeleteList.Purge();

    //CBaseEntity::m_nDebugPlayer = -1;
    //CBaseEntity::m_bInDebugSelect = false; 
    m_iHighestEnt = 0;
    m_iNumEnts = 0;

    m_bClearingEntities = false;
}


int CGlobalEntityList::NumberOfEntities( void )
{
    return m_iNumEnts;
}

int CGlobalEntityList::NumberOfEdicts( void )
{
    return m_iNumEdicts;
}

CBaseEntity *CGlobalEntityList::NextEnt( CBaseEntity *pCurrentEnt ) 
{ 
    if ( !pCurrentEnt )
    {
        const CEntInfo *pInfo = FirstEntInfo();
        if ( !pInfo )
            return NULL;

        return (CBaseEntity *)pInfo->m_pEntity;
    }

    // Run through the list until we get a CBaseEntity.
    const CEntInfo *pList = GetEntInfoPtr( VFuncs::GetRefEHandle(pCurrentEnt) );
    if ( pList )
        pList = NextEntInfo(pList);

    while ( pList )
    {
#if 0
        if ( pList->m_pEntity )
        {
            IServerUnknown *pUnk = static_cast<IServerUnknown*>(const_cast<IHandleEntity*>(pList->m_pEntity));
            CBaseEntity *pRet = pUnk->GetBaseEntity();
            if ( pRet )
                return pRet;
        }
#else
        return (CBaseEntity *)pList->m_pEntity;
#endif
        pList = pList->m_pNext;
    }
    
    return NULL;

}


void CGlobalEntityList::ReportEntityFlagsChanged( CBaseEntity *pEntity, unsigned int flagsOld, unsigned int flagsNow )
{
    if ( VFuncs::IsMarkedForDeletion(pEntity) )
        return;
    // UNDONE: Move this into IEntityListener instead?
    unsigned int flagsChanged = flagsOld ^ flagsNow;
    if ( flagsChanged & FL_AIMTARGET )
    {
        unsigned int flagsAdded = flagsNow & flagsChanged;
        unsigned int flagsRemoved = flagsOld & flagsChanged;

        if ( flagsAdded & FL_AIMTARGET )
        {
            //g_AimManager.AddEntity( pEntity );
        }
        if ( flagsRemoved & FL_AIMTARGET )
        {
            //g_AimManager.RemoveEntity( pEntity );
        }
    }
}

void CGlobalEntityList::AddPostClientMessageEntity( CBaseEntity *pEntity )
{
    //g_PostClientManager.AddEntity( pEntity );
}
void CGlobalEntityList::PostClientMessagesSent()
{
    //g_PostClientManager.PostClientMessagesSent();
}

//-----------------------------------------------------------------------------
// Purpose: Used to confirm a pointer is a pointer to an entity, useful for
//			asserts.
//-----------------------------------------------------------------------------
bool CGlobalEntityList::IsEntityPtr( void *pTest )
{
    if ( pTest )
    {
        const CEntInfo *pInfo = FirstEntInfo();
        for ( ;pInfo; pInfo = pInfo->m_pNext )
        {
            if ( pTest == (void *)pInfo->m_pEntity )
                return true;
        }
    }

    return false; 
}

//-----------------------------------------------------------------------------
// Purpose: Iterates the entities with a given classname.
// Input  : pStartEntity - Last entity found, NULL to start a new iteration.
//			szName - Classname to search for.
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityByClassname( CBaseEntity *pStartEntity, const char *szName )
{
    const CEntInfo *pInfo = pStartEntity ? GetEntInfoPtr( VFuncs::GetRefEHandle(pStartEntity) )->m_pNext : FirstEntInfo();

    for ( ;pInfo; pInfo = pInfo->m_pNext )
    {
        CBaseEntity *pEntity = (CBaseEntity *)pInfo->m_pEntity;
        if ( !pEntity )
        {
            DevWarning( "NULL entity in global entity list!\n" );
            continue;
        }

        if ( VFuncs::ClassMatches(pEntity,szName) )
            return pEntity;
    }

    return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Finds an entity given a procedural name.
// Input  : szName - The procedural name to search for, should start with '!'.
//			pSearchingEntity - 
//			pActivator - The activator entity if this was called from an input
//				or Use handler.
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityProcedural( const char *szName, CBaseEntity *pSearchingEntity, CBaseEntity *pActivator, CBaseEntity *pCaller )
{
    //
    // Check for the name escape character.
    //
    if ( szName[0] == '!' )
    {
        const char *pName = szName + 1;

        //
        // It is a procedural name, look for the ones we understand.
        //
        if ( FStrEq( pName, "player" ) )
        {
            return (CBaseEntity *)UTIL_PlayerByIndex( 1 );
        }
        else if ( FStrEq( pName, "pvsplayer" ) )
        {
            if ( pSearchingEntity )
            {
                return VFuncs::Instance( UTIL_FindClientInPVS( servergameents->BaseEntityToEdict(pSearchingEntity) ) );
            }
            else if ( pActivator )
            {
                // FIXME: error condition?
                return VFuncs::Instance( UTIL_FindClientInPVS( servergameents->BaseEntityToEdict(pActivator) ) );
            }
            else
            {
                // FIXME: error condition?
                return (CBaseEntity *)UTIL_PlayerByIndex( 1 );
            }

        }
        else if ( FStrEq( pName, "activator" ) )
        {
            return pActivator;
        }
        else if ( FStrEq( pName, "caller" ) )
        {
            return pCaller;
        }
        else if ( FStrEq( pName, "picker" ) )
        {
            return FindPickerEntity( UTIL_PlayerByIndex(1) );
        }
        else if ( FStrEq( pName, "self" ) )
        {
            return pSearchingEntity;
        }
        else 
        {
            Warning( "Invalid entity search name %s\n", szName );
            Assert(0);
        }
    }

    return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Iterates the entities with a given name.
// Input  : pStartEntity - Last entity found, NULL to start a new iteration.
//			szName - Name to search for.
//			pActivator - Activator entity if this was called from an input
//				handler or Use handler.
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityByName( CBaseEntity *pStartEntity, const char *szName, CBaseEntity *pSearchingEntity, CBaseEntity *pActivator, CBaseEntity *pCaller, IEntityFindFilter *pFilter )
{
    if ( !szName || szName[0] == 0 )
        return NULL;

    if ( szName[0] == '!' )
    {
        //
        // Avoid an infinite loop, only find one match per procedural search!
        //
        if (pStartEntity == NULL)
            return FindEntityProcedural( szName, pSearchingEntity, pActivator, pCaller );

        return NULL;
    }
    
    const CEntInfo *pInfo = pStartEntity ? GetEntInfoPtr( VFuncs::GetRefEHandle(pStartEntity) )->m_pNext : FirstEntInfo();

    for ( ;pInfo; pInfo = pInfo->m_pNext )
    {
        CBaseEntity *ent = (CBaseEntity *)pInfo->m_pEntity;
        if ( !ent )
        {
            DevWarning( "NULL entity in global entity list!\n" );
            //continue;
            return NULL;
        }

        if ( !VFuncs::GetEntityName(ent) )
            continue;

        if ( VFuncs::NameMatches( ent, szName ) )
        {
            if ( pFilter && !pFilter->ShouldFindEntity(ent) )
                continue;

            return ent;
        }
    }

    return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pStartEntity - 
//			szModelName - 
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityByModel( CBaseEntity *pStartEntity, const char *szModelName )
{
    const CEntInfo *pInfo = pStartEntity ? GetEntInfoPtr( VFuncs::GetRefEHandle(pStartEntity) )->m_pNext : FirstEntInfo();

    for ( ;pInfo; pInfo = pInfo->m_pNext )
    {
        CBaseEntity *ent = (CBaseEntity *)pInfo->m_pEntity;
        if ( !ent )
        {
            DevWarning( "NULL entity in global entity list!\n" );
            continue;
        }

        if ( !servergameents->BaseEntityToEdict(ent) || !VFuncs::GetModelName(pStartEntity) )
            continue;

        if ( FStrEq( STRING(VFuncs::GetModelName(ent)), szModelName ) )
            return ent;
    }

    return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Iterates the entities with a given target.
// Input  : pStartEntity - 
//			szName - 
//-----------------------------------------------------------------------------
// FIXME: obsolete, remove
CBaseEntity	*CGlobalEntityList::FindEntityByTarget( CBaseEntity *pStartEntity, const char *szName )
{
    const CEntInfo *pInfo = pStartEntity ? GetEntInfoPtr( VFuncs::GetRefEHandle(pStartEntity) )->m_pNext : FirstEntInfo();

    for ( ;pInfo; pInfo = pInfo->m_pNext )
    {
        CBaseEntity *ent = (CBaseEntity *)pInfo->m_pEntity;
        if ( !ent )
        {
            DevWarning( "NULL entity in global entity list!\n" );
            continue;
        }

        if ( !VFuncs::GetTarget(ent) )
            continue;

        if ( FStrEq( STRING(VFuncs::GetTarget(ent)), szName ) )
            return ent;
    }

    return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Used to iterate all the entities within a sphere.
// Input  : pStartEntity - 
//			vecCenter - 
//			flRadius - 
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityInSphere( CBaseEntity *pStartEntity, const Vector &vecCenter, float flRadius )
{
    const CEntInfo *pInfo = pStartEntity ? GetEntInfoPtr( VFuncs::GetRefEHandle(pStartEntity) )->m_pNext : FirstEntInfo();

    for ( ;pInfo; pInfo = pInfo->m_pNext )
    {
        CBaseEntity *ent = (CBaseEntity *)pInfo->m_pEntity;
        if ( !ent )
        {
            DevWarning( "NULL entity in global entity list!\n" );
            continue;
        }

        if ( !servergameents->BaseEntityToEdict(ent) )
            continue;

        Vector vecRelativeCenter;
        CCollisionProperty *pProp = VFuncs::CollisionProp(ent);
        static int unsafeWarns = 0;
        if(unsafeWarns++ < 5)
            Warning("[SOURCEOP] FindEntityInSphere may be unsafe.\n");
        pProp->WorldToCollisionSpace( vecCenter, &vecRelativeCenter );
        if ( !IsBoxIntersectingSphere( pProp->OBBMins(), pProp->OBBMaxs(), vecRelativeCenter, flRadius ) )
            continue;

        return ent;
    }

    // nothing found
    return NULL; 
}


//-----------------------------------------------------------------------------
// Purpose: Finds the nearest entity by name within a radius
// Input  : szName - Entity name to search for.
//			vecSrc - Center of search radius.
//			flRadius - Search radius for classname search, 0 to search everywhere.
//			pSearchingEntity - The entity that is doing the search.
//			pActivator - The activator entity if this was called from an input
//				or Use handler, NULL otherwise.
// Output : Returns a pointer to the found entity, NULL if none.
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityByNameNearest( const char *szName, const Vector &vecSrc, float flRadius, CBaseEntity *pSearchingEntity, CBaseEntity *pActivator, CBaseEntity *pCaller )
{
    CBaseEntity *pEntity = NULL;

    //
    // Check for matching class names within the search radius.
    //
    float flMaxDist2 = flRadius * flRadius;
    if (flMaxDist2 == 0)
    {
        flMaxDist2 = MAX_TRACE_LENGTH * MAX_TRACE_LENGTH;
    }

    CBaseEntity *pSearch = NULL;
    while ((pSearch = gEntList.FindEntityByName( pSearch, szName, pSearchingEntity, pActivator, pCaller )) != NULL)
    {
        if ( !servergameents->BaseEntityToEdict(pSearch) )
            continue;

        float flDist2 = (VFuncs::GetAbsOrigin(pSearch) - vecSrc).LengthSqr();

        if (flMaxDist2 > flDist2)
        {
            pEntity = pSearch;
            flMaxDist2 = flDist2;
        }
    }

    return pEntity;
}



//-----------------------------------------------------------------------------
// Purpose: Finds the first entity by name within a radius
// Input  : pStartEntity - The entity to start from when doing the search.
//			szName - Entity name to search for.
//			vecSrc - Center of search radius.
//			flRadius - Search radius for classname search, 0 to search everywhere.
//			pSearchingEntity - The entity that is doing the search.
//			pActivator - The activator entity if this was called from an input
//				or Use handler, NULL otherwise.
// Output : Returns a pointer to the found entity, NULL if none.
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityByNameWithin( CBaseEntity *pStartEntity, const char *szName, const Vector &vecSrc, float flRadius, CBaseEntity *pSearchingEntity, CBaseEntity *pActivator, CBaseEntity *pCaller )
{
    //
    // Check for matching class names within the search radius.
    //
    CBaseEntity *pEntity = pStartEntity;
    float flMaxDist2 = flRadius * flRadius;
    if (flMaxDist2 == 0)
    {
        return gEntList.FindEntityByName( pEntity, szName, pSearchingEntity, pActivator, pCaller );
    }

    while ((pEntity = gEntList.FindEntityByName( pEntity, szName, pSearchingEntity, pActivator, pCaller )) != NULL)
    {
        if ( !servergameents->BaseEntityToEdict(pEntity) )
            continue;

        float flDist2 = (VFuncs::GetAbsOrigin(pEntity) - vecSrc).LengthSqr();

        if (flMaxDist2 > flDist2)
        {
            return pEntity;
        }
    }

    return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Finds the nearest entity by class name withing given search radius.
// Input  : szName - Entity name to search for. Treated as a target name first,
//				then as an entity class name, ie "info_target".
//			vecSrc - Center of search radius.
//			flRadius - Search radius for classname search, 0 to search everywhere.
// Output : Returns a pointer to the found entity, NULL if none.
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityByClassnameNearest( const char *szName, const Vector &vecSrc, float flRadius )
{
    CBaseEntity *pEntity = NULL;

    //
    // Check for matching class names within the search radius.
    //
    float flMaxDist2 = flRadius * flRadius;
    if (flMaxDist2 == 0)
    {
        flMaxDist2 = MAX_TRACE_LENGTH * MAX_TRACE_LENGTH;
    }

    CBaseEntity *pSearch = NULL;
    while ((pSearch = gEntList.FindEntityByClassname( pSearch, szName )) != NULL)
    {
        if ( !servergameents->BaseEntityToEdict(pSearch) )
            continue;

        float flDist2 = (VFuncs::GetAbsOrigin(pSearch) - vecSrc).LengthSqr();

        if (flMaxDist2 > flDist2)
        {
            pEntity = pSearch;
            flMaxDist2 = flDist2;
        }
    }

    return pEntity;
}



//-----------------------------------------------------------------------------
// Purpose: Finds the first entity within radius distance by class name.
// Input  : pStartEntity - The entity to start from when doing the search.
//			szName - Entity class name, ie "info_target".
//			vecSrc - Center of search radius.
//			flRadius - Search radius for classname search, 0 to search everywhere.
// Output : Returns a pointer to the found entity, NULL if none.
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityByClassnameWithin( CBaseEntity *pStartEntity, const char *szName, const Vector &vecSrc, float flRadius )
{
    //
    // Check for matching class names within the search radius.
    //
    CBaseEntity *pEntity = pStartEntity;
    float flMaxDist2 = flRadius * flRadius;
    if (flMaxDist2 == 0)
    {
        return gEntList.FindEntityByClassname( pEntity, szName );
    }

    while ((pEntity = gEntList.FindEntityByClassname( pEntity, szName )) != NULL)
    {
        if ( !servergameents->BaseEntityToEdict(pEntity) )
            continue;

        float flDist2 = (VFuncs::GetAbsOrigin(pEntity) - vecSrc).LengthSqr();

        if (flMaxDist2 > flDist2)
        {
            return pEntity;
        }
    }

    return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Finds an entity by target name or class name.
// Input  : pStartEntity - The entity to start from when doing the search.
//			szName - Entity name to search for. Treated as a target name first,
//				then as an entity class name, ie "info_target".
//			vecSrc - Center of search radius.
//			flRadius - Search radius for classname search, 0 to search everywhere.
//			pSearchingEntity - The entity that is doing the search.
//			pActivator - The activator entity if this was called from an input
//				or Use handler, NULL otherwise.
// Output : Returns a pointer to the found entity, NULL if none.
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityGeneric( CBaseEntity *pStartEntity, const char *szName, CBaseEntity *pSearchingEntity, CBaseEntity *pActivator, CBaseEntity *pCaller )
{
    CBaseEntity *pEntity = NULL;

    pEntity = gEntList.FindEntityByName( pStartEntity, szName, pSearchingEntity, pActivator, pCaller );
    if (!pEntity)
    {
        pEntity = gEntList.FindEntityByClassname( pStartEntity, szName );
    }

    return pEntity;
} 


//-----------------------------------------------------------------------------
// Purpose: Finds the first entity by target name or class name within a radius
// Input  : pStartEntity - The entity to start from when doing the search.
//			szName - Entity name to search for. Treated as a target name first,
//				then as an entity class name, ie "info_target".
//			vecSrc - Center of search radius.
//			flRadius - Search radius for classname search, 0 to search everywhere.
//			pSearchingEntity - The entity that is doing the search.
//			pActivator - The activator entity if this was called from an input
//				or Use handler, NULL otherwise.
// Output : Returns a pointer to the found entity, NULL if none.
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityGenericWithin( CBaseEntity *pStartEntity, const char *szName, const Vector &vecSrc, float flRadius, CBaseEntity *pSearchingEntity, CBaseEntity *pActivator, CBaseEntity *pCaller )
{
    CBaseEntity *pEntity = NULL;

    pEntity = gEntList.FindEntityByNameWithin( pStartEntity, szName, vecSrc, flRadius, pSearchingEntity, pActivator, pCaller );
    if (!pEntity)
    {
        pEntity = gEntList.FindEntityByClassnameWithin( pStartEntity, szName, vecSrc, flRadius );
    }

    return pEntity;
} 

//-----------------------------------------------------------------------------
// Purpose: Finds the nearest entity by target name or class name within a radius.
// Input  : pStartEntity - The entity to start from when doing the search.
//			szName - Entity name to search for. Treated as a target name first,
//				then as an entity class name, ie "info_target".
//			vecSrc - Center of search radius.
//			flRadius - Search radius for classname search, 0 to search everywhere.
//			pSearchingEntity - The entity that is doing the search.
//			pActivator - The activator entity if this was called from an input
//				or Use handler, NULL otherwise.
// Output : Returns a pointer to the found entity, NULL if none.
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityGenericNearest( const char *szName, const Vector &vecSrc, float flRadius, CBaseEntity *pSearchingEntity, CBaseEntity *pActivator, CBaseEntity *pCaller )
{
    CBaseEntity *pEntity = NULL;

    pEntity = gEntList.FindEntityByNameNearest( szName, vecSrc, flRadius, pSearchingEntity, pActivator, pCaller );
    if (!pEntity)
    {
        pEntity = gEntList.FindEntityByClassnameNearest( szName, vecSrc, flRadius );
    }

    return pEntity;
} 


//-----------------------------------------------------------------------------
// Purpose: Find the nearest entity along the facing direction from the given origin
//			within the angular threshold (ignores worldspawn) with the
//			given classname.
// Input  : origin - 
//			facing - 
//			threshold - 
//			classname - 
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityClassNearestFacing( const Vector &origin, const Vector &facing, float threshold, char *classname)
{
    float bestDot = threshold;
    CBaseEntity *best_ent = NULL;

    const CEntInfo *pInfo = FirstEntInfo();

    for ( ;pInfo; pInfo = pInfo->m_pNext )
    {
        CBaseEntity *ent = (CBaseEntity *)pInfo->m_pEntity;
        if ( !ent )
        {
            DevWarning( "NULL entity in global entity list!\n" );
            continue;
        }

        // FIXME: why is this skipping pointsize entities?
        //if (ent->IsPointSized() )
        //	continue;

        // Make vector to entity
        Vector	to_ent = (VFuncs::GetAbsOrigin(ent) - origin);

        VectorNormalize( to_ent );
        float dot = DotProduct (facing , to_ent );
        if (dot > bestDot) 
        {
            if (VFuncs::ClassMatches(ent,classname))
            {
                // Ignore if worldspawn
                if (!VFuncs::ClassMatches( ent, "worldspawn" )  && !VFuncs::ClassMatches( ent, "soundent")) 
                {
                    bestDot	= dot;
                    best_ent = ent;
                }
            }
        }
    }
    return best_ent;
}


//-----------------------------------------------------------------------------
// Purpose: Find the nearest entity along the facing direction from the given origin
//			within the angular threshold (ignores worldspawn)
// Input  : origin - 
//			facing - 
//			threshold - 
//-----------------------------------------------------------------------------
CBaseEntity *CGlobalEntityList::FindEntityNearestFacing( const Vector &origin, const Vector &facing, float threshold)
{
    float bestDot = threshold;
    CBaseEntity *best_ent = NULL;

    const CEntInfo *pInfo = FirstEntInfo();

    for ( ;pInfo; pInfo = pInfo->m_pNext )
    {
        CBaseEntity *ent = (CBaseEntity *)pInfo->m_pEntity;
        if ( !ent )
        {
            DevWarning( "NULL entity in global entity list!\n" );
            continue;
        }

        // Ignore logical entities
        if (!servergameents->BaseEntityToEdict(ent))
            continue;

        // Make vector to entity
        Vector	to_ent = VFuncs::WorldSpaceCenter(ent) - origin;
        VectorNormalize(to_ent);

        float dot = DotProduct( facing, to_ent );
        if (dot <= bestDot) 
            continue;

        // Ignore if worldspawn
        if (!FStrEq( VFuncs::GetClassname(ent), "worldspawn")  && !FStrEq( VFuncs::GetClassname(ent), "soundent")) 
        {
            bestDot	= dot;
            best_ent = ent;
        }
    }
    return best_ent;
}


void CGlobalEntityList::OnAddEntity( IHandleEntity *pEnt, CBaseHandle handle )
{
    int i = handle.GetEntryIndex();

    // record current list details
    m_iNumEnts++;
    if ( i > m_iHighestEnt )
        m_iHighestEnt = i;

    // If it's a CBaseEntity, notify the listeners.
    CBaseEntity *pBaseEnt = static_cast<IServerUnknown*>(pEnt)->GetBaseEntity();
    if ( servergameents->BaseEntityToEdict(pBaseEnt) )
        m_iNumEdicts++;
    
    // NOTE: Must be a CBaseEntity on server
    Assert( pBaseEnt );
    //DevMsg(2,"Created %s\n", pBaseEnt->GetClassname() );
    for ( i = m_entityListeners.Count()-1; i >= 0; i-- )
    {
        m_entityListeners[i]->OnEntityCreated( pBaseEnt );
    }
}


void CGlobalEntityList::OnRemoveEntity( IHandleEntity *pEnt, CBaseHandle handle )
{
#ifdef DEBUG
    if ( !g_fInCleanupDelete )
    {
        int i;
        for ( i = 0; i < g_DeleteList.Count(); i++ )
        {
            if ( g_DeleteList[i]->GetEntityHandle() == pEnt )
            {
                g_DeleteList.FastRemove( i );
                Msg( "ERROR: Entity being destroyed but previously threaded on g_DeleteList\n" );
                break;
            }
        }
    }
#endif

    CBaseEntity *pBaseEnt = static_cast<IServerUnknown*>(pEnt)->GetBaseEntity();
    if ( pBaseEnt->edict() )
        m_iNumEdicts--;

    m_iNumEnts--;
}

void CGlobalEntityList::NotifyCreateEntity( CBaseEntity *pEnt )
{
    if ( !pEnt )
        return;

    //DevMsg(2,"Deleted %s\n", pBaseEnt->GetClassname() );
    for ( int i = m_entityListeners.Count()-1; i >= 0; i-- )
    {
        m_entityListeners[i]->OnEntityCreated( pEnt );
    }
}

void CGlobalEntityList::NotifySpawn( CBaseEntity *pEnt )
{
    if ( !pEnt )
        return;

    //DevMsg(2,"Deleted %s\n", pBaseEnt->GetClassname() );
    for ( int i = m_entityListeners.Count()-1; i >= 0; i-- )
    {
        m_entityListeners[i]->OnEntitySpawned( pEnt );
    }
}

// NOTE: This doesn't happen in OnRemoveEntity() specifically because 
// listeners may want to reference the object as it's being deleted
// OnRemoveEntity isn't called until the destructor and all data is invalid.
void CGlobalEntityList::NotifyRemoveEntity( CBaseHandle hEnt )
{
    CBaseEntity *pBaseEnt = GetBaseEntity( hEnt );
    if ( !pBaseEnt )
        return;

    //DevMsg(2,"Deleted %s\n", pBaseEnt->GetClassname() );
    for ( int i = m_entityListeners.Count()-1; i >= 0; i-- )
    {
        m_entityListeners[i]->OnEntityDeleted( pBaseEnt );
    }
}