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
// HACK HACK HACK : BASEENTITY ACCESS
#include "takedamageinfo.h"
#include "ammodef.h"
#include "toolframework/itoolentity.h"

#include "AdminOP.h"
#include "cvars.h"

#include "tier0/memdbgon.h"

//ConVar phys_pushscale( "phys_pushscale", "1", FCVAR_REPLICATED );

BEGIN_SIMPLE_DATADESC( CTakeDamageInfo )
	DEFINE_FIELD( m_vecDamageForce, FIELD_VECTOR ),
	DEFINE_FIELD( m_vecDamagePosition, FIELD_POSITION_VECTOR),
	DEFINE_FIELD( m_vecReportedPosition, FIELD_POSITION_VECTOR),
	DEFINE_FIELD( m_hInflictor, FIELD_EHANDLE),
	DEFINE_FIELD( m_hAttacker, FIELD_EHANDLE),
	DEFINE_FIELD( m_hWeapon, FIELD_EHANDLE),
	DEFINE_FIELD( m_flDamage, FIELD_FLOAT),
	DEFINE_FIELD( m_flMaxDamage, FIELD_FLOAT),
	DEFINE_FIELD( m_flBaseDamage, FIELD_FLOAT ),
	DEFINE_FIELD( m_bitsDamageType, FIELD_INTEGER),
	DEFINE_FIELD( m_iDamageCustom, FIELD_INTEGER),
	DEFINE_FIELD( m_iDamageStats, FIELD_INTEGER),
	DEFINE_FIELD( m_iAmmoType, FIELD_INTEGER),
END_DATADESC()

void CTakeDamageInfo::Init( CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, const Vector &damageForce, const Vector &damagePosition, const Vector &reportedPosition, float flDamage, int bitsDamageType, int iCustomDamage )
{
	m_hInflictor = pInflictor;
	if ( pAttacker )
	{
		m_hAttacker = pAttacker;
	}
	else
	{
		m_hAttacker = pInflictor;
	}

    m_hWeapon = pWeapon;

	m_flDamage = flDamage;

	m_flBaseDamage = BASEDAMAGE_NOT_SPECIFIED;

	m_bitsDamageType = bitsDamageType;
	m_iDamageCustom = iCustomDamage;

	m_flMaxDamage = flDamage;
	m_vecDamageForce = damageForce;
	m_vecDamagePosition = damagePosition;
	m_vecReportedPosition = reportedPosition;
	m_iAmmoType = -1;
}

CTakeDamageInfo::CTakeDamageInfo()
{
	Init( NULL, NULL, NULL, vec3_origin, vec3_origin, vec3_origin, 0, 0, 0 );
}

CTakeDamageInfo::CTakeDamageInfo( CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType, int iKillType )
{
	Set( pInflictor, pAttacker, flDamage, bitsDamageType, iKillType );
}

CTakeDamageInfo::CTakeDamageInfo( CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, float flDamage, int bitsDamageType, int iKillType )
{
	Set( pInflictor, pAttacker, pWeapon, flDamage, bitsDamageType, iKillType );
}

CTakeDamageInfo::CTakeDamageInfo( CBaseEntity *pInflictor, CBaseEntity *pAttacker, const Vector &damageForce, const Vector &damagePosition, float flDamage, int bitsDamageType, int iKillType, Vector *reportedPosition )
{
	Set( pInflictor, pAttacker, damageForce, damagePosition, flDamage, bitsDamageType, iKillType, reportedPosition );
}

CTakeDamageInfo::CTakeDamageInfo( CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, const Vector &damageForce, const Vector &damagePosition, float flDamage, int bitsDamageType, int iKillType, Vector *reportedPosition )
{
	Set( pInflictor, pAttacker, pWeapon, damageForce, damagePosition, flDamage, bitsDamageType, iKillType, reportedPosition );
}

void CTakeDamageInfo::Set( CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType, int iKillType )
{
	Init( pInflictor, pAttacker, NULL, vec3_origin, vec3_origin, vec3_origin, flDamage, bitsDamageType, iKillType );
}

void CTakeDamageInfo::Set( CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, float flDamage, int bitsDamageType, int iKillType )
{
	Init( pInflictor, pAttacker, pWeapon, vec3_origin, vec3_origin, vec3_origin, flDamage, bitsDamageType, iKillType );
}

