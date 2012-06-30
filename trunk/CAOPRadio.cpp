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
#include "cvars.h"
#include "recipientfilter.h"
#include "beam_flags.h"
#include "sourcehooks.h"
#include "bitbuf.h"
#include "CAOPEntity.h"
#include "CAOPRadio.h"
#include "vfuncs.h"

#include "tier0/memdbgon.h"

#define RADIO_RANGE 1200

bool PhysModelParseSolidByIndex( solid_t &solid, CBaseEntity *pEntity, vcollide_t *pCollide, int solidIndex );

BEGIN_DATADESC( CAOPRadio )
    DEFINE_KEYFIELD( m_iOwner, FIELD_INTEGER, "ownerplayer" ),
    DEFINE_FIELD( m_bBeams, FIELD_BOOLEAN ),
    DEFINE_FIELD( m_bRings, FIELD_BOOLEAN ),
    DEFINE_FIELD( m_bFunnels, FIELD_BOOLEAN ),
END_DATADESC()

SOP_LINK_ENTITY_TO_CLASS_FEAT(sop_radio, CAOPRadio, FEAT_RADIO);

CAOPRadio::CAOPRadio() : CAutoSOPGameSystem( "CAOPRadio" )
{
    m_iOwner = 0;
    m_bBeams = 0;
    m_bRings = 0;
    m_bFunnels = 0;
}

CAOPRadio::~CAOPRadio()
{
}

void CAOPRadio::Spawn() 
{
    CBaseEntity *pBase = GetBase();

    SetDamage(1);
    SetDamageRadius(1);
    VFuncs::SetModel( pBase, radio_model.GetString() );
    if(radio_passable.GetBool())
        VFuncs::SetCollisionGroup( pBase, COLLISION_GROUP_WEAPON );
    else
        VFuncs::SetCollisionGroup( pBase, COLLISION_GROUP_PLAYER );
    CreateVPhysics();
    VFuncs::SetHealth(pBase, 100);
    m_flNextThink = gpGlobals->curtime;
    MakeMyThink(&CAOPRadio::Think);
    pAdminOP.myThinkEnts.AddToTail((CAOPEntity *)this);
    m_flNextBeam = 0;
    m_flNextRing = 0;
    m_flNextFunnel = 0;
    m_beamIndex = engine->PrecacheModel( "sprites/laserbeam.spr" );
    inRange.Purge();
}

