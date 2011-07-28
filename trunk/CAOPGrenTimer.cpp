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
#include "CAOPGrenTimer.h"
#include "vfuncs.h"

#include "tier0/memdbgon.h"

BEGIN_DATADESC( CAOPGrenTimer )
    DEFINE_KEYFIELD( m_iOwner, FIELD_INTEGER, "ownerplayer" ),
    DEFINE_KEYFIELD( m_flInterval, FIELD_FLOAT, "interval" ),
    DEFINE_KEYFIELD( m_flLife, FIELD_FLOAT, "life" ),
END_DATADESC()

SOP_LINK_ENTITY_TO_CLASS(sop_grenade_timer, CAOPGrenTimer);

CAOPGrenTimer::CAOPGrenTimer()
{
    m_iOwner = 0;
    m_flInterval = 0;
    m_flLife = 0;
    m_flFinishTime = 0;
}

void CAOPGrenTimer::Spawn() 
{
    CBaseEntity *pBase = GetBase();

    if(!m_iOwner || !m_flInterval || !m_flLife)
    {
        UTIL_Remove(pBase);
        RETURN_META(MRES_SUPERCEDE);
    }

    m_flFinishTime = gpGlobals->curtime + m_flLife;
    MakeMyThink(&CAOPGrenTimer::BeepThink);
    pAdminOP.myThinkEnts.AddToTail((CAOPEntity *)this);

    VFuncs::AddEffects( pBase, EF_NODRAW );
    UTIL_SetSize( pBase, Vector(0,0,0), Vector(0,0,0) );
    VFuncs::SetCollisionGroup( pBase, COLLISION_GROUP_NONE );

    // Think immediately
    BeepThink();

    RETURN_META(MRES_SUPERCEDE);
}

void CAOPGrenTimer::Precache()
{
    enginesound->PrecacheSound("buttons/button8.wav", true);
    enginesound->PrecacheSound("buttons/button9.wav", true);

    BaseClass::Precache();
}

void CAOPGrenTimer::BeepThink()
{
    CReliableBroadcastRecipientFilter filter;
    filter.RemoveAllRecipients();
    filter.AddRecipient(m_iOwner);

    enginesound->EmitSound((CRecipientFilter&)filter, m_iOwner, CHAN_ITEM, "buttons/button9.wav", 0.4, SNDLVL_NORM, 0, 100, &pAdminOP.pAOPPlayers[m_iOwner-1].EyePosition());
    
    m_flNextThink = gpGlobals->curtime + m_flInterval;
    if(m_flFinishTime <= m_flNextThink)
    {
        MakeMyThink(&CAOPGrenTimer::FinalBeepThink);
    }
}

void CAOPGrenTimer::FinalBeepThink()
{
    CReliableBroadcastRecipientFilter filter;
    filter.RemoveAllRecipients();
    filter.AddRecipient(m_iOwner);

    enginesound->EmitSound((CRecipientFilter&)filter, m_iOwner, CHAN_ITEM, "buttons/button8.wav", 0.4, SNDLVL_NORM, 0, 100, &pAdminOP.pAOPPlayers[m_iOwner-1].EyePosition());

    UTIL_Remove(GetBase());
}