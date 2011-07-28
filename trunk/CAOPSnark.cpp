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

#include "utlvector.h"
#include "recipientfilter.h"

// HACK HACK HACK : BASEENTITY ACCESS
#define GAME_DLL
#include "cbase.h"
#include "baseentity.h"
#include "basegrenade_shared.h"
#include "player.h"
// HACK HACK HACK : BASEENTITY ACCESS

#include "vmatrix.h"
#include "util.h"
#include "vphysics_interface.h"
#include "vcollide_parse.h"

#include <stdio.h>
#include <time.h>

#include "AdminOP.h"
#include "recipientfilter.h"
#include "beam_flags.h"
#include "sourcehooks.h"
#include "bitbuf.h"
#include "CAOPEntity.h"
#include "CAOPSnark.h"
#include "cvars.h"
#include "vfuncs.h"

#include "tier0/memdbgon.h"

#define SNARK_MODEL "models/w_squeak.mdl"
//#define SNARK_MODEL "models/Weapons/w_grenade.mdl"

#define SQUEEK_DETONATE_DELAY   15.0
#define SNARK_EXPLOSION_VOLUME  512


enum w_squeak_e {
    WSQUEAK_IDLE1 = 0,
    WSQUEAK_FIDGET,
    WSQUEAK_JUMP,
    WSQUEAK_RUN,
};

bool PhysModelParseSolidByIndex( solid_t &solid, CBaseEntity *pEntity, vcollide_t *pCollide, int solidIndex );

BEGIN_DATADESC( CAOPSnark )
    DEFINE_KEYFIELD( m_iOwner, FIELD_INTEGER, "ownerplayer" ),
    DEFINE_OUTPUT(m_SnarkBite, "OnSnarkBite"), 
    DEFINE_INPUTFUNC( FIELD_INPUT, "Explode", InputExplode ),
END_DATADESC()

SOP_LINK_ENTITY_TO_CLASS_FEAT(sop_snark, CAOPSnark, FEAT_SNARK);

/*
datamap_t CAOPSnark::m_DataMap = { 0, 0, "CAOPSnark", __null };
datamap_t *CAOPSnark::GetDataDescMap( void ) { return &m_DataMap; }
datamap_t *CAOPSnark::GetBaseMap() {
    datamap_t *pResult;
    DataMapAccess((BaseClass *)__null, &pResult);
    return pResult;
}
template <typename T> datamap_t *DataMapInit(T *);
template <> datamap_t *DataMapInit<CAOPSnark>( CAOPSnark * );
namespace CAOPSnark_DataDescInit {
    datamap_t *g_DataMapHolder = DataMapInit( (CAOPSnark *)__null );
}
template <> datamap_t *DataMapInit<CAOPSnark>( CAOPSnark * ) {
    typedef CAOPSnark classNameTypedef;
    static CDatadescGeneratedNameHolder nameHolder("CAOPSnark");
    CAOPSnark::m_DataMap.baseMap = CAOPSnark::GetBaseMap();
    static typedescription_t dataDesc[] = {
        { FIELD_VOID,0, {0,0},0,0,0,0,0,0},
        { FIELD_INTEGER, "m_iOwner", { __builtin_offsetof (classNameTypedef, m_iOwner), 0 }, 1, 0x0004 | 0x0002, "ownerplayer", __null, __null, __null, sizeof( ((classNameTypedef *)0)->m_iOwner ), __null, 0, 0 },
        { FIELD_CUSTOM, "m_SnarkBite", { __builtin_offsetof (classNameTypedef, m_SnarkBite), 0 }, 1, 0x0010 | 0x0002 | 0x0004, "OnSnarkBite", eventFuncs },
        { FIELD_INPUT, "InputExplode", { __null, __null }, 1, 0x0008, "Explode", __null, (inputfunc_t)static_cast <sopinputfunc_t> (&classNameTypedef::InputExplode) },
    };
    if ( sizeof( dataDesc ) > sizeof( dataDesc[0] ) )
    {
        classNameTypedef::m_DataMap.dataNumFields = (sizeof(dataDesc)/sizeof((dataDesc)[0])) - 1;
        classNameTypedef::m_DataMap.dataDesc = &dataDesc[1];
    }
    else
    {
        classNameTypedef::m_DataMap.dataNumFields = 1;
        classNameTypedef::m_DataMap.dataDesc = dataDesc;
    }
    return &classNameTypedef::m_DataMap;
}

static CSOPEntityFactory<CAOPSnark> sop_snark( "sop_snark", FEAT_SNARK );
*/


CAOPSnark::CAOPSnark()
{
    m_hKiller = NULL;
    m_iOwner = 0;
    m_hOwner = NULL;
    m_iGround = -1;
    m_bOnGround = 0;
    m_flNextAttack = 0.0f;
}

void CAOPSnark::Spawn() 
{
    CBaseEntity *pBase = GetBase();
    CBaseAnimating *pAnim = (CBaseAnimating *)pBase;

    VFuncs::SetModel(pBase, SNARK_MODEL );

    VFuncs::AddFlag(pBase, FL_OBJECT);

    CreateVPhysics();
    VFuncs::SetHealth(pBase, BIG_HEALTH);
    m_flNextThink = gpGlobals->curtime;
    MakeMyThink(&CAOPSnark::HuntThink);
    pAdminOP.myThinkEnts.AddToTail((CAOPEntity *)this);
    //MakeUse(&CAOPSnark::Use);
    MakeTouch(&CAOPSnark::SuperBounceTouch);
    MakeTouchPost(&CAOPSnark::SuperBounceTouchPost);

    m_flNextHit             = gpGlobals->curtime;
    m_flNextHunt            = gpGlobals->curtime + 1E6;
    m_flNextBounceSoundTime = gpGlobals->curtime;

    VFuncs::SetGravity( pBase, 0.5f );
    VFuncs::SetFriction( pBase, 0.01f );

    m_hEnemy = NULL;
    m_flDamage = sk_snark_dmg_pop.GetFloat();

    m_iTeam = -1;

    m_flDie = gpGlobals->curtime + SQUEEK_DETONATE_DELAY;

    //m_flFieldOfView = 0; // 180 degrees

    if ( m_iOwner )
    {
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+m_iOwner);
        if(info)
        {
            m_iTeam = info->GetTeamIndex();
        }
        m_hOwner = CBaseEntity::Instance(pAdminOP.GetEntityList() + m_iOwner);
        m_hKiller = m_hOwner;
    }

    VFuncs::ResetSequence( pAnim, WSQUEAK_RUN );

    m_posPrev = Vector( 0, 0, 0 );

    RETURN_META(MRES_SUPERCEDE);
}