void CAOPRadio::Precache()
{
    // precache radio loops
    for(unsigned short i = 0; i < pAdminOP.radioLoops.Count(); i++)
    {
        radioloop_t *radioloop = pAdminOP.radioLoops.Base() + i; // fast access
        char path[256];

        sprintf(path, "SourceOP/radio/%s", radioloop->File);
        enginesound->PrecacheSound(path, true);
    }

    engine->PrecacheModel(radio_model.GetString());
    engine->PrecacheModel("sprites/flare6.vmt", true);

    INetworkStringTable *pDownloadablesTable = networkstringtable->FindTable("downloadables");
    if(pDownloadablesTable)
    {
        char file[256];
        bool save = engine->LockNetworkStringTables(false);

        for(unsigned short i = 0; i < pAdminOP.radioLoops.Count(); i++)
        {
            radioloop_t *radioloop = pAdminOP.radioLoops.Base() + i; // fast access

            sprintf(file, "sound/SourceOP/radio/%s", radioloop->File);
            pDownloadablesTable->AddString(true, file);
        }
        sprintf(file, "models/sourceop/radio.mdl");
        pDownloadablesTable->AddString(true, file, sizeof(file));
        sprintf(file, "models/sourceop/radio.phy");
        pDownloadablesTable->AddString(true, file, sizeof(file));
        sprintf(file, "models/sourceop/radio.dx80.vtx");
        pDownloadablesTable->AddString(true, file, sizeof(file));
        sprintf(file, "models/sourceop/radio.dx90.vtx");
        pDownloadablesTable->AddString(true, file, sizeof(file));
        sprintf(file, "models/sourceop/radio.sw.vtx");
        pDownloadablesTable->AddString(true, file, sizeof(file));
        sprintf(file, "models/sourceop/radio.vvd");
        pDownloadablesTable->AddString(true, file, sizeof(file));
        sprintf(file, "materials/sourceop/radio_base.vmt");
        pDownloadablesTable->AddString(true, file, sizeof(file));
        sprintf(file, "materials/sourceop/radio_base.vtf");
        pDownloadablesTable->AddString(true, file, sizeof(file));
        sprintf(file, "materials/sourceop/radio_controles.vmt");
        pDownloadablesTable->AddString(true, file, sizeof(file));
        sprintf(file, "materials/sourceop/radio_controles.vtf");
        pDownloadablesTable->AddString(true, file, sizeof(file));
        sprintf(file, "materials/sourceop/radio_speaker.vmt");
        pDownloadablesTable->AddString(true, file, sizeof(file));
        sprintf(file, "materials/sourceop/radio_speaker.vtf");
        pDownloadablesTable->AddString(true, file, sizeof(file));
        sprintf(file, "materials/sprites/flare6.vmt");
        pDownloadablesTable->AddString(true, file, sizeof(file));
        sprintf(file, "materials/sprites/flare6.vtf");
        pDownloadablesTable->AddString(true, file, sizeof(file));

        engine->LockNetworkStringTables(save);
    }

    BaseClass::Precache();
}

void CAOPRadio::CreateVPhysics()
{
    solid_t tmpSolid;
    CBaseEntity *pBase = GetBase();
    vcollide_t *pCollide = modelinfo->GetVCollide( VFuncs::GetModelIndex(pBase) );
    if ( pCollide && pCollide->solidCount )
    {
        originOverride = Vector(0,0,0);
        PhysModelParseSolidByIndex( tmpSolid, pBase, pCollide, -1 );
        tmpSolid.params.massCenterOverride = &originOverride;
        tmpSolid.massCenterOverride = originOverride;
        tmpSolid.params.mass = 100;
        pBase->VPhysicsInitNormal( SOLID_VPHYSICS, 0, false, &tmpSolid );
    }
    else
    {
        pBase->VPhysicsInitNormal( SOLID_VPHYSICS, 0, false );
    }
}

void CAOPRadio::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
    int entindex = VFuncs::entindex(pActivator);

    if(entindex > 0 && entindex <= pAdminOP.GetMaxClients())
    {
        CAdminOPPlayer *pAOPPlayer = &pAdminOP.pAOPPlayers[entindex-1];
        if(entindex == m_iOwner)
        {
            if(radio_on.GetBool())
                ShowRadioMenu(m_iOwner, CCommand());
            else
                pAOPPlayer->SayTextChatHud(UTIL_VarArgs("[%s] Radio currently disabled.\n", pAdminOP.adminname));
        }
        else
        {
            pAOPPlayer->SayTextChatHud(UTIL_VarArgs("[%s] This radio does not belong to you. Say !help radio to learn more.\n", pAdminOP.adminname));
        }
        RETURN_META(MRES_SUPERCEDE);
    }
    BaseClass::Use(pActivator, pCaller, useType, value);
}

void CAOPRadio::Play( const char *pszFile )
{
    if(!radio_on.GetBool()) return;

    CBaseEntity *pBase = GetBase();
    Vector origin = GetAbsOrigin() + Vector(0,0,1);
    CBroadcastRecipientFilter filter;

    enginesound->EmitSound((CRecipientFilter&)filter, VFuncs::entindex(pBase), CHAN_VOICE, pszFile, 1.0, SNDLVL_NORM, 0, PITCH_NORM, 0, &origin);
}

void CAOPRadio::SetRadioOwner( int index )
{
    m_iOwner = index;
}