void CTakeDamageInfo::Set( CBaseEntity *pInflictor, CBaseEntity *pAttacker, const Vector &damageForce, const Vector &damagePosition, float flDamage, int bitsDamageType, int iKillType, Vector *reportedPosition )
{
	Set( pInflictor, pAttacker, NULL, damageForce, damagePosition, flDamage, bitsDamageType, iKillType, reportedPosition );
}

void CTakeDamageInfo::Set( CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, const Vector &damageForce, const Vector &damagePosition, float flDamage, int bitsDamageType, int iKillType, Vector *reportedPosition )
{
	Vector vecReported = vec3_origin;
	if ( reportedPosition )
	{
		vecReported = *reportedPosition;
	}
	Init( pInflictor, pAttacker, pWeapon, damageForce, damagePosition, vecReported, flDamage, bitsDamageType, iKillType );
}


// -------------------------------------------------------------------------------------------------- //
// MultiDamage
// Collects multiple small damages into a single damage
// -------------------------------------------------------------------------------------------------- //
BEGIN_SIMPLE_DATADESC_( CMultiDamage, CTakeDamageInfo )
	DEFINE_FIELD( m_hTarget, FIELD_EHANDLE),
END_DATADESC()

CMultiDamage g_MultiDamage;

CMultiDamage::CMultiDamage()
{
	m_hTarget = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CMultiDamage::Init( CBaseEntity *pTarget, CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, const Vector &damageForce, const Vector &damagePosition, const Vector &reportedPosition, float flDamage, int bitsDamageType, int iKillType )
{
	m_hTarget = pTarget;
	BaseClass::Init( pInflictor, pAttacker, pWeapon, damageForce, damagePosition, reportedPosition, flDamage, bitsDamageType, iKillType );
}

//-----------------------------------------------------------------------------
// Purpose: Resets the global multi damage accumulator
//-----------------------------------------------------------------------------
void ClearMultiDamage( void )
{
    if ( g_nServerToolsVersion >= 2 )
        servertools->ClearMultiDamage();
    else if(_ClearMultiDamage)
        (_ClearMultiDamage)();
}

//-----------------------------------------------------------------------------
// Purpose: inflicts contents of global multi damage register on gMultiDamage.pEntity
//-----------------------------------------------------------------------------
void ApplyMultiDamage( void )
{
    if ( g_nServerToolsVersion >= 2 )
        servertools->ApplyMultiDamage();
    else if(_ApplyMultiDamage)
        (_ApplyMultiDamage)();
}

//-----------------------------------------------------------------------------
// Purpose: Add damage to the existing multidamage, and apply if it won't fit
//-----------------------------------------------------------------------------
void AddMultiDamage( const CTakeDamageInfo &info, CBaseEntity *pEntity )
{
    if ( g_nServerToolsVersion >= 2 )
        servertools->AddMultiDamage( info, pEntity );
}


//============================================================================================================
// Utility functions for physics damage force calculation 
//============================================================================================================
//-----------------------------------------------------------------------------
// Purpose: Returns an impulse scale required to push an object.
// Input  : flTargetMass - Mass of the target object, in kg
//			flDesiredSpeed - Desired speed of the target, in inches/sec.
//-----------------------------------------------------------------------------
float ImpulseScale( float flTargetMass, float flDesiredSpeed )
{
	return (flTargetMass * flDesiredSpeed);
}

//-----------------------------------------------------------------------------
// Purpose: Fill out a takedamageinfo with a damage force for a melee impact
//-----------------------------------------------------------------------------
void CalculateMeleeDamageForce( CTakeDamageInfo *info, const Vector &vecMeleeDir, const Vector &vecForceOrigin, float flScale )
{
	info->SetDamagePosition( vecForceOrigin );

	// Calculate an impulse large enough to push a 75kg man 4 in/sec per point of damage
	float flForceScale = info->GetBaseDamage() * ImpulseScale( 75, 4 );
	Vector vecForce = vecMeleeDir;
	VectorNormalize( vecForce );
	vecForce *= flForceScale;
	if(phys_pushscale_game) vecForce *= phys_pushscale_game->GetFloat();
	vecForce *= flScale;
	info->SetDamageForce( vecForce );
}