void CAOPSnark::Precache()
{
    // this is an invalid particle effect in tf2 (and probably others)
    if(!pAdminOP.isTF2)
    {
        INetworkStringTable *pStringTableParticleEffectNames = networkstringtable->FindTable( "ParticleEffectNames" );
        if(pStringTableParticleEffectNames)
        {
            pStringTableParticleEffectNames->AddString(true, "blood_impact_yellow_01");
        }
    }

    engine->PrecacheModel("models/w_squeak.mdl", true);
    enginesound->PrecacheSound(SNARKDEPLOYSND, true);
    enginesound->PrecacheSound(SNARKHUNTSND1, true);
    enginesound->PrecacheSound(SNARKHUNTSND2, true);
    enginesound->PrecacheSound(SNARKHUNTSND3, true);
    enginesound->PrecacheSound(SNARKBLASTSND, true);
    enginesound->PrecacheSound(SNARKDIESND, true);

    INetworkStringTable *pDownloadablesTable = networkstringtable->FindTable("downloadables");
    if(pDownloadablesTable)
    {
        char file[256];
        bool save = engine->LockNetworkStringTables(false);

        sprintf(file, "models/w_squeak.mdl");
        pDownloadablesTable->AddString(true, file, sizeof(file));
        //sprintf(file, "models/w_squeak.phy");
        //pDownloadablesTable->AddString(true, file, sizeof(file));
        sprintf(file, "models/w_squeak.dx80.vtx");
        pDownloadablesTable->AddString(true, file, sizeof(file));
        sprintf(file, "models/w_squeak.dx90.vtx");
        pDownloadablesTable->AddString(true, file, sizeof(file));
        sprintf(file, "models/w_squeak.sw.vtx");
        pDownloadablesTable->AddString(true, file, sizeof(file));
        sprintf(file, "models/w_squeak.vvd");
        pDownloadablesTable->AddString(true, file, sizeof(file));
        sprintf(file, "materials/models/v_squeak/SQK_Eye1.vmt");
        pDownloadablesTable->AddString(true, file, sizeof(file));
        sprintf(file, "materials/models/v_squeak/SQK_Eye1.vtf");
        pDownloadablesTable->AddString(true, file, sizeof(file));
        sprintf(file, "materials/models/v_squeak/SQK_Side1.vmt");
        pDownloadablesTable->AddString(true, file, sizeof(file));
        sprintf(file, "materials/models/v_squeak/SQK_Side1.vtf");
        pDownloadablesTable->AddString(true, file, sizeof(file));
        sprintf(file, "sound/%s", SNARKDEPLOYSND);
        pDownloadablesTable->AddString(true, file, sizeof(file));
        sprintf(file, "sound/%s", SNARKHUNTSND1);
        pDownloadablesTable->AddString(true, file, sizeof(file));
        sprintf(file, "sound/%s", SNARKHUNTSND2);
        pDownloadablesTable->AddString(true, file, sizeof(file));
        sprintf(file, "sound/%s", SNARKHUNTSND3);
        pDownloadablesTable->AddString(true, file, sizeof(file));
        sprintf(file, "sound/%s", SNARKBLASTSND);
        pDownloadablesTable->AddString(true, file, sizeof(file));
        sprintf(file, "sound/%s", SNARKDIESND);
        pDownloadablesTable->AddString(true, file, sizeof(file));
        engine->LockNetworkStringTables(save);
    }

    BaseClass::Precache();
}

void CAOPSnark::CreateVPhysics()
{
    CBaseEntity *pBase = GetBase();
    
    //Msg("%i %i %i %i %i %i %i\n", VFuncs::GetSolidFlags(pBase), VFuncs::GetFlags(pBase), pBase->GetSolid(), pBase->GetCollisionGroup(), pBase->IsSimulatedEveryTick(), pBase->GetSimulationTime(), pBase->GetEffects());
    VFuncs::SetSolid( pBase, SOLID_BBOX );
    VFuncs::SetMoveType( pBase, MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_SLIDE );
    VFuncs::SetCollisionGroup( pBase, COLLISION_GROUP_PROJECTILE );
    VFuncs::AddSolidFlags( pBase, FSOLID_NOT_STANDABLE );
    UTIL_SetSize(pBase, Vector( -4, -4, 0 ), Vector( 4, 4, 8 ) );

    //VFuncs::RemoveSolidFlags( pBase, FSOLID_NOT_SOLID );
    //VFuncs::AddSolidFlags( pBase, FSOLID_NOT_STANDABLE );
    //VFuncs::SetCollisionGroup( pBase, COLLISION_GROUP_PROJECTILE );
    //pBase->SetSize( Vector( -4, -4, 0 ), Vector( 4, 4, 8 ) );

    /*pBase->SetSize( -Vector(4,4,4), Vector(4,4,4) );
    VFuncs::SetCollisionGroup( pBase, COLLISION_GROUP_WEAPON );
    //pBase->VPhysicsInitStatic( );
    // create a static physics objct
    IPhysicsObject *pPhysicsObject = NULL;
    //pPhysicsObject = PhysModelCreateBox( pBase, pBase->WorldAlignMins(), pBase->WorldAlignMaxs(), GetAbsOrigin(), false );
    pPhysicsObject = PhysModelCreateBox( pBase, -Vector(4,4,4), Vector(4,4,4), GetAbsOrigin(), false );
    VFuncs::VPhysicsSetObject( pBase, pPhysicsObject );*/
}

void CAOPSnark::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
    RETURN_META(MRES_IGNORED);
}