void CAOPRadio::ClientDisconnect(CBaseEntity *pEntity)
{
    CBaseEntity *pRadio = GetBase();

    if(!pEntity || m_iOwner <= 0 || m_iOwner >= gpGlobals->maxClients)
    {
        return;
    }
    int playerID = VFuncs::entindex(pEntity);
    if(playerID != m_iOwner)
    {
        return;
    }

    UTIL_Remove(pRadio);
}

int CAOPRadio::ClientCommand(CBaseEntity *pEntity, const CCommand &args)
{
    CBaseEntity *pRadio = GetBase();

    if(!pEntity || m_iOwner <= 0 || m_iOwner >= gpGlobals->maxClients)
    {
        return COMMAND_IGNORED;
    }
    int playerID = VFuncs::entindex(pEntity);
    if(playerID != m_iOwner)
    {
        return COMMAND_IGNORED;
    }

    // block buildradio command now that the player has one
    if(FStrEq(args[0], "buildradio"))
    {
        pAdminOP.pAOPPlayers[m_iOwner-1].SayTextChatHud(UTIL_VarArgs("[%s] You already have a radio.\n", pAdminOP.adminname));
        return COMMAND_STOP;
    }
    if(FStrEq(args[0], "detradio"))
    {
        CTakeDamageInfo info = CTakeDamageInfo();
        VFuncs::KeyValue(pRadio, "health", "-1");
        VFuncs::Event_Killed(pRadio, info);
        return COMMAND_HANDLED;
    }
    if(FStrEq(args[0], "radioplay"))
    {
        if(radio_on.GetBool())
        {
            radioloop_t *radioloop = NULL;
            for(int i = 0; i < pAdminOP.radioLoops.Count(); i++)
            {
                radioloop = pAdminOP.radioLoops.Base() + i;
                if(!stricmp(radioloop->ShortName, args.Arg(1))) break;
                radioloop = NULL;
            }

            if(radioloop)
            {
                Play(UTIL_VarArgs("SourceOP/radio/%s", radioloop->File));
            }
            else
            {
                pAdminOP.pAOPPlayers[m_iOwner-1].SayTextChatHud(UTIL_VarArgs("[%s] Radio loop not found.\n", pAdminOP.adminname));
            }
        }
        else
        {
            pAdminOP.pAOPPlayers[m_iOwner-1].SayTextChatHud(UTIL_VarArgs("[%s] Radio currently disabled.\n", pAdminOP.adminname));
        }
        return COMMAND_HANDLED;
    }
    if(FStrEq(args[0], "radiostop"))
    {
        StopRadio();
        return COMMAND_HANDLED;
    }
    if(FStrEq(args[0], "radiomenu"))
    {
        ShowRadioMenu(m_iOwner, args);
        // only let one radio respond to radiomenu
        return COMMAND_STOP;
    }
    if(FStrEq(args[0], "radiocolor"))
    {
        VFuncs::SetRenderColor(pRadio, atoi(args.Arg(1)), atoi(args.Arg(2)), atoi(args.Arg(3)));
        return COMMAND_HANDLED;
    }
    if(FStrEq(args[0], "radiomode"))
    {
        int mode = atoi(args.Arg(1));
        switch(mode)
        {
        case 0:
            VFuncs::SetRenderMode(pRadio, (RenderMode_t)kRenderTransTexture);
            VFuncs::SetRenderColorA(pRadio, 255);
            break;
        case 1:
            VFuncs::SetRenderMode(pRadio, (RenderMode_t)kRenderTransTexture);
            VFuncs::SetRenderColorA(pRadio, 128);
            break;
        case 2:
            VFuncs::SetRenderMode(pRadio, (RenderMode_t)kRenderTransTexture);
            VFuncs::SetRenderColorA(pRadio, 32);
            break;
        }
        return COMMAND_HANDLED;
    }
    if(FStrEq(args[0], "radiolight"))
    {
        if(atoi(args.Arg(1)))
        {
            VFuncs::AddEffects( pRadio, EF_BRIGHTLIGHT );
        }
        else
        {
            VFuncs::SetEffects( pRadio, 0 );
        }
        return COMMAND_HANDLED;
    }
    if(FStrEq(args[0], "radioeffect"))
    {
        int effect = atoi(args.Arg(1));
        switch(effect)
        {
        case 0:
            m_bBeams = m_bBeams ? 0 : 1;
            break;
        case 1:
            m_bRings = m_bRings ? 0 : 1;
            break;
        case 2:
            m_bFunnels = m_bFunnels ? 0 : 1;
            break;
        }
        return COMMAND_HANDLED;
    }

    return COMMAND_IGNORED;
}

