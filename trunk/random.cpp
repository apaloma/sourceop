// HACK HACK HACK : BASEENTITY ACCESS
#define GAME_DLL
#include "fixdebug.h"
#ifdef _L4D_PLUGIN
#include "convar_l4d.h"
#include "datamap_l4d.h"
#include "dt_send_l4d.h"
#endif
#include "cbase.h"
#include "baseentity.h"
// HACK HACK HACK : BASEENTITY ACCESS
#include "baseanimating.h"
#include "animation.h"
#include "studio.h"
#include "bone_setup.h"
#include "datacache/imdlcache.h"

#include "beam_flags.h" 

#include <stdio.h>
#include <ctype.h>
#include <time.h>
#ifndef __linux__
#include <IO.H>
#endif
#include "AdminOP.h"
#include "recipientfilter.h"

#include "vphysics_interface.h"
#include "isaverestore.h"
#include "mempool.h"

#include "bitbuf.h"
#include "cvars.h"
#include "download.h"
#include "mempatcher.h"
#include "randomstuff.h"
#include "vfuncs.h"

#include "tier0/memdbgon.h"

int g_nInsideDispatchUpdateTransmitState = 0;

// When this is false, throw an assert in debug when GetAbsAnything is called. Used when hierachy is incomplete/invalid.
bool CBaseEntity::s_bAbsQueriesValid = true;

bool CGameTrace::DidHitNonWorldEntity() const
{
    return GetEntityIndex() > 0;
}


int CGameTrace::GetEntityIndex() const
{
    if ( m_pEnt )
        return VFuncs::entindex(m_pEnt);
    else
        return -1;
}

#ifdef _L4D_PLUGIN
#ifdef _LINUX
long __cdecl _InterlockedIncrement(volatile long*)
{
}
long ThreadInterlockedIncrement( long volatile * )
{
    Msg("[SOURCEOP] Warning: ThreadInterlockedIncrement called in SourceOP.\n");
}
#else
void ConCommandBase::RemoveFlags( int flags )
{
	m_nFlags &= ~flags;
}
int ConCommandBase::GetFlags() const
{
	return m_nFlags;
}
#endif
#endif

IPhysicsConstraint *MakeConstraint( CBaseEntity *pObj, CBaseEntity *pObject )
{
    IPhysicsConstraint  *m_pConstraint;
    IPhysicsObject *pReference = VFuncs::VPhysicsGetObject(pObj);
    /*if ( GetMoveParent() )
    {
        pReference = GetMoveParentVFuncs::VPhysicsGetObject(());
    }*/
    IPhysicsObject *pAttached = VFuncs::VPhysicsGetObject(pObject);
    if ( !pReference || !pAttached )
    {
        return NULL;
    }

    constraint_fixedparams_t fixed;
    fixed.Defaults();
    fixed.InitWithCurrentObjectState( pReference, pAttached );

    m_pConstraint = physenv->CreateFixedConstraint( pReference, pAttached, NULL, fixed );
    //m_pConstraint->SetGameData( (void *)pObj );

    //MakeInert();
    return m_pConstraint;
}


void DoPlant(CBaseEntity *pEntity)
{
#ifdef _WIN32
    _asm
    {
        mov     ecx,dword ptr [esp+4]
        mov     eax,dword ptr [ecx]
        jmp     PlantBomb
    }
#else
    /*
    __asm__(".intel_syntax noprefix");
    __asm__("mov ecx,dword ptr [esp+4]\n\t"
            "mov eax,dword ptr [ecx]\n\t"
            "jmp PlantBomb");*/
    __asm__("movl 4(%esp),%ecx\n\t"
            "movl (%ecx),%eax\n\t"
            "jmp PlantBomb");
#endif
}

void CBaseEntity::VPhysicsSwapObject( IPhysicsObject *pSwap )
{
    if ( !pSwap )
    {
        //PhysRemoveShadow(this);
    }

    if ( !m_pPhysicsObject )
    {
        Warning( "Bad vphysics swap for %s\n", VFuncs::GetClassname(this) );
    }
    VFuncs::VPhysicsSetObject(this, pSwap);
}

void CBaseEntity::CalcAbsolutePosition( void )
{
}

void CBaseEntity::CalcAbsoluteVelocity( void )
{
}

int CBaseEntity::DispatchUpdateTransmitState()
{
    edict_t *ed = servergameents->BaseEntityToEdict((CBaseEntity*)this);
    if ( m_nTransmitStateOwnedCounter != 0 )
        return ed ? ed->m_fStateFlags : 0;
    
    g_nInsideDispatchUpdateTransmitState++;
    int ret = VFuncs::UpdateTransmitState(this);
    g_nInsideDispatchUpdateTransmitState--;
    
    return ret;
}

#include <vcollide_parse.h>
#include "solidsetdefaults.h"
#include "model_types.h"

void CSolidSetDefaults::ParseKeyValue( void *pData, const char *pKey, const char *pValue )
{
    if ( !Q_stricmp( pKey, "contents" ) )
    {
        m_contentsMask = atoi( pValue );
    }
}