bool CAOPSnark::KeyValue( const char *szKeyName, const char *szValue )
{
    return BaseClass::KeyValue(szKeyName, szValue);
}

void CAOPSnark::SetSnarkOwner( int index )
{
    m_iOwner = index;
}

void CAOPSnark::InputExplode( inputdata_t &inputdata )
{
    CBaseEntity *pBase = GetBase();

    // if snark is already exploding, don't explode again
    if(VFuncs::GetTakeDamage(pBase) != DAMAGE_NO)
    {
        CTakeDamageInfo info( pBase, pBase, 1, DMG_GENERIC );
        Event_Killed(info);
    }
}

bool CAOPSnark::IsOwnerStillValid()
{
    CBaseEntity *pBase = GetBase();

    // if we don't have an owner, then we should still stick around
    if(!m_iOwner)
        return true;

    IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+m_iOwner);
    if(!info)
    {
        return false;
    }
    if(!info->IsConnected())
    {
        return false;
    }
    if(m_iTeam != -1)
    {
        if(info->GetTeamIndex() != m_iTeam)
        {
            return false;
        }
    }
    if (pBase == NULL || !pBase->IsInWorld())
    {
        return false;
    }
    return true;
}

void CAOPSnark::HuntThink()
{
    CBaseEntity *pBase = GetBase();
    CBaseAnimating *pAnim = (CBaseAnimating *)pBase;

    if(!IsOwnerStillValid())
    {
        UTIL_Remove(pBase);
        return;
    }

    VFuncs::StudioFrameAdvance( pAnim );
    m_flNextThink = gpGlobals->curtime + 0.1f;

    /*Msg("Ground: %s\n", VFuncs::GetFlags(pBase) & FL_ONGROUND ? "Yes" : "No");
    if(VFuncs::GetFlags(pBase) & FL_ONGROUND)
    {
        Vector velocity = GetAbsVelocity();

        // Are we really on ground?
        trace_t pm;
        Ray_t ray;
        ray.Init( GetAbsOrigin(), GetAbsOrigin() - Vector(0,0,1), Vector( -4, -4, 0 ), Vector( 4, 4, 8 ) );
        CTraceFilterSimple traceFilter( pBase, COLLISION_GROUP_PLAYER_MOVEMENT );
        enginetrace->TraceRay( ray, MASK_PLAYERSOLID, &traceFilter, &pm );
        Msg("Touch has plane: %f %f %f\n", pm.plane.normal[0], pm.plane.normal[1], pm.plane.normal[2]);

        if(pm.plane.normal.z <= 0.7f) // no floor
        {
            velocity.z = +50; // HACKHACKHACK slam to ground
            VFuncs::SetAbsVelocity(pBase, velocity);
        }
    }*/

    // explode when ready
    if((gpGlobals->curtime >= m_flDie) || (VFuncs::GetHealth(pBase) < BIG_HEALTH - sk_snark_health.GetInt()))
    {
        g_vecAttackDir = GetAbsVelocity();
        VectorNormalize( g_vecAttackDir );
        VFuncs::SetHealth(pBase, -1);
        CTakeDamageInfo info( pBase, pBase, 1, DMG_GENERIC );
        Event_Killed( info );
        return;
    }

    // float
    if(VFuncs::GetWaterLevel(pBase) != 0)
    {
        if(VFuncs::GetMoveType(pBase) == MOVETYPE_FLYGRAVITY)
        {
            VFuncs::SetMoveType( pBase, MOVETYPE_FLY, MOVECOLLIDE_FLY_SLIDE );
        }
        Vector vecVel = GetAbsVelocity();
        vecVel *= 0.9;
        vecVel.z += 8.0;
        VFuncs::SetAbsVelocity(pBase, vecVel);
    }
    else if(VFuncs::GetMoveType(pBase) == MOVETYPE_FLY)
    {
        VFuncs::SetMoveType( pBase, MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_SLIDE );
    }


    // squeek if it's about time blow up
    if ( (m_flDie - gpGlobals->curtime <= 0.5) && (m_flDie - gpGlobals->curtime >= 0.3) )
    {
        Vector vecSndOrigin = GetAbsOrigin();
        CPASFilter filter( vecSndOrigin );
        enginesound->EmitSound( filter, VFuncs::entindex(pBase), CHAN_VOICE, SNARKDIESND, 1, ATTN_NORM, 0, 100 + random->RandomInt( 0, 0x3F ) );
    }

    // higher pitch as squeeker gets closer to detonation time
    float flpitch = 155.0 - 60.0 * ( (m_flDie - gpGlobals->curtime) / SQUEEK_DETONATE_DELAY );
    if ( flpitch < 80 )
        flpitch = 80;

    if ( m_bOnGround )
    {
        VFuncs::SetLocalAngularVelocity( pBase, QAngle( 0, 0, 0 ) );
    }
    else
    {
        QAngle angVel = VFuncs::GetLocalAngularVelocity(pBase);
        if ( angVel == QAngle( 0, 0, 0 ) )
        {
            angVel.x = random->RandomFloat( -100, 100 );
            angVel.z = random->RandomFloat( -100, 100 );
            VFuncs::SetLocalAngularVelocity( pBase, angVel );
        }
    }

    if(abs(m_flBounce) > 1 )
    {
        Vector velocity = GetAbsVelocity();
        velocity.z = m_flBounce;
        VFuncs::SetAbsVelocity(pBase, velocity);
        //Msg("DoneTouch: %f %f %f\n", velocity.x, velocity.y, velocity.z);
        m_flBounce = 0;
        m_bOnGround = 0;
    }

    // if snark get's stuck, randomly hop
    if ( ( GetAbsOrigin() - m_posPrev ).Length() < 1.0 )
    {
        Vector vecVel = GetAbsVelocity();
        vecVel.x = random->RandomFloat( -100, 100 );
        vecVel.y = random->RandomFloat( -100, 100 );
        vecVel.z = 100;
        VFuncs::SetAbsVelocity(pBase,  vecVel );
    }

    m_posPrev = GetAbsOrigin();

    // return if not time to hunt
    if ( m_flNextHunt > gpGlobals->curtime )
        return;

    m_flNextHunt = gpGlobals->curtime + 2.0;

    CBaseEntity *pEnemy = GetSnarkEnemy();
    if ( pEnemy == NULL )
    {
        // find target, bounce a bit towards it.
        FindSnarkEnemy();
        //GetSenses()->Look( 512 );
        //SetEnemy( BestEnemy() );
    }

    if ( pEnemy != NULL )
    {
        if ( VFuncs::FVisible(pBase, pEnemy) )
        {
            m_vecTarget = VFuncs::EyePosition(pEnemy) - GetAbsOrigin();
            VectorNormalize( m_vecTarget );
        }

        float flVel = GetAbsVelocity().Length();
        float flAdj = 50.0 / ( flVel + 10.0 );

        if ( flAdj > 1.2 )
            flAdj = 1.2;
        
        // ALERT( at_console, "think : enemy\n");

        // ALERT( at_console, "%.0f %.2f %.2f %.2f\n", flVel, m_vecTarget.x, m_vecTarget.y, m_vecTarget.z );

        VFuncs::SetAbsVelocity(pBase,  GetAbsVelocity() * flAdj + (m_vecTarget * 300) );
        ResetAngles();
    }

    if(m_bOnGround)
    {
        ResetAngles();
    }
}