void CAOPRadio::Think()
{
    UpdateRangeSample();
    if(te)
    {
        if(radio_on.GetBool())
        {
            if(m_bBeams && m_flNextBeam <= gpGlobals->curtime)
            {
                BeamEffect();
                m_flNextBeam = gpGlobals->curtime + 0.4f;
            }
            if(m_bRings && m_flNextRing <= gpGlobals->curtime)
            {
                RingEffect();
                m_flNextRing = gpGlobals->curtime + 1.5f;
            }
            if(m_bFunnels && m_flNextFunnel <= gpGlobals->curtime)
            {
                Vector origin = GetAbsOrigin() + Vector(0,0,1);
                CPVSFilter filter(origin);
                //              filter, delay, origin, modelindex,  reversed
                te->LargeFunnel(filter, 0.0f, &origin, m_beamIndex, 0);
                m_flNextFunnel = gpGlobals->curtime + 15.0f;
            }
        }
    }
    m_flNextThink = gpGlobals->curtime + 0.1;
}

void CAOPRadio::BeamEffect()
{
    CBaseEntity *pBase = GetBase();
    Vector forward, right, up;
    Vector start = GetAbsOrigin() + Vector(0,0,1);
    QAngle randAngles = QAngle(random->RandomFloat(180,360), random->RandomFloat(180,360), random->RandomFloat(0,360));
    AngleVectors( randAngles, &forward, &right, &up );
    Vector end = GetAbsOrigin() + ( forward * 8192 );
    trace_t tr;
    CTraceFilterSimple traceFilter( pBase, COLLISION_GROUP_NONE );
    Ray_t ray;
    ray.Init( start, end );
    enginetrace->TraceRay( ray, MASK_NPCSOLID, &traceFilter, &tr );

    CPVSFilter filter(start);
    te->BeamEntPoint(filter, 0, VFuncs::entindex(pBase), &Vector(0,0,0), -1, &tr.endpos, m_beamIndex, 0, 0, 0, random->RandomFloat(5.4,7.4), random->RandomFloat(0.4,0.6), random->RandomFloat(0.4,0.6), 0, 0, random->RandomInt(0,255), random->RandomInt(0,255), random->RandomInt(0,255), 255, 0);
}

void CAOPRadio::RingEffect()
{
    Vector radioOrigin = GetAbsOrigin();
    Vector effectOrigin = radioOrigin + Vector(random->RandomFloat(-96,96), random->RandomFloat(-96,96), random->RandomFloat(12,56));
    CPASFilter filter(effectOrigin);
    switch(random->RandomInt(0,1))
    {
    case 0:
        //                filter, delay, center, start_radius, end_radius, model, halo, startframe, framerate, life, width, spread, ampllitude, r, g, b,                           a, speed, flags
        te->BeamRingPoint(filter, 0.0f, effectOrigin, 0, 48, m_beamIndex, 0, 0, 0, 2.0f, 2.0f, 0, 0, random->RandomInt(0,255), random->RandomInt(0,255), random->RandomInt(0,255), 255, 0, FBEAM_FADEOUT);
        break;
    case 1:
        te->BeamRingPoint(filter, 0.0f, effectOrigin, 0, 240, m_beamIndex, 0, 0, 0, 2.0f, 2.0f, 0, 0, random->RandomInt(0,255), random->RandomInt(0,255), random->RandomInt(0,255), 255, 0, FBEAM_FADEOUT|FBEAM_SINENOISE);
        break;
    }
}