void CSolidSetDefaults::SetDefaults( void *pData )
{
    solid_t *pSolid = (solid_t *)pData;
    pSolid->params = g_PhysDefaultObjectParams;
    m_contentsMask = CONTENTS_SOLID;
}

CSolidSetDefaults g_SolidSetup;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void PhysGetDefaultAABBSolid( solid_t &solid )
{
    solid.params = g_PhysDefaultObjectParams;
    solid.params.mass = 85.0f;
    solid.params.inertia = 1e24f;
    Q_strncpy( solid.surfaceprop, "default", sizeof( solid.surfaceprop ) );
}

bool PhysModelParseSolidByIndex( solid_t &solid, CBaseEntity *pEntity, vcollide_t *pCollide, int solidIndex )
{
    bool parsed = false;

    memset( &solid, 0, sizeof(solid) );
    solid.params = g_PhysDefaultObjectParams;

    IVPhysicsKeyParser *pParse = physcollision->VPhysicsKeyParserCreate( pCollide->pKeyValues );
    while ( !pParse->Finished() )
    {
        const char *pBlock = pParse->GetCurrentBlockName();
        if ( !strcmpi( pBlock, "solid" ) )
        {
            solid_t tmpSolid;
            memset( &tmpSolid, 0, sizeof(tmpSolid) );
            tmpSolid.params = g_PhysDefaultObjectParams;

            pParse->ParseSolid( &tmpSolid, &g_SolidSetup );

            if ( solidIndex < 0 || tmpSolid.index == solidIndex )
            {
                parsed = true;
                solid = tmpSolid;
                // just to be sure we aren't ever getting a non-zero solid by accident
                Assert( solidIndex >= 0 || solid.index == 0 );
                break;
            }
        }
        else
        {
            pParse->SkipBlock();
        }
    }
    physcollision->VPhysicsKeyParserDestroy( pParse );

    // collisions are off by default
    solid.params.enableCollisions = true;

    solid.params.pGameData = static_cast<void *>(pEntity);
    solid.params.pName = STRING(VFuncs::GetModelName(pEntity));
    return parsed;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &mins - 
//          &maxs - 
// Output : CPhysCollide
//-----------------------------------------------------------------------------
CPhysCollide *PhysCreateBbox( const Vector &minsIn, const Vector &maxsIn )
{
    // UNDONE: Track down why this causes errors for the player controller and adjust/enable
    //float radius = 0.5 - DIST_EPSILON;
    Vector mins = minsIn;// + Vector(radius, radius, radius);
    Vector maxs = maxsIn;// - Vector(radius, radius, radius);

    // VPHYSICS caches/cleans up these
    CPhysCollide *pResult = physcollision->BBoxToCollide( mins, maxs );

    //g_pPhysSaveRestoreManager->NoteBBox( mins, maxs, pResult );
    
    return pResult;
}

//-----------------------------------------------------------------------------
// Purpose: Create a vphysics object based on an existing collision model
// Input  : *pEntity - 
//          *pModel - 
//          &origin - 
//          &angles - 
//          *pName - 
//          isStatic - 
//          *pSolid - 
// Output : IPhysicsObject
//-----------------------------------------------------------------------------
IPhysicsObject *PhysModelCreateCustom( CBaseEntity *pEntity, const CPhysCollide *pModel, const Vector &origin, const QAngle &angles, const char *pName, bool isStatic, solid_t *pSolid )
{
    solid_t tmpSolid;
    if ( !pSolid )
    {
        PhysGetDefaultAABBSolid( tmpSolid );
        pSolid = &tmpSolid;
    }
    int surfaceProp = physprops->GetSurfaceIndex( pSolid->surfaceprop );
    pSolid->params.pGameData = static_cast<void *>(pEntity);
    pSolid->params.pName = pName;
    IPhysicsObject *pObject = NULL;
    if ( isStatic )
    {
        pObject = physenv->CreatePolyObjectStatic( pModel, surfaceProp, origin, angles, &pSolid->params );
    }
    else
    {
        pObject = physenv->CreatePolyObject( pModel, surfaceProp, origin, angles, &pSolid->params );
    }

    //if ( pObject )
    //  g_pPhysSaveRestoreManager->AssociateModel( pObject, pModel);

    return pObject;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntity - 
//          &mins - 
//          &maxs - 
//          &origin - 
//          isStatic - 
// Output : static IPhysicsObject
//-----------------------------------------------------------------------------
IPhysicsObject *PhysModelCreateBox( CBaseEntity *pEntity, const Vector &mins, const Vector &maxs, const Vector &origin, bool isStatic )
{
    int modelIndex = VFuncs::GetModelIndex(pEntity);
    const char *pSurfaceProps = "flesh";
    solid_t solid;
    PhysGetDefaultAABBSolid( solid );
    Vector dims = maxs - mins;
    solid.params.volume = dims.x * dims.y * dims.z;

    if ( modelIndex )
    {
        const model_t *model = modelinfo->GetModel( modelIndex );
        if ( model )
        {
            if ( modelinfo->GetModelType( model ) == mod_studio )
            {
                studiohdr_t *pstudiohdr = static_cast< studiohdr_t * >( modelinfo->GetModelExtraData( model ) );
                //pSurfaceProps = Studio_GetDefaultSurfaceProps( pstudiohdr );
                pSurfaceProps = pstudiohdr->pszSurfaceProp();
            }
        }
    }
    Q_strncpy( solid.surfaceprop, pSurfaceProps, sizeof( solid.surfaceprop ) );

    CPhysCollide *pCollide = PhysCreateBbox( mins, maxs );
    if ( !pCollide )
        return NULL;
    
    return PhysModelCreateCustom( pEntity, pCollide, origin, vec3_angle, STRING(VFuncs::GetModelName(pEntity)), isStatic, &solid );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntity - 
//          modelIndex - 
//          &origin - 
//          &angles - 
// Output : IPhysicsObject
//-----------------------------------------------------------------------------
IPhysicsObject *PhysModelCreateUnmoveable( CBaseEntity *pEntity, int modelIndex, const Vector &origin, const QAngle &angles )
{
    vcollide_t *pCollide = modelinfo->GetVCollide( modelIndex );
    if ( !pCollide || !pCollide->solidCount )
        return NULL;

    solid_t solid;

    if ( !PhysModelParseSolidByIndex( solid, pEntity, pCollide, -1 ) )
        return NULL;

    // collisions are off by default
    solid.params.enableCollisions = true;
    //solid.params.mass = 1.0;
    int surfaceProp = -1;
    if ( solid.surfaceprop[0] )
    {
        surfaceProp = physprops->GetSurfaceIndex( solid.surfaceprop );
    }
    solid.params.pGameData = static_cast<void *>(pEntity);
    solid.params.pName = STRING(VFuncs::GetModelName(pEntity));
    IPhysicsObject *pObject = physenv->CreatePolyObjectStatic( pCollide->solids[0], surfaceProp, origin, angles, &solid.params );

    //PhysCheckAdd( pObject, VFuncs::GetClassname(pEntity) );
    if ( pObject )
    {
        if ( modelinfo->GetModelType(modelinfo->GetModel(modelIndex)) == mod_brush )
        {
            unsigned int contents = modelinfo->GetModelContents( modelIndex );
            Assert(contents!=0);
            if ( contents != pObject->GetContents() && contents != 0 )
            {
                pObject->SetContents( contents );
                pObject->RecheckCollisionFilter();
            }
        }
        //g_pPhysSaveRestoreManager->AssociateModel( pObject, modelIndex);
    }

    return pObject;
}

IPhysicsObject *PhysModelCreate( CBaseEntity *pEntity, int modelIndex, const Vector &origin, const QAngle &angles, solid_t *pSolid )
{
    vcollide_t *pCollide = modelinfo->GetVCollide( modelIndex );
    if ( !pCollide || !pCollide->solidCount )
        return NULL;
    
    solid_t tmpSolid;
    if ( !pSolid )
    {
        pSolid = &tmpSolid;
        if ( !PhysModelParseSolidByIndex( tmpSolid, pEntity, pCollide, -1 ) )
            return NULL;
    }

    int surfaceProp = -1;
    if ( pSolid->surfaceprop[0] )
    {
        surfaceProp = physprops->GetSurfaceIndex( pSolid->surfaceprop );
    }
    //Msg("%x %x\n", pCollide, pSolid);
    //Msg("%i\n", pSolid->index);
    IPhysicsObject *pObject = physenv->CreatePolyObject( pCollide->solids[pSolid->index], surfaceProp, origin, angles, &pSolid->params );
    //PhysCheckAdd( pObject, VFuncs::GetClassname(pEntity) );

    if ( pObject )
    {
        if ( modelinfo->GetModelType(modelinfo->GetModel(modelIndex)) == mod_brush )
        {
            unsigned int contents = modelinfo->GetModelContents( modelIndex );
            Assert(contents!=0);
            // HACKHACK: contents is used to filter collisions
            // HACKHACK: So keep solid on for water brushes since they should pass collision rules (as triggers)
            if ( contents & MASK_WATER )
            {
                contents |= CONTENTS_SOLID;
            }
            if ( contents != pObject->GetContents() && contents != 0 )
            {
                pObject->SetContents( contents );
                pObject->RecheckCollisionFilter();
            }
        }

        //g_pPhysSaveRestoreManager->AssociateModel( pObject, modelIndex);
    }

    return pObject;
}

//-----------------------------------------------------------------------------
// Purpose: This creates a normal vphysics simulated object
//          physics alone determines where it goes (gravity, friction, etc)
//          and the entity receives updates from vphysics.  SetAbsOrigin(), etc do not affect the object!
//-----------------------------------------------------------------------------
IPhysicsObject *CBaseEntity::VPhysicsInitNormal( SolidType_t solidType, int nSolidFlags, bool createAsleep, solid_t *pSolid )
{
    //if ( !VPhysicsInitSetup() )
    //  return NULL;

    // NOTE: This has to occur before PhysModelCreate because that call will
    // call back into ShouldCollide(), which uses solidtype for rules.
    VFuncs::SetSolid( this, solidType );
    VFuncs::SetSolidFlags( this, nSolidFlags );

    // No physics
    if ( solidType == SOLID_NONE )
    {
        return NULL;
    }

    // create a normal physics object
    IPhysicsObject *pPhysicsObject = PhysModelCreate( this, VFuncs::GetModelIndex(this), VFuncs::GetAbsOrigin(this), VFuncs::GetAbsAngles(this), pSolid );
    if ( pPhysicsObject )
    {
        VFuncs::VPhysicsSetObject( this, pPhysicsObject );
        VFuncs::SetMoveType( this, MOVETYPE_VPHYSICS );

        if ( !createAsleep )
        {
            pPhysicsObject->Wake();
        }
    }

    return pPhysicsObject;
}

bool PhysIsInCallback()
{
    if ( physenv->IsInSimulation() /*|| g_Collisions.IsInCallback()*/ )
        return true;

    return false;
}

void CBaseEntity::CollisionRulesChanged()
{
    // ivp maintains state based on recent return values from the collision filter, so anything
    // that can change the state that a collision filter will return (like m_Solid) needs to call RecheckCollisionFilter.
    if ( VFuncs::VPhysicsGetObject(this) )
    {
#ifndef CLIENT_DLL
        extern bool PhysIsInCallback();
        if ( PhysIsInCallback() )
        {
            Warning("[SOURCEOP] Changing collision rules within a callback is likely to cause crashes!\n");
            Assert(0);
        }
#endif
        IPhysicsObject *pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
        int count = VFuncs::VPhysicsGetObjectList( this, pList, ARRAYSIZE(pList) );
        for ( int i = 0; i < count; i++ )
        {
            if ( pList[i] != NULL ) //this really shouldn't happen, but it does >_<
                pList[i]->RecheckCollisionFilter();
        }
    }
}

model_t *CBaseEntity::GetModel( void )
{
    return (model_t *)modelinfo->GetModel( VFuncs::GetModelIndex(this) );
}

bool CBaseEntity::IsInWorld( void ) const
{  
    if ( !servergameents->BaseEntityToEdict((CBaseEntity*)this) )
        return true;

    Vector vecOrigin = VFuncs::GetAbsOrigin(this);
    Vector vecVelocity = VFuncs::GetAbsVelocity(this);
    // position 
    if (vecOrigin.x >= MAX_COORD_INTEGER) return false;
    if (vecOrigin.y >= MAX_COORD_INTEGER) return false;
    if (vecOrigin.z >= MAX_COORD_INTEGER) return false;
    if (vecOrigin.x <= MIN_COORD_INTEGER) return false;
    if (vecOrigin.y <= MIN_COORD_INTEGER) return false;
    if (vecOrigin.z <= MIN_COORD_INTEGER) return false;
    // speed
    if (vecVelocity.x >= 2000) return false;
    if (vecVelocity.y >= 2000) return false;
    if (vecVelocity.z >= 2000) return false;
    if (vecVelocity.x <= -2000) return false;
    if (vecVelocity.y <= -2000) return false;
    if (vecVelocity.z <= -2000) return false;

    return true;
}




//=========================================================
//=========================================================
void CBaseAnimating::SetSequence( int nSequence )
{
    //Assert( GetModelPtr( ) && nSequence < GetModelPtr( )->GetNumSeq() );
    VFuncs::SetSequence(this, nSequence);
    //m_nSequence = nSequence;
}



trace_t g_TouchTrace;
const trace_t &CBaseEntity::GetTouchTrace( void )
{
    return g_TouchTrace;
}


//-----------------------------------------------------------------------------
// Purpose: Marks the fact that two edicts are in contact
// Input  : *other - other entity
//-----------------------------------------------------------------------------
void CBaseEntity::PhysicsMarkEntitiesAsTouching( CBaseEntity *other, trace_t &trace )
{
    g_TouchTrace = trace;
    //PhysicsMarkEntityAsTouched( other );
    //other->PhysicsMarkEntityAsTouched( this );
}

//-----------------------------------------------------------------------------
// Purpose: Init this object's physics as a static
//-----------------------------------------------------------------------------
IPhysicsObject *CBaseEntity::VPhysicsInitStatic( void )
{
    //if ( !VPhysicsInitSetup() )
    //  return NULL;

#ifndef CLIENT_DLL
    // If this entity has a move parent, it needs to be shadow, not static
    /*if ( GetMoveParent() )
    {
        // must be SOLID_VPHYSICS if in hierarchy to solve collisions correctly
        if ( GetSolid() == SOLID_BSP && GetRootMoveParent()->GetSolid() != SOLID_BSP )
        {
            VFuncs::SetSolid( this, SOLID_VPHYSICS );
        }

        return VPhysicsInitShadow( false, false );
    }*/
#endif

    // No physics
    if ( GetSolid() == SOLID_NONE )
        return NULL;

    // create a static physics objct
    IPhysicsObject *pPhysicsObject = NULL;
    if ( GetSolid() == SOLID_BBOX )
    {
        pPhysicsObject = PhysModelCreateBox( this, WorldAlignMins(), WorldAlignMaxs(), GetAbsOrigin(), true );
    }
    else
    {
        pPhysicsObject = PhysModelCreateUnmoveable( this, GetModelIndex(), GetAbsOrigin(), GetAbsAngles() );
    }
    VFuncs::VPhysicsSetObject( this, pPhysicsObject );
    return pPhysicsObject;
}

VMatrix SetupMatrixOrgAngles(const Vector &origin, const QAngle &vAngles)
{
    VMatrix mRet;
    mRet.SetupMatrixOrgAngles( origin, vAngles );
    return mRet;
}

void VMatrix::SetupMatrixOrgAngles( const Vector &origin, const QAngle &vAngles )
{
    float       sr, sp, sy, cr, cp, cy;

    SinCos( DEG2RAD( vAngles[YAW] ), &sy, &cy );
    SinCos( DEG2RAD( vAngles[PITCH] ), &sp, &cp );
    SinCos( DEG2RAD( vAngles[ROLL] ), &sr, &cr );

    // matrix = (YAW * PITCH) * ROLL
    m[0][0] = cp*cy;
    m[1][0] = cp*sy;
    m[2][0] = -sp;
    m[0][1] = sr*sp*cy+cr*-sy;
    m[1][1] = sr*sp*sy+cr*cy;
    m[2][1] = sr*cp;
    m[0][2] = (cr*sp*cy+-sr*-sy);
    m[1][2] = (cr*sp*sy+-sr*cy);
    m[2][2] = cr*cp;
    m[0][3] = 0.f;
    m[1][3] = 0.f;
    m[2][3] = 0.f;
    
    // Add translation
    m[0][3] = origin.x;
    m[1][3] = origin.y;
    m[2][3] = origin.z;
    m[3][0] = 0.0f;
    m[3][1] = 0.0f;
    m[3][2] = 0.0f;
    m[3][3] = 1.0f;
}

void DebugDrawLine( const Vector& vecAbsStart, const Vector& vecAbsEnd, int r, int g, int b, bool test, float duration )
{
    // do nothing
}

void CBaseEntity::DispatchTraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
//#ifdef GAME_DLL
    // Make sure our damage filter allows the damage.
    if ( !VFuncs::PassesDamageFilter( this, info ))
    {
        return;
    }
//#endif

    VFuncs::TraceAttack( this, info, vecDir, ptr, NULL );
}

//------------------------------------------------------------------------------
// Purpose : If name exists returns name, otherwise returns classname
// Input   :
// Output  :
//------------------------------------------------------------------------------
const char *CBaseEntity::GetDebugName(void)
{
    if ( this == NULL )
        return "<<null>>";

    if ( VFuncs::GetEntityName(this) != NULL_STRING ) 
    {
        return STRING(VFuncs::GetEntityName(this));
    }
    else
    {
        return VFuncs::GetClassname(this);
    }
}

//-----------------------------------------------------------------------------
// Purpose: Finds the nearest entity in front of the player, preferring
//          collidable entities, but allows selection of enities that are
//          on the other side of walls or objects
// Input  :
// Output :
//-----------------------------------------------------------------------------
CBaseEntity *FindPickerEntity( CBasePlayer *pPlayer )
{
    MDLCACHE_CRITICAL_SECTION();

    // First try to trace a hull to an entity
    int entid = VFuncs::entindex(pPlayer);
    CBaseEntity *pEntity = NULL;
    if(entid > 0 && entid <= pAdminOP.GetMaxClients())
    {
        pEntity = pAdminOP.pAOPPlayers[entid-1].FindEntityForward( );
    }

    // If that fails just look for the nearest facing entity
    if (!pEntity) 
    {
        Vector forward;
        Vector origin;
        QAngle playerAngles = VFuncs::EyeAngles(pPlayer);

        AngleVectors( playerAngles, &forward, NULL, NULL );
        //pPlayer->EyeVectors( &forward );
        origin = VFuncs::WorldSpaceCenter(pPlayer);     
        pEntity = gEntList.FindEntityNearestFacing( origin, forward,0.95);
    }
    return pEntity;
}

//-----------------------------------------------------------------------------
// Purpose: iterates through a typedescript data block, so it can insert key/value data into the block
// Input  : *pObject - pointer to the struct or class the data is to be insterted into
//          *pFields - description of the data
//          iNumFields - number of fields contained in pFields
//          char *szKeyName - name of the variable to look for
//          char *szValue - value to set the variable to
// Output : Returns true if the variable is found and set, false if the key is not found.
//-----------------------------------------------------------------------------
bool ParseKeyvalue( void *pObject, typedescription_t *pFields, int iNumFields, const char *szKeyName, const char *szValue )
{
    int i;
    typedescription_t   *pField;

    for ( i = 0; i < iNumFields; i++ )
    {
        pField = &pFields[i];

        int fieldOffset = pField->fieldOffset[ TD_OFFSET_NORMAL ];

        // Check the nested classes, but only if they aren't in array form.
        if ((pField->fieldType == FIELD_EMBEDDED) && (pField->fieldSize == 1))
        {
            for ( datamap_t *dmap = pField->td; dmap != NULL; dmap = dmap->baseMap )
            {
                void *pEmbeddedObject = (void*)((char*)pObject + fieldOffset);
                if ( ParseKeyvalue( pEmbeddedObject, dmap->dataDesc, dmap->dataNumFields, szKeyName, szValue) )
                    return true;
            }
        }

        if ( (pField->flags & FTYPEDESC_KEY) && !stricmp(pField->externalName, szKeyName) )
        {
            switch( pField->fieldType )
            {
            case FIELD_MODELNAME:
            case FIELD_SOUNDNAME:
            case FIELD_STRING:
                (*(string_t *)((char *)pObject + fieldOffset)) = AllocPooledString( szValue );
                return true;

            case FIELD_TIME:
            case FIELD_FLOAT:
                (*(float *)((char *)pObject + fieldOffset)) = atof( szValue );
                return true;

            case FIELD_BOOLEAN:
                (*(bool *)((char *)pObject + fieldOffset)) = (bool)(atoi( szValue ) != 0);
                return true;

            case FIELD_CHARACTER:
                (*(char *)((char *)pObject + fieldOffset)) = (char)atoi( szValue );
                return true;

            case FIELD_SHORT:
                (*(short *)((char *)pObject + fieldOffset)) = (short)atoi( szValue );
                return true;

            case FIELD_INTEGER:
            case FIELD_TICK:
                (*(int *)((char *)pObject + fieldOffset)) = atoi( szValue );
                return true;

            case FIELD_POSITION_VECTOR:
            case FIELD_VECTOR:
                UTIL_StringToVector( (float *)((char *)pObject + fieldOffset), szValue );
                return true;

            case FIELD_VMATRIX:
            case FIELD_VMATRIX_WORLDSPACE:
                UTIL_StringToFloatArray( (float *)((char *)pObject + fieldOffset), 16, szValue );
                return true;

            case FIELD_MATRIX3X4_WORLDSPACE:
                UTIL_StringToFloatArray( (float *)((char *)pObject + fieldOffset), 12, szValue );
                return true;

            case FIELD_COLOR32:
                UTIL_StringToColor32( (color32 *) ((char *)pObject + fieldOffset), szValue );
                return true;

            case FIELD_CUSTOM:
            {
                SaveRestoreFieldInfo_t fieldInfo =
                {
                    (char *)pObject + fieldOffset,
                    pObject,
                    pField
                };
                pField->pSaveRestoreOps->Parse( fieldInfo, szValue );
                return true;
            }

            default:
            case FIELD_INTERVAL: // Fixme, could write this if needed
            case FIELD_CLASSPTR:
            case FIELD_MODELINDEX:
            case FIELD_MATERIALINDEX:
            case FIELD_EDICT:
                Warning( "Bad field in entity!!\n" );
                Assert(0);
                break;
            }
        }
    }

    return false;
}

//-----------------------------------------------------------------------------
// Purpose: this entity is exploding, or otherwise needs to inflict damage upon 
//          entities within a certain range.  only damage ents that can clearly 
//          be seen by the explosion!
// Input  :
// Output :
//----------------------------------------------------------------------------- 
void RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrc, float flRadius, int iClassIgnore, CBaseEntity *pEntityIgnore )
{
    if(_RadiusDamage)
    {
        (_RadiusDamage)(info, vecSrc, flRadius, iClassIgnore, pEntityIgnore);
    }
}

////////////////////////// variant_t implementation //////////////////////////

// BUGBUG: Add support for function pointer save/restore to variants
// BUGBUG: Must pass datamap_t to read/write fields 
void variant_t::Set( fieldtype_t ftype, void *data )
{
    fieldType = ftype;

    switch ( ftype )
    {
    case FIELD_BOOLEAN:     bVal = *((bool *)data);             break;
    case FIELD_CHARACTER:   iVal = *((char *)data);             break;
    case FIELD_SHORT:       iVal = *((short *)data);            break;
    case FIELD_INTEGER:     iVal = *((int *)data);              break;
    case FIELD_STRING:      iszVal = *((string_t *)data);       break;
    case FIELD_FLOAT:       flVal = *((float *)data);           break;
    case FIELD_COLOR32:     rgbaVal = *((color32 *)data);       break;

    case FIELD_VECTOR:
    case FIELD_POSITION_VECTOR:
    {
        vecVal[0] = ((float *)data)[0];
        vecVal[1] = ((float *)data)[1];
        vecVal[2] = ((float *)data)[2];
        break;
    }

    case FIELD_EHANDLE:     eVal = *((EHANDLE *)data);          break;
    case FIELD_CLASSPTR:    eVal = *((CBaseEntity **)data);     break;
    case FIELD_VOID:        
    default:
        iVal = 0; fieldType = FIELD_VOID;   
        break;
    }
}

//-----------------------------------------------------------------------------
// Purpose: Copies the value in the variant into a block of memory
// Input  : *data - the block to write into
//-----------------------------------------------------------------------------
void variant_t::SetOther( void *data )
{
    switch ( fieldType )
    {
    case FIELD_BOOLEAN:     *((bool *)data) = bVal != 0;        break;
    case FIELD_CHARACTER:   *((char *)data) = iVal;             break;
    case FIELD_SHORT:       *((short *)data) = iVal;            break;
    case FIELD_INTEGER:     *((int *)data) = iVal;              break;
    case FIELD_STRING:      *((string_t *)data) = iszVal;       break;
    case FIELD_FLOAT:       *((float *)data) = flVal;           break;
    case FIELD_COLOR32:     *((color32 *)data) = rgbaVal;       break;

    case FIELD_VECTOR:
    case FIELD_POSITION_VECTOR:
    {
        ((float *)data)[0] = vecVal[0];
        ((float *)data)[1] = vecVal[1];
        ((float *)data)[2] = vecVal[2];
        break;
    }

    case FIELD_EHANDLE:     *((EHANDLE *)data) = eVal;          break;
    case FIELD_CLASSPTR:    *((CBaseEntity **)data) = eVal;     break;
    }
}


//-----------------------------------------------------------------------------
// Purpose: Converts the variant to a new type. This function defines which I/O
//          types can be automatically converted between. Connections that require
//          an unsupported conversion will cause an error message at runtime.
// Input  : newType - the type to convert to
// Output : Returns true on success, false if the conversion is not legal
//-----------------------------------------------------------------------------
bool variant_t::Convert( fieldtype_t newType )
{
    if ( newType == fieldType )
    {
        return true;
    }

    //
    // Converting to a null value is easy.
    //
    if ( newType == FIELD_VOID )
    {
        Set( FIELD_VOID, NULL );
        return true;
    }

    //
    // FIELD_INPUT accepts the variant type directly.
    //
    if ( newType == FIELD_INPUT )
    {
        return true;
    }

    switch ( fieldType )
    {
        case FIELD_INTEGER:
        {
            switch ( newType )
            {
                case FIELD_FLOAT:
                {
                    SetFloat( (float) iVal );
                    return true;
                }

                case FIELD_BOOLEAN:
                {
                    SetBool( iVal != 0 );
                    return true;
                }
            }
            break;
        }

        case FIELD_FLOAT:
        {
            switch ( newType )
            {
                case FIELD_INTEGER:
                {
                    SetInt( (int) flVal );
                    return true;
                }

                case FIELD_BOOLEAN:
                {
                    SetBool( flVal != 0 );
                    return true;
                }
            }
            break;
        }

        //
        // Everyone must convert from FIELD_STRING if possible, since
        // parameter overrides are always passed as strings.
        //
        case FIELD_STRING:
        {
            switch ( newType )
            {
                case FIELD_INTEGER:
                {
                    if (iszVal != NULL_STRING)
                    {
                        SetInt(atoi(STRING(iszVal)));
                    }
                    else
                    {
                        SetInt(0);
                    }
                    return true;
                }

                case FIELD_FLOAT:
                {
                    if (iszVal != NULL_STRING)
                    {
                        SetFloat(atof(STRING(iszVal)));
                    }
                    else
                    {
                        SetFloat(0);
                    }
                    return true;
                }

                case FIELD_BOOLEAN:
                {
                    if (iszVal != NULL_STRING)
                    {
                        SetBool( atoi(STRING(iszVal)) != 0 );
                    }
                    else
                    {
                        SetBool(false);
                    }
                    return true;
                }

                case FIELD_VECTOR:
                {
                    Vector tmpVec = vec3_origin;
                    if (sscanf(STRING(iszVal), "[%f %f %f]", &tmpVec[0], &tmpVec[1], &tmpVec[2]) == 0)
                    {
                        // Try sucking out 3 floats with no []s
                        sscanf(STRING(iszVal), "%f %f %f", &tmpVec[0], &tmpVec[1], &tmpVec[2]);
                    }
                    SetVector3D( tmpVec );
                    return true;
                }

                case FIELD_COLOR32:
                {
                    int nRed = 0;
                    int nGreen = 0;
                    int nBlue = 0;
                    int nAlpha = 255;

                    sscanf(STRING(iszVal), "%d %d %d %d", &nRed, &nGreen, &nBlue, &nAlpha);
                    SetColor32( nRed, nGreen, nBlue, nAlpha );
                    return true;
                }

                case FIELD_EHANDLE:
                {
                    // convert the string to an entity by locating it by classname
                    CBaseEntity *ent = NULL;
                    if ( iszVal != NULL_STRING )
                    {
                        // FIXME: do we need to pass an activator in here?
                        ent = gEntList.FindEntityByName( NULL, iszVal );
                    }
                    SetEntity( ent );
                    //CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] FIELD_EHANDLE not supported for new type!\n");
                    return true;
                }
            }
        
            break;
        }

        case FIELD_EHANDLE:
        {
            switch ( newType )
            {
                case FIELD_STRING:
                {
                    // take the entities targetname as the string
                    string_t iszStr = NULL_STRING;
                    if ( eVal != NULL )
                    {
                        SetString( eVal->GetEntityName() );
                    }
                    //CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] FIELD_EHANDLE not supported!\n");
                    return true;
                }
            }
            break;
        }
    }

    // invalid conversion
    return false;
}

//-----------------------------------------------------------------------------
// Purpose: All types must be able to display as strings for debugging purposes.
// Output : Returns a pointer to the string that represents this value.
//
//          NOTE: The returned pointer should not be stored by the caller as
//                subsequent calls to this function will overwrite the contents
//                of the buffer!
//-----------------------------------------------------------------------------
const char *variant_t::ToString( void ) const
{
    COMPILE_TIME_ASSERT( sizeof(string_t) == sizeof(int) );

    static char szBuf[512];

    switch (fieldType)
    {
    case FIELD_STRING:
        {
            return(STRING(iszVal));
        }

    case FIELD_BOOLEAN:
        {
            if (bVal == 0)
            {
                Q_strncpy(szBuf, "false",sizeof(szBuf));
            }
            else
            {
                Q_strncpy(szBuf, "true",sizeof(szBuf));
            }
            return(szBuf);
        }

    case FIELD_INTEGER:
        {
            Q_snprintf( szBuf, sizeof( szBuf ), "%i", iVal );
            return(szBuf);
        }

    case FIELD_FLOAT:
        {
            Q_snprintf(szBuf,sizeof(szBuf), "%g", flVal);
            return(szBuf);
        }

    case FIELD_COLOR32:
        {
            Q_snprintf(szBuf,sizeof(szBuf), "%d %d %d %d", (int)rgbaVal.r, (int)rgbaVal.g, (int)rgbaVal.b, (int)rgbaVal.a);
            return(szBuf);
        }

    case FIELD_VECTOR:
        {
            Q_snprintf(szBuf,sizeof(szBuf), "[%g %g %g]", (double)vecVal[0], (double)vecVal[1], (double)vecVal[2]);
            return(szBuf);
        }

    case FIELD_VOID:
        {
            szBuf[0] = '\0';
            return(szBuf);
        }

    case FIELD_EHANDLE:
        {
            const char *pszName = (Entity()) ? STRING(VFuncs::GetEntityName(Entity())) : "<<null entity>>";
            Q_strncpy( szBuf, pszName, 512 );
            return (szBuf);
        }
    }

    return("No conversion to string");
}

void variant_t::SetEntity( CBaseEntity *val ) 
{ 
    eVal = val;
    fieldType = FIELD_EHANDLE; 
}

// not implemented
class CEventsSaveDataOps : public ISaveRestoreOps
{
    virtual void Save( const SaveRestoreFieldInfo_t &fieldInfo, ISave *pSave )
    {
    }

    virtual void Restore( const SaveRestoreFieldInfo_t &fieldInfo, IRestore *pRestore )
    {
    }

    virtual bool IsEmpty( const SaveRestoreFieldInfo_t &fieldInfo )
    {
        AssertMsg( fieldInfo.pTypeDesc->fieldSize == 1, "CEventsSaveDataOps does not support arrays");
        
        // check all the elements of the array (usually only 1)
        CBaseEntityOutput *ev = (CBaseEntityOutput*)fieldInfo.pField;
        const int fieldSize = fieldInfo.pTypeDesc->fieldSize;
        for ( int i = 0; i < fieldSize; i++, ev++ )
        {
            // It's not empty if it has events or if it has a non-void variant value
            if (( ev->NumberOfElements() != 0 ) || ( ev->ValueFieldType() != FIELD_VOID ))
                return 0;
        }

        // variant has no data
        return 1;
    }

    virtual void MakeEmpty( const SaveRestoreFieldInfo_t &fieldInfo )
    {
        // Don't no how to. This is okay, since objects of this type
        // are always born clean before restore, and not reused
    }

    virtual bool Parse( const SaveRestoreFieldInfo_t &fieldInfo, char const* szValue )
    {
        CBaseEntityOutput *ev = (CBaseEntityOutput*)fieldInfo.pField;
        ev->ParseEventAction( szValue );
        return true;
    }
};

//CVariantSaveDataOps g_VariantSaveDataOps;
//ISaveRestoreOps *variantFuncs = &g_VariantSaveDataOps;

CEventsSaveDataOps g_EventsSaveDataOps;
ISaveRestoreOps *eventFuncs = &g_EventsSaveDataOps;