void CAOPSnark::SuperBounceTouch( CBaseEntity *pOther )
{
    CBaseEntity *pBase = GetBase();
    float   flpitch;
    bool    playsound = 0;
    trace_t *tr = &g_TouchTrace;

    if(!IsOwnerStillValid())
    {
        // don't delete, think will take care of it
        RETURN_META(MRES_SUPERCEDE);
    }

    // don't hit the guy that launched this grenade
    if ( m_hOwner && ( VFuncs::entindex(pOther) == VFuncs::entindex(m_hOwner) ) )
    {
        RETURN_META(MRES_SUPERCEDE);
    }

    if(VFuncs::GetSolidFlags(pOther) & FSOLID_TRIGGER)
    {
        RETURN_META(MRES_SUPERCEDE);
    }

    // at least until we've bounced once
    m_hOwner = NULL;

    Vector velocity = GetAbsVelocity();
    /*if(velocity.Length() > 20)
    {
        Msg("Touch: %f %f %f\n", velocity.x, velocity.y, velocity.z);
        velocity.x += pm.plane.normal[0] * 1.5 * (-velocity.x);
        velocity.y += pm.plane.normal[1] * 1.5 * (-velocity.y);
        velocity.z += pm.plane.normal[2] * 1.5 * (-velocity.z);
        VFuncs::SetAbsVelocity(pBase, velocity);        
        Msg("NewTouch: %f %f %f\n", velocity.x, velocity.y, velocity.z);
    }*/

    // don't bounce off players
    if(!VFuncs::IsPlayer(pOther))
    {
        // floor
        if(abs(velocity.z) > 10)
        {
            trace_t pm;
            Ray_t ray;
            ray.Init( GetAbsOrigin(), GetAbsOrigin() + GetAbsVelocity() * 1, Vector( -4, -4, 0 ), Vector( 4, 4, 8 ) );
            //UTIL_TraceRay( ray, MASK_PLAYERSOLID, pBase, COLLISION_GROUP_PROJECTILE, &pm );
            CTraceFilterSimple traceFilter( pBase, COLLISION_GROUP_PROJECTILE );
            enginetrace->TraceRay( ray, MASK_PLAYERSOLID, &traceFilter, &pm );

            if(pm.plane.normal[2] > 0.7f && pBase->GetWaterLevel() == 0)
            {
                // Get the impact surface's elasticity.
                float flSurfaceElasticity;
                physprops->GetPhysicsProperties( pm.surface.surfaceProps, NULL, NULL, NULL, &flSurfaceElasticity );
                m_flBounce = velocity.z * -flSurfaceElasticity;
                if(!m_bOnGround) playsound = 1;
                m_bOnGround = 1;
                if(pm.m_pEnt) m_iGround = VFuncs::entindex(pm.m_pEnt);
                ResetAngles();
            }
        }
        else
        {
            if(pBase->GetWaterLevel() == 0)
            {
                trace_t pm;
                Ray_t ray;
                ray.Init( GetAbsOrigin(), GetAbsOrigin() - Vector(0,0,4), Vector( -4, -4, 0 ), Vector( 4, 4, 8 ) );
                //UTIL_TraceRay( ray, MASK_PLAYERSOLID, pBase, COLLISION_GROUP_PROJECTILE, &pm );
                CTraceFilterSimple traceFilter( pBase, COLLISION_GROUP_PROJECTILE );
                enginetrace->TraceRay( ray, MASK_PLAYERSOLID, &traceFilter, &pm );

                // if we're not world and we didn't fall onto this object, play touch sound
                if(pm.fraction != 1.0 && pm.m_pEnt && !(pm.contents & MASK_WATER) )
                {
                    m_iGround = VFuncs::entindex(pm.m_pEnt);
                    if(!m_bOnGround)
                    {
                        ResetAngles();
                        m_bOnGround = 1;
                    }
                }
                else
                {
                    m_bOnGround = 0;
                }
            }
            else
            {
                m_bOnGround = 0;
            }
        }
        if(VFuncs::entindex(pOther) != m_iGround) playsound = 1;
    }

    QAngle angles = pBase->GetLocalAngles();
    angles.x = 0;
    angles.z = 0;
    VFuncs::SetAbsAngles(pBase,  angles );

    // avoid bouncing too much
    if ( m_flNextHit > gpGlobals->curtime)
    {
        RETURN_META(MRES_SUPERCEDE);
    }

    // higher pitch as squeeker gets closer to detonation time
    flpitch = 155.0 - 60.0 * ( ( m_flDie - gpGlobals->curtime ) / SQUEEK_DETONATE_DELAY );

    if ( VFuncs::GetTakeDamage(pOther) && m_flNextAttack < gpGlobals->curtime )
    {
        // attack!

        if(VFuncs::IsPlayer(pOther))
        {
            // ALERT( at_console, "hit enemy\n");
            ClearMultiDamage();

            Vector vecForward;
            AngleVectors( VFuncs::GetLocalAngles(pBase), &vecForward );

            if ( m_hKiller != NULL )
            {
                CTakeDamageInfo info( pBase, m_hKiller, sk_snark_dmg_bite.GetFloat(), DMG_SLASH );
                CalculateMeleeDamageForce( &info, vecForward, tr->endpos );
                pOther->DispatchTraceAttack( info, vecForward, tr );
                m_SnarkBite.FireOutput(m_hKiller, pBase);
            }
            else
            {
                CTakeDamageInfo info( pBase, pBase, sk_snark_dmg_bite.GetFloat(), DMG_SLASH );
                CalculateMeleeDamageForce( &info, vecForward, tr->endpos );
                pOther->DispatchTraceAttack( info, vecForward, tr );
                m_SnarkBite.FireOutput(pBase, pBase);
            }

            ApplyMultiDamage();

            m_flDamage += sk_snark_dmg_pop.GetFloat(); // add more explosion damage
            // m_flDie += 2.0; // add more life

            // make bite sound
            CPASFilter filterBite( VFuncs::WorldSpaceCenter(pBase) );
            enginesound->EmitSound( filterBite, VFuncs::entindex(pBase), CHAN_WEAPON, SNARKDEPLOYSND, 1, ATTN_NORM, 0, (int)flpitch );
            m_flNextAttack = gpGlobals->curtime + 0.5;
        }
    }

    m_flNextHit = gpGlobals->curtime + 0.1;
    m_flNextHunt = gpGlobals->curtime;

    //if ( g_pGameRules->IsMultiplayer() )
    //{
        // in multiplayer, we limit how often snarks can make their bounce sounds to prevent overflows.
        if ( gpGlobals->curtime < m_flNextBounceSoundTime )
        {
            // too soon!
            RETURN_META(MRES_SUPERCEDE);
        }
    //}

    if ( !( VFuncs::GetFlags(pBase) & FL_ONGROUND ) && playsound )
    {
        // play bounce sound
        Vector vecSndOrigin = GetAbsOrigin();
        CPASFilter filter2( vecSndOrigin );
        switch ( random->RandomInt( 0, 2 ) )
        {
        case 0:
            enginesound->EmitSound( filter2, VFuncs::entindex(pBase), CHAN_VOICE, SNARKHUNTSND1, 1, ATTN_NORM, 0, (int)flpitch );   break;
        case 1:
            enginesound->EmitSound( filter2, VFuncs::entindex(pBase), CHAN_VOICE, SNARKHUNTSND2, 1, ATTN_NORM, 0, (int)flpitch );   break;
        case 2:
            enginesound->EmitSound( filter2, VFuncs::entindex(pBase), CHAN_VOICE, SNARKHUNTSND3, 1, ATTN_NORM, 0, (int)flpitch );   break;
        }

        m_flNextBounceSoundTime = gpGlobals->curtime + 0.5;// half second.
    }
    RETURN_META(MRES_SUPERCEDE);
}