void CAOPRadio::GetRangeSample()
{
    for(int i = 1; i <= pAdminOP.GetMaxClients(); i++)
    {
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+i);
        if(!info)
            continue;
        if(!info->IsConnected())
            continue;

        Vector difference = GetAbsOrigin() - pAdminOP.pAOPPlayers[i-1].GetPosition();
        if(difference.Length() <= RADIO_RANGE)
        {
            inRange.AddToTail(i);
        }
    }
}

void CAOPRadio::UpdateRangeSample()
{
    int i;
    for(i = 0; i < inRange.Count(); i++)
    {
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+inRange.Element(i));
        if(!info)
        {
            inRange.Remove(i);
            //Msg("RADIO: Disconnect1\n");
            i--;
            continue;
        }
        if(!info->IsConnected())
        {
            inRange.Remove(i);
            //Msg("RADIO: Disconnect2\n");
            i--;
            continue;
        }
        Vector difference = GetAbsOrigin() - pAdminOP.pAOPPlayers[inRange.Element(i)-1].GetPosition();
        if(difference.Length() > RADIO_RANGE)
        {
            inRange.Remove(i);
            //Msg("RADIO: Out of range\n");
            i--;
            continue;
        }
    }
    for(int i = 1; i <= pAdminOP.GetMaxClients(); i++)
    {
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+i);
        if(!info)
            continue;
        if(!info->IsConnected())
            continue;

        Vector difference = GetAbsOrigin() - pAdminOP.pAOPPlayers[i-1].GetPosition();
        if(difference.Length() <= RADIO_RANGE)
        {
            if(inRange.Find(i) == inRange.InvalidIndex())
            {
                //Msg("RADIO: In range\n");
                inRange.AddToTail(i);
            }
        }
    }
}

