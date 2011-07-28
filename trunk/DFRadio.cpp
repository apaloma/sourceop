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

#ifdef DFENTS

// HACK HACK HACK : BASEENTITY ACCESS
#define GAME_DLL
#include "cbase.h"
#include "baseentity.h"
// HACK HACK HACK : BASEENTITY ACCESS

#include "vmatrix.h"
#include "util.h"
#include "vphysics_interface.h"

#include <stdio.h>

#include "AdminOP.h"
#include "DFRadio.h"
#include "cvars.h"

void DFRadio :: Spawn(edict_t *pEntity, Vector origin)
{
    edict_t *pRadio = Get ( );

    MakeThink(&DFRadio::RadioThink);
}

void DFRadio :: KillOff(void)
{
    edict_t *pEntity = Get ( );
    CBaseEntity *pEnt = CBaseEntity::Instance(pEntity);
    int EntIndex = ENTINDEX(pEntity);

    if(pEnt)
        UTIL_Remove(pEnt);
    MakeTouch(NULL);
    MakeThink(NULL);
    pAdminOP.RemoveDFEnt(EntIndex);
}

void DFRadio :: RadioThink(void)
{
    edict_t *pEntity = Get ( );

    //pEntity->v.nextthink = gpGlobals->time + 0.1;
}

#endif