void CAOPSnark::SuperBounceTouchPost( CBaseEntity *pOther )
{
    //CBaseEntity *pBase = GetBase();
}

CBaseEntity *CAOPSnark::GetSnarkEnemy()
{
    if(m_hEnemy)
    {
        int entindex = VFuncs::entindex(m_hEnemy);
        if(entindex > 0 && entindex <= pAdminOP.GetMaxClients())
        {
            IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+entindex);
            if(info)
            {
                if(info->IsConnected())
                {
                    if(!pAdminOP.pAOPPlayers[entindex-1].IsAlive())
                    {
                        m_hEnemy = NULL;
                    }
                }
                else
                {
                    m_hEnemy = NULL;
                }
            }
            else
            {
                m_hEnemy = NULL;
            }
        }
        else
        {
            m_hEnemy = NULL;
        }
    }
    return m_hEnemy;
}

void CAOPSnark::FindSnarkEnemy()
{
    CBaseEntity *pBase = GetBase();
    // max search range is 512
    float flClosest = 512;
    CBaseEntity *pClosestEnemy = NULL;
    Vector snarkOrigin = VFuncs::GetAbsOrigin(pBase);

    for(int i = 1; i <= pAdminOP.GetMaxClients(); i++)
    {
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+i);

        if(info)
        {
            if(info->IsConnected())
            {
                // certain games are always team games
                if(pAdminOP.isCstrike || pAdminOP.isDod || pAdminOP.isTF2 || pAdminOP.isFF || mp_teamplay->GetBool())
                {
                    if(info->GetTeamIndex() == m_iTeam)
                    {
                        continue;
                    }
                }
                CBaseEntity *pPotential = CBaseEntity::Instance(pAdminOP.GetEntityList()+i);
                Vector enemyOrigin = VFuncs::GetAbsOrigin(pPotential);
                float distance = snarkOrigin.DistTo(enemyOrigin);
                bool visible = VFuncs::FVisible(pBase, pPotential);
                if(visible && distance < flClosest)
                {
                    flClosest = distance;
                    pClosestEnemy = pPotential;
                }
            }
        }
    }
    m_hEnemy = pClosestEnemy;
}

