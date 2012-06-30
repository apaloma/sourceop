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
#include "bitbuf.h"
#include "CAOPEntity.h"
#include "CAOPExploding.h"
#include "vfuncs.h"

#include "tier0/memdbgon.h"

BEGIN_DATADESC( CAOPExploding )
END_DATADESC()

CAOPExploding::CAOPExploding()
{
    CBaseEntity *pProp = CreateEntityByName( "grenade" );
    if(pProp)
    {
        ADD_TO_VP_ENTLIST(pProp, this);
        pAdminOP.ServiceEnt(pProp, this);
        Set(servergameents->BaseEntityToEdict(pProp));
    }
    else
    {
        BaseClass::Set(NULL);
    }
}

void CAOPExploding::Set(edict_t *pent) 
{
    CBaseEntity *pBase;
    BaseClass::Set(pent);
    MakeThink(&CAOPExploding::Think);
    MakeTouch(&CAOPExploding::Touch);
    pBase = GetBase();
    SetDamage(100);
    SetDamageRadius(100);
    VFuncs::SetTakeDamage(pBase, DAMAGE_YES);
    VFuncs::SetHealth(pBase, 1);
    VFuncs::SetCollisionGroup( pBase, COLLISION_GROUP_PLAYER );
    VFuncs::Spawn(pBase);
    VFuncs::Activate(pBase);

    VFuncs::SetCollisionGroup( pBase, COLLISION_GROUP_PLAYER );
    VFuncs::ClearSolidFlags(pBase);
}

void CAOPExploding::SetDamage(float damage)
{
    CBaseGrenade *pGren = (CBaseGrenade *)GetBase();
    VFuncs::SetDamage(pGren, damage);
}

void CAOPExploding::SetDamageRadius(float dmgradius)
{
    CBaseGrenade *pGren = (CBaseGrenade *)GetBase();
    VFuncs::SetDamageRadius(pGren, dmgradius);
}

/*void CAOPExploding::SetOwner(CBaseEntity *pOwner)
{
    // need vfuncs
    GetBase()->SetOwnerEntity(pOwner);
}*/

void CAOPExploding::Touch(CBaseEntity *pOther)
{
}

void CAOPExploding::Think()
{
}

void CAOPExploding::UpdateOnRemove(void)
{
    BaseClass::UpdateOnRemove();
    pAdminOP.UnhookEnt(GetBase(), this, GetIndex());
}
