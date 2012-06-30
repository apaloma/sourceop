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
#include "sourcehooks.h"
#include "bitbuf.h"
#include "CAOPEntity.h"
#include "CAOPNormal.h"
#include "vfuncs.h"

#include "tier0/memdbgon.h"

BEGIN_DATADESC( CAOPNormal )
END_DATADESC()

CAOPNormal::CAOPNormal()
{
    CBaseEntity *pProp = CreateEntityByName( "item_sodacan" );
    if(pProp)
    {
        ADD_TO_VP_ENTLIST(pProp, this);
        pAdminOP.ServiceEnt(pProp, this);
        Set(servergameents->BaseEntityToEdict(pProp));
    }
    else
    {
        CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Failed to create CAOPNormal.\n");
        BaseClass::Set(NULL);
    }
}

void CAOPNormal::Set(edict_t *pent) 
{
    CBaseEntity *pBase;
    BaseClass::Set(pent);
    MakeThink(&CAOPNormal::Think);
    MakeTouch(&CAOPNormal::Touch);
    pBase = GetBase();
    VFuncs::SetTakeDamage(pBase, DAMAGE_YES);
    VFuncs::SetHealth(pBase, 1);
    VFuncs::SetCollisionGroup( pBase, COLLISION_GROUP_PLAYER );
    VFuncs::Spawn(pBase);
    VFuncs::Activate(pBase);

    VFuncs::SetCollisionGroup( pBase, COLLISION_GROUP_PLAYER );
    VFuncs::ClearSolidFlags(pBase);
}

/*void CAOPNormal::SetOwner(CBaseEntity *pOwner)
{
    // need vfuncs
    GetBase()->SetOwnerEntity(pOwner);
}*/

void CAOPNormal::Touch(CBaseEntity *pOther)
{
    RETURN_META(MRES_SUPERCEDE);
}

void CAOPNormal::Think()
{
    RETURN_META(MRES_SUPERCEDE);
}

void CAOPNormal::UpdateOnRemove(void)
{
    BaseClass::UpdateOnRemove();
    pAdminOP.UnhookEnt(GetBase(), this, GetIndex());
}