void CAOPSnark::ResetAngles()
{
    CBaseEntity *pBase = GetBase();

    QAngle angles;
    VectorAngles( GetAbsVelocity(), angles );
    angles.z = 0;
    angles.x = 0;
    VFuncs::SetAbsAngles(pBase,  angles );
}

void CAOPSnark::Event_Killed( const CTakeDamageInfo &inputInfo )
{
    CBaseEntity *pBase = GetBase();

    // since squeak grenades never leave a body behind, clear out their takedamage now.
    // Squeaks do a bit of radius damage when they pop, and that radius damage will
    // continue to call this function unless we acknowledge the Squeak's death now. (sjb)
    VFuncs::SetTakeDamage(pBase, DAMAGE_NO);

    // play squeek blast
    Vector vecSndOrigin = GetAbsOrigin();
    CPASFilter filter( vecSndOrigin );
    enginesound->EmitSound( filter, VFuncs::entindex(pBase), CHAN_ITEM, SNARKBLASTSND, 1, 0.5, 0, PITCH_NORM );

    // invalid particle system in tf2
    if(!pAdminOP.isTF2)
    {
        UTIL_BloodDrips( VFuncs::WorldSpaceCenter(pBase), Vector( 0, 0, 0 ), BLOOD_COLOR_YELLOW, 80 );
    }
    //UTIL_BloodSpray( VFuncs::WorldSpaceCenter(pBase), Vector( 0,0,0 ),
    //  BLOOD_COLOR_YELLOW, 8, FX_BLOODSPRAY_GORE );
    UTIL_BloodSpray( VFuncs::WorldSpaceCenter(pBase)+Vector(0,0,4), Vector( 0,0,-1 ),
        BLOOD_COLOR_YELLOW, 4, FX_BLOODSPRAY_ALL );

    // reset the owner entity
    m_hOwner = NULL;
    if(m_iOwner && m_iOwner > 0 && m_iOwner < gpGlobals->maxClients && m_iOwner < MAX_AOP_PLAYERS)
    {
        if(pAdminOP.pAOPPlayers[m_iOwner-1].GetPlayerState() >= 2)
        {
            m_hOwner = CBaseEntity::Instance(pAdminOP.GetEntityList() + m_iOwner);
        }
    }

    if ( m_hOwner != NULL )
    {
        RadiusDamage( CTakeDamageInfo( pBase, m_hOwner, m_flDamage, DMG_BLAST ), VFuncs::GetAbsOrigin(pBase), m_flDamage * 2.5, CLASS_NONE, NULL );
    }
    else
    {
        RadiusDamage( CTakeDamageInfo( pBase, pBase, m_flDamage, DMG_BLAST ), VFuncs::GetAbsOrigin(pBase), m_flDamage * 2.5, CLASS_NONE, NULL );
    }

    UTIL_Remove( pBase );
}

void CAOPSnark::UpdateOnRemove( void )
{
    BaseClass::UpdateOnRemove();
}