void CAOPRadio::ShowRadioMenu(int playerID, const CCommand &args)
{
    if(!playerID)
        return;

    CBaseEntity *pRadio = GetBase();
    IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+playerID);
    unsigned int menuopt = atoi(args.Arg(1));

    if(!info)
        return;

    if(!info->IsConnected())
        return;

    if(!pRadio)
        return;

    edict_t *pEntity = pAdminOP.GetEntityList()+playerID;

    KeyValues *kv = new KeyValues( "menu" );
    kv->SetString( "title", "Radio menu, hit ESC" );
    kv->SetInt( "level", 3 );
    kv->SetColor( "color", Color( 0, 128, 255, 255 ));
    kv->SetInt( "time", 20 );
    kv->SetString( "msg", "SourceOP Radio menu.\nPick a option." );

    KeyValues *item;
    switch(menuopt)
    {
    case 1:
        {
            int page = atoi(args.Arg(2));
            int i;
            radioloop_t *radioloop;
            for(i = 1; i < min(7, pAdminOP.radioLoops.Count() - (page*6)); i++)
            {
                radioloop = pAdminOP.radioLoops.Base() + (page*6)+i;
                item = kv->FindKey( UTIL_VarArgs("%i", i), true );
                item->SetString( "msg", UTIL_VarArgs("%s", radioloop->Name) );
                item->SetString( "command", UTIL_VarArgs("radioplay %s;cancelselect", radioloop->ShortName) );
            }
            if(pAdminOP.radioLoops.Count() > (page+1)*6)
            {
                item = kv->FindKey( UTIL_VarArgs("%i", i), true );
                item->SetString( "msg", "More >>" );
                item->SetString( "command", UTIL_VarArgs("radiomenu 1 %i", page+1) );
                i++;
            }
            item = kv->FindKey( UTIL_VarArgs("%i", i), true );
            item->SetString( "msg", "<< Back" );
            if(page)
                item->SetString( "command", UTIL_VarArgs("radiomenu 1 %i", page-1) );
            else
                item->SetString( "command", "radiomenu 0" );
            break;
        }
    case 2:
        item = kv->FindKey( "1", true );
        item->SetString( "msg", "Equalizer" );
        item->SetString( "command", "radiomenu 0" );
        item = kv->FindKey( "2", true );
        item->SetString( "msg", "Is" );
        item->SetString( "command", "radiomenu 0" );
        item = kv->FindKey( "3", true );
        item->SetString( "msg", "Not" );
        item->SetString( "command", "radiomenu 0" );
        item = kv->FindKey( "4", true );
        item->SetString( "msg", "Implemented" );
        item->SetString( "command", "radiomenu 0" );
        item = kv->FindKey( "5", true );
        item->SetString( "msg", "<< Back" );
        item->SetString( "command", "radiomenu 0" );
        break;
    case 3:
        item = kv->FindKey( "1", true );
        item->SetString( "msg", "Render" );
        item->SetString( "command", "radiomenu 30" );
        item = kv->FindKey( "2", true );
        item->SetString( "msg", "Lighting" );
        item->SetString( "command", "radiomenu 31" );
        item = kv->FindKey( "3", true );
        item->SetString( "msg", "Effects" );
        item->SetString( "command", "radiomenu 32" );
        item = kv->FindKey( "4", true );
        item->SetString( "msg", "<< Back" );
        item->SetString( "command", "radiomenu 0" );
        break;
    case 30:
        item = kv->FindKey( "1", true );
        item->SetString( "msg", "Color" );
        item->SetString( "command", "radiomenu 300" );
        item = kv->FindKey( "2", true );
        item->SetString( "msg", "Opacity/Transparency" );
        item->SetString( "command", "radiomenu 302" );
        item = kv->FindKey( "3", true );
        item->SetString( "msg", "<< Back" );
        item->SetString( "command", "radiomenu 3" );
        break;
    case 300:
        item = kv->FindKey( "1", true );
        item->SetString( "msg", "Red" );
        item->SetString( "command", "radiocolor 255 0 0;cancelselect" );
        item = kv->FindKey( "2", true );
        item->SetString( "msg", "Green" );
        item->SetString( "command", "radiocolor 0 255 0;cancelselect" );
        item = kv->FindKey( "3", true );
        item->SetString( "msg", "Blue" );
        item->SetString( "command", "radiocolor 0 0 255;cancelselect" );
        item = kv->FindKey( "4", true );
        item->SetString( "msg", "Yellow" );
        item->SetString( "command", "radiocolor 255 255 0;cancelselect" );
        item = kv->FindKey( "5", true );
        item->SetString( "msg", "Magenta" );
        item->SetString( "command", "radiocolor 255 0 255;cancelselect" );
        item = kv->FindKey( "6", true );
        item->SetString( "msg", "Cyan" );
        item->SetString( "command", "radiocolor 0 255 255;cancelselect" );
        item = kv->FindKey( "7", true );
        item->SetString( "msg", "More >>" );
        item->SetString( "command", "radiomenu 301" );
        item = kv->FindKey( "8", true );
        item->SetString( "msg", "<< Back" );
        item->SetString( "command", "radiomenu 30" );
        break;
    case 301:
        item = kv->FindKey( "1", true );
        item->SetString( "msg", "Orange" );
        item->SetString( "command", "radiocolor 255 0 0;cancelselect" );
        item = kv->FindKey( "2", true );
        item->SetString( "msg", "White" );
        item->SetString( "command", "radiocolor 255 255 255;cancelselect" );
        item = kv->FindKey( "3", true );
        item->SetString( "msg", "Black" );
        item->SetString( "command", "radiocolor 0 0 0;cancelselect" );
        item = kv->FindKey( "4", true );
        item->SetString( "msg", "Off/Normal" );
        item->SetString( "command", "radiocolor 255 255 255;cancelselect" );
        item = kv->FindKey( "5", true );
        item->SetString( "msg", "<< Back" );
        item->SetString( "command", "radiomenu 300" );
        break;
    case 302:
        item = kv->FindKey( "1", true );
        item->SetString( "msg", "Fully-Opaque" );
        item->SetString( "command", "radiomode 0;cancelselect" );
        item = kv->FindKey( "2", true );
        item->SetString( "msg", "Semi-Opaque" );
        item->SetString( "command", "radiomode 1;cancelselect" );
        item = kv->FindKey( "3", true );
        item->SetString( "msg", "Almost Transparent" );
        item->SetString( "command", "radiomode 2;cancelselect" );
        item = kv->FindKey( "4", true );
        item->SetString( "msg", "<< Back" );
        item->SetString( "command", "radiomenu 30" );
        break;
    case 31:
        item = kv->FindKey( "1", true );
        item->SetString( "msg", "On" );
        item->SetString( "command", "radiolight 1;cancelselect" );
        item = kv->FindKey( "2", true );
        item->SetString( "msg", "Off" );
        item->SetString( "command", "radiolight 0;cancelselect" );
        item = kv->FindKey( "3", true );
        item->SetString( "msg", "<< Back" );
        item->SetString( "command", "radiomenu 3" );
        break;
    case 32:
        item = kv->FindKey( "1", true );
        item->SetString( "msg", UTIL_VarArgs("Lasers %s", m_bBeams ? "Off" : "On") );
        item->SetString( "command", "radioeffect 0;cancelselect" );
        item = kv->FindKey( "2", true );
        item->SetString( "msg", UTIL_VarArgs("Rings %s", m_bRings ? "Off" : "On") );
        item->SetString( "command", "radioeffect 1;cancelselect" );
        item = kv->FindKey( "3", true );
        item->SetString( "msg", UTIL_VarArgs("Funnels %s", m_bFunnels ? "Off" : "On") );
        item->SetString( "command", "radioeffect 2;cancelselect" );
        item = kv->FindKey( "4", true );
        item->SetString( "msg", "<< Back" );
        item->SetString( "command", "radiomenu 3" );
        break;
    default:
        item = kv->FindKey( "1", true );
        item->SetString( "msg", "Song List" );
        item->SetString( "command", "radiomenu 1" );
        item = kv->FindKey( "2", true );
        item->SetString( "msg", "Equalizer Settings" );
        item->SetString( "command", "radiomenu 2" );
        item = kv->FindKey( "3", true );
        item->SetString( "msg", "Appearance" );
        item->SetString( "command", "radiomenu 3" );
        item = kv->FindKey( "4", true );
        item->SetString( "msg", "Detonate" );
        item->SetString( "command", "detradio;cancelselect" );
        item = kv->FindKey( "5", true );
        item->SetString( "msg", "Stop" );
        item->SetString( "command", "radiostop;cancelselect" );
        break;
    }

    helpers->CreateMessage( pEntity, DIALOG_MENU, kv, &g_ServerPlugin );
    kv->deleteThis();
}

void CAOPRadio::StopRadio()
{
    CBaseEntity *pBase = GetBase();
    Vector origin = GetAbsOrigin() + Vector(0,0,1);
    CBroadcastRecipientFilter filter;
    enginesound->EmitSound((CRecipientFilter&)filter, VFuncs::entindex(pBase), CHAN_VOICE, "common/null.wav", 1.0, SNDLVL_NORM, 0, PITCH_NORM, 0, &origin);
}

void CAOPRadio::UpdateOnRemove( void )
{
    StopRadio();

    BaseClass::UpdateOnRemove();
}
