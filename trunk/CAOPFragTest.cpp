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
#include "player.h"
// HACK HACK HACK : BASEENTITY ACCESS

#include "vmatrix.h"
#include "util.h"
#include "vphysics_interface.h"
#include "vcollide_parse.h"

#include <stdio.h>
#include <time.h>

#include "AdminOP.h"
#include "sourcehooks.h"
#include "bitbuf.h"
#include "CAOPEntity.h"
#include "CAOPFragTest.h"
#include "vfuncs.h"

#include "tier0/memdbgon.h"

#define GRENADE_MODEL "models/weapons/w_grenade.mdl"
//#define GRENADE_MODEL "models/SourceOP/radio.mdl"

BEGIN_DATADESC( CAOPFragTest )
    DEFINE_KEYFIELD( m_iOwner, FIELD_INTEGER, "ownerplayer" ),
    DEFINE_KEYFIELD( m_flLife, FIELD_FLOAT, "life" ),
END_DATADESC()

SOP_LINK_ENTITY_TO_CLASS(sop_grenade, CAOPFragTest);

CAOPFragTest::CAOPFragTest()
{
    m_iOwner = 0;
    m_flLife = 0;
}

void CAOPFragTest::Spawn() 
{
    CBaseEntity *pBase = GetBase();

    if(m_iOwner)
    {
        VFuncs::SetThrower(pBase, CBaseEntity::Instance(pAdminOP.GetEntityList() + m_iOwner));
    }
    VFuncs::SetHealth(pBase, BIG_HEALTH);
    SetDamage(100);
    SetDamageRadius(400);

    if(m_flLife)
    {
        m_flNextThink = gpGlobals->curtime + m_flLife;
        MakeMyThink(&CAOPFragTest::ExplodeThink);
        pAdminOP.myThinkEnts.AddToTail((CAOPEntity *)this);
    }

    VFuncs::SetModel( pBase, GRENADE_MODEL );
    UTIL_SetSize( pBase, -Vector(4,4,4), Vector(4,4,4) );
    VFuncs::SetCollisionGroup( pBase, COLLISION_GROUP_WEAPON );
    pBase->VPhysicsInitNormal( SOLID_BBOX, 0, false );

    CreateTrail();

    if(m_iOwner)
    {
        Vector orig = GetAbsOrigin();
        CPASFilter filter( orig );
        enginesound->EmitSound((CRecipientFilter&)filter, m_iOwner, CHAN_ITEM, "weapons/grenade_throw.wav", 0.5, SNDLVL_NORM, 0, random->RandomInt(100, 110), &orig);  
    }

    RETURN_META(MRES_SUPERCEDE);
}

void CAOPFragTest::Precache()
{
    engine->PrecacheModel(GRENADE_MODEL);
    enginesound->PrecacheSound("weapons/grenade_throw.wav", true);
    engine->PrecacheModel("sprites/smoke.vmt");

    BaseClass::Precache();
}

void CAOPFragTest::CreateTrail()
{
    CBaseEntity *pGrenade = GetBase();
    Vector orig = GetAbsOrigin();
    CBaseEntity *pTrail = CAdminOP::CreateSpriteTrail("sprites/smoke.vmt", orig, true);
    if(pTrail)
    {
        //m_hTrail->SetTransparency( kRenderTransAdd, 224, 224, 255, 255, kRenderFxNone );
        VFuncs::SetRenderMode(pTrail, kRenderTransAdd);
        VFuncs::SetRenderColor(pTrail, 224, 224, 255, 255);
        if(m_iOwner)
        {
            IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+m_iOwner);

            if(info)
            {
                if(info->IsConnected())
                {
                    if(pAdminOP.isCstrike || pAdminOP.isTF2)
                    {
                        switch(info->GetTeamIndex())
                        {
                            case 2:
                                VFuncs::SetRenderColor(pTrail, 224, 24, 8, 255);
                                break;
                            case 3:
                                VFuncs::SetRenderColor(pTrail, 8, 24, 224, 255);
                                break;
                        }
                    }
                }
            }
        }
        //m_hTrail->SetAttachment( this, 0 );
        VFuncs::SpriteSetAttachment(pTrail, pGrenade, 0);
        //m_hTrail->SetStartWidth( 32.0 );
        VFuncs::KeyValue(pTrail, "startwidth", "6.0");
        //m_hTrail->SetEndWidth( 200.0 );
        VFuncs::KeyValue(pTrail, "endwidth", "4.0");
        //m_hTrail->SetStartWidthVariance( 15.0f );
        //m_hTrail->SetTextureResolution( 0.002 );
        //m_hTrail->SetLifeTime( ENV_HEADCRABCANISTER_TRAIL_TIME );
        VFuncs::KeyValue(pTrail, "lifetime", "0.4");
        //m_hTrail->SetMinFadeLength( 1000.0f );
    }
}

void CAOPFragTest::ExplodeThink()
{
    CBaseEntity *pBase = GetBase();
    CTakeDamageInfo info = CTakeDamageInfo();
    VFuncs::KeyValue(pBase, "health", "-1");
    VFuncs::Event_Killed(pBase, info);
}
