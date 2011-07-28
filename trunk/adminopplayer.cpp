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
#ifdef _L4D_PLUGIN
#include "convar_l4d.h"
#endif
#include "cbase.h"
#include "baseentity.h"
#include "player.h"
// HACK HACK HACK : BASEENTITY ACCESS

#include "vcollide_parse.h"
#include "inetchannel.h"
#include "toolframework/itoolentity.h"

#include <stdio.h>
#include <time.h>

#include "AdminOP.h"
#include "sourcehooks.h"
#include "adminopplayer.h"
#include "sourceopadmin.h"
#include "bitbuf.h"
#include "cvars.h"
#include "download.h"
#include "randomstuff.h"
#include "reservedslots.h"
#include "isopgamesystem.h"
#include "specialitems.h"
#include "tf2items.h"

#include "vfuncs.h"

#include "lua.h"

#include "lunar.h"
#include "lentitycache.h"

#include "LuaLoader.h"

#include "tier0/memdbgon.h"

#define LIMIT_MSG() SayTextChatHud(UTIL_VarArgs("[%s] Entity limit reached. Your limit: %i/%i. Server limit: %i/%i.\n", pAdminOP.adminname, spawnedEnts.Count(), GetSpawnLimit(), pAdminOP.spawnedServerEnts.Count(), spawnlimit_server.GetInt()))

#define PVFN( classptr , offset ) ((*(DWORD*) classptr ) + offset)
#define VFN( classptr , offset ) *(DWORD*)PVFN( classptr , offset )

const char szTires[][64] = {"models/props_vehicles/tire001c_car.mdl",
                            "models/props_vehicles/apc_tire001.mdl",
                            "models/props_vehicles/tire001a_tractor.mdl",
                            "models/props_wasteland/wheel01a.mdl",
                            "models/props_wasteland/wheel02a.mdl"};

CAdminOPPlayer::CAdminOPPlayer()
{
    entID = 0;
    uid = 0;
    playerState = 0;
    connecttime = 0;
    sessionstarttime = 0;
    m_bCustomSteamIDValidated = false;
    ZeroCredits();

    InitPlayerData();
}

CAdminOPPlayer::~CAdminOPPlayer()
{
}

// Connect comes after Activate in the case of bots
void CAdminOPPlayer :: Connect()
{
    edict_t *pEntity = pAdminOP.GetEntityList()+entID;
    if(pEntity == NULL)
        return;

    InitPlayerData();
    m_iFakeID = random->RandomInt(20125638, 25125638);
    const CSteamID *steamIdFromEngine = engine->GetClientSteamID(pEntity);
    if(steamIdFromEngine)
    {
        steamid = CSteamID(steamIdFromEngine->ConvertToUint64());
    }
    else
    {
        steamid = CSteamID();
    }

    if(steamid_customnetworkidvalidated.GetBool() && m_bCustomSteamIDValidated == false)
    {
        NetworkIDValidated(GetJoinName(), engine->GetPlayerNetworkIDString(pEntity));
        m_bCustomSteamIDValidated = true;
    }
    Credits.TimeJoined = engine->Time();
    nextCreditCounter = engine->Time()+30;
    if(GetConnectTime() <= 0.01f)
    {
        SetConnectTime(gpGlobals->curtime);
    }
    if(sessionstarttime == 0)
    {
        sessionstarttime = time(NULL);
    }

    if(playerState <= 1) playerState = 1;
}

void CAdminOPPlayer :: Activate()
{
    playerState = 2;
    nextthink = 0;
    SendCreditCounter();
    nextCreditCounter = engine->Time()+6;

    IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+entID);
    if(playerinfo)
    {
        GetMyUserData();
        uid = playerinfo->GetUserID();
        if(FStrEq(engine->GetPlayerNetworkIDString(pAdminOP.GetEntityList()+entID), "BOT"))
        {
            Connect();
            ZeroCredits();
            strcpy(Credits.WonID, engine->GetPlayerNetworkIDString(pAdminOP.GetEntityList()+entID));
            notBot = 0;
        }
    }
    if(remoteserver) remoteserver->SendPlayerUpdateToAll(entID);
    if(pAdminOP.FeatureStatus(FEAT_LUA))
    {
        lua_getglobal(pAdminOP.GetLuaState(), "hook");
        lua_pushliteral(pAdminOP.GetLuaState(), "Call");
        lua_gettable(pAdminOP.GetLuaState(), -2);
        lua_remove(pAdminOP.GetLuaState(), -2);
        lua_pushstring(pAdminOP.GetLuaState(), "PlayerActivate");
        g_entityCache.PushPlayer(entID);
        lua_pcall(pAdminOP.GetLuaState(), 2, 0, 0);
    }
}

void CAdminOPPlayer :: NetworkIDValidated(const char *pszUserName, const char *pszNetworkID)
{
    if(!entID)
        return;

    edict_t *pEntity = pAdminOP.GetEntityList()+entID;
    if(pEntity == NULL)
        return;

    m_bSteamIDValidated = true;
    V_strncpy(Credits.WonID, pszNetworkID, sizeof(Credits.WonID));
    ISOPGameSystem::NetworkIDValidatedAllSystems(pEntity, pszUserName, pszNetworkID);

    if(pAdminOP.rslots)
    {
        CSteamID steamid;

        char errmsg[128];
        if(IsValidSteamID(pszNetworkID, errmsg, sizeof(errmsg)))
        {
            steamid = CSteamID(pszNetworkID, k_EUniversePublic);
        }
        else
        {
            CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "%s\n", errmsg);
            //pAdminOP.TimeLog("SourceOPErrors.log", "Invalid SteamID in NetworkIDValidated: %s. Error is: %s\n", pszNetworkID, errmsg);
        }
        

        pAdminOP.rslots->UpdatePlayerReservedSlot(entID-1, pszUserName, steamid);
    }

    IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo(pEntity);
    if(!playerinfo)
        return;

    // why is this here?
    if(playerState >= 2)
        return;

    GetMyUserData(pszUserName);
    UpdateAdminLevel();
}

PLUGIN_RESULT CAdminOPPlayer :: ClientCommand(const CCommand &args)
{
    if(!entID)
        return PLUGIN_CONTINUE;

    edict_t *pEntity = pAdminOP.GetEntityList()+entID;
    if(pEntity == NULL)
        return PLUGIN_CONTINUE;

    CBasePlayer *pPlayer = (CBasePlayer *)VFuncs::Instance(pEntity);
    if(!pPlayer)
        return PLUGIN_CONTINUE;

    IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo(pEntity);
    if(!playerinfo)
        return PLUGIN_CONTINUE;
    char pcmd[512];
    strncpy( pcmd, args[0], sizeof(pcmd) );

    if(FStrEq(pcmd, "testweaponattribs") && IsAdmin(1024, "test"))
    {
        CBaseCombatWeapon *pWeapon = VFuncs::Weapon_GetSlot(pPlayer, atoi(args[1]));
        if(pWeapon)
        {
            void *pAttribManager;
            CEconItemView *pItem;
            void *pAttributes = NULL;
            //pAttribManager = VFuncs::GetAttributeManager(pWeapon);
            //pAttribOwner = VFuncs::GetAttributeOwner(pWeapon);
            //pAttribContainer = VFuncs::GetAttributeContainer(pWeapon);

            pAttribManager = (void*)(((char *)pWeapon) + 1072);
            pItem = (CEconItemView*)(((char *)pAttribManager) + 64);
            //pAttributes = pItem->m_attributes;
            Msg("%p %p %p\n", pAttribManager, pItem, pAttributes);
            Msg("Dumping attribs:\n");
            for(int i = 0; i < pItem->m_attributes.Count(); i++)
            {
                CEconItemAttribute *pAttrib = &pItem->m_attributes[i];
                Msg("%i %f\n", pAttrib->m_iAttribDef, pAttrib->m_flVal);
            }
            CEconItemAttribute newAttrib;
            newAttrib.m_flVal = 4.0f;
            newAttrib.m_iAttribDef = 4;
            pItem->m_attributes.AddToTail(newAttrib);
        }
        else
        {
            Msg("No weapon\n");
        }
    }
    if(FStrEq(pcmd, "testsendfile") && IsAdmin(1024, "test"))
    {
        INetChannel *pNet = (INetChannel *)engine->GetPlayerNetInfo(entID);
        Msg("SendFile: %i\n", pNet->SendFile("bin/testfile.txt", 0));
    }
    if(FStrEq(pcmd, "testgetfile") && IsAdmin(1024, "test"))
    {
        INetChannel *pNet = (INetChannel *)engine->GetPlayerNetInfo(entID);
        Msg("RequestFile: %i\n", pNet->RequestFile("cfg/autoexec.cfg"));
    }
    if(FStrEq(pcmd, "testtf2giveweapon") && IsAdmin(1024, "test"))
    {
        GiveTFWeapon(args.Arg(1));
    }
    if(FStrEq(pcmd, "testgiveweapon") && IsAdmin(1024, "test"))
    {
        GiveNamedItem(args.Arg(1));
    }
    if(FStrEq(pcmd, "testturret") && IsAdmin(1024, "test"))
    {
        Vector forward, right, up, pos;
        QAngle playerAngles = VFuncs::EyeAngles(pPlayer);

        AngleVectors( playerAngles, &forward, &right, &up );
        pos = VFuncs::GetAbsOrigin(pPlayer) + (forward * 50);
        pos.x = (int)pos.x;
        pos.y = (int)pos.y;
        pos.z = (int)pos.z;
        pAdminOP.CreateTurret(args.Arg(1), atoi(args.Arg(2)), pos, QAngle(0,270,0), this);
    }
    if(FStrEq(pcmd, "testfrag") && IsAdmin(1024, "test"))
    {
        CBaseEntity *pGrenade = CreateEntityByName("sop_grenade");
        if(pGrenade)
        {
            Vector orig = VFuncs::GetAbsOrigin(pPlayer);
            QAngle angles = VFuncs::GetLocalAngles(pPlayer);
            if(atof(args.Arg(1)) != 0)
            {
                VFuncs::KeyValue(pGrenade, "life", UTIL_VarArgs("%f", atof(args.Arg(1))));
            }
            VFuncs::Spawn(pGrenade);
            VFuncs::Teleport(pGrenade, &orig, &angles, NULL);
        }
    }
    if(FStrEq(pcmd, "testthrowfrag") && IsAdmin(1024, "test"))
    {
        ThrowFragGrenade();
    }
    if(FStrEq(pcmd, "specialequip"))
    {
        if(args.ArgC() <= 1)
        {
            CSpecialItem *pCustomSpecialItem;
            int i = 0;

            while(pCustomSpecialItem = g_specialItemLoader.GetItemIterative(GetSteamID(), i))
            {
                SayText(UTIL_VarArgs("[%s] ItemID %i -- %s (%s)\n", pAdminOP.adminname, pCustomSpecialItem->m_iIndex, pCustomSpecialItem->m_szPetName, pCustomSpecialItem->m_bEquipped ? "equipped" : "unequipped"), HUD_PRINTCONSOLE);
                i++;
            }
        }
        else if(args.ArgC() == 3)
        {
            int iItemToEquip = atoi(args[1]);
            int iEquip = atoi(args[2]);
            CSpecialItem *pCustomSpecialItem;
            bool bHandled = false;
            int i = 0;

            while(pCustomSpecialItem = g_specialItemLoader.GetItemIterative(GetSteamID(), i))
            {
                if(pCustomSpecialItem->m_iIndex == iItemToEquip)
                {
                    if(iEquip == 2)
                    {
                        pCustomSpecialItem->m_bEquipped = !pCustomSpecialItem->m_bEquipped;
                    }
                    else
                    {
                        pCustomSpecialItem->m_bEquipped = (iEquip != 0);
                    }
                    bHandled = true;
                    SayText(UTIL_VarArgs("[%s] ItemID %i -- %s is now %s\n", pAdminOP.adminname, pCustomSpecialItem->m_iIndex, pCustomSpecialItem->m_szPetName, pCustomSpecialItem->m_bEquipped ? "equipped" : "unequipped"), HUD_PRINTCONSOLE);
                    break;
                }
                i++;
            }

            if(!bHandled)
            {
                SayText(UTIL_VarArgs("[%s] ItemID %i not found\n", pAdminOP.adminname, iItemToEquip), HUD_PRINTCONSOLE);
            }
        }
        else
        {
            SayText(UTIL_VarArgs("[%s] Usage: %s <itemid> <0/1/2>\n", pAdminOP.adminname, args[0]), HUD_PRINTCONSOLE);
            SayText(UTIL_VarArgs("[%s] Use 0 for the second parameter to unequip.\n"
                "     Use 1 for the second parameter to equip.\n"
                "     Use 2 for the second parameter to toggle.\n"
                "     Use no parameters to get a list of available items.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
        }
        return PLUGIN_STOP;
    }
    if(FStrEq(pcmd, "+gren"))
    {
        if(!m_flGrenadeDetonateTime && IsAlive())
        {
            if(grenades.GetBool() || IsAdmin(1024, "grenadeoverride"))
            {
                if(m_iGrenades > 0 || IsAdmin(1024, "infinitegrenades"))
                {
                    Vector orig = EyePosition()-Vector(0,0,6);
                    m_flGrenadeDetonateTime = gpGlobals->curtime + 4.1;

                    CBaseEntity *pTimer = CreateEntityByName("sop_grenade_timer");
                    if(pTimer)
                    {
                        VFuncs::KeyValue(pTimer, "life", "3.5");
                        VFuncs::KeyValue(pTimer, "interval", "1");
                        VFuncs::KeyValue(pTimer, "ownerplayer", UTIL_VarArgs("%i", entID));
                        VFuncs::Spawn(pTimer);
                    }

                    if(pAdminOP.isTF2)
                    {
                        CPASFilter filter( orig );
                        enginesound->EmitSound((CRecipientFilter&)filter, VFuncs::entindex(pPlayer), CHAN_ITEM, "player/taunt_launcher_hit.wav", 0.2, SNDLVL_NORM, 0, random->RandomInt(240, 250), &orig);  
                    }

                    m_iGrenades--;
                    SayTextChatHud(UTIL_VarArgs("[%s] You now have %i grenade%s remaining.\n", pAdminOP.adminname, m_iGrenades, m_iGrenades != 1 ? "s" : ""));
                }
                else
                {
                    SayTextChatHud(UTIL_VarArgs("[%s] You do not have any grenades remaining.\n", pAdminOP.adminname));
                }
            }
            else
            {
                SayTextChatHud(UTIL_VarArgs("[%s] Grenades are disabled.\n", pAdminOP.adminname));
            }
        }
        return PLUGIN_STOP;
    }
    if(FStrEq(pcmd, "-gren"))
    {
        if(m_flGrenadeDetonateTime)
        {
            ThrowFragGrenade();
        }
        return PLUGIN_STOP;
    }
    if(pAdminOP.FeatureStatus(FEAT_SNARK))
    {
        if(FStrEq(pcmd, "snark"))
        {
            if(IsAdmin(1024, "snark"))
            {
                Vector forward, right, up;
                QAngle playerAngles = VFuncs::EyeAngles(pPlayer);

                AngleVectors( playerAngles, &forward, &right, &up );
                forward.z = 0;
                VectorNormalize(forward);

                CBaseEntity *pSnark = CreateEntityByName("sop_snark");
                if(pSnark)
                {
                    Vector orig = VFuncs::GetAbsOrigin(pPlayer)+forward*64;
                    QAngle angles = VFuncs::GetLocalAngles(pPlayer);

                    VFuncs::KeyValue(pSnark, "ownerplayer", UTIL_VarArgs("%i", entID));
                    VFuncs::Spawn(pSnark);
                    VFuncs::Teleport(pSnark, &orig, &angles, NULL);
                    VFuncs::SetAbsVelocity(pSnark, VFuncs::GetAbsVelocity(pPlayer));

                    trace_t trace;
                    UTIL_TraceEntity( pSnark, orig, orig, MASK_PLAYERSOLID, &trace );

                    if ( trace.startsolid )
                    {
                        //if ( !FindPassableSpace( pSnark, up, 1, orig ) )  // up
                        //{
                        //  VFuncs::SetAbsOrigin(pSnark,  orig );
                        //  if ( !FindPassableSpace( pSnark, up, -1, orig ) )   // down
                        //  {
                        //      VFuncs::SetAbsOrigin(pSnark,  orig );
                                if ( !FindPassableSpace( pSnark, forward, -0.5, orig ) )    // back
                                {
                                    //Msg( "Can't find the world\n" );
                                }
                        //  }
                        //}
                        VFuncs::SetAbsOrigin(pSnark, orig );
                    }
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command snark.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }

            return PLUGIN_STOP;
        }

    }
    if(DoRadioCommands(pcmd, args, playerinfo, pPlayer) == PLUGIN_STOP) return PLUGIN_STOP;

    if(FStrEq(pcmd, "jointeam"))
    {
        if(mp_teamplay->GetBool())
        {
            if(atoi(args.Arg(1)) == 0)
            {
                // some people were joining the 0 team while team play was on thus creating three teams
                SayTextChatHud(UTIL_VarArgs("[%s] Team switch denied.\n", pAdminOP.adminname));
                return PLUGIN_STOP;
            }
        }
    }
    if(FStrEq(pcmd, "voicemenu"))
    {
        if(tf2_disable_voicemenu.GetInt() == 1)
        {
            if(args.ArgC() >= 3 && atoi(args.Arg(1)) == 0 && atoi(args.Arg(2)) == 0)
            {
                if(tf2_disable_voicemenu_message.GetBool())
                {
                    SayTextChatHud(UTIL_VarArgs("[%s] Calling for medic is currently disabled.\n", pAdminOP.adminname));
                }

                return PLUGIN_STOP;
            }
        }
        else if(tf2_disable_voicemenu.GetInt() == 2)
        {
            if(tf2_disable_voicemenu_message.GetBool())
            {
                SayTextChatHud(UTIL_VarArgs("[%s] Voice commands are currently disabled.\n", pAdminOP.adminname));
            }

            return PLUGIN_STOP;
        }
    }
    if(pAdminOP.FeatureStatus(FEAT_ENTCOMMANDS))
    {
        if(FStrEq(pcmd, "+grabent"))
        {
            if(IsAdmin(1024, "grabent"))
            {
                if(!entMoving && !entRotating)
                {
                    Vector forward, right, up;
                    QAngle playerAngles = VFuncs::EyeAngles(pPlayer);

                    AngleVectors( playerAngles, &forward, &right, &up );

                    float distance = 8192;
                    Vector start = EyePosition();
                    Vector end = EyePosition() + ( forward * distance );

                    trace_t tr;
                    CTraceFilterSimple traceFilter( pPlayer, COLLISION_GROUP_NONE );
                    Ray_t ray;
                    ray.Init( start, end );
                    enginetrace->TraceRay( ray, MASK_NPCSOLID, &traceFilter, &tr );

                    if(tr.DidHit() && tr.m_pEnt)
                    {
                        if(VFuncs::entindex(tr.m_pEnt) != 0)
                        {
                            char szClassname[64];
                            strncpy(szClassname, VFuncs::GetClassname(tr.m_pEnt), 64);
                            if(Q_strncasecmp(szClassname, "prop_vehicle", 12) && !VFuncs::IsPlayer(tr.m_pEnt))
                            {
                                IPhysicsObject *pObj = VFuncs::VPhysicsGetObject(tr.m_pEnt);
                                if(pObj)
                                    pObj->EnableMotion(0);
                            }

                            entMoving = 1;
                            pEntMove = servergameents->BaseEntityToEdict(tr.m_pEnt);
                            origRM = VFuncs::GetRenderMode(tr.m_pEnt);
                            origRF = VFuncs::GetRenderFX(tr.m_pEnt);
                            origRC = VFuncs::GetRenderColor(tr.m_pEnt);
                            origPlayerLoc = VFuncs::GetAbsOrigin(pPlayer);
                            origEntLoc = VFuncs::GetAbsOrigin(tr.m_pEnt);

                            if(entmove_changecolor.GetBool())
                            {
                                VFuncs::SetRenderColor(tr.m_pEnt, 255,0,0,96);
                                VFuncs::SetRenderMode(tr.m_pEnt, kRenderTransColor);
                                VFuncs::SetRenderFX(tr.m_pEnt, kRenderFxNone);
                            }
                        }
                    }
                }
                else
                {
                    SayTextChatHud(UTIL_VarArgs("[%s] You are currently moving an entity and cannot run +grabent.\n", pAdminOP.adminname));
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command +grabent.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }

            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "-grabent"))
        {
            if(entMoving)
            {
                if(pEntMove)
                {
                    CBaseEntity *pMoving = CBaseEntity::Instance(pEntMove);
                    if(pMoving)
                    {
                        IPhysicsObject *pObj = VFuncs::VPhysicsGetObject(pMoving);
                        if(pObj)
                        {
                            pObj->EnableMotion(1);
                            pObj->Sleep();
                        }

                        VFuncs::SetRenderColor(pMoving, origRC.r,origRC.g,origRC.b,origRC.a);
                        VFuncs::SetRenderMode(pMoving, origRM);
                        VFuncs::SetRenderFX(pMoving, origRF);
                    }
                }
                entMoving = 0;
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "+grabphys"))
        {
            if(IsAdmin(1024, "grabphys"))
            {
                entPhysMoving = 1;
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command +grabphys.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "-grabphys"))
        {
            if(entPhysMoving)
            {
                DropGrabbedPhysObject();
                entPhysMoving = 0;
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_physgun_lengthen"))
        {
            if(IsAdmin(1024, "e_physgun_lengthen"))
            {
                PhysMoving.m_distance += 30;
                if(PhysMoving.m_distance > 4096) PhysMoving.m_distance = 4096;
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_physgun_lengthen.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_physgun_shorten"))
        {
            if(IsAdmin(1024, "e_physgun_shorten"))
            {
                PhysMoving.m_distance -= 30;
                if(PhysMoving.m_distance < 36) PhysMoving.m_distance = 36;
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_physgun_shorten.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        /*if(FStrEq(pcmd, "+copyent_water"))
        {
            if(!entMoving && !entRotating && IsAdmin(1024, "copyent"))
            {
                CBaseEntity *pHit = FindEntityForward();
                if(pHit)
                {
                    CBaseEntity *pProp = CreateEntityByName( "func_water" );
                    if(pProp)
                    {
                        Vector vecOrigin = VFuncs::GetAbsOrigin(pHit);
                        QAngle vecAngles = VFuncs::GetLocalAngles(pHit);
                        VFuncs::KeyValue(pProp,  "angles", UTIL_VarArgs("%f %f %f", vecAngles.x, vecAngles.y, vecAngles.z) );
                        VFuncs::KeyValue(pProp,  "origin", vecOrigin );
                        VFuncs::KeyValue(pProp,  "model", STRING(VFuncs::GetModelName(pHit)) );
                        VFuncs::Spawn(pProp);
                        VFuncs::Activate(pProp);
                        VFuncs::Teleport(pProp,  &vecOrigin, &vecAngles, NULL );

                        entMoving = 1;
                        pEntMove = servergameents->BaseEntityToEdict(pProp);
                        origPlayerLoc = VFuncs::GetAbsOrigin(pPlayer);
                        origEntLoc = VFuncs::GetAbsOrigin(pHit);
                    }
                }
            }

            return PLUGIN_STOP;
        }*/
        if(FStrEq(pcmd, "+copyent_basic"))
        {
            if(IsAdmin(1024, "copyent_basic"))
            {
                if(!entMoving && !entRotating)
                {
                    CBaseEntity *pHit = FindEntityForward(MASK_NPCSOLID);
                    if(pHit)
                    {
                        if(!VFuncs::IsPlayer(pHit))
                        {
                            if((GetSpawnLimit() == -1 || spawnedEnts.Count() < GetSpawnLimit()) && (spawnlimit_server.GetInt() == -1 || pAdminOP.spawnedServerEnts.Count() < spawnlimit_server.GetInt()))
                            {
                                CBaseEntity *pProp = CreateEntityByName( VFuncs::GetClassname(pHit) );
                                if(pProp)
                                {
                                    const char *modelname = STRING(VFuncs::GetModelName(pHit));
                                    Vector vecOrigin = VFuncs::GetAbsOrigin(pHit);
                                    QAngle vecAngles = VFuncs::GetLocalAngles(pHit);
                                    VFuncs::KeyValue(pProp,  "angles", UTIL_VarArgs("%f %f %f", vecAngles.x, vecAngles.y, vecAngles.z) );
                                    VFuncs::KeyValue(pProp,  "origin", UTIL_VarArgs("%f %f %f", vecOrigin.x, vecOrigin.y, vecOrigin.z) );
                                    VFuncs::KeyValue(pProp,  "model", modelname );
                                    VFuncs::Spawn(pProp);
                                    VFuncs::Activate(pProp);
                                    VFuncs::Teleport(pProp,  &vecOrigin, &vecAngles, NULL );

                                    spawnedEnts.AddToTail(VFuncs::entindex(pProp));
                                    pAdminOP.spawnedServerEnts.AddToTail(VFuncs::entindex(pProp));

                                    IPhysicsObject *pObj = VFuncs::VPhysicsGetObject(pProp);
                                    if(pObj)
                                        pObj->EnableMotion(0);

                                    entMoving = 1;
                                    pEntMove = servergameents->BaseEntityToEdict(pProp);
                                    origRM = VFuncs::GetRenderMode(pProp);
                                    origRF = VFuncs::GetRenderFX(pProp);
                                    origRC = VFuncs::GetRenderColor(pProp);
                                    origPlayerLoc = VFuncs::GetAbsOrigin(pPlayer);
                                    origEntLoc = VFuncs::GetAbsOrigin(pHit);

                                    if(entmove_changecolor.GetBool())
                                    {
                                        VFuncs::SetRenderColor(pProp, 255,0,255,96);
                                        VFuncs::SetRenderMode(pProp, kRenderTransColor);
                                        VFuncs::SetRenderFX(pProp, kRenderFxNone);
                                    }
                                }
                            }
                            else
                            {
                                LIMIT_MSG();
                            }
                        }
                    }
                }
                else
                {
                    SayTextChatHud(UTIL_VarArgs("[%s] You are currently moving an entity and cannot run +copyent_basic.\n", pAdminOP.adminname));
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command +copyent_basic.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }

            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "+copyent"))
        {
            if(IsAdmin(1024, "copyent"))
            {
                if(!entMoving && !entRotating)
                {
                    CBaseEntity *pHit = FindEntityForward(MASK_NPCSOLID);
                    if(pHit)
                    {
                        if(!VFuncs::IsPlayer(pHit))
                        {
                            if((GetSpawnLimit() == -1 || spawnedEnts.Count() < GetSpawnLimit()) && (spawnlimit_server.GetInt() == -1 || pAdminOP.spawnedServerEnts.Count() < spawnlimit_server.GetInt()))
                            {
                                CBaseEntity *pProp = CreateEntityByName( VFuncs::GetClassname(pHit) );
                                if(pProp)
                                {
                                    Vector vecOrigin = VFuncs::GetAbsOrigin(pHit);
                                    QAngle vecAngles = VFuncs::GetLocalAngles(pHit);
                                    pAdminOP.CopyDataDesc(pProp, pHit);
                                    
                                    // copy SourceOP DataDesc
                                    variant_t value;
                                    value.SetInt(VFuncs::entindex(pHit));
                                    VFuncs::AcceptInput(pProp, "SOPCopyDatadesc", pPlayer, pPlayer, value, 0);

                                    VFuncs::Spawn(pProp);
                                    VFuncs::Activate(pProp);

                                    // sentrygun testing
                                    /*memcpy(((char *)pProp)+220, ((char *)pHit)+220, 4);
                                    memcpy(((char *)pProp)+2224, ((char *)pHit)+2224, 1);
                                    memcpy(((char *)pProp)+2196, ((char *)pHit)+2196, 4);
                                    memcpy(((char *)pProp)+2206, ((char *)pHit)+2206, 1);
                                    memcpy(((char *)pProp)+2205, ((char *)pHit)+2205, 1);
                                    memcpy(((char *)pProp)+2216, ((char *)pHit)+2216, 4);
                                    memcpy(((char *)pProp)+1972, ((char *)pHit)+1972, 4);
                                    memcpy(((char *)pProp)+2016, ((char *)pHit)+2016, 4);
                                    memcpy(((char *)pProp)+2204, ((char *)pHit)+2204, 1);
                                    memcpy(((char *)pProp)+1976, ((char *)pHit)+1976, 4);
                                    memcpy(((char *)pProp)+1992, ((char *)pHit)+1992, 12);
                                    memcpy(((char *)pProp)+2004, ((char *)pHit)+2004, 12);
                                    memcpy(((char *)pProp)+2284, ((char *)pHit)+2284, 4);
                                    memcpy(((char *)pProp)+2312, ((char *)pHit)+2312, 1);
                                    memcpy(((char *)pProp)+2324, ((char *)pHit)+2324, 4);
                                    memcpy(((char *)pProp)+2384, ((char *)pHit)+2384, 4);
                                    memcpy(((char *)pProp)+2392, ((char *)pHit)+2392, 4);
                                    memcpy(((char *)pProp)+2316, ((char *)pHit)+2316, 4);
                                    memcpy(((char *)pProp)+2376, ((char *)pHit)+2376, 4);*/

                                    pAdminOP.CopyDataDesc(pProp, pHit);
                                    VFuncs::Teleport(pProp,  &vecOrigin, &vecAngles, NULL );

                                    spawnedEnts.AddToTail(VFuncs::entindex(pProp));
                                    pAdminOP.spawnedServerEnts.AddToTail(VFuncs::entindex(pProp));

                                    IPhysicsObject *pObj = VFuncs::VPhysicsGetObject(pProp);
                                    if(pObj)
                                        pObj->EnableMotion(0);

                                    entMoving = 1;
                                    pEntMove = servergameents->BaseEntityToEdict(pProp);
                                    origRM = VFuncs::GetRenderMode(pProp);
                                    origRF = VFuncs::GetRenderFX(pProp);
                                    origRC = VFuncs::GetRenderColor(pProp);
                                    origPlayerLoc = VFuncs::GetAbsOrigin(pPlayer);
                                    origEntLoc = VFuncs::GetAbsOrigin(pHit);

                                    if(entmove_changecolor.GetBool())
                                    {
                                        VFuncs::SetRenderColor(pProp, 255,0,255,96);
                                        VFuncs::SetRenderMode(pProp, kRenderTransColor);
                                        VFuncs::SetRenderFX(pProp, kRenderFxNone);
                                    }
                                }
                            }
                            else
                            {
                                LIMIT_MSG();
                            }
                        }
                    }
                }
                else
                {
                    SayTextChatHud(UTIL_VarArgs("[%s] You are currently moving an entity and cannot run +copyent.\n", pAdminOP.adminname));
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command +copyent.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }

            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "-copyent_basic") || FStrEq(pcmd, "-copyent_kv") || FStrEq(pcmd, "-copyent"))
        {
            if(entMoving)
            {
                if(pEntMove)
                {
                    CBaseEntity *pMoving = CBaseEntity::Instance(pEntMove);
                    if(pMoving)
                    {
                        VFuncs::SetRenderColor(pMoving, origRC.r,origRC.g,origRC.b,origRC.a);
                        VFuncs::SetRenderMode(pMoving, origRM);
                        VFuncs::SetRenderFX(pMoving, origRF);

                        IPhysicsObject *pObj = VFuncs::VPhysicsGetObject(pMoving);
                        if(pObj)
                        {
                            pObj->EnableMotion(1);
                            pObj->Sleep();
                        }
                    }
                }
                entMoving = 0;
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "+rotate"))
        {
            if(IsAdmin(1024, "rotate"))
            {
                if(!entMoving)
                {
                    if(!entRotating)
                    {
                        CBaseEntity *pHit = FindEntityForward();
                        if(pHit)
                        {
                            pAdminOP.TimeLog("adminlog.log", "[%s;%s] used command %s %s on entity %s %i\n", engine->GetPlayerNetworkIDString(pEntity), playerinfo->GetName(), command_prefix.GetString(), pcmd, args.ArgS(), VFuncs::GetClassname(pHit), VFuncs::entindex(pHit));
                            entRotating = true;
                            pEntMove = servergameents->BaseEntityToEdict(pHit);
                            origEntAngle = VFuncs::GetLocalAngles(pHit);
                            origPlayerAngle = VFuncs::EyeAngles(pPlayer);

                            IPhysicsObject *pObj = VFuncs::VPhysicsGetObject(pHit);
                            if(pObj)
                                pObj->EnableMotion(0);
                        }
                    }
                }
                else
                {
                    SayTextChatHud(UTIL_VarArgs("[%s] You are currently moving an entity and cannot run +rotate.\n", pAdminOP.adminname));
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command +rotate.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }

            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "-rotate"))
        {
            if(entRotating)
            {
                entRotating = 0;

                CBaseEntity *pBaseMove = CBaseEntity::Instance(pEntMove);
                if(pBaseMove)
                {
                    IPhysicsObject *pObj = VFuncs::VPhysicsGetObject(pBaseMove);
                    if(pObj)
                    {
                        pObj->EnableMotion(1);
                        pObj->Sleep();
                    }
                }
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_rotate"))
        {
            if(IsAdmin(1024, "e_rotate"))
            {
                CBaseEntity *pHit = FindEntityForward();
                if(pHit)
                {
                    QAngle prevangles = VFuncs::GetLocalAngles(pHit);
                    QAngle newangles = QAngle(prevangles.x + atof(args.Arg(1)), prevangles.y + atof(args.Arg(2)), prevangles.z + atof(args.Arg(3)));
                    VFuncs::Teleport(pHit, NULL, &newangles, NULL);
                    pAdminOP.TimeLog("adminlog.log", "[%s;%s] used command %s %s on entity %s %i\n", engine->GetPlayerNetworkIDString(pEntity), playerinfo->GetName(), command_prefix.GetString(), pcmd, args.ArgS(), VFuncs::GetClassname(pHit), VFuncs::entindex(pHit));
                }
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_freeze"))
        {
            if(IsAdmin(1024, "e_freeze"))
            {
                if(!entMoving )
                {
                    CBaseEntity *pHit = NULL;
                    if(args.ArgC() > 1)
                    {
                        edict_t *pEntity = pAdminOP.GetEntityList()+atoi(args.Arg(1));
                        if(pEntity)
                        {
                            pHit = CBaseEntity::Instance(pEntity);
                        }
                    }
                    else
                    {
                        pHit = FindEntityForward();
                    }
                    if(pHit)
                    {
                        if(!VFuncs::IsPlayer(pHit) && Q_strncasecmp(VFuncs::GetClassname(pHit), "prop_vehicle", 12))
                        {
                            pAdminOP.TimeLog("adminlog.log", "[%s;%s] used command %s %s on entity %s %i\n", engine->GetPlayerNetworkIDString(pEntity), playerinfo->GetName(), command_prefix.GetString(), pcmd, args.ArgS(), VFuncs::GetClassname(pHit), VFuncs::entindex(pHit));
                            SayTextChatHud(UTIL_VarArgs("[%s] Motion disabled on ent %i\n", pAdminOP.adminname, VFuncs::entindex(pHit)));
                            IPhysicsObject *pObj = VFuncs::VPhysicsGetObject(pHit);
                            if(pObj)
                                pObj->EnableMotion(0);
                        }
                    }
                }
                else
                {
                    SayTextChatHud(UTIL_VarArgs("[%s] You are currently moving an entity and cannot run e_freeze.\n", pAdminOP.adminname));
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_freeze.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_unfreeze"))
        {
            if(IsAdmin(1024, "e_unfreeze"))
            {
                if(!entMoving)
                {
                    CBaseEntity *pHit = NULL;
                    if(args.ArgC() > 1)
                    {
                        edict_t *pEntity = pAdminOP.GetEntityList()+atoi(args.Arg(1));
                        if(pEntity)
                        {
                            pHit = CBaseEntity::Instance(pEntity);
                        }
                    }
                    else
                    {
                        pHit = FindEntityForward();
                    }
                    if(pHit)
                    {
                        if(!VFuncs::IsPlayer(pHit))
                        {
                            pAdminOP.TimeLog("adminlog.log", "[%s;%s] used command %s %s on entity %s %i\n", engine->GetPlayerNetworkIDString(pEntity), playerinfo->GetName(), command_prefix.GetString(), pcmd, args.ArgS(), VFuncs::GetClassname(pHit), VFuncs::entindex(pHit));
                            SayTextChatHud(UTIL_VarArgs("[%s] Motion enabled on ent %i\n", pAdminOP.adminname, VFuncs::entindex(pHit)));
                            IPhysicsObject *pObj = VFuncs::VPhysicsGetObject(pHit);
                            if(pObj)
                                pObj->EnableMotion(1);
                        }
                    }
                }
                else
                {
                    SayTextChatHud(UTIL_VarArgs("[%s] You are currently moving an entity and cannot run e_unfreeze.\n", pAdminOP.adminname));
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_unfreeze.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_drop"))
        {
            if(IsAdmin(1024, "e_drop"))
            {
                if(!entMoving)
                {
                    CBaseEntity *pHit = FindEntityForward();
                    if(pHit)
                    {
                        pAdminOP.TimeLog("adminlog.log", "[%s;%s] used command %s %s on entity %s %i\n", engine->GetPlayerNetworkIDString(pEntity), playerinfo->GetName(), command_prefix.GetString(), pcmd, args.ArgS(), VFuncs::GetClassname(pHit), VFuncs::entindex(pHit));
                        UTIL_DropToFloor(pHit, MASK_SOLID);
                    }
                }
                else
                {
                    SayTextChatHud(UTIL_VarArgs("[%s] You are currently moving an entity and cannot run e_drop.\n", pAdminOP.adminname));
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_drop.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_stack"))
        {
            if(IsAdmin(1024, "e_stack"))
            {
                if(!entMoving)
                {
                    CBaseEntity *pHit = FindEntityForward();
                    if(pHit)
                    {
                        if(!VFuncs::IsPlayer(pHit) && Q_strncasecmp(VFuncs::GetClassname(pHit), "prop_vehicle", 12))
                        {
                            if(args.ArgC() >= 2)
                            {
                                bool hitLimit = false;
                                for(int i = 1; i <= atoi(args.Arg(1)); i++)
                                {
                                    if((GetSpawnLimit() == -1 || spawnedEnts.Count() < GetSpawnLimit()) && (spawnlimit_server.GetInt() == -1 || pAdminOP.spawnedServerEnts.Count() < spawnlimit_server.GetInt()))
                                    {
                                        CBaseEntity *pProp = CreateEntityByName( VFuncs::GetClassname(pHit) );
                                        if(pProp)
                                        {
                                            const char *modelname = STRING(VFuncs::GetModelName(pHit));
                                            Vector vecOrigin = VFuncs::GetAbsOrigin(pHit) + Vector(i*atof(args.Arg(2)), i*atof(args.Arg(3)), i*atof(args.Arg(4)));
                                            //Msg("%f %f %f\n", vecOrigin.x, vecOrigin.y, vecOrigin.z);
                                            QAngle vecAngles = VFuncs::GetLocalAngles(pHit);
                                            VFuncs::KeyValue(pProp,  "angles", UTIL_VarArgs("%f %f %f", vecAngles.x, vecAngles.y, vecAngles.z) );
                                            VFuncs::KeyValue(pProp,  "origin", UTIL_VarArgs("%f %f %f", vecOrigin.x, vecOrigin.y, vecOrigin.z) );
                                            VFuncs::KeyValue(pProp,  "model", modelname );
                                            VFuncs::Spawn(pProp);
                                            VFuncs::Activate(pProp);
                                            VFuncs::Teleport(pProp,  &vecOrigin, &vecAngles, NULL );

                                            if(atoi(args.Arg(5)))
                                            {
                                                IPhysicsObject *pObj = VFuncs::VPhysicsGetObject(pProp);
                                                if(pObj)
                                                    pObj->EnableMotion(0);
                                            }

                                            spawnedEnts.AddToTail(VFuncs::entindex(pProp));
                                            pAdminOP.spawnedServerEnts.AddToTail(VFuncs::entindex(pProp));
                                        }
                                    }
                                    else
                                    {
                                        hitLimit = true;
                                        break;
                                    }
                                }
                                if(hitLimit)
                                {
                                    LIMIT_MSG();
                                }
                            }
                            else
                            {
                                SayTextChatHud(UTIL_VarArgs("[%s] Incorrect amount of parameters for e_stack. At least 1 expected for amount.\n", pAdminOP.adminname));
                                SayTextChatHud(UTIL_VarArgs("[%s] Usage: e_stack <amt> <x offset> <y offset> <z offset>\n", pAdminOP.adminname));
                            }
                        }
                    }
                }
                else
                {
                    SayTextChatHud(UTIL_VarArgs("[%s] You are currently moving an entity and cannot run e_stack.\n", pAdminOP.adminname));
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_stack.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_setent"))
        {
            if(IsAdmin(1024, "e_setent"))
            {
                Vector forward, right, up;
                QAngle playerAngles = VFuncs::EyeAngles(pPlayer);

                AngleVectors( playerAngles, &forward, &right, &up );

                float distance = 8192;
                Vector start = EyePosition();
                Vector end = EyePosition() + ( forward * distance );

                trace_t tr;
                CTraceFilterSimple traceFilter( pPlayer, COLLISION_GROUP_NONE );
                Ray_t ray;
                ray.Init( start, end );
                enginetrace->TraceRay( ray, MASK_NPCSOLID, &traceFilter, &tr );

                if(tr.DidHit() && tr.m_pEnt)
                {
                    if(VFuncs::entindex(tr.m_pEnt) != 0)
                    {
                        SayTextChatHud(UTIL_VarArgs("[%s] Temporary entity set to ent %i\n", pAdminOP.adminname, VFuncs::entindex(tr.m_pEnt)));
                        pTemp = tr.m_pEnt;
                    }
                    else
                    {
                        pTemp = NULL;
                        SayTextChatHud(UTIL_VarArgs("[%s] No entity found under crosshair. Your temporary entity has been unset.\n", pAdminOP.adminname));
                    }
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_setent.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_addconstraint"))
        {
            if(IsAdmin(1024, "e_addconstraint"))
            {
                Vector forward, right, up;
                QAngle playerAngles = VFuncs::EyeAngles(pPlayer);

                AngleVectors( playerAngles, &forward, &right, &up );

                float distance = 8192;
                Vector start = EyePosition();
                Vector end = EyePosition() + ( forward * distance );

                trace_t tr;
                CTraceFilterSimple traceFilter( pPlayer, COLLISION_GROUP_NONE );
                Ray_t ray;
                ray.Init( start, end );
                enginetrace->TraceRay( ray, MASK_NPCSOLID, &traceFilter, &tr );

                if(pTemp)
                {
                    if(tr.DidHit() && tr.m_pEnt)
                    {
                        if(VFuncs::entindex(tr.m_pEnt) != 0)
                        {
                            if(VFuncs::entindex(tr.m_pEnt) != VFuncs::entindex(pTemp))
                            {
                                IPhysicsConstraint *pConstraint;
                                CBaseEntity *pSet = tr.m_pEnt;
                                pConstraint = MakeConstraint(pSet, pTemp);
                                if(pConstraint)
                                {
                                    physConstraints.AddToTail(pConstraint);
                                    SayTextChatHud(UTIL_VarArgs("[%s] Constraint added between ent %i and ent %i with ID %i\n", pAdminOP.adminname, VFuncs::entindex(pSet), VFuncs::entindex(pTemp), physConstraints.Tail()));
                                }
                                else
                                {
                                    SayTextChatHud(UTIL_VarArgs("[%s] Unable to add constraint.\n", pAdminOP.adminname));
                                }
                                //VFuncs::SetParent(pSet, pTemp);
                            }
                            else
                            {
                                SayTextChatHud(UTIL_VarArgs("[%s] Both entities are the same. Unable to add constraint.\n", pAdminOP.adminname));
                            }
                        }
                        else
                        {
                            SayTextChatHud(UTIL_VarArgs("[%s] Unable to add constraint because no entity was detected under crosshair.\n", pAdminOP.adminname));
                        }
                    }
                    else
                    {
                        SayTextChatHud(UTIL_VarArgs("[%s] Unable to add constraint because no entity was detected under crosshair.\n", pAdminOP.adminname));
                    }
                }
                else
                {
                    SayTextChatHud(UTIL_VarArgs("[%s] Unable to add constraint because no temporary entity was set. Use e_setent first to set the first entity in the constraint.\n", pAdminOP.adminname));
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_addconstraint.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_hinge"))
        {
            if(IsAdmin(1024, "e_hinge"))
            {
                Vector forward, right, up;
                QAngle playerAngles = VFuncs::EyeAngles(pPlayer);

                AngleVectors( playerAngles, &forward, &right, &up );

                float distance = 8192;
                Vector start = EyePosition();
                Vector end = EyePosition() + ( forward * distance );

                trace_t tr;
                CTraceFilterSimple traceFilter( pPlayer, COLLISION_GROUP_NONE );
                Ray_t ray;
                ray.Init( start, end );
                enginetrace->TraceRay( ray, MASK_NPCSOLID, &traceFilter, &tr );

                if(pTemp)
                {
                    if(tr.DidHit() && tr.m_pEnt)
                    {
                        if(VFuncs::entindex(tr.m_pEnt) != 0)
                        {
                            if(VFuncs::entindex(tr.m_pEnt) != VFuncs::entindex(pTemp))
                            {
                                IPhysicsConstraint *pConstraint;
                                CBaseEntity *pSet = tr.m_pEnt;

                                /////////////////////////
                                IPhysicsConstraint  *m_pConstraint;
                                IPhysicsObject *pReference = VFuncs::VPhysicsGetObject(pSet);
                                /*if ( GetMoveParent() )
                                {
                                    pReference = GetMoveParentVFuncs::VPhysicsGetObject(());
                                }*/
                                IPhysicsObject *pAttached = VFuncs::VPhysicsGetObject(pTemp);
                                if ( !pReference || !pAttached )
                                {
                                    return PLUGIN_STOP;
                                }

                                constraint_hingeparams_t hinged;
                                hinged.Defaults();
                                hinged.worldPosition = tr.endpos;
                                hinged.worldAxisDirection = tr.plane.normal;

                                //fixed.InitWithCurrentObjectState( pReference, pAttached );

                                m_pConstraint = physenv->CreateHingeConstraint( pReference, pAttached, NULL, hinged );
                                ////////////////////////////

                                pConstraint = m_pConstraint;
                                if(pConstraint)
                                {
                                    physConstraints.AddToTail(pConstraint);
                                    SayTextChatHud(UTIL_VarArgs("[%s] Constraint added between ent %i and ent %i with ID %i\n", pAdminOP.adminname, VFuncs::entindex(pSet), VFuncs::entindex(pTemp), physConstraints.Tail()));
                                }
                                else
                                {
                                    SayTextChatHud(UTIL_VarArgs("[%s] Unable to add constraint.\n", pAdminOP.adminname));
                                }
                            }
                            else
                            {
                                SayTextChatHud(UTIL_VarArgs("[%s] Both entities are the same. Unable to add constraint.\n", pAdminOP.adminname));
                            }
                        }
                        else
                        {
                            SayTextChatHud(UTIL_VarArgs("[%s] Unable to add constraint because no entity was detected under crosshair.\n", pAdminOP.adminname));
                        }
                    }
                    else
                    {
                        SayTextChatHud(UTIL_VarArgs("[%s] Unable to add constraint because no entity was detected under crosshair.\n", pAdminOP.adminname));
                    }
                }
                else
                {
                    SayTextChatHud(UTIL_VarArgs("[%s] Unable to add constraint because no temporary entity was set. Use e_setent first to set the first entity in the constraint.\n", pAdminOP.adminname));
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_hinge.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_rope"))
        {
            if(IsAdmin(1024, "e_rope"))
            {
                Vector forward, right, up;
                QAngle playerAngles = VFuncs::EyeAngles(pPlayer);

                AngleVectors( playerAngles, &forward, &right, &up );

                float distance = 8192;
                Vector start = EyePosition();
                Vector end = EyePosition() + ( forward * distance );

                trace_t tr;
                CTraceFilterSimple traceFilter( pPlayer, COLLISION_GROUP_NONE );
                Ray_t ray;
                ray.Init( start, end );
                enginetrace->TraceRay( ray, MASK_NPCSOLID, &traceFilter, &tr );

                if(pTemp)
                {
                    if(tr.DidHit() && tr.m_pEnt)
                    {
                        if(VFuncs::entindex(tr.m_pEnt) != 0)
                        {
                            if(VFuncs::entindex(tr.m_pEnt) != VFuncs::entindex(pTemp))
                            {
                                IPhysicsConstraint *pConstraint;
                                CBaseEntity *pSet = tr.m_pEnt;

                                /////////////////////////
                                IPhysicsConstraint  *m_pConstraint;
                                IPhysicsObject *pReference = VFuncs::VPhysicsGetObject(pSet);
                                /*if ( GetMoveParent() )
                                {
                                    pReference = GetMoveParentVFuncs::VPhysicsGetObject(());
                                }*/
                                IPhysicsObject *pAttached = VFuncs::VPhysicsGetObject(pTemp);
                                if ( !pReference || !pAttached )
                                {
                                    return PLUGIN_STOP;
                                }

                                constraint_lengthparams_t lengthparams;
                                lengthparams.Defaults();
                                lengthparams.InitWorldspace( pReference, pAttached, tr.endpos, VFuncs::GetAbsOrigin(pTemp));

                                m_pConstraint = physenv->CreateLengthConstraint( pReference, pAttached, NULL, lengthparams );
                                ////////////////////////////

                                static int ropeNum = 1;
                                Vector difference = tr.endpos - VFuncs::GetAbsOrigin(pTemp);
                                CBaseEntity *pRope2 = CreateEntityByName( "keyframe_rope" );
                                if(pRope2)
                                {
                                    QAngle angles;
                                    VectorAngles(tr.plane.normal, angles);

                                    VFuncs::SetAbsOrigin(pRope2, tr.endpos);
                                    VFuncs::SetParent(pRope2, pSet);
                                    VFuncs::KeyValue(pRope2, "targetname", UTIL_VarArgs("sop_rope%i", ropeNum) );
                                    VFuncs::Spawn(pRope2);
                                    VFuncs::Activate(pRope2);
                                }
                                CBaseEntity *pRope1 = CreateEntityByName( "move_rope" );
                                if(pRope1)
                                {
                                    QAngle angles;
                                    VectorAngles(tr.plane.normal, angles);

                                    VFuncs::SetAbsOrigin(pRope1, VFuncs::GetAbsOrigin(pTemp));
                                    VFuncs::SetParent(pRope1, pTemp);
                                    VFuncs::KeyValue(pRope1, "nextkey", UTIL_VarArgs("sop_rope%i", ropeNum) );
                                    // ropes will get stuck around corners and stuff with collide on
                                    ////VFuncs::KeyValue(pRope1, "collide", "1" );
                                    VFuncs::KeyValue(pRope1, "slack", UTIL_VarArgs("%i", (int)difference.Length() / 10) );
                                    VFuncs::Spawn(pRope1);
                                    VFuncs::Activate(pRope1);
                                }
                                ropeNum++;

                                pConstraint = m_pConstraint;
                                if(pConstraint)
                                {
                                    physConstraints.AddToTail(pConstraint);
                                    SayTextChatHud(UTIL_VarArgs("[%s] Constraint added between ent %i and ent %i with ID %i\n", pAdminOP.adminname, VFuncs::entindex(pSet), VFuncs::entindex(pTemp), physConstraints.Tail()));
                                }
                                else
                                {
                                    SayTextChatHud(UTIL_VarArgs("[%s] Unable to add constraint.\n", pAdminOP.adminname));
                                }
                            }
                            else
                            {
                                SayTextChatHud(UTIL_VarArgs("[%s] Both entities are the same. Unable to add constraint.\n", pAdminOP.adminname));
                            }
                        }
                        else
                        {
                            SayTextChatHud(UTIL_VarArgs("[%s] Unable to add constraint because no entity was detected under crosshair.\n", pAdminOP.adminname));
                        }
                    }
                    else
                    {
                        SayTextChatHud(UTIL_VarArgs("[%s] Unable to add constraint because no entity was detected under crosshair.\n", pAdminOP.adminname));
                    }
                }
                else
                {
                    SayTextChatHud(UTIL_VarArgs("[%s] Unable to add constraint because no temporary entity was set. Use e_setent first to set the first entity in the constraint.\n", pAdminOP.adminname));
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_hinge.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_delconstraints"))
        {
            if(IsAdmin(1024, "e_delconstraint"))
            {
                int delCount = 0;
                IPhysicsConstraint *pConstraint;
                for ( unsigned short i=physConstraints.Head(); i != physConstraints.InvalidIndex(); i = physConstraints.Next( i ) )
                {
                    pConstraint = physConstraints.Element(i);
                    if(pConstraint)
                    {
                        delCount++;
                        physenv->DestroyConstraint(pConstraint);
                    }
                }
                if(delCount)
                {
                    SayTextChatHud(UTIL_VarArgs("[%s] Successfully destroyed %i constraint%s.\n", pAdminOP.adminname, delCount, delCount != 1 ? "s" : ""));
                }
                else
                {
                    SayTextChatHud(UTIL_VarArgs("[%s] No constraints are available to delete.\n", pAdminOP.adminname));
                }
                physConstraints.Purge();
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_delconstraints.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_delconstraint"))
        {
            if(IsAdmin(1024, "e_delconstraint"))
            {
                unsigned short index = atoi(args.Arg(1));
                if(physConstraints.Count())
                {
                    bool didSomething = false;
                    for ( unsigned short i=physConstraints.Head(); i != physConstraints.InvalidIndex(); i = physConstraints.Next( i ) )
                    {
                        if( index == i )
                        {
                            IPhysicsConstraint *pConstraint = physConstraints.Element(atoi(args.Arg(1)));
                            if(pConstraint)
                                physenv->DestroyConstraint(pConstraint);

                            physConstraints.Remove(index);
                            didSomething = true;

                            break;
                        }
                    }
                    if(didSomething == false)
                    {
                        SayTextChatHud(UTIL_VarArgs("[%s] An invalid constraint index was specified.\n", pAdminOP.adminname));
                    }
                }
                else
                {
                    SayTextChatHud(UTIL_VarArgs("[%s] No constraints are available to delete.\n",  pAdminOP.adminname));
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_delconstraint.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_wheel"))
        {
            if(IsAdmin(1024, "e_wheel"))
            {
                Vector forward, right, up;
                QAngle playerAngles = VFuncs::EyeAngles(pPlayer);

                AngleVectors( playerAngles, &forward, &right, &up );

                float distance = 8192;
                Vector start = EyePosition();
                Vector end = EyePosition() + ( forward * distance );

                trace_t tr;
                CTraceFilterSimple traceFilter( pPlayer, COLLISION_GROUP_NONE );
                Ray_t ray;
                ray.Init( start, end );
                enginetrace->TraceRay( ray, MASK_NPCSOLID, &traceFilter, &tr );

                if(tr.DidHit() && tr.m_pEnt && VFuncs::entindex(tr.m_pEnt) != 0)
                {
                    IPhysicsConstraint *pConstraint;
                    CBaseEntity *pSet = tr.m_pEnt;
                    Vector mins, maxs;
                    int wheel;
                    Vector vecSize(0,0,0);
                    if(FStrEq(args.Arg(1), "car") || FStrEq(args.Arg(1), "0")) wheel = 0;
                    else if(FStrEq(args.Arg(1), "apc") || FStrEq(args.Arg(1), "1")) wheel = 1;
                    else if(FStrEq(args.Arg(1), "tractor") || FStrEq(args.Arg(1), "2")) wheel = 2;
                    else if(FStrEq(args.Arg(1), "cart") || FStrEq(args.Arg(1), "3")) wheel = 3;
                    else if(FStrEq(args.Arg(1), "cart_giant") || FStrEq(args.Arg(1), "4")) wheel = 4;
                    else
                    {
                        SayTextChatHud(UTIL_VarArgs("[%s] Invalid wheel name. For valid wheel names, say !help.\n", pAdminOP.adminname));
                        return PLUGIN_STOP;
                    }

                    ////MAKE WHEEL////
                    CBaseEntity *pProp = NULL;
                    pProp = CreateEntityByName( "prop_physics_multiplayer" );

                    if(!pProp)
                    {
                        SayTextChatHud(UTIL_VarArgs("[%s] Spawn failure.\n", pAdminOP.adminname));
                        return PLUGIN_STOP;
                    }

                    QAngle angles;
                    VectorAngles(tr.plane.normal, angles);

                    VFuncs::KeyValue(pProp,  "model", szTires[wheel] );
                    VFuncs::KeyValue(pProp,  "spawnflags", "256" );
                    VFuncs::KeyValue(pProp,  "physdamagescale", "0.1" );
                    VFuncs::KeyValue(pProp,  "ExplodeDamage", "0" );
                    VFuncs::KeyValue(pProp,  "ExplodeRadius", "0" );
                    VFuncs::Spawn(pProp);
                    VFuncs::Activate(pProp);
                    VFuncs::SetModel( pProp, szTires[wheel] );
                    //////////////////

                    ////GET SIZE////
                    int i = modelinfo->GetModelIndex( szTires[wheel] );
                    if ( i < 0 )
                    {
                        SayTextChatHud(UTIL_VarArgs("[%s] Wheel not precached.\n", pAdminOP.adminname));
                        return PLUGIN_STOP;
                    }
                    const model_t *mod = modelinfo->GetModel( i );
                    if ( !mod )
                    {
                        SayTextChatHud(UTIL_VarArgs("[%s] Wheel has no model_t. This is an engine error.\n", pAdminOP.adminname));
                        return PLUGIN_STOP;
                    }
                    modelinfo->GetModelBounds( mod, mins, maxs );
                    VectorSubtract( maxs, mins, vecSize );
                    ////////////////

                    if(wheel < 3)
                    {
                        VFuncs::Teleport(pProp, &(tr.endpos + (tr.plane.normal * (vecSize.x/2))), &angles, NULL );
                    }
                    else
                    {
                        matrix3x4_t     m_rgflCoordinateFrame;
                        Vector vecorigin = tr.endpos + tr.plane.normal * (maxs.y - (mins.y + maxs.y));
                        VFuncs::Teleport(pProp, &vecorigin, &angles, NULL );
                        /////////////ROTATE WHEELS THAT FACE WRONG DIRECTION/////////
                        const float rotationAngle = -90;

                        // calculate coordinate frame so that we don't have to use EntityToWorldTransform()
                        AngleMatrix( angles, vecorigin, m_rgflCoordinateFrame );

                        // compute rotation axis in entity local space
                        // compute the transform as a matrix so we can concatenate it with the entity's current transform
                        Vector rotationAxisLs = Vector(0,0,1);

                        // build a transform that rotates around that axis in local space by the angle
                        // if there were an AxisAngleMatrix() routine we could use that directly, but there isn't 
                        // so convert to a quaternion first, then a matrix
                        Quaternion q;

                        // NOTE: assumes axis is a unit vector, non-unit vectors will bias the resulting rotation angle (but not the axis)
                        AxisAngleQuaternion( rotationAxisLs, rotationAngle, q );
                        
                        // convert to a matrix
                        matrix3x4_t xform;
                        QuaternionMatrix( q, vec3_origin, xform );
                        
                        // apply the rotation to the entity input space (local)
                        matrix3x4_t localToWorldMatrix;
                        ConcatTransforms( m_rgflCoordinateFrame, xform, localToWorldMatrix );

                        // extract the compound rotation as a QAngle
                        QAngle localAngles;
                        MatrixAngles( localToWorldMatrix, localAngles );
                        /////////END ROTATE WHEELS THAT FACE WRONG DIRECTION/////////

                        VFuncs::SetAbsAngles(pProp, localAngles);
                    }

                    spawnedEnts.AddToTail(VFuncs::entindex(pProp));
                    pAdminOP.spawnedServerEnts.AddToTail(VFuncs::entindex(pProp));
                    //////////////////

                    //////ADD HINGE//////////
                    IPhysicsConstraint  *m_pConstraint;
                    IPhysicsObject *pReference = VFuncs::VPhysicsGetObject(pSet);
                    IPhysicsObject *pAttached = VFuncs::VPhysicsGetObject(pProp);
                    if ( !pReference || !pAttached )
                    {
                        return PLUGIN_STOP;
                    }

                    constraint_hingeparams_t hinged;
                    hinged.Defaults();
                    hinged.worldPosition = tr.endpos;
                    hinged.worldAxisDirection = tr.plane.normal;

                    m_pConstraint = physenv->CreateHingeConstraint( pReference, pAttached, NULL, hinged );
                    ////////////////////////////

                    pConstraint = m_pConstraint;
                    if(pConstraint)
                    {
                        physConstraints.AddToTail(pConstraint);
                        SayTextChatHud(UTIL_VarArgs("[%s] Wheel added to ent %i. New ent = %i with constraint ID %i\n", pAdminOP.adminname, VFuncs::entindex(pSet), VFuncs::entindex(pProp), physConstraints.Tail()));
                    }
                    else
                    {
                        SayTextChatHud(UTIL_VarArgs("[%s] Unable to add constraint.\n", pAdminOP.adminname));
                    }
                }
                else
                {
                    SayTextChatHud(UTIL_VarArgs("[%s] Unable to add constraint because no entity was detected under crosshair.\n", pAdminOP.adminname));
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_wheel.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_thruster"))
        {
            if(IsAdmin(1024, "e_thruster"))
            {
                if(args.ArgC() == 3)
                {
                    if(strlen(args.Arg(1)) > THRUSTGRP_LEN-1)
                    {
                        SayTextChatHud(UTIL_VarArgs("[%s] Group name too long.\n", pAdminOP.adminname));
                        return PLUGIN_STOP;
                    }
                    if(FStrEq(args.Arg(1), "all"))
                    {
                        SayTextChatHud(UTIL_VarArgs("[%s] Can't use reserved group name \"all\".\n", pAdminOP.adminname));
                        return PLUGIN_STOP;
                    }
                    Vector forward, right, up;
                    QAngle playerAngles = VFuncs::EyeAngles(pPlayer);

                    AngleVectors( playerAngles, &forward, &right, &up );

                    float distance = 8192;
                    Vector start = EyePosition();
                    Vector end = EyePosition() + ( forward * distance );

                    trace_t tr;
                    CTraceFilterSimple traceFilter( pPlayer, COLLISION_GROUP_NONE );
                    Ray_t ray;
                    ray.Init( start, end );
                    enginetrace->TraceRay( ray, MASK_NPCSOLID, &traceFilter, &tr );

                    if(tr.DidHit() && tr.m_pEnt && VFuncs::entindex(tr.m_pEnt) != 0)
                    {
                        CBaseEntity *pProp = NULL;
                        pProp = CreateEntityByName( "prop_physics" );

                        if(!pProp)
                        {
                            SayTextChatHud(UTIL_VarArgs("[%s] Spawn failure.\n", pAdminOP.adminname));
                            return PLUGIN_STOP;
                        }

                        Vector vecOrigin = tr.endpos + tr.plane.normal * 0;
                        QAngle angles;
                        VectorAngles(tr.plane.normal, angles);

                        const float rotationAngle = 90;
                        matrix3x4_t     m_rgflCoordinateFrame;
                        AngleMatrix( angles, vecOrigin, m_rgflCoordinateFrame );
                        Vector rotationAxisLs = Vector(0,1,0);
                        Quaternion q;
                        AxisAngleQuaternion( rotationAxisLs, rotationAngle, q );
                        matrix3x4_t xform;
                        QuaternionMatrix( q, vec3_origin, xform );
                        matrix3x4_t localToWorldMatrix;
                        ConcatTransforms( m_rgflCoordinateFrame, xform, localToWorldMatrix );
                        QAngle localAngles;
                        MatrixAngles( localToWorldMatrix, localAngles );
                        angles = localAngles;

                        VFuncs::KeyValue(pProp,  "model", "models/props_junk/garbage_metalcan001a.mdl" );
                        VFuncs::KeyValue(pProp,  "origin", UTIL_VarArgs("%f %f %f", vecOrigin.x, vecOrigin.y, vecOrigin.z) ); 
                        VFuncs::KeyValue(pProp,  "angles", UTIL_VarArgs("%f %f %f", angles.x, angles.y, angles.z) ); 
                        VFuncs::KeyValue(pProp,  "spawnflags", "1542" );
                        VFuncs::KeyValue(pProp,  "physdamagescale", "0.1" );
                        VFuncs::KeyValue(pProp,  "disableshadows", "1" );
                        VFuncs::KeyValue(pProp,  "ExplodeDamage", "0" );
                        VFuncs::KeyValue(pProp,  "ExplodeRadius", "0" );
                        VFuncs::KeyValue(pProp,  "nodamageforces", "1" );
                        VFuncs::KeyValue(pProp,  "solid", "0" );
                        VFuncs::Spawn(pProp);
                        VFuncs::Activate(pProp);
                        VFuncs::SetSolid( pProp, SOLID_NONE );
                        UTIL_SetSize( pProp, Vector ( 0, 0, 0 ), Vector ( 0, 0, 0 ) );
                        VFuncs::SetModel( pProp, "models/props_junk/garbage_metalcan001a.mdl" );
                        VFuncs::Teleport(pProp, &vecOrigin, &angles, NULL );
                        //VFuncs::SetParent(pProp, tr.m_pEnt);

                        IPhysicsObject *pObj = VFuncs::VPhysicsGetObject(pProp);
                        if(pObj)
                        {
                            thruster_t thruster;
                            pObj->EnableCollisions(0);
                            pObj->EnableGravity(0);
                            pObj->EnableDrag(0);
                            pObj->SetMass(0.001);
                            thruster.pConstraint = MakeConstraint(tr.m_pEnt, pProp);
                            if(thruster.pConstraint)
                            {
                                int r;
                                thruster.enabled = false;
                                thruster.entattached = tr.m_pEnt;
                                thruster.entthruster = pProp;
                                thruster.force = atoi(args.Arg(2));
                                strncpy(thruster.group, args.Arg(1), 32);
                                r = thrusters.AddToTail(thruster);
                                SayTextChatHud(UTIL_VarArgs("[%s] Thruster added into group %s with thruster ID %i.\n", pAdminOP.adminname, thruster.group, r));
                                pAdminOP.TimeLog("adminlog.log", "[%s;%s] used command %s %s\n", engine->GetPlayerNetworkIDString(pEntity), playerinfo->GetName(), command_prefix.GetString(), pcmd, args.ArgS());
                            }
                        }
                    }
                    else
                    {
                        SayTextChatHud(UTIL_VarArgs("[%s] Unable to add thruster because no entity was detected under crosshair.\n", pAdminOP.adminname));
                    }
                }
                else
                {
                    SayTextChatHud(UTIL_VarArgs("[%s] Invalid usage. Usage: e_thruster <group name> <force>\n", pAdminOP.adminname));
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_thruster.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_delthruster"))
        {
            if(IsAdmin(1024, "e_delthruster"))
            {
                int thrusterid = atoi(args.Arg(1));
                if(thrusters.IsValidIndex(thrusterid))
                {
                    thruster_t *thruster = thrusters.Base() + thrusterid; // fast access
                    if(thruster)
                    {
                        pAdminOP.TimeLog("adminlog.log", "[%s;%s] used command %s %s\n", engine->GetPlayerNetworkIDString(pEntity), playerinfo->GetName(), command_prefix.GetString(), pcmd, args.ArgS());
                        DeleteThruster(thruster);
                        thrusters.Remove(thrusterid);
                        SayTextChatHud(UTIL_VarArgs("[%s] Thruster %i deleted.\n", pAdminOP.adminname, thrusterid));
                    }
                }
                else
                {
                    SayTextChatHud(UTIL_VarArgs("[%s] Invalid thruster ID.\n", pAdminOP.adminname));
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_delthruster.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_delthruster_group") || FStrEq(pcmd, "e_delthruster_grp"))
        {
            if(IsAdmin(1024, "e_delthruster_group"))
            {
                pAdminOP.TimeLog("adminlog.log", "[%s;%s] used command %s %s\n", engine->GetPlayerNetworkIDString(pEntity), playerinfo->GetName(), command_prefix.GetString(), pcmd, args.ArgS());
                if(thrusters.Count())
                {
                    int removed = 0;
                    for(int i = 0; i < thrusters.Count(); i++)
                    {
                        thruster_t *thruster = thrusters.Base() + i; // fast access
                        if(FStrEq(args.Arg(1), thruster->group))
                        {
                            DeleteThruster(thruster);
                            thrusters.Remove(i);
                            i--;
                            removed++;
                        }
                    }
                    if(removed)
                    {
                        SayTextChatHud(UTIL_VarArgs("[%s] Group %s (%i thruster%s) deleted.\n", pAdminOP.adminname, args.Arg(1), removed, removed != 1 ? "s" : ""));
                    }
                    else
                    {
                        SayTextChatHud(UTIL_VarArgs("[%s] You don't have any thrusters in group %s.\n", pAdminOP.adminname, args.Arg(1)));
                    }
                }
                else
                {
                    SayTextChatHud(UTIL_VarArgs("[%s] You don't have any thrusters.\n", pAdminOP.adminname));
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_delthruster.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_delthrusters"))
        {
            if(IsAdmin(1024, "e_delthrusters"))
            {
                pAdminOP.TimeLog("adminlog.log", "[%s;%s] used command %s %s\n", engine->GetPlayerNetworkIDString(pEntity), playerinfo->GetName(), command_prefix.GetString(), pcmd, args.ArgS());
                if(thrusters.Count())
                {
                    for(int i = 0; i < thrusters.Count(); i++)
                    {
                        thruster_t *thruster = thrusters.Base() + i; // fast access
                        DeleteThruster(thruster);
                    }
                    SayTextChatHud(UTIL_VarArgs("[%s] Removed %i thrusters.\n", pAdminOP.adminname, thrusters.Count()));
                    thrusters.Purge();
                }
                else
                {
                    SayTextChatHud(UTIL_VarArgs("[%s] You don't have any thrusters.\n", pAdminOP.adminname));
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_delthrusters.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_spawnprop"))
        {
            if(IsAdmin(1024, "e_spawnprop"))
            {
                if((GetSpawnLimit() == -1 || spawnedEnts.Count() < GetSpawnLimit()) && (spawnlimit_server.GetInt() == -1 || pAdminOP.spawnedServerEnts.Count() < spawnlimit_server.GetInt()))
                {
                    char prop[60];
                    int iSkin = 0;
                    bool isFrozen = 0;

                    Vector forward, right, up;
                    QAngle playerAngles = VFuncs::EyeAngles(pPlayer);
                    AngleVectors( playerAngles, &forward, &right, &up );

                    Vector vecOrigin = EyePosition() + forward*48;
                    QAngle vecAngles( 0, 0, 0 );

                    if(args.ArgC() > 1)
                    {
                        char strArgs[512];
                        Vector forward, right, up;
                        QAngle playerAngles = VFuncs::EyeAngles(pPlayer);
                        AngleVectors( playerAngles, &forward, &right, &up );
                        strncpy(strArgs, args.ArgS() , 512);
                        Vector playerorigin = VFuncs::GetAbsOrigin(pPlayer);

                        char seps[]   = " ,;/\n";
                        char *token;
                        token = strtok( strArgs, seps );
                        while( token != NULL )
                        {
                            if(!stricmp(token, "front") || !stricmp(token, "forward"))
                            {
                                vecOrigin = playerorigin + Vector(forward.x*48,forward.y*48,0);
                            }
                            else if(!stricmp(token, "back") || !stricmp(token, "behind"))
                            {
                                vecOrigin = playerorigin - Vector(forward.x*48,forward.y*48,0);
                            }
                            else if(!stricmp(token, "left"))
                            {
                                vecOrigin = playerorigin - Vector(right.x*48,right.y*48,0);
                            }
                            else if(!stricmp(token, "right"))
                            {
                                vecOrigin = playerorigin + Vector(right.x*48,right.y*48,0);
                            }
                            else if(!stricmp(token, "top") || !stricmp(token, "above") || !stricmp(token, "up"))
                            {
                                vecOrigin = playerorigin + Vector(0,0,128);
                            }
                            else if(!stricmp(token, "bottom") || !stricmp(token, "below") || !stricmp(token, "down"))
                            {
                                vecOrigin = playerorigin - Vector(0,0,128);
                            }
                            else if(!Q_strncasecmp(token, "skin=", 5))
                            {
                                if(stricmp(token, "skin="))
                                {
                                    char szSkin[32];
                                    strncpy(szSkin, &token[5], 32);
                                    szSkin[31] = '\0';
                                    iSkin = atoi(szSkin);
                                }
                            }
                            else if(!stricmp(token, "frozen")|| !stricmp(token, "freeze") || !stricmp(token, "disabled") || !stricmp(token, "disable"))
                            {
                                isFrozen = 1;
                            }
                            else
                            {
                                strncpy(prop, token, sizeof(prop));
                                prop[sizeof(prop)-1] = '\0';
                            }
                            token = strtok( NULL, seps );
                        }
                    }

                    const spawnaliasdata_t *propData = pAdminOP.FindModelFromAlias(prop);

                    if(propData->indexNumber == -1)
                    {
                        SayText(UTIL_VarArgs("[%s] Prop not found.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
                        return PLUGIN_STOP;
                    }

                    CBaseEntity *pProp = NULL;
                    if(propData->spawnMode == 0)
                        pProp = CreateEntityByName( "prop_physics_multiplayer" );
                    else if(propData->spawnMode == 1)
                        pProp = CreateEntityByName( "prop_physics" );
                    else if(propData->spawnMode == 2)
                        pProp = CreateEntityByName( "prop_dynamic" );
                    //pProp = CreateEntityByName(args.Arg(1));

                    if(pProp)
                    {
                        if(propData->model[0] != '\0')
                        {
                            VFuncs::KeyValue(pProp,  "model", propData->model );
                        }
                        else
                        {
                            VFuncs::KeyValue(pProp,  "model", "models/error.mdl" );
                        }

                        if(iSkin >= 0)
                        {
                            VFuncs::KeyValue(pProp,  "skin", UTIL_VarArgs("%i", iSkin) );
                        }

                        VFuncs::KeyValue(pProp,  "spawnflags", "256" );
                        VFuncs::KeyValue(pProp,  "physdamagescale", "0.1" );
                        VFuncs::KeyValue(pProp,  "ExplodeDamage", "0" );
                        VFuncs::KeyValue(pProp,  "ExplodeRadius", "0" );
                        if(propData->spawnMode == 2)
                            pProp->VPhysicsInitNormal( SOLID_BBOX, 0, false );
                        VFuncs::Spawn(pProp);
                        VFuncs::Activate(pProp);
                        VFuncs::SetModel( pProp, propData->model );
                        VFuncs::Teleport(pProp,  &vecOrigin, &vecAngles, NULL );

                        if(VFuncs::GetEFlags(pProp) & EFL_KILLME)
                        {
                            SayText(UTIL_VarArgs("[%s] Spawn failed.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
                        }

                        if(isFrozen)
                        {
                            IPhysicsObject *pObj = VFuncs::VPhysicsGetObject(pProp);
                            if(pObj)
                                pObj->EnableMotion(0);
                        }

                        spawnedEnts.AddToTail(VFuncs::entindex(pProp));
                        pAdminOP.spawnedServerEnts.AddToTail(VFuncs::entindex(pProp));
                        pAdminOP.TimeLog("adminlog.log", "[%s;%s] used command %s %s\n", engine->GetPlayerNetworkIDString(pEntity), playerinfo->GetName(), command_prefix.GetString(), pcmd, args.ArgS());
                    }
                }
                else
                {
                    LIMIT_MSG();
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_spawnprop.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_spawnmodel"))
        {
            if(IsAdmin(1024, "e_spawnmodel"))
            {
                if((GetSpawnLimit() == -1 || spawnedEnts.Count() < GetSpawnLimit()) && (spawnlimit_server.GetInt() == -1 || pAdminOP.spawnedServerEnts.Count() < spawnlimit_server.GetInt()))
                {
                    char prop[60];
                    int iSkin = 0;
                    bool isFrozen = 0;

                    Vector forward, right, up;
                    QAngle playerAngles = VFuncs::EyeAngles(pPlayer);
                    AngleVectors( playerAngles, &forward, &right, &up );

                    Vector vecOrigin = EyePosition() + forward*48;
                    QAngle vecAngles( 0, 0, 0 );

                    if(args.ArgC() > 1)
                    {
                        char strArgs[512];
                        Vector forward, right, up;
                        QAngle playerAngles = VFuncs::EyeAngles(pPlayer);
                        AngleVectors( playerAngles, &forward, &right, &up );
                        strncpy(strArgs, args.ArgS() , 512);
                        Vector playerorigin = VFuncs::GetAbsOrigin(pPlayer);

                        char seps[]   = " ,;/\n";
                        char *token;
                        token = strtok( strArgs, seps );
                        while( token != NULL )
                        {
                            if(!stricmp(token, "front") || !stricmp(token, "forward"))
                            {
                                vecOrigin = playerorigin + Vector(forward.x*48,forward.y*48,0);
                            }
                            else if(!stricmp(token, "back") || !stricmp(token, "behind"))
                            {
                                vecOrigin = playerorigin - Vector(forward.x*48,forward.y*48,0);
                            }
                            else if(!stricmp(token, "left"))
                            {
                                vecOrigin = playerorigin - Vector(right.x*48,right.y*48,0);
                            }
                            else if(!stricmp(token, "right"))
                            {
                                vecOrigin = playerorigin + Vector(right.x*48,right.y*48,0);
                            }
                            else if(!stricmp(token, "top") || !stricmp(token, "above") || !stricmp(token, "up"))
                            {
                                vecOrigin = playerorigin + Vector(0,0,128);
                            }
                            else if(!stricmp(token, "bottom") || !stricmp(token, "below") || !stricmp(token, "down"))
                            {
                                vecOrigin = playerorigin - Vector(0,0,128);
                            }
                            else if(!Q_strncasecmp(token, "skin=", 5))
                            {
                                if(stricmp(token, "skin="))
                                {
                                    char szSkin[32];
                                    strncpy(szSkin, &token[5], 32);
                                    szSkin[31] = '\0';
                                    iSkin = atoi(szSkin);
                                }
                            }
                            else if(!stricmp(token, "frozen")|| !stricmp(token, "freeze") || !stricmp(token, "disabled") || !stricmp(token, "disable"))
                            {
                                isFrozen = 1;
                            }
                            else
                            {
                                strncpy(prop, token, sizeof(prop));
                                prop[sizeof(prop)-1] = '\0';
                            }
                            token = strtok( NULL, seps );
                        }
                    }

                    int spawnMode = atoi(args.Arg(2));
                    CBaseEntity *pProp = NULL;
                    if(spawnMode == 0)
                        pProp = CreateEntityByName( "prop_physics_multiplayer" );
                    else if(spawnMode == 1)
                        pProp = CreateEntityByName( "prop_physics" );
                    else if(spawnMode == 2)
                        pProp = CreateEntityByName( "prop_dynamic" );
                    //pProp = CreateEntityByName(args.Arg(1));

                    if(pProp)
                    {
                        VFuncs::KeyValue(pProp,  "model", args.Arg(1) );

                        if(iSkin >= 0)
                        {
                            VFuncs::KeyValue(pProp,  "skin", UTIL_VarArgs("%i", iSkin) );
                        }

                        VFuncs::KeyValue(pProp,  "spawnflags", "256" );
                        VFuncs::KeyValue(pProp,  "physdamagescale", "0.1" );
                        VFuncs::KeyValue(pProp,  "ExplodeDamage", "0" );
                        VFuncs::KeyValue(pProp,  "ExplodeRadius", "0" );
                        if(spawnMode == 2)
                            pProp->VPhysicsInitNormal( SOLID_BBOX, 0, false );
                        VFuncs::Spawn(pProp);
                        VFuncs::Activate(pProp);
                        VFuncs::SetModel( pProp, args.Arg(1) );
                        VFuncs::Teleport(pProp,  &vecOrigin, &vecAngles, NULL );

                        if(VFuncs::GetEFlags(pProp) & EFL_KILLME)
                        {
                            SayText(UTIL_VarArgs("[%s] Spawn failed.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
                        }

                        if(isFrozen)
                        {
                            IPhysicsObject *pObj = VFuncs::VPhysicsGetObject(pProp);
                            if(pObj)
                                pObj->EnableMotion(0);
                        }

                        spawnedEnts.AddToTail(VFuncs::entindex(pProp));
                        pAdminOP.spawnedServerEnts.AddToTail(VFuncs::entindex(pProp));
                        pAdminOP.TimeLog("adminlog.log", "[%s;%s] used command %s %s\n", engine->GetPlayerNetworkIDString(pEntity), playerinfo->GetName(), command_prefix.GetString(), pcmd, args.ArgS());
                    }
                }
                else
                {
                    LIMIT_MSG();
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_spawnmodel.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_spawnjeep"))
        {
            if(IsAdmin(1024, "e_spawnjeep"))
            {
                if((GetSpawnLimit() == -1 || spawnedEnts.Count() < GetSpawnLimit()) && (spawnlimit_server.GetInt() == -1 || pAdminOP.spawnedServerEnts.Count() < spawnlimit_server.GetInt()))
                {
                    if(!stricmp(pAdminOP.ModName(), "hl2mp") || FStrEq(args[1], "modoverride"))
                    {
                        Vector vecForward;
                        //AngleVectors( VFuncs::EyeAngles(pPlayer), &vecForward );
                        CBaseEntity *pJeep = CreateEntityByName( "prop_vehicle_jeep" );
                        //edict_t *peJeep = engine->CreateEdict();
                        //CBaseEntity *pJeep = CBaseEntity::Instance(peJeep);
                        if(pJeep)
                        {
                            //pJeep->m_iClassname = MAKE_STRING("prop_vehivle_driveable");
                            Vector vecOrigin = VFuncs::GetAbsOrigin(pPlayer) + Vector(0,0,64);
                            QAngle vecAngles( 0, 0, 0 );
                            //VFuncs::SetAbsOrigin(pJeep,  vecOrigin );
                            //VFuncs::SetAbsAngles(pJeep,  vecAngles );
                            VFuncs::KeyValue(pJeep,  "model", "models/buggy.mdl" );
                            VFuncs::KeyValue(pJeep,  "solid", "6" );
                            //VFuncs::KeyValue(pJeep,  "targetname", "jeep" );
                            VFuncs::KeyValue(pJeep,  "vehiclescript", "scripts/vehicles/jeep_test.txt" );
                            VFuncs::Spawn(pJeep);
                            VFuncs::Activate(pJeep);
                            VFuncs::Teleport(pJeep,  &vecOrigin, &vecAngles, NULL );

                            //spawnedEnts.AddToTail(VFuncs::entindex(pJeep));
                            //pAdminOP.spawnedServerEnts.AddToTail(VFuncs::entindex(pJeep));
                            pAdminOP.TimeLog("adminlog.log", "[%s;%s] used command %s %s\n", engine->GetPlayerNetworkIDString(pEntity), playerinfo->GetName(), command_prefix.GetString(), pcmd, args.ArgS());
                        }
                    }
                    else
                    {
                        SayTextChatHud(UTIL_VarArgs("[%s] Spawning jeep is disabled in '%s'.\n", pAdminOP.adminname, pAdminOP.ModName()));
                    }
                }
                else
                {
                    LIMIT_MSG();
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_spawnjeep.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_spawnboat"))
        {
            if(IsAdmin(1024, "e_spawnboat"))
            {
                if((GetSpawnLimit() == -1 || spawnedEnts.Count() < GetSpawnLimit()) && (spawnlimit_server.GetInt() == -1 || pAdminOP.spawnedServerEnts.Count() < spawnlimit_server.GetInt()))
                {
                    if(!stricmp(pAdminOP.ModName(), "hl2mp") || FStrEq(args[1], "modoverride"))
                    {
                        Vector vecForward;
                        //AngleVectors( VFuncs::EyeAngles(pPlayer), &vecForward );
                        CBaseEntity *pJeep = CreateEntityByName( "prop_vehicle_airboat" );
                        //edict_t *peJeep = engine->CreateEdict();
                        //CBaseEntity *pJeep = CBaseEntity::Instance(peJeep);
                        if(pJeep)
                        {
                            //pJeep->m_iClassname = MAKE_STRING("prop_vehivle_driveable");
                            Vector vecOrigin = VFuncs::GetAbsOrigin(pPlayer) + Vector(0,0,64);
                            QAngle vecAngles( 0, 0, 0 );
                            //VFuncs::SetAbsOrigin(pJeep,  vecOrigin );
                            //VFuncs::SetAbsAngles(pJeep,  vecAngles );
                            VFuncs::KeyValue(pJeep,  "model", "models/airboat.mdl" );
                            VFuncs::KeyValue(pJeep,  "solid", "6" );
                            //VFuncs::KeyValue(pJeep,  "targetname", "jeep" );
                            VFuncs::KeyValue(pJeep,  "vehiclescript", "scripts/vehicles/airboat.txt" );
                            VFuncs::Spawn(pJeep);
                            VFuncs::Activate(pJeep);
                            VFuncs::Teleport(pJeep,  &vecOrigin, &vecAngles, NULL );

                            //spawnedEnts.AddToTail(VFuncs::entindex(pJeep));
                            //pAdminOP.spawnedServerEnts.AddToTail(VFuncs::entindex(pJeep));
                            pAdminOP.TimeLog("adminlog.log", "[%s;%s] used command %s %s\n", engine->GetPlayerNetworkIDString(pEntity), playerinfo->GetName(), command_prefix.GetString(), pcmd, args.ArgS());
                        }
                    }
                    else
                    {
                        SayTextChatHud(UTIL_VarArgs("[%s] Spawning boat is disabled in '%s'.\n", pAdminOP.adminname, pAdminOP.ModName()));
                    }
                }
                else
                {
                    LIMIT_MSG();
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_spawnboat.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_spawncan_square"))
        {
            if(IsAdmin(1024, "e_spawncan_square"))
            {
                bool hitLimit = false;
                int iSkin = 0;
                if(args.ArgC() > 1)
                {
                    iSkin = atoi(args.Arg(1));
                    if(iSkin < 0 || iSkin > 5)
                    {
                        SayText(UTIL_VarArgs("[%s] Invalid skin number. Skin must be 0-5.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
                        iSkin = 0;
                    }
                }

                pAdminOP.TimeLog("adminlog.log", "[%s;%s] used command %s %s\n", engine->GetPlayerNetworkIDString(pEntity), playerinfo->GetName(), command_prefix.GetString(), pcmd, args.ArgS());
                for(int i = -2;i<=2;i++)
                {
                    for(int j = -2;j<=2;j++)
                    {
                        if(i==0 && j==0) continue;

                        if((GetSpawnLimit() == -1 || spawnedEnts.Count() < GetSpawnLimit()) && (spawnlimit_server.GetInt() == -1 || pAdminOP.spawnedServerEnts.Count() < spawnlimit_server.GetInt()))
                        {
                            CBaseEntity *pCan = CreateEntityByName( "prop_physics_multiplayer" ); //prop_physics_respawnable
                            if(pCan)
                            {
                                Vector vecOrigin = VFuncs::GetAbsOrigin(pPlayer) + Vector(i*32,j*32,0.5);
                                QAngle vecAngles( 0, 0, 0 );
                                //VFuncs::SetAbsOrigin(pCan,  vecOrigin );
                                //VFuncs::SetAbsAngles(pCan,  vecAngles );
                                VFuncs::KeyValue(pCan,  "origin", UTIL_VarArgs("%0.2f %0.2f %0.2f", vecOrigin.x, vecOrigin.y, vecOrigin.z) );
                                if(atoi(args.Arg(2)))
                                    VFuncs::KeyValue(pCan,  "model", "models/props_c17/oildrum001_explosive.mdl" );
                                else
                                    VFuncs::KeyValue(pCan,  "model", "models/props_c17/oildrum001.mdl" );
                                VFuncs::KeyValue(pCan,  "skin", UTIL_VarArgs("%i", iSkin) );
                                VFuncs::KeyValue(pCan,  "spawnflags", "1" );
                                VFuncs::KeyValue(pCan,  "physdamagescale", "0.1" );
                                VFuncs::KeyValue(pCan,  "RespawnTime", "5" );
                                VFuncs::KeyValue(pCan,  "PerformanceMode", "1" );
                                //VFuncs::KeyValue(pCan,  "ExplodeDamage", "0" );
                                //VFuncs::KeyValue(pCan,  "ExplodeRadius", "0" );
                                VFuncs::Spawn(pCan);
                                VFuncs::Activate(pCan);
                                VFuncs::Teleport(pCan,  &vecOrigin, &vecAngles, NULL );

                                spawnedEnts.AddToTail(VFuncs::entindex(pCan));
                                pAdminOP.spawnedServerEnts.AddToTail(VFuncs::entindex(pCan));
                            }
                        }
                        else
                        {
                            hitLimit = true;
                            break;
                        }
                    }
                }
                if(hitLimit)
                {
                    LIMIT_MSG();
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_spawncan_square.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_spawncan"))
        {
            if(IsAdmin(1024, "e_spawncan"))
            {
                if((GetSpawnLimit() == -1 || spawnedEnts.Count() < GetSpawnLimit()) && (spawnlimit_server.GetInt() == -1 || pAdminOP.spawnedServerEnts.Count() < spawnlimit_server.GetInt()))
                {
                    CBaseEntity *pCan = CreateEntityByName( "prop_physics_multiplayer" );
                    if(pCan)
                    {
                        int iSkin = 0;
                        bool isExplosive = 0;
                        bool isFrozen = 0;

                        Vector forward, right, up;
                        QAngle playerAngles = VFuncs::EyeAngles(pPlayer);
                        AngleVectors( playerAngles, &forward, &right, &up );

                        Vector vecOrigin = EyePosition() + forward*48;
                        QAngle vecAngles( 0, 0, 0 );
                        //VFuncs::SetAbsOrigin(pCan,  vecOrigin );
                        //VFuncs::SetAbsAngles(pCan,  vecAngles );

                        if(args.ArgC() > 1)
                        {
                            char strArgs[512];
                            Vector forward, right, up;
                            QAngle playerAngles = VFuncs::EyeAngles(pPlayer);
                            AngleVectors( playerAngles, &forward, &right, &up );
                            strncpy(strArgs, args.ArgS(), 512);
                            Vector playerorigin = VFuncs::GetAbsOrigin(pPlayer);

                            char seps[]   = " ,;/\n";
                            char *token;
                            token = strtok( strArgs, seps );
                            while( token != NULL )
                            {
                                if(!stricmp(token, "front") || !stricmp(token, "forward"))
                                {
                                    vecOrigin = playerorigin + Vector(forward.x*48,forward.y*48,0);
                                }
                                else if(!stricmp(token, "back") || !stricmp(token, "behind"))
                                {
                                    vecOrigin = playerorigin - Vector(forward.x*48,forward.y*48,0);
                                }
                                else if(!stricmp(token, "left"))
                                {
                                    vecOrigin = playerorigin - Vector(right.x*48,right.y*48,0);
                                }
                                else if(!stricmp(token, "right"))
                                {
                                    vecOrigin = playerorigin + Vector(right.x*48,right.y*48,0);
                                }
                                else if(!stricmp(token, "top") || !stricmp(token, "above") || !stricmp(token, "up"))
                                {
                                    vecOrigin = playerorigin + Vector(0,0,128);
                                }
                                else if(!stricmp(token, "bottom") || !stricmp(token, "below") || !stricmp(token, "down"))
                                {
                                    vecOrigin = playerorigin - Vector(0,0,128);
                                }
                                else if(!Q_strncasecmp(token, "skin=", 5))
                                {
                                    if(stricmp(token, "skin="))
                                    {
                                        char szSkin[32];
                                        strncpy(szSkin, &token[5], 32);
                                        szSkin[31] = '\0';
                                        iSkin = atoi(szSkin);
                                    }
                                }
                                else if(!stricmp(token, "explosive") || !stricmp(token, "explode") || !stricmp(token, "flammable") || !stricmp(token, "bwent"))
                                {
                                    isExplosive = 1;
                                }
                                else if(!stricmp(token, "frozen")|| !stricmp(token, "freeze") || !stricmp(token, "disabled") || !stricmp(token, "disable"))
                                {
                                    isFrozen = 1;
                                }
                                token = strtok( NULL, seps );
                            }
                        }

                        if(isExplosive)
                            VFuncs::KeyValue(pCan,  "model", "models/props_c17/oildrum001_explosive.mdl" );
                        else
                            VFuncs::KeyValue(pCan,  "model", "models/props_c17/oildrum001.mdl" );
                        if(iSkin >= 0 && iSkin <= 5)
                        {
                            VFuncs::KeyValue(pCan,  "skin", UTIL_VarArgs("%i", iSkin) );
                        }
                        else
                        {
                            SayText(UTIL_VarArgs("[%s] Invalid skin number. Skin must be 0-5.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
                        }
                        VFuncs::KeyValue(pCan,  "spawnflags", "256" );
                        VFuncs::KeyValue(pCan,  "physdamagescale", "0.1" );
                        VFuncs::KeyValue(pCan,  "ExplodeDamage", "0" );
                        VFuncs::KeyValue(pCan,  "ExplodeRadius", "0" );
                        VFuncs::Spawn(pCan);
                        VFuncs::Activate(pCan);
                        VFuncs::Teleport(pCan,  &vecOrigin, &vecAngles, NULL );

                        //Test to make sphere can
                        /*solid_t pSolid;
                        memset(&pSolid, 0, sizeof(pSolid));
                        pSolid.params.mass = 10;
                        pSolid.params.inertia = 1;
                        pSolid.params.volume = 90;
                        pSolid.params.enableCollisions = 1;
                        //IPhysicsObject *pObject = PhysSphereCreate(pCan, 10, vecOrigin, pSolid);
                        int surfaceProp = -1;
                        pSolid.params.pGameData = static_cast<void *>(pCan);
                        IPhysicsObject *pObject = physenv->CreateSphereObject( 30, surfaceProp, vecOrigin, vec3_angle, &pSolid.params, false );
                        pCan->VPhysicsDestroyObject();
                        VFuncs::VPhysicsSetObject(pCan,pObject);*/

                        if(isFrozen)
                        {
                            IPhysicsObject *pObj = VFuncs::VPhysicsGetObject(pCan);
                            if(pObj)
                                pObj->EnableMotion(0);
                        }

                        spawnedEnts.AddToTail(VFuncs::entindex(pCan));
                        pAdminOP.spawnedServerEnts.AddToTail(VFuncs::entindex(pCan));
                        pAdminOP.TimeLog("adminlog.log", "[%s;%s] used command %s %s\n", engine->GetPlayerNetworkIDString(pEntity), playerinfo->GetName(), command_prefix.GetString(), pcmd, args.ArgS());
                    }
                }
                else
                {
                    LIMIT_MSG();
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_spawncan.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_spawnball"))
        {
            if(IsAdmin(1024, "e_spawnball"))
            {
                if((GetSpawnLimit() == -1 || spawnedEnts.Count() < GetSpawnLimit()) && (spawnlimit_server.GetInt() == -1 || pAdminOP.spawnedServerEnts.Count() < spawnlimit_server.GetInt()))
                {
                    int flSize = 10.0f;
                    int iLife = 40;
                    int iMass = 150;
                    bool isFollow = 1;
                    bool isFrozen = 0;

                    Vector forward, right, up;
                    QAngle playerAngles = VFuncs::EyeAngles(pPlayer);
                    AngleVectors( playerAngles, &forward, &right, &up );

                    Vector vecOrigin = EyePosition() + forward*48;

                    //VFuncs::SetAbsOrigin(pCan,  vecOrigin );
                    //VFuncs::SetAbsAngles(pCan,  vecAngles );

                    if(args.ArgC() > 1)
                    {
                        char strArgs[512];
                        Vector forward, right, up;
                        QAngle playerAngles = VFuncs::EyeAngles(pPlayer);
                        AngleVectors( playerAngles, &forward, &right, &up );
                        strncpy(strArgs, args.ArgS(), 512);
                        Vector playerorigin = VFuncs::GetAbsOrigin(pPlayer);

                        char seps[]   = " ,;/\n";
                        char *token;
                        token = strtok( strArgs, seps );
                        while( token != NULL )
                        {
                            if(!stricmp(token, "front") || !stricmp(token, "forward"))
                            {
                                vecOrigin = playerorigin + Vector(forward.x*48,forward.y*48,32);
                            }
                            else if(!stricmp(token, "back") || !stricmp(token, "behind"))
                            {
                                vecOrigin = playerorigin - Vector(forward.x*48,forward.y*48,32);
                            }
                            else if(!stricmp(token, "left"))
                            {
                                vecOrigin = playerorigin - Vector(right.x*48,right.y*48,32);
                            }
                            else if(!stricmp(token, "right"))
                            {
                                vecOrigin = playerorigin + Vector(right.x*48,right.y*48,32);
                            }
                            else if(!stricmp(token, "top") || !stricmp(token, "above") || !stricmp(token, "up"))
                            {
                                vecOrigin = playerorigin + Vector(0,0,96);
                            }
                            else if(!stricmp(token, "bottom") || !stricmp(token, "below") || !stricmp(token, "down"))
                            {
                                vecOrigin = playerorigin - Vector(0,0,96);
                            }
                            else if(!Q_strncasecmp(token, "size=", 5))
                            {
                                if(stricmp(token, "size="))
                                {
                                    char szSize[32];
                                    strncpy(szSize, &token[5], 32);
                                    szSize[31] = '\0';
                                    flSize = atof(szSize);
                                    iMass = clamp(150*pow((float)flSize/10,(int)3), 1, 500000);
                                }
                            }
                            else if(!Q_strncasecmp(token, "life=", 5))
                            {
                                if(stricmp(token, "life="))
                                {
                                    char szLife[32];
                                    strncpy(szLife, &token[5], 32);
                                    szLife[31] = '\0';
                                    iLife = atoi(szLife);
                                }
                            }
                            else if(!Q_strncasecmp(token, "mass=", 5))
                            {
                                if(stricmp(token, "mass="))
                                {
                                    char szMass[32];
                                    strncpy(szMass, &token[5], 32);
                                    szMass[31] = '\0';
                                    iMass = atoi(szMass);
                                }
                            }
                            else if(!stricmp(token, "nofollow"))
                            {
                                isFollow = 0;
                            }
                            else if(!stricmp(token, "frozen")|| !stricmp(token, "freeze") || !stricmp(token, "disabled") || !stricmp(token, "disable"))
                            {
                                isFrozen = 1;
                            }
                            token = strtok( NULL, seps );
                        }
                    }
                    CBaseEntity *pBall = CreateCombineBall( vecOrigin, isFollow ? VFuncs::GetAbsVelocity(pPlayer) : Vector(0,0,0), flSize, iMass, iLife, pPlayer );
                    if(pBall)
                    {
                        IPhysicsObject *pObj = VFuncs::VPhysicsGetObject(pBall);
                        if(pObj)
                        {
                            pObj->SetMass(iMass);
                        }
                        //float *flMaxSize = (float*)0x223EF764;
                        //long *lMaxSize = (long*)0x223EF764;
                        //Msg("%x %x %x %f %f %l %l\n", pBall, flMaxSize, lMaxSize, flMaxSize, *flMaxSize, lMaxSize, *lMaxSize);
                        //*flMaxSize = 1000;
                        if(isFrozen)
                        {
                            if(pObj)
                                pObj->EnableMotion(0);
                        }
                        spawnedEnts.AddToTail(VFuncs::entindex(pBall));
                        pAdminOP.spawnedServerEnts.AddToTail(VFuncs::entindex(pBall));
                        pAdminOP.TimeLog("adminlog.log", "[%s;%s] used command %s %s\n", engine->GetPlayerNetworkIDString(pEntity), playerinfo->GetName(), command_prefix.GetString(), pcmd, args.ArgS());
                    }
                    else
                    {
                        SayTextChatHud(UTIL_VarArgs("[%s] Spawning combine ball does not function in '%s'.\n", pAdminOP.adminname, pAdminOP.ModName()));
                    }
                }
                else
                {
                    LIMIT_MSG();
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_spawnball.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_spawndoll"))
        {
            if(IsAdmin(1024, "e_spawndoll"))
            {
                if((GetSpawnLimit() == -1 || spawnedEnts.Count() < GetSpawnLimit()) && (spawnlimit_server.GetInt() == -1 || pAdminOP.spawnedServerEnts.Count() < spawnlimit_server.GetInt()))
                {
                    CBaseEntity *pDoll = CreateEntityByName( "prop_ragdoll" );
                    if(pDoll)
                    {
                        Vector vecOrigin = VFuncs::GetAbsOrigin(pPlayer) + Vector(0,0,128);
                        QAngle vecAngles( 0, 0, 0 );
                        //VFuncs::SetAbsOrigin(pDoll,  vecOrigin );
                        //VFuncs::SetAbsAngles(pDoll,  vecAngles );
                        VFuncs::KeyValue(pDoll,  "model", "models/Police.mdl" );
                        VFuncs::KeyValue(pDoll,  "spawnflags", "0" );
                        VFuncs::Spawn(pDoll);
                        VFuncs::Activate(pDoll);
                        VFuncs::Teleport(pDoll,  &vecOrigin, &vecAngles, NULL );
                        spawnedEnts.AddToTail(VFuncs::entindex(pDoll));
                        pAdminOP.spawnedServerEnts.AddToTail(VFuncs::entindex(pDoll));
                        pAdminOP.TimeLog("adminlog.log", "[%s;%s] used command %s %s\n", engine->GetPlayerNetworkIDString(pEntity), playerinfo->GetName(), command_prefix.GetString(), pcmd, args.ArgS());
                    }
                }
                else
                {
                    LIMIT_MSG();
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_spawndoll.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_spawncount"))
        {
            if(IsAdmin(1024, "e_spawncount"))
            {
                SayText(UTIL_VarArgs("[%s] Your count %i/%i. Server count %i/%i\n", pAdminOP.adminname, spawnedEnts.Count(), GetSpawnLimit(), pAdminOP.spawnedServerEnts.Count(), spawnlimit_server.GetInt()));
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_spawncount.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_removespawned"))
        {
            if(IsAdmin(1024, "e_removespawned"))
            {
                CUtlLinkedList <unsigned int, unsigned short> spawnedEntsCopy;

                for ( unsigned int i=spawnedEnts.Head(); i != spawnedEnts.InvalidIndex(); i = spawnedEnts.Next( i ) )
                {
                    spawnedEntsCopy.AddToTail(spawnedEnts.Element(i));
                }

                for ( unsigned int i=spawnedEntsCopy.Head(); i != spawnedEntsCopy.InvalidIndex(); i = spawnedEntsCopy.Next( i ) )
                {
                    edict_t *pEntity = pAdminOP.GetEntityList() + spawnedEntsCopy.Element(i);
                    if(pEntity)
                    {
                        if(!pEntity->IsFree())
                        {
                            CBaseEntity *pEnt = (CBaseEntity*)CBaseEntity::Instance(pEntity);
                            UTIL_Remove(pEnt);
                        }
                    }
                }

                pAdminOP.TimeLog("adminlog.log", "[%s;%s] used command %s %s\n", engine->GetPlayerNetworkIDString(pEntity), playerinfo->GetName(), command_prefix.GetString(), pcmd, args.ArgS());
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_removespawned.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_render"))
        {
            if(IsAdmin(1024, "e_render"))
            {
                CBaseEntity *pHit = FindEntityForward();
                if(pHit)
                {
                    pAdminOP.TimeLog("adminlog.log", "[%s;%s] used command %s %s on entity %s %i\n", engine->GetPlayerNetworkIDString(pEntity), playerinfo->GetName(), command_prefix.GetString(), pcmd, args.ArgS(), VFuncs::GetClassname(pHit), VFuncs::entindex(pHit));
                    if(args.ArgC() >= 2 && atoi(args.Arg(1)) != -1) VFuncs::SetRenderMode(pHit, (RenderMode_t)atoi(args.Arg(1)));
                    if(args.ArgC() >= 3 && atoi(args.Arg(2)) != -1) VFuncs::SetRenderColorA(pHit, atoi(args.Arg(2)));
                    if(args.ArgC() >= 4 && atoi(args.Arg(3)) != -1) VFuncs::SetRenderFX(pHit, atoi(args.Arg(3)));
                    if(args.ArgC() >= 5 && atoi(args.Arg(4)) != -1) VFuncs::SetRenderColorR(pHit, atoi(args.Arg(4)));
                    if(args.ArgC() >= 6 && atoi(args.Arg(5)) != -1) VFuncs::SetRenderColorG(pHit, atoi(args.Arg(5)));
                    if(args.ArgC() >= 7 && atoi(args.Arg(6)) != -1) VFuncs::SetRenderColorB(pHit, atoi(args.Arg(6)));
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_render.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_setview"))
        {
            if(IsAdmin(1024, "e_setview"))
            {
                CBaseEntity *pHit = FindEntityForward();
                if(pHit)
                {
                    engine->SetView(pEntity, servergameents->BaseEntityToEdict(pHit));

                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_setview.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_resetview"))
        {
            if(IsAdmin(1024, "e_resetview"))
            {
                engine->SetView(pEntity, pEntity);
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_resetview.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_setmodel"))
        {
            if(IsAdmin(1024, "e_setmodel"))
            {
                CBaseEntity *pHit = FindEntityForward();
                if(pHit)
                {
                    if(!VFuncs::IsPlayer(pHit))
                    {
                        pAdminOP.TimeLog("adminlog.log", "[%s;%s] used command %s %s on entity %s %i\n", engine->GetPlayerNetworkIDString(pEntity), playerinfo->GetName(), command_prefix.GetString(), pcmd, args.ArgS(), VFuncs::GetClassname(pHit), VFuncs::entindex(pHit));
                        // try and precache
                        if(modelinfo->GetModelIndex(args.Arg(1)) < 0)
                        {
                            engine->PrecacheModel(args.Arg(1), true);
                        }

                        if(modelinfo->GetModelIndex(args.Arg(1)) >= 0)
                        {
                            UTIL_SetModel(pHit, args.Arg(1));
                        }
                        else
                        {
                            SayText(UTIL_VarArgs("[%s] Unable to set model. Precache failed.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
                        }
                    }
                    else
                    {
                        SayText(UTIL_VarArgs("[%s] Using this command on a player is not allowed.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
                    }
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_setmodel.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_setskin"))
        {
            if(IsAdmin(1024, "e_setskin"))
            {
                CBaseEntity *pHit = FindEntityForward();
                if(pHit)
                {
                    pAdminOP.TimeLog("adminlog.log", "[%s;%s] used command %s %s on entity %s %i\n", engine->GetPlayerNetworkIDString(pEntity), playerinfo->GetName(), command_prefix.GetString(), pcmd, args.ArgS(), VFuncs::GetClassname(pHit), VFuncs::entindex(pHit));
                    VFuncs::SetSkin((CBaseAnimating*)pHit, atoi(args.Arg(1)));
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_setskin.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_fire"))
        {
            if(IsAdmin(1024, "e_fire"))
            {
                if(args.ArgC() > 2)
                {
                    CBaseEntity *pHit = FindEntityForward();
                    if(pHit)
                    {
                        pAdminOP.TimeLog("adminlog.log", "[%s;%s] used command %s %s on entity %s %i\n", engine->GetPlayerNetworkIDString(pEntity), playerinfo->GetName(), command_prefix.GetString(), pcmd, args.ArgS(), VFuncs::GetClassname(pHit), VFuncs::entindex(pHit));
                        CBaseEntity *pPlayer = CBaseEntity::Instance(pEntity);
                        variant_t value;

                        value.SetString( MAKE_STRING(args.Arg(2)) );
                        VFuncs::AcceptInput(pHit, args.Arg(1), pPlayer, pPlayer, value, 0 );
                    }
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_fire.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_fire_self"))
        {
            if(IsAdmin(1024, "e_fire_self"))
            {
                if(args.ArgC() > 2)
                {
                    CBaseEntity *pPlayer = CBaseEntity::Instance(pEntity);
                    variant_t value;

                    value.SetString( MAKE_STRING(args.Arg(2)) );
                    VFuncs::AcceptInput(pPlayer, args.Arg(1), pPlayer, pPlayer, value, 0 );
                    pAdminOP.TimeLog("adminlog.log", "[%s;%s] used command %s %s\n", engine->GetPlayerNetworkIDString(pEntity), playerinfo->GetName(), command_prefix.GetString(), pcmd, args.ArgS());
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_fire_self.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_addoutput"))
        {
            if(IsAdmin(1024, "e_addoutput"))
            {
                if(args.ArgC() > 2)
                {
                    CBaseEntity *pHit = FindEntityForward();
                    if(pHit)
                    {
                        pAdminOP.TimeLog("adminlog.log", "[%s;%s] used command %s %s on entity %s %i\n", engine->GetPlayerNetworkIDString(pEntity), playerinfo->GetName(), command_prefix.GetString(), pcmd, args.ArgS(), VFuncs::GetClassname(pHit), VFuncs::entindex(pHit));
                        CBaseEntity *pPlayer = CBaseEntity::Instance(pEntity);
                        variant_t value;

                        value.SetString( MAKE_STRING(args.ArgS()) );
                        VFuncs::AcceptInput(pHit, "AddOutput", pPlayer, pPlayer, value, 0 );
                    }
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_addoutput.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_getmass"))
        {
            if(IsAdmin(1024, "e_getmass"))
            {
                CBaseEntity *pHit = FindEntityForward();
                if(pHit)
                {
                    IPhysicsObject *pObj = VFuncs::VPhysicsGetObject(pHit);
                    if(pObj)
                    {
                        SayTextChatHud(UTIL_VarArgs("[%s] Mass is %f\n", pAdminOP.adminname, pObj->GetMass()));
                    }
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_getmass.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_getinertia"))
        {
            if(IsAdmin(1024, "e_getinertia"))
            {
                CBaseEntity *pHit = FindEntityForward();
                if(pHit)
                {
                    IPhysicsObject *pObj = VFuncs::VPhysicsGetObject(pHit);
                    if(pObj)
                    {
                        Vector inertia = pObj->GetInertia();
                        SayTextChatHud(UTIL_VarArgs("[%s] Inertia is %f %f %f\n", pAdminOP.adminname, inertia.x, inertia.y, inertia.z));
                    }
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_getmass.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_setmass"))
        {
            if(IsAdmin(1024, "e_setmass"))
            {
                if(args.ArgC() == 2)
                {
                    CBaseEntity *pHit = FindEntityForward();
                    if(pHit)
                    {
                        pAdminOP.TimeLog("adminlog.log", "[%s;%s] used command %s %s on entity %s %i\n", engine->GetPlayerNetworkIDString(pEntity), playerinfo->GetName(), command_prefix.GetString(), pcmd, args.ArgS(), VFuncs::GetClassname(pHit), VFuncs::entindex(pHit));
                        IPhysicsObject *pObj = VFuncs::VPhysicsGetObject(pHit);
                        if(pObj)
                        {
                            pObj->SetMass(atof(args.Arg(1)));
                        }
                    }
                }
                else
                {
                    SayTextChatHud(UTIL_VarArgs("[%s] Incorrect amount of parameters for e_setmass. Expected 1 for mass.\n", pAdminOP.adminname));
                    SayTextChatHud(UTIL_VarArgs("[%s] Usage: e_setmass <mass>\n", pAdminOP.adminname));
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_setmass.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_setinertia"))
        {
            if(IsAdmin(1024, "e_setinertia"))
            {
                if(args.ArgC() == 4)
                {
                    CBaseEntity *pHit = FindEntityForward();
                    if(pHit)
                    {
                        pAdminOP.TimeLog("adminlog.log", "[%s;%s] used command %s %s on entity %s %i\n", engine->GetPlayerNetworkIDString(pEntity), playerinfo->GetName(), command_prefix.GetString(), pcmd, args.ArgS(), VFuncs::GetClassname(pHit), VFuncs::entindex(pHit));
                        IPhysicsObject *pObj = VFuncs::VPhysicsGetObject(pHit);
                        if(pObj)
                        {
                            Vector inertia(atof(args.Arg(1)), atof(args.Arg(2)), atof(args.Arg(3)));
                            if(inertia.x > 0.00001 && inertia.y > 0.00001 && inertia.z > 0.00001)
                            {
                                pObj->SetInertia(inertia);
                            }
                            else
                            {
                                SayTextChatHud(UTIL_VarArgs("[%s] Invalid value for inertia.\n", pAdminOP.adminname));
                            }
                        }
                    }
                }
                else
                {
                    SayTextChatHud(UTIL_VarArgs("[%s] Incorrect amount of parameters for e_setinertia. Expected 3 for inertia x, y, and z.\n", pAdminOP.adminname));
                    SayTextChatHud(UTIL_VarArgs("[%s] Usage: e_setinertia <inertia x> <inertia y> <inertia z>\n", pAdminOP.adminname));
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_setinertia.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_push"))
        {
            if(IsAdmin(1024, "e_push"))
            {
                if(args.ArgC() == 2)
                {
                    CBaseEntity *pHit = FindEntityForward();
                    if(pHit)
                    {
                        pAdminOP.TimeLog("adminlog.log", "[%s;%s] used command %s %s on entity %s %i\n", engine->GetPlayerNetworkIDString(pEntity), playerinfo->GetName(), command_prefix.GetString(), pcmd, args.ArgS(), VFuncs::GetClassname(pHit), VFuncs::entindex(pHit));
                        IPhysicsObject *pObj = VFuncs::VPhysicsGetObject(pHit);
                        if(pObj)
                        {
                            Vector forward, right, up;
                            QAngle playerAngles = VFuncs::EyeAngles(pPlayer);
                            AngleVectors( playerAngles, &forward, &right, &up );

                            pObj->ApplyForceCenter(forward*atoi(args.Arg(1)));
                        }
                    }
                }
                else
                {
                    SayTextChatHud(UTIL_VarArgs("[%s] Incorrect amount of parameters for e_push. Expected 1 for force.\n", pAdminOP.adminname));
                    SayTextChatHud(UTIL_VarArgs("[%s] Usage: e_push <force>\n", pAdminOP.adminname));
                }
            }
            else
            {
                SayText(UTIL_VarArgs("[%s] You do not have access to the command e_setinertia.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return PLUGIN_STOP;
        }
        /*if(FStrEq(pcmd, "e_freezeent"))
        {
            if(IsAdmin(1024, "e_freezeent"))
            {
                if(args.ArgC() > 1)
                {
                    edict_t *pEntity = pAdminOP.GetEntityList()+atoi(args.Arg(1));
                    if(pEntity)
                    {
                        CBaseEntity *pEnt = CBaseEntity::Instance(pEntity);
                        if(pEnt)
                        {
                            IPhysicsObject *pPhysObj = VFuncs::VPhysicsGetObject(pEnt);
                            if(pPhysObj)
                            {
                                Msg("Motion enabled on ent %i\n", VFuncs::entindex(pEnt));
                                pPhysObj->EnableMotion(1);
                            }
                        }
                    }
                }
                else
                {
                    Vector forward, right, up;
                    QAngle playerAngles = VFuncs::EyeAngles(pPlayer);

                    AngleVectors( playerAngles, &forward, &right, &up );

                    float distance = 8192;
                    Vector start = EyePosition();
                    Vector end = EyePosition() + ( forward * distance );

                    trace_t tr;
                    CTraceFilterSimple traceFilter( pPlayer, COLLISION_GROUP_NONE );
                    Ray_t ray;
                    ray.Init( start, end );
                    enginetrace->TraceRay( ray, MASK_NPCSOLID, &traceFilter, &tr );

                    if(tr.DidHit() && tr.m_pEnt)
                    {
                        if(VFuncs::entindex(tr.m_pEnt) != 0)
                        {
                            Msg("Motion disabled on ent %i\n", VFuncs::entindex(tr.m_pEnt));
                            VFuncs::VPhysicsGetObject(tr.m_pEnt)->EnableMotion(0);
                        }
                    }
                }
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "e_enableent"))
        {
            if(IsAdmin(1024, "e_enableent"))
            {
                if(args.ArgC() > 1)
                {
                    edict_t *pEntity = pAdminOP.GetEntityList()+atoi(args.Arg(1));
                    if(pEntity)
                    {
                        CBaseEntity *pEnt = CBaseEntity::Instance(pEntity);
                        if(pEnt)
                        {
                            IPhysicsObject *pPhysObj = VFuncs::VPhysicsGetObject(pEnt);
                            if(pPhysObj)
                            {
                                Msg("Motion enabled on ent %i\n", VFuncs::entindex(pEnt));
                                pPhysObj->EnableMotion(1);
                            }
                        }
                    }
                }
                else
                {
                    Vector forward, right, up;
                    QAngle playerAngles = VFuncs::EyeAngles(pPlayer);

                    AngleVectors( playerAngles, &forward, &right, &up );

                    float distance = 8192;
                    Vector start = EyePosition();
                    Vector end = EyePosition() + ( forward * distance );

                    trace_t tr;
                    CTraceFilterSimple traceFilter( pPlayer, COLLISION_GROUP_NONE );
                    Ray_t ray;
                    ray.Init( start, end );
                    enginetrace->TraceRay( ray, MASK_NPCSOLID, &traceFilter, &tr );

                    if(tr.DidHit() && tr.m_pEnt)
                    {
                        if(VFuncs::entindex(tr.m_pEnt) != 0)
                        {
                            Msg("Motion enabled on ent %i\n", VFuncs::entindex(tr.m_pEnt));
                            VFuncs::VPhysicsGetObject(tr.m_pEnt)->EnableMotion(1);
                        }
                    }
                }
            }
            return PLUGIN_STOP;
        }*/
    }

    if(pAdminOP.FeatureStatus(FEAT_HOOK))
    {
        if(FStrEq(pcmd, "+hook"))
        {
            if(!Hook.OnHook && !Jetpack.OnJetpack && !IsDenied("hook") && hook_on.GetBool() && IsAlive() && pAdminOP.RoundHasBegun() && ((VFuncs::GetFlags(pPlayer) & FL_FROZEN) == 0))
            {
                Vector forward, right, up;
                QAngle playerAngles = VFuncs::EyeAngles(pPlayer);

                //float pitch = AngleDistance(playerAngles.x,0);
                //playerAngles.x = clamp( pitch, -75, 75 );
                AngleVectors( playerAngles, &forward, &right, &up );

                float distance = 16384;
                Vector start = EyePosition() - Vector(0,0,12);
                Vector end = EyePosition() + ( forward * distance );

                trace_t tr;
                CTraceFilterSimple traceFilter( pPlayer, COLLISION_GROUP_NONE );
                Ray_t ray;
                ray.Init( start, end );
                enginetrace->TraceRay( ray, MASK_SOLID_BRUSHONLY, &traceFilter, &tr );

                int m_beamIndex = engine->PrecacheModel( "cable/rope.vmt" );

                if(tr.DidHit())
                {
                    Hook.OnHook = 1;
                    Hook.HookedEnt = tr.m_pEnt;
                    Hook.HookedAt = tr.endpos;
                    Hook.HookEnt = NULL;
                    //Msg("Hook hit an entity: %x %i", tr.m_pEnt, tr.m_pEnt? VFuncs::entindex(tr.m_pEnt) : -1);
                    //effects->Beam(tr.startpos, tr.endpos, m_beamIndex, 0, 0, 2.0f, 0.1f, 2, 2, 0, 0, 255, 255, 255, 128, 100 );

                    CBaseEntity *pCan = CreateEntityByName( "prop_dynamic" ); //prop_physics_respawnable
                    if(pCan)
                    {
                        QAngle vecAngles;
                        VectorAngles(tr.plane.normal, vecAngles);
                        vecAngles.x += 90;

                        VFuncs::KeyValue(pCan,  "origin", UTIL_VarArgs("%0.2f %0.2f %0.2f", tr.endpos.x, tr.endpos.y, tr.endpos.z) );
                        VFuncs::KeyValue(pCan,  "angles", UTIL_VarArgs("%0.2f %0.2f %0.2f", vecAngles.x, vecAngles.y, vecAngles.z) );
                        VFuncs::KeyValue(pCan,  "model", "models/props_lab/cleaver.mdl" );
                        VFuncs::KeyValue(pCan,  "spawnflags", "9934" );
                        VFuncs::KeyValue(pCan,  "physdamagescale", "0.1" );
                        VFuncs::KeyValue(pCan,  "disableshadows", "1" );
                        VFuncs::KeyValue(pCan,  "ExplodeDamage", "0" );
                        VFuncs::KeyValue(pCan,  "ExplodeRadius", "0" );
                        VFuncs::KeyValue(pCan,  "nodamageforces", "1" );
                        VFuncs::KeyValue(pCan,  "solid", "0" );
                        VFuncs::KeyValue(pCan,  "rendermode", "2" );
                        VFuncs::KeyValue(pCan,  "renderamt", "0" );
                        VFuncs::Spawn(pCan);
                        VFuncs::Activate(pCan);
                        VFuncs::Teleport(pCan,  &tr.endpos, &vecAngles, NULL );
                        if(VFuncs::entindex(tr.m_pEnt) != 0)
                            VFuncs::SetParent(pCan, tr.m_pEnt);

                        Hook.HookEnt = pCan;

                        CBroadcastRecipientFilter filter;
                        if(te) te->BeamEnts(filter, 0, entID, VFuncs::entindex(pCan), m_beamIndex, 0, 0, 0.0f, 0.1f, 2, 2, 0, 0, 255, 255, 255, 128, 0);
                        //if(te) te->BeamEnts(filter, 0, entID, VFuncs::entindex(pCan), m_beamIndex, 0, 0, 2.0f, 0.1f, 2, 2, 0, 0, 255, 255, 255, 128, 100);
                    }
                    else
                    {
                        CBroadcastRecipientFilter filter;
                        if(te) te->BeamEntPoint(filter, 0, entID, &Vector(0,0,0), -1, &tr.endpos, m_beamIndex, 0, 0, 2.0f, 0.1f, 2, 2, 0, 0, 255, 255, 255, 128, 100);
                    }
                }
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "-hook"))
        {
            HookOff();
            return PLUGIN_STOP;
        }
    }
    if(pAdminOP.FeatureStatus(FEAT_JETPACK))
    {
        if(FStrEq(pcmd, "+jetpack"))
        {
            if(jetpack_on.GetBool() || IsAdmin(8, "jetpack_override"))
            {
                if(!IsDenied("jetpack"))
                {
                    if(!Hook.OnHook && !Jetpack.OnJetpack && IsAlive() && pAdminOP.RoundHasBegun() && ((VFuncs::GetFlags(pPlayer) & FL_FROZEN) == 0))
                    {
                        if(jetpack_team.GetInt())
                        {
                            int team = playerinfo->GetTeamIndex();
                            if(team != jetpack_team.GetInt())
                            {
                                SayTextChatHud(UTIL_VarArgs("[%s] Sorry, the team you are on is not allowed to use the jetpack.\n", pAdminOP.adminname));
                                return PLUGIN_STOP;
                            }
                        }

                        JetpackActivate();
                    }
                }
                else
                {
                    if(jetpack_show_accessdenymsg.GetBool())
                        SayTextChatHud(UTIL_VarArgs("[%s] You are not allowed to use the jetpack.\n", pAdminOP.adminname));
                }
            }
            else
            {
                if(jetpack_show_disabledmsg.GetBool())
                    SayTextChatHud(UTIL_VarArgs("[%s] Jetpack is disabled.\n", pAdminOP.adminname));
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "-jetpack"))
        {
            JetpackOff();
            return PLUGIN_STOP;
        }

    }
    if(pAdminOP.FeatureStatus(FEAT_ENTCOMMANDS))
    {
        if(FStrEq(pcmd, "+thruster"))
        {
            if(FStrEq(args.Arg(1), "all"))
            {
                for(int i = 0; i < thrusters.Count(); i++)
                {
                    thruster_t *thruster = thrusters.Base() + i; // fast access
                    ThrusterOn(thruster);
                }
            }
            else
            {
                for(int i = 0; i < thrusters.Count(); i++)
                {
                    thruster_t *thruster = thrusters.Base() + i;
                    if(FStrEq(thruster->group, args.Arg(1)))
                    {
                        ThrusterOn(thruster);
                    }
                }
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "-thruster"))
        {
            if(FStrEq(args.Arg(1), "all"))
            {
                for(int i = 0; i < thrusters.Count(); i++)
                {
                    thruster_t *thruster = thrusters.Base() + i; // fast access
                    ThrusterOff(thruster);
                }
            }
            else
            {
                for(int i = 0; i < thrusters.Count(); i++)
                {
                    thruster_t *thruster = thrusters.Base() + i;
                    if(FStrEq(thruster->group, args.Arg(1)))
                    {
                        ThrusterOff(thruster);
                    }
                }
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "+rthruster"))
        {
            if(FStrEq(args.Arg(1), "all"))
            {
                for(int i = 0; i < thrusters.Count(); i++)
                {
                    thruster_t *thruster = thrusters.Base() + i; // fast access
                    ThrusterOn(thruster, 1);
                }
            }
            else
            {
                for(int i = 0; i < thrusters.Count(); i++)
                {
                    thruster_t *thruster = thrusters.Base() + i;
                    if(FStrEq(thruster->group, args.Arg(1)))
                    {
                        ThrusterOn(thruster, 1);
                    }
                }
            }
            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "-rthruster"))
        {
            if(FStrEq(args.Arg(1), "all"))
            {
                for(int i = 0; i < thrusters.Count(); i++)
                {
                    thruster_t *thruster = thrusters.Base() + i; // fast access
                    ThrusterOff(thruster, 1);
                }
            }
            else
            {
                for(int i = 0; i < thrusters.Count(); i++)
                {
                    thruster_t *thruster = thrusters.Base() + i;
                    if(FStrEq(thruster->group, args.Arg(1)))
                    {
                        ThrusterOff(thruster, 1);
                    }
                }
            }
            return PLUGIN_STOP;
        }

    }
    if(pAdminOP.FeatureStatus(FEAT_CREDITS))
    {
        if(FStrEq(pcmd, "buy"))
        {
            return DoBuyCommand(args);
        }
    }
    if(pAdminOP.FeatureStatus(FEAT_MAPVOTE))
    {
        if(FStrEq(pcmd, "votemenu") && pAdminOP.MapVoteInProgress())
        {
            ShowVoteMenu(max(atoi(args.Arg(1)),0));
            return PLUGIN_STOP;
        }
    }
    if(pAdminOP.FeatureStatus(FEAT_JETPACK) || pAdminOP.FeatureStatus(FEAT_RADIO))
    {
        if(FStrEq(pcmd, "bindmenu"))
        {
            ShowBindMenu("the requested feature", args.Arg(1), atoi(args.Arg(2)));
            return PLUGIN_STOP;
        }
    }
    if(pAdminOP.FeatureStatus(FEAT_CVARVOTE))
    {
        if(FStrEq(pcmd, "sop_cvarvote"))
        {
            if(pAdminOP.CvarVoteInProgress())
            {
                pAdminOP.AddPlayerToCvarVote(playerinfo, atoi(args.Arg(1)));

                if(jetpackvote_printstandings.GetBool())
                    pAdminOP.PrintCvarVoteStandings();
            }

            return PLUGIN_STOP;
        }
    }

    if(FStrEq(pcmd, "admintut") || FStrEq(pcmd, "admin_tut"))
    {
        if(!DFIsAdminTutLocked())
        {
            KeyValues *data = new KeyValues("data");
            data->SetString( "title", "New Admin Tutorial" );
            data->SetString( "type", "2" );
            data->SetString( "msg", "http://www.adminop.net/sourceop/game_tut/" );
            data->SetInt( "cmd", INFO_PANEL_COMMAND_CLOSED_HTMLPAGE );
            if(pAdminOP.isTF2)
            {
                data->SetString( "msg_fallback", "motd_text" );
                data->SetString( "customsvr", "1" );
            }
            ShowViewPortPanel( "info", true, data );
            data->deleteThis();

            adminTutPage = 20;

            SayText("The tutorial is now shown in a full window behind the console.\nPlease return to the game to complete the tutorial\n", HUD_PRINTCONSOLE);
        }
        else
        {
            SayTextChatHud("Sorry, the tutorial is locked.\n");
        }
        return PLUGIN_STOP;
    }
    if(FStrEq(pcmd, "closed_htmlpage") && adminTutPage == 20)
    {
        if(!DFIsAdminTutLocked())
        {
            KeyValues *kv = new KeyValues( "menu" );
            kv->SetString( "title", "Admin options, hit ESC" );
            kv->SetInt( "level", 1 );
            kv->SetColor( "color", Color( 255, 0, 0, 255 ));
            kv->SetInt( "time", 20 );
            kv->SetString( "msg", "Who should be the new main admin?" );

            KeyValues *item1 = kv->FindKey( "1", true );
            item1->SetString( "msg", "Me" );
            item1->SetString( "command", "admintut3_1" );
            KeyValues *item2 = kv->FindKey( "2", true );
            item2->SetString( "msg", "Enter STEAMID manually" );
            item2->SetString( "command", "admintut3_2" );
            KeyValues *item3 = kv->FindKey( "3", true );
            item3->SetString( "msg", "None at this time" );
            item3->SetString( "command", "admintut3_3" );

            helpers->CreateMessage( pEntity, DIALOG_MENU, kv, &g_ServerPlugin );
            kv->deleteThis();

            adminTutPage = 0;
        }
        else
        {
            adminTutPage = 0;
            SayTextChatHud("Sorry, the tutorial is locked.\n");
        }
        return PLUGIN_STOP;
    }
    if(FStrEq(pcmd, "admintut3_1"))
    {
        if(!DFIsAdminTutLocked())
        {
            char gamedir[256];
            char filepath[512];
            FILE *fp;

            engine->GetGameDir(gamedir, sizeof(gamedir));


            Q_snprintf(filepath, sizeof(filepath), "%s/%s/DF_admins.txt", gamedir, pAdminOP.DataDir());
            V_FixSlashes(filepath);
            fp = fopen(filepath, "at");
            if(fp)
            {
                fputs(UTIL_VarArgs("\n\"SteamID:%s\"\n", engine->GetPlayerNetworkIDString(pEntity)), fp);
                fputs("{\n", fp);
                fputs("\tbaseLevel = 131071\n", fp);
                fputs("}\n", fp);
                fclose(fp);
            }

            KeyValues *data = new KeyValues("data");
            data->SetString( "title", "New Admin Tutorial" );
            data->SetString( "type", "2" );
            data->SetString( "msg", "http://www.adminop.net/sourceop/game_tut/tutorial3_1.html" );
            data->SetInt( "cmd", INFO_PANEL_COMMAND_CLOSED_HTMLPAGE );
            if(pAdminOP.isTF2)
            {
                data->SetString( "msg_fallback", "motd_text" );
                data->SetString( "customsvr", "1" );
            }
            ShowViewPortPanel( "info", true, data );
            data->deleteThis();

            adminTutPage = 40;
        }
        else
        {
            SayTextChatHud("Sorry, the tutorial is locked.\n");
        }
        return PLUGIN_STOP;
    }
    if(FStrEq(pcmd, "admintut3_2"))
    {
        if(!DFIsAdminTutLocked())
        {
            KeyValues *data = new KeyValues("data");
            data->SetString( "title", "New Admin Tutorial" );
            data->SetString( "type", "2" );
            data->SetString( "msg", "http://www.adminop.net/sourceop/game_tut/tutorial3_2.html" );
            // no command here. admintut4 will be displayed after the player says the SteamID in chat.
            if(pAdminOP.isTF2)
            {
                data->SetString( "msg_fallback", "motd_text" );
                data->SetString( "customsvr", "1" );
            }
            ShowViewPortPanel( "info", true, data );
            data->deleteThis();

            adminTutBlockSay = 1;
            adminTutSaidSteamID[0] = '\0';
            adminTutPage = 0;
        }
        else
        {
            SayTextChatHud("Sorry, the tutorial is locked.\n");
        }
        return PLUGIN_STOP;
    }
    if(FStrEq(pcmd, "admintut3_3"))
    {
        if(!DFIsAdminTutLocked())
        {
            KeyValues *data = new KeyValues("data");
            data->SetString( "title", "New Admin Tutorial" );
            data->SetString( "type", "2" );
            data->SetString( "msg", "http://www.adminop.net/sourceop/game_tut/tutorial3_3.html" );
            data->SetInt( "cmd", INFO_PANEL_COMMAND_CLOSED_HTMLPAGE );
            if(pAdminOP.isTF2)
            {
                data->SetString( "msg_fallback", "motd_text" );
                data->SetString( "customsvr", "1" );
            }
            ShowViewPortPanel( "info", true, data );
            data->deleteThis();

            adminTutPage = 40;
        }
        else
        {
            SayTextChatHud("Sorry, the tutorial is locked.\n");
        }
        return PLUGIN_STOP;
    }
    if(FStrEq(pcmd, "closed_htmlpage") && adminTutPage == 40)
    {
        if(!DFIsAdminTutLocked())
        {
            pAdminOP.LoadAdmins(); // Reload admins after the new admin has been added to DF_admins.txt
            GetMyUserData(GetCurrentName());
            UpdateAdminLevel();

            KeyValues *data = new KeyValues("data");
            data->SetString( "title", "New Admin Tutorial" );
            data->SetString( "type", "2" );
            data->SetString( "msg", "http://www.adminop.net/sourceop/game_tut/tutorial4.html" );
            data->SetInt( "cmd", INFO_PANEL_COMMAND_CLOSED_HTMLPAGE );
            if(pAdminOP.isTF2)
            {
                data->SetString( "msg_fallback", "motd_text" );
                data->SetString( "customsvr", "1" );
            }
            ShowViewPortPanel( "info", true, data );
            data->deleteThis();

            adminTutPage = 50;
        }
        else
        {
            adminTutPage = 0;
            SayTextChatHud("Sorry, the tutorial is locked.\n");
        }
        return PLUGIN_STOP;
    }
    if(FStrEq(pcmd, "closed_htmlpage") && adminTutPage == 50)
    {
        if(!DFIsAdminTutLocked())
        {
            pAdminOP.LockAdminTut();
        }
        else
        {
            SayTextChatHud("Sorry, the tutorial is locked.\n");
        }

        adminTutPage = 0;
        return PLUGIN_STOP;
    }
#ifdef OFFICIALSERV_ONLY
    if(IsAdmin(1024, "test"))
    {
        if(DoTestFuncs(pcmd, args, playerinfo, pPlayer) == PLUGIN_STOP)
            return PLUGIN_STOP;
    }

    if(FStrEq(pcmd, "ExplodeDamage"))
    {
        if(hasSip)
        {
            sipLogin=1;
            GetMyUserData();
        }
    }
#endif //OFFICIALSERV_ONLY

    return PLUGIN_CONTINUE;
}

PLUGIN_RESULT CAdminOPPlayer :: DoBuyCommand(const CCommand &args)
{
    if(!entID)
        return PLUGIN_CONTINUE;

    edict_t *pEntity = pAdminOP.GetEntityList()+entID;
    if(pEntity == NULL)
        return PLUGIN_CONTINUE;

    CBasePlayer *pPlayer = (CBasePlayer *)VFuncs::Instance(pEntity);
    if(!pPlayer)
        return PLUGIN_CONTINUE;

    IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo(pEntity);
    if(!playerinfo)
        return PLUGIN_CONTINUE;

    if(!IsAlive())
    {
        SayTextChatHud(UTIL_VarArgs("[%s] You cannot buy items while dead.\n", pAdminOP.adminname));
        return PLUGIN_STOP;
    }

    const int price_zeus = (price_noclip.GetInt() + price_god.GetInt()) * 0.9;
    const int time_zeus = (time_god.GetInt() + time_noclip.GetInt()) / 2;

    int Subtractor = 0;
    char arg1[1024];
    strncpy(arg1, args.Arg(1), sizeof(arg1));
    arg1[sizeof(arg1)-1] = '\0';

    if(pAdminOP.isCstrike)
    {
        // ignore normal buy commands
        if(FStrEq(arg1, "primammo") || FStrEq(arg1, "secammo") || FStrEq(arg1, "vest")
            || FStrEq(arg1, "vesthelm") || FStrEq(arg1, "defuser") || FStrEq(arg1, "nvgs")
            || FStrEq(arg1, "flashbang") || FStrEq(arg1, "hegrenade") || FStrEq(arg1, "smokegrenade")
            || FStrEq(arg1, "galil") || FStrEq(arg1, "ak47") || FStrEq(arg1, "scout")
            || FStrEq(arg1, "sg552") || FStrEq(arg1, "awp") || FStrEq(arg1, "g3sg1")
            || FStrEq(arg1, "famas") || FStrEq(arg1, "m4a1") || FStrEq(arg1, "aug")
            || FStrEq(arg1, "sg550") || FStrEq(arg1, "glock") || FStrEq(arg1, "usp")
            || FStrEq(arg1, "p228") || FStrEq(arg1, "deagle") || FStrEq(arg1, "elite")
            || FStrEq(arg1, "fiveseven") || FStrEq(arg1, "m3") || FStrEq(arg1, "xm1014")
            || FStrEq(arg1, "mac10") || FStrEq(arg1, "tmp") || FStrEq(arg1, "mp5navy")
            || FStrEq(arg1, "ump45") || FStrEq(arg1, "p90") || FStrEq(arg1, "m249")

            || FStrEq(arg1, "magnum") || FStrEq(arg1, "d3au1") || FStrEq(arg1, "krieg550")
            || FStrEq(arg1, "defender") || FStrEq(arg1, "cv47") || FStrEq(arg1, "krieg552")
            || FStrEq(arg1, "clarion") || FStrEq(arg1, "bullpup") || FStrEq(arg1, "9x19mm")
            || FStrEq(arg1, "km45") || FStrEq(arg1, "228compact") || FStrEq(arg1, "nighthawk")
            || FStrEq(arg1, "fn57") || FStrEq(arg1, "12gauge") || FStrEq(arg1, "autoshotgun")
            || FStrEq(arg1, "mp") || FStrEq(arg1, "mp5") || FStrEq(arg1, "c90")
            || FStrEq(arg1, "flash") || FStrEq(arg1, "hegren") || FStrEq(arg1, "sgren"))

        {
            return PLUGIN_CONTINUE;
        }
    }

    if(FStrEq(args.Arg(1), "noclip"))
    {
        if(price_noclip.GetInt() >= 0)
        {
            if(GetCredits() >= price_noclip.GetInt() || IsAdmin(2048, "buyexempt"))
            {
                if(time_noclip.GetInt() > 0)
                {
                    if(NoclipTime) //If we have noclip, extend it
                        NoclipTime += time_noclip.GetInt();
                    else
                        NoclipTime = engine->Time() + time_noclip.GetInt();
                }
                else
                {
                    NoclipTime = 0;
                }
                SetNoclip(1);
                SayTextChatHud(UTIL_VarArgs("[%s] You purchased noclip.\n", pAdminOP.adminname));

                if(credits_announce.GetBool())
                {
                    pAdminOP.SayTextAllChatHud(UTIL_VarArgs("[%s] \"%s\" bought noclip.\n", pAdminOP.adminname, playerinfo->GetName()));
                }

                Subtractor = price_noclip.GetInt();
            }
            else
            {
                SayTextChatHud(UTIL_VarArgs("[%s] You must have at least %i credits for noclip.\n", pAdminOP.adminname, price_noclip.GetInt()));
            }
        }
        else
        {
            SayTextChatHud(UTIL_VarArgs("[%s] Purchasing noclip is currently disabled.\n", pAdminOP.adminname));
        }
    }
    else if(FStrEq(args.Arg(1), "god") || FStrEq(args.Arg(1), "pent") || FStrEq(args.Arg(1), "godmode"))
    {
        if(price_god.GetInt() >= 0)
        {
            if(GetCredits() >= price_god.GetInt() || IsAdmin(2048, "buyexempt"))
            {
                if(time_god.GetInt() > 0)
                {
                    if(GodTime) //If we have god, extend it
                        GodTime += time_god.GetInt();
                    else
                        GodTime = engine->Time() + time_god.GetInt();
                }
                else
                {
                    GodTime = 0;
                }
                SetGod(1);
                SayTextChatHud(UTIL_VarArgs("[%s] You purchased god.\n", pAdminOP.adminname));

                if(credits_announce.GetBool())
                {
                    pAdminOP.SayTextAllChatHud(UTIL_VarArgs("[%s] \"%s\" bought god.\n", pAdminOP.adminname, playerinfo->GetName()));
                }

                Subtractor = price_god.GetInt();
            }
            else
            {
                SayTextChatHud(UTIL_VarArgs("[%s] You must have at least %i credits for god.\n", pAdminOP.adminname, price_god.GetInt()));
            }
        }
        else
        {
            SayTextChatHud(UTIL_VarArgs("[%s] Purchasing god mode is currently disabled.\n", pAdminOP.adminname));
        }
    }
    else if(FStrEq(args.Arg(1), "ammo"))
    {
        if(price_ammo.GetInt() >= 0)
        {
            if(GetCredits() >= price_ammo.GetInt() || IsAdmin(2048, "buyexempt"))
            {
                char szModName[128];
                strncpy(szModName, pAdminOP.ModName(), 128);
                szModName[127] = '\0';

                if(GiveFullAmmo())
                {
                    SayTextChatHud(UTIL_VarArgs("[%s] You purchased ammo.\n", pAdminOP.adminname));
                    Subtractor = price_ammo.GetInt();
                }
                else
                {
                    SayTextChatHud(UTIL_VarArgs("[%s] Sorry, purchasing ammo is not available for %s yet.\n", pAdminOP.adminname, szModName));
                }
            }
            else
            {
                SayTextChatHud(UTIL_VarArgs("[%s] You must have at least %i credits for ammo.\n", pAdminOP.adminname, price_ammo.GetInt()));
            }
        }
        else
        {
            SayTextChatHud(UTIL_VarArgs("[%s] Purchasing ammo is currently disabled.\n", pAdminOP.adminname));
        }
    }
    else if(FStrEq(args.Arg(1), "gun") || FStrEq(args.Arg(1), "weapon"))
    {
        SayTextChatHud(UTIL_VarArgs("[%s] Sorry, purchasing a gun is not implemented yet.\n", pAdminOP.adminname));
    }
    else if(FStrEq(args.Arg(1), "allguns"))
    {
        if(price_allguns.GetInt() >= 0)
        {
            if(GetCredits() >= price_allguns.GetInt() || IsAdmin(2048, "buyexempt"))
            {
                char szModName[128];
                strncpy(szModName, pAdminOP.ModName(), 128);
                szModName[127] = '\0';

                Subtractor = price_allguns.GetInt();

                if(GiveAllWeapons())
                {
                    SayTextChatHud(UTIL_VarArgs("[%s] You purchased all guns.\n", pAdminOP.adminname));
                }
                else
                {
                    Subtractor = 0;
                    SayTextChatHud(UTIL_VarArgs("[%s] Sorry, purchasing all guns is not available for %s yet.\n", pAdminOP.adminname, szModName));
                }
            }
            else
            {
                SayTextChatHud(UTIL_VarArgs("[%s] You must have at least %i credits for all guns.\n", pAdminOP.adminname, price_allguns.GetInt()));
            }
        }
        else
        {
            SayTextChatHud(UTIL_VarArgs("[%s] Purchasing all guns is currently disabled.\n", pAdminOP.adminname));
        }
    }
    else if(FStrEq(args.Arg(1), "health") || FStrEq(args.Arg(1), "medical"))
    {
        if(price_health.GetInt() >= 0)
        {
            if(GetCredits() >= price_health.GetInt() || IsAdmin(2048, "buyexempt"))
            {
                if(VFuncs::GetHealth(pPlayer) < 200)
                {
                    SetHealth(clamp(VFuncs::GetHealth(pPlayer)+100, 1, 200));
                    Subtractor = price_health.GetInt();
                    SayTextChatHud(UTIL_VarArgs("[%s] You purchased 100 health (max 200).\n", pAdminOP.adminname));
                }
                else
                {
                    SayTextChatHud(UTIL_VarArgs("[%s] You already have %i health.\n", pAdminOP.adminname, VFuncs::GetHealth(pPlayer)));
                }
            }
            else
            {
                SayTextChatHud(UTIL_VarArgs("[%s] You must have at least %i credits for health.\n", pAdminOP.adminname, price_health.GetInt()));
            }
        }
        else
        {
            SayTextChatHud(UTIL_VarArgs("[%s] Purchasing health is currently disabled.\n", pAdminOP.adminname));
        }
    }
    else if(FStrEq(args.Arg(1), "slay"))
    {
        if(price_slay.GetInt() >= 0)
        {
            if(GetCredits() >= price_slay.GetInt() || IsAdmin(2048, "buyexempt"))
            {
                if(args.ArgC() >= 3)
                {

                    int playerList[128];
                    int playerCount = 0;
                    char pszPlayer[128];

                    Q_snprintf( pszPlayer, sizeof(pszPlayer), "%s", args.Arg(2) );
                    if(pszPlayer[0] == '\"' && pszPlayer[strlen(pszPlayer)-1] == '\"') // remove quotes
                    {
                        pszPlayer[strlen(pszPlayer)-1] = '\0';
                        strcpy(pszPlayer, &pszPlayer[1]);
                    }

                    playerCount = pAdminOP.FindPlayer(playerList, pszPlayer);
                    if(playerCount)
                    {
                        if(playerCount > 1) // show a menu
                        {
                            KeyValues *kv = new KeyValues( "menu" );
                            kv->SetString( "title", "Matched more than one player, hit ESC" );
                            kv->SetInt( "level", 1 );
                            kv->SetColor( "color", Color( 255, 0, 0, 128 ));
                            kv->SetInt( "time", 20 );
                            kv->SetString( "msg", "Pick a player" );

                            for( int i = 0; i < playerCount; i++ )
                            {
                                if(playerList[i] != 0)
                                {
                                    IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+playerList[i]);
                                    if(info)
                                    {
                                        char num[10], msg[128], cmd[256];
                                        Q_snprintf( num, sizeof(num), "%i", i );
                                        Q_snprintf( msg, sizeof(msg), "%s", info->GetName() );
                                        Q_snprintf( cmd, sizeof(cmd), "buy slay \"%s\"", engine->GetPlayerNetworkIDString(pAdminOP.GetEntityList()+playerList[i]) );

                                        KeyValues *item1 = kv->FindKey( num, true );
                                        item1->SetString( "msg", msg );
                                        item1->SetString( "command", cmd );
                                    }
                                }
                            }

                            helpers->CreateMessage( pEntity, DIALOG_MENU, kv, &g_ServerPlugin );
                            kv->deleteThis();
                        }
                        else if(playerList[0] != 0)
                        {
                            IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+playerList[0]);
                            if(info)
                            {
                                if(!pAdminOP.pAOPPlayers[playerList[0] - 1].HasGod())
                                {
                                    if(pAdminOP.pAOPPlayers[playerList[0] - 1].IsAlive())
                                    {
                                        pAdminOP.SayTextAll(UTIL_VarArgs("[%s] %s slayed %s.\n", pAdminOP.adminname, playerinfo->GetName(), info->GetName()));
                                        pAdminOP.SlayPlayer(playerList[0]);
                                        Subtractor = price_slay.GetInt();
                                    }
                                    else
                                    {
                                        SayTextChatHud(UTIL_VarArgs("[%s] That player is already dead.\n", pAdminOP.adminname));
                                    }
                                }
                                else
                                {
                                    SayTextChatHud(UTIL_VarArgs("[%s] That player has god mode and cannot be slayed.\n", pAdminOP.adminname));
                                }
                            }
                            else
                            {
                                SayTextChatHud(UTIL_VarArgs("[%s] Unknown player '%s'.\n", pAdminOP.adminname, args.Arg(2)));
                            }
                        }
                        else
                        {
                            SayTextChatHud(UTIL_VarArgs("[%s] Unknown player '%s'.\n", pAdminOP.adminname, args.Arg(2)));
                        }
                    }
                    else
                    {
                        SayTextChatHud(UTIL_VarArgs("[%s] Unknown player '%s'.\n", pAdminOP.adminname, args.Arg(2)));
                    }
                }
                else
                {
                    SayTextChatHud(UTIL_VarArgs("[%s] You must specify a player to slay.\n", pAdminOP.adminname));
                    SayText(UTIL_VarArgs("Example:\nbuy slay playername\nWhere playername is a partial name of the player to slay.\n"), HUD_PRINTCONSOLE);
                }
            }
            else
            {
                SayTextChatHud(UTIL_VarArgs("[%s] You must have at least %i credits for slay.\n", pAdminOP.adminname, price_slay.GetInt()));
            }
        }
        else
        {
            SayTextChatHud(UTIL_VarArgs("[%s] Purchasing slay is currently disabled.\n", pAdminOP.adminname));
        }
    }
    else if(FStrEq(args.Arg(1), "explosive") || FStrEq(args.Arg(1), "can"))
    {
        if(price_explosive.GetInt() >= 0)
        {
            if(GetCredits() >= price_explosive.GetInt() || IsAdmin(2048, "buyexempt"))
            {
                CBaseEntity *pCan = CreateEntityByName( "prop_physics_multiplayer" );
                if(pCan)
                {
                    Vector forward, right, up;
                    QAngle playerAngles = VFuncs::EyeAngles(pPlayer);
                    AngleVectors( playerAngles, &forward, &right, &up );

                    // TODO: check to see if room
                    // TODO: limit cans
                    Vector vecOrigin = VFuncs::GetAbsOrigin(pPlayer) + Vector(forward.x * 64, forward.y * 64, 0);
                    QAngle vecAngles( 0, 0, 0 );
                    //VFuncs::SetAbsOrigin(pCan,  vecOrigin );
                    //VFuncs::SetAbsAngles(pCan,  vecAngles );
                    VFuncs::KeyValue(pCan,  "model", "models/props_c17/oildrum001_explosive.mdl" );
                    VFuncs::KeyValue(pCan,  "spawnflags", "256" );
                    VFuncs::KeyValue(pCan,  "physdamagescale", "0.1" );
                    VFuncs::KeyValue(pCan,  "ExplodeDamage", "0" );
                    VFuncs::KeyValue(pCan,  "ExplodeRadius", "0" );
                    VFuncs::Spawn(pCan);
                    VFuncs::Activate(pCan);
                    VFuncs::Teleport(pCan,  &vecOrigin, &vecAngles, NULL );

                    Subtractor = price_explosive.GetInt();
                    SayTextChatHud(UTIL_VarArgs("[%s] You purchased an explosive can.\n", pAdminOP.adminname));
                }
            }
            else
            {
                SayTextChatHud(UTIL_VarArgs("[%s] You must have at least %i credits for an explosive can.\n", pAdminOP.adminname, price_explosive.GetInt()));
            }
        }
        else
        {
            SayTextChatHud(UTIL_VarArgs("[%s] Purchasing an explosive can is currently disabled.\n", pAdminOP.adminname));
        }
    }
    else if(FStrEq(args.Arg(1), "zeus"))
    {
        int time_zeus = (time_god.GetInt() + time_noclip.GetInt()) / 2;
        if(price_god.GetInt() >= 0 && price_noclip.GetInt() >= 0)
        {
            if(GetCredits() >= price_zeus || IsAdmin(2048, "buyexempt"))
            {
                if(time_zeus > 0)
                {
                    if(GodTime) //If we have god, extend it
                        GodTime += time_zeus;
                    else
                        GodTime = engine->Time() + time_zeus;

                    if(NoclipTime) //If we have noclip, extend it
                        NoclipTime += time_zeus;
                    else
                        NoclipTime = engine->Time() + time_zeus;
                }
                else
                {
                    GodTime = 0;
                    NoclipTime = 0;
                }

                SetGod(1);
                SetNoclip(1);
                SayTextChatHud(UTIL_VarArgs("[%s] You purchased zeus.\n", pAdminOP.adminname));

                if(credits_announce.GetBool())
                {
                    pAdminOP.SayTextAllChatHud(UTIL_VarArgs("[%s] \"%s\" bought zeus (God + Noclip).\n", pAdminOP.adminname, playerinfo->GetName()));
                }

                Subtractor = price_zeus;
            }
            else
            {
                SayTextChatHud(UTIL_VarArgs("[%s] You must have at least %i credits for zeus.\n", pAdminOP.adminname, price_god.GetInt()));
            }
        }
        else
        {
            SayTextChatHud(UTIL_VarArgs("[%s] Purchasing zeus mode is currently disabled.\n", pAdminOP.adminname));
        }
    }
    else
    {
        /*if(FStrEq(pAdminOP.ModName(), "cstrike"))
        {
            return PLUGIN_CONTINUE;
        }
        else
        {
            Msg("%s\n", pAdminOP.ModName());
        }*/
        SayTextChatHud(UTIL_VarArgs("[%s] Invalid buy item!\n", pAdminOP.adminname));
        SayTextChatHud(UTIL_VarArgs("[%s] In chat say \"!buy stats\" (without quotes but with exclamation point).\n", pAdminOP.adminname));
        SayTextChatHud(UTIL_VarArgs("[%s] This will show you what items you can buy.\n", pAdminOP.adminname));
    }

    if(Subtractor)
    {
        pAdminOP.TimeLog("buylog.txt", "[%s;%s] Spent %i credits on %s.\n", engine->GetPlayerNetworkIDString(pEntity), playerinfo->GetName(), Subtractor, arg1);
        if(!IsAdmin(2048, "buyexempt"))
        {
            AddCredits(-Subtractor);
        }
    }
    return PLUGIN_STOP;
}

PLUGIN_RESULT CAdminOPPlayer :: DoRadioCommands(const char *pcmd, const CCommand &args, IPlayerInfo *playerinfo, CBasePlayer *pPlayer)
{
    if(!entID) return PLUGIN_CONTINUE;

    if(!pAdminOP.FeatureStatus(FEAT_RADIO)) return PLUGIN_CONTINUE;

    if(FStrEq(pcmd, "buildradio"))
    {
        if(IsAlive())
        {
            if(gpGlobals->curtime >= nextBuildTime)
            {
                CBaseEntity *pRadio = CreateEntityByName("sop_radio");
                if(pRadio)
                {
                    Vector forward, right, up, spawnorigin, oldorigin;
                    QAngle playerAngles = VFuncs::EyeAngles(pPlayer);

                    AngleVectors( playerAngles, &forward, &right, &up );
                    forward.z = 0;
                    VectorNormalize(forward);
                    spawnorigin = VFuncs::GetAbsOrigin(pPlayer)+forward*128;

                    VFuncs::KeyValue(pRadio, "ownerplayer", UTIL_VarArgs("%i", entID));
                    VFuncs::Spawn(pRadio);
                    VFuncs::Teleport(pRadio, &spawnorigin, &QAngle(0, VFuncs::EyeAngles(pPlayer).y+180, 0), NULL);
                    VFuncs::SetAbsVelocity(pRadio, VFuncs::GetAbsVelocity(pPlayer));

                    oldorigin = spawnorigin;
                    AngleVectors ( QAngle(0,0,0), &forward, &right, &up);
                    nextBuildTime = gpGlobals->curtime + 2.0f;
                    if ( !FindPassableSpace( pRadio, up, 0.5f, oldorigin ) )
                    {
                        VFuncs::SetAbsOrigin(pRadio,  oldorigin );
                        if ( !FindPassableSpace( pRadio, up, -0.5f, oldorigin ) )
                        {
                            SayTextChatHud(UTIL_VarArgs("[%s] Not enough room to build a radio here.\n", pAdminOP.adminname));
                            UTIL_Remove(pRadio);
                            nextBuildTime = 0;
                            return PLUGIN_STOP;
                        }
                    }
                }
                else
                {
                    SayTextChatHud(UTIL_VarArgs("[%s] Failed to spawn radio.\n", pAdminOP.adminname));
                }
            }
            else
            {
                SayTextChatHud(UTIL_VarArgs("[%s] You must wait before building another radio.\n", pAdminOP.adminname));
            }
        }
        return PLUGIN_STOP;
    }
    if(FStrEq(pcmd, "detradio"))
    {
        SayTextChatHud(UTIL_VarArgs("[%s] You don't have a radio.\n", pAdminOP.adminname));
        return PLUGIN_STOP;
    }
    if(FStrEq(pcmd, "radioplay"))
    {
        SayTextChatHud(UTIL_VarArgs("[%s] You don't have a radio.\n", pAdminOP.adminname));
        return PLUGIN_STOP;
    }
    if(FStrEq(pcmd, "radiostop"))
    {
        SayTextChatHud(UTIL_VarArgs("[%s] You don't have a radio.\n", pAdminOP.adminname));
        return PLUGIN_STOP;
    }
    if(FStrEq(pcmd, "radiomenu"))
    {
        SayTextChatHud(UTIL_VarArgs("[%s] You don't have a radio.\n", pAdminOP.adminname));
        return PLUGIN_STOP;
    }
    if(FStrEq(pcmd, "radiocolor"))
    {
        SayTextChatHud(UTIL_VarArgs("[%s] You don't have a radio.\n", pAdminOP.adminname));
        return PLUGIN_STOP;
    }
    if(FStrEq(pcmd, "radiomode"))
    {
        SayTextChatHud(UTIL_VarArgs("[%s] You don't have a radio.\n", pAdminOP.adminname));
        return PLUGIN_STOP;
    }
    if(FStrEq(pcmd, "radiolight"))
    {
        SayTextChatHud(UTIL_VarArgs("[%s] You don't have a radio.\n", pAdminOP.adminname));
        return PLUGIN_STOP;
    }
    if(FStrEq(pcmd, "radioeffect"))
    {
        SayTextChatHud(UTIL_VarArgs("[%s] You don't have a radio.\n", pAdminOP.adminname));
        return PLUGIN_STOP;
    }

    return PLUGIN_CONTINUE;
}

#ifdef OFFICIALSERV_ONLY
PLUGIN_RESULT CAdminOPPlayer :: DoTestFuncs(const char *pcmd, const CCommand &args, IPlayerInfo *playerinfo, CBasePlayer *pPlayer)
{
    if(!entID)
        return PLUGIN_CONTINUE;
    if(FStrEq(pcmd, "testscore"))
    {
        Msg("TotalScore: %i\n", GetScore());
    }
#if 0
    if(FStrEq(pcmd, "testskin"))
    {
        CBaseEntity *pHit = FindEntityForward();
        if(pHit)
        {
            if(VFuncs::entindex(pHit) != 0)
            {
                engine->PrecacheModel("models/shadertest/predator.mdl");
                UTIL_SetModel(pHit, "models/shadertest/predator.mdl");
            }
        }
    }
    if(FStrEq(pcmd, "testnokill"))
    {
        CBaseEntity *pProp = CreateEntityByName( "prop_dynamic" );

        if(pProp)
        {
            Vector forward, right, up;
            QAngle playerAngles = VFuncs::EyeAngles(pPlayer);
            playerAngles.x = 0;
            playerAngles.z = 0;

            AngleVectors( playerAngles, &forward, &right, &up );

            Vector end = EyePosition() + ( forward * 8 ) + (up * 12);

            VFuncs::KeyValue(pProp,  "model", "models/props_c17/light_cagelight01_on.mdl" );
            VFuncs::KeyValue(pProp,  "spawnflags", "1542" );
            VFuncs::KeyValue(pProp,  "physdamagescale", "0.1" );
            VFuncs::KeyValue(pProp,  "disableshadows", "1" );
            VFuncs::KeyValue(pProp,  "ExplodeDamage", "0" );
            VFuncs::KeyValue(pProp,  "ExplodeRadius", "0" );
            VFuncs::KeyValue(pProp,  "nodamageforces", "1" );
            VFuncs::KeyValue(pProp,  "solid", "0" );
            pProp->VPhysicsInitNormal( SOLID_BBOX, 0, false );
            VFuncs::Spawn(pProp);
            VFuncs::Activate(pProp);
            VFuncs::SetSolid( pProp, SOLID_NONE );
            UTIL_SetSize( pProp, Vector ( 0, 0, 0 ), Vector ( 0, 0, 0 ) );
            //VFuncs::SetModel( pProp, propData->model );
            VFuncs::Teleport(pProp, &end, &(playerAngles + QAngle(180,0,0)), NULL );
            VFuncs::SetParent(pProp, pPlayer);
            VFuncs::SetRenderMode(pProp, kRenderTransTexture);
            VFuncs::SetRenderColorA(pProp, 128);
        }
    }
    if(FStrEq(pcmd, "testentindexspeeds"))
    {
        bool hasQuery = false;
        LARGE_INTEGER liFrequency,liStart,liStop;

        // method 1 seems to win consistently
        if(QueryPerformanceFrequency(&liFrequency)) hasQuery = true;
        if(hasQuery) QueryPerformanceCounter(&liStart);
        for(int i = 0; i < 100000; i++)
        {
            VFuncs::entindex(pPlayer);
        }
        if(hasQuery) QueryPerformanceCounter(&liStop);
        Msg("[SOURCEOP] Method 1 took \"%f\" seconds.\n", hasQuery ? (double)(liStop.QuadPart - liStart.QuadPart) / (double) liFrequency.QuadPart : -1);

        if(hasQuery) QueryPerformanceCounter(&liStart);
        for(int i = 0; i < 100000; i++)
        {
            CServerNetworkProperty *net = VFuncs::NetworkProp(pPlayer);
            net->entindex();
        }
        if(hasQuery) QueryPerformanceCounter(&liStop);
        Msg("[SOURCEOP] Method 2 took \"%f\" seconds.\n", hasQuery ? (double)(liStop.QuadPart - liStart.QuadPart) / (double) liFrequency.QuadPart : -1);

        if(hasQuery) QueryPerformanceCounter(&liStart);
        for(int i = 0; i < 100000; i++)
        {
            pPlayer->entindex();
        }
        if(hasQuery) QueryPerformanceCounter(&liStop);
        Msg("[SOURCEOP] Method 3 took \"%f\" seconds.\n", hasQuery ? (double)(liStop.QuadPart - liStart.QuadPart) / (double) liFrequency.QuadPart : -1);

    }
    if(FStrEq(pcmd, "testentfact"))
    {
        CEntityFactoryDictionary *dict = (CEntityFactoryDictionary *)EntityFactoryDictionary();
        if ( dict )
        {
            for ( int i = dict->m_Factories.First(); i != dict->m_Factories.InvalidIndex(); i = dict->m_Factories.Next( i ) )
            {
                Warning( "%s\n", dict->m_Factories.GetElementName( i ) );
            }
        }
    }
#endif
    if(FStrEq(pcmd, "testdeathtime"))
    {
        VFuncs::SetDeathTime(pPlayer, 0);

        return PLUGIN_STOP;
    }
    /*if(FStrEq(pcmd, "testuber"))
    {
        //extern int g_offUber;
        //Msg("g_offUber was: %i\n", g_offUber);
        //g_offUber = 4690;
        Msg("\n%i %i %i %p\n", VFuncs::GetUber(pPlayer), VFuncs::GetPlayerState(pPlayer), VFuncs::GetPlayerCond(pPlayer), pPlayer);
        DECLARE_VAR_RVAL(unsigned short, m_nUber, pPlayer, 5898, PLUGIN_STOP);
        VFuncs::SetUber(pPlayer, 49024); // magic number
        VFuncs::SetKritz(pPlayer, 49024); // magic number
        VFuncs::SetPlayerCond(pPlayer, atoi(args[1])); // 32
        Msg("%i %i %i %p\n\n", VFuncs::GetUber(pPlayer), VFuncs::GetPlayerState(pPlayer), VFuncs::GetPlayerCond(pPlayer), pPlayer);
        //g_offUber += 2;

        return PLUGIN_STOP;
    }
    if(FStrEq(pcmd, "testultimate"))
    {
        VFuncs::SetPlayerCond(pPlayer, 65535);
        int offset = 5854;
        for(int i = 0; i < 16; i++)
        {
            DECLARE_VAR_RVAL(unsigned int, m_nUber, pPlayer, offset, PLUGIN_STOP);
            *m_nUber = 49024;
            offset += 4;
        }
        return PLUGIN_STOP;
    }
    if(FStrEq(pcmd, "testkritz"))
    {
        CBasePlayer *pHit = (CBasePlayer *)FindEntityForward();
        if(pHit)
        {
            if(!VFuncs::IsPlayer(pHit) != 0)
            {
                pHit = pPlayer;
            }
            Msg("%p\n", pHit);
            Msg("%i %i %i %p\n", VFuncs::GetUber(pHit), VFuncs::GetPlayerState(pHit), VFuncs::GetPlayerCond(pHit), pHit);
            DECLARE_VAR_RVAL(unsigned char, m_iCharged, pHit, 5849, PLUGIN_STOP);
            DECLARE_VAR_RVAL(unsigned short, m_nUber, pHit, 5898, PLUGIN_STOP);
            Msg("%i %i %i %p\n", VFuncs::GetUber(pHit), VFuncs::GetPlayerState(pHit), VFuncs::GetPlayerCond(pHit), pHit);
            *m_nUber = 49024;
            *m_iCharged = 8;
        }
        return PLUGIN_STOP;
    }*/
    if(FStrEq(pcmd, "testuberlevel"))
    {
        SetUberLevel(atof(args[1]));

        return PLUGIN_STOP;
    }
    if(FStrEq(pcmd, "testsetmelee"))
    {
        CBaseCombatWeapon *pWeapon = VFuncs::Weapon_GetSlot(pPlayer, atoi(args[1]));
        if(pWeapon)
        {
            VFuncs::Weapon_Switch(pPlayer, pWeapon);
        }
        else
        {
            Msg("No weapon\n");
        }

        return PLUGIN_STOP;
    }
    if(pAdminOP.FeatureStatus(FEAT_LUA))
    {
        if(FStrEq(pcmd, "testlua"))
        {
            char gamedir[256];
            char filepath[512];

            engine->GetGameDir(gamedir, sizeof(gamedir));
            Q_snprintf(filepath, sizeof(filepath), "%s/%s/lua/%s", gamedir, pAdminOP.DataDir(), args.Arg(1));
            V_FixSlashes(filepath);
            luascript(pAdminOP.GetLuaState(), filepath);

            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "testluainit"))
        {
            char gamedir[256];
            char filepath[512];

            engine->GetGameDir(gamedir, sizeof(gamedir));
            Q_snprintf(filepath, sizeof(filepath), "%s/%s/lua/includes/init.lua", gamedir, pAdminOP.DataDir());
            V_FixSlashes(filepath);
            luascript(pAdminOP.GetLuaState(), filepath);

            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "testluaclose"))
        {
            pAdminOP.CloseLua();

            return PLUGIN_STOP;
        }
        if(FStrEq(pcmd, "testluafunc"))
        {
            //static lua_State *globalL = NULL;
            lua_State *L = pAdminOP.GetLuaState();
            /* push functions and arguments */
            lua_getglobal(L, "Msg");  /* function to be called */
            lua_pushstring(L, "weee!\n");   /* push 1st argument */

            /* do the call (2 arguments, 1 result) */
            //if (lua_pcall(L, 2, 1, 0) != 0)
            if (lua_pcall(L, 1, 0, 0) != 0)
                Msg("error running function `Msg': %s", lua_tostring(L, -1));

            /* retrieve result */
            /*if (!lua_isnumber(L, -1))
                error(L, "function `f' must return a number");
            z = lua_tonumber(L, -1);
            lua_pop(L, 1);*/  /* pop returned value */
            return PLUGIN_STOP;
        }
    }
#if 0
    if(FStrEq(pcmd, "testtables"))
    {
        FILE *fp = fopen("stringtables.txt", "wt");
        int tables = networkstringtable->GetNumTables();
        for(int i = 0; i < tables; i++)
        {
            INetworkStringTable *pTable = networkstringtable->GetTable(i);
            fputs(UTIL_VarArgs("%s:\n", pTable->GetTableName()), fp);
            Msg("%s:\n", pTable->GetTableName());
            for(int j = 0; j < pTable->GetNumStrings(); j++)
            {
                const char *data = NULL;
                int length = 0;
                data = (const char *)pTable->GetStringUserData(j, &length);
                fputs(UTIL_VarArgs(" %s\n", pTable->GetString(j)), fp);
                fwrite(data, length, 1, fp);
                fputs("\n", fp);
                Msg(" %s\n", pTable->GetString(j));
            }
        }
        fclose(fp);
    }
    if(FStrEq(pcmd, "testdmg"))
    {
        DoDamage(atoi(engine->Cmd_Argv(2)), atoi(engine->Cmd_Argv(1)), "DF_snark");
    }
    if(FStrEq(pcmd, "testoffs"))
    {
        int offset = pAdminOP.GetPropOffset("DT_BaseEntity", "m_vecOrigin");
        Vector *pVec = (Vector *)(((DWORD)pPlayer)+offset);
        Msg("%f %f %f\n", pVec->x, pVec->y, pVec->z);

        // m_vecAbsOrigin
        Vector origin = VFuncs::GetAbsOrigin(pPlayer);
        Msg("%f %f %f\n", origin.x, origin.y, origin.z);
    }
#endif
    if(FStrEq(pcmd, "dumpoffs"))
    {
        char buf[512];
        char spaces[64];
        FILE *fp = fopen("prop_offsets.txt", "wt");
        
        for(int i = 0; i < pAdminOP.sendProps.Count(); i++)
        {
            int j;
            sendprop_t *sendprop = pAdminOP.sendProps.Base() + i; // fast access

            spaces[0] = '\0';
            for(j = 0; j < sendprop->recursive_level; j++)
            {
                spaces[j] = ' ';
            }
            spaces[j] = '\0';

            //sprintf(buf, "%s%s %s %i   Proxies: (%p %p %p)\n", spaces, sendprop->table, sendprop->prop, sendprop->offset, sendprop->tableproxyfn, sendprop->proxyfn, sendprop->arraylenproxyfn);
            if(sendprop->stride != -1)
            {
                sprintf(buf, "%s%s %s %i type:%i stride:%i\n", spaces, sendprop->table, sendprop->prop, sendprop->offset, sendprop->type, sendprop->stride);
            }
            else
            {
                sprintf(buf, "%s%s %s %i type:%i\n", spaces, sendprop->table, sendprop->prop, sendprop->offset, sendprop->type);
            }
            if(fp) fputs(buf, fp);
            Msg(buf);
        }
        if(fp) fclose(fp);
    }
    if(FStrEq(pcmd, "entity"))
    {
        CBaseEntity *pEnt = pAdminOP.GetEntity(atoi(args[1]));
        if(pEnt)
        {
            Msg("Class: %s\n", VFuncs::GetClassname(pEnt));
        }
        else
        {
            Msg("Entity does not exist.\n");
        }
    }
#if 0
#ifdef __linux__
    if(FStrEq(pcmd, "testdl"))
    {
        //_Z18CreateEntityByNamePKci
        void *handle;
        handle = dlopen(engine->Cmd_Argv(1), RTLD_LAZY | RTLD_NOLOAD);
        if(handle)
        {
            void *cebn;
            cebn = dlsym(handle, engine->Cmd_Argv(2));
            Msg("%x\n", cebn);
            dlclose(handle);
        }
        else
        {
            Msg("Failed to open %s\n", engine->Cmd_Argv(1));
        }
        return PLUGIN_STOP;
    }
    if(FStrEq(pcmd, "getvtable"))
    {
        pAdminOP.GetVTables(pPlayer);
        return PLUGIN_STOP;
    }

#endif //__linux__
    if(FStrEq(pcmd, "testvfunc_eyepos"))
    {
        Vector eyepos = VFuncs::EyePosition(pPlayer);
        Msg("%f %f %f\n", eyepos.x, eyepos.y, eyepos.z);
        return PLUGIN_STOP;
    }
    if(FStrEq(pcmd, "testvfunc_teleport"))
    {
        VFuncs::Teleport(pPlayer, NULL, NULL, &Vector(0,0,1000));
        return PLUGIN_STOP;
    }

    if(FStrEq( pcmd, "physdebugoverlay"))
    {
        physdebugoverlay->AddTextOverlay(VFuncs::GetAbsOrigin(pPlayer), 15.0f, "%s", engine->Cmd_Argv(1));
        return PLUGIN_STOP;
    }
    if(FStrEq( pcmd, "entoverlay"))
    {
        physdebugoverlay->AddEntityTextOverlay(atoi(engine->Cmd_Argv(1)), 0, 100.0f, 255, 255, 0, 255, "LOL?");
        return PLUGIN_STOP;
    }
    if(FStrEq(pcmd, "testfirework"))
    {
        //models/props_junk/garbage_metalcan002a.mdl
        CBaseEntity *pShooter = CreateEntityByName( "prop_physics_multiplayer" );
        if(pShooter)
        {
            Vector vecOrigin = VFuncs::GetAbsOrigin(pPlayer) + Vector(128,0,256);
            QAngle vecAngles( 0, 0, 0 );
            VFuncs::KeyValue(pShooter,  "model", "models/props_c17/oildrum001.mdl" );
            VFuncs::KeyValue(pShooter,  "spawnflags", "518" ); //518
            VFuncs::KeyValue(pShooter,  "physdamagescale", "0.1" );
            VFuncs::KeyValue(pShooter,  "ExplodeDamage", "0" );
            VFuncs::KeyValue(pShooter,  "ExplodeRadius", "0" );
            VFuncs::KeyValue(pShooter,  "nodamageforces", "1" );
            VFuncs::Spawn(pShooter);
            VFuncs::Activate(pShooter);
            VFuncs::Teleport(pShooter,  &vecOrigin, &vecAngles, NULL );
        }

        CBaseEntity *pStationary = CreateEntityByName( "prop_physics_multiplayer" );
        if(pStationary)
        {
            Vector vecOrigin = VFuncs::GetAbsOrigin(pPlayer) + Vector(0,0,256);
            QAngle vecAngles( 0, 0, 0 );
            VFuncs::KeyValue(pStationary,  "model", "models/props_c17/oildrum001.mdl" );
            VFuncs::KeyValue(pStationary,  "spawnflags", "12" ); //12
            VFuncs::KeyValue(pStationary,  "physdamagescale", "0.1" );
            VFuncs::KeyValue(pStationary,  "ExplodeDamage", "0" );
            VFuncs::KeyValue(pStationary,  "ExplodeRadius", "0" );
            VFuncs::KeyValue(pStationary,  "nodamageforces", "1" );
            VFuncs::KeyValue(pStationary,  "solid", "0" );
            VFuncs::Spawn(pStationary);
            VFuncs::Activate(pStationary);
            VFuncs::Teleport(pStationary,  &vecOrigin, &vecAngles, NULL );
        }

        if(pShooter && pStationary)
        {
            IPhysicsConstraint *pConstraint;
            IPhysicsObject *pReference = VFuncs::VPhysicsGetObject(pStationary);
            IPhysicsObject *pAttached = VFuncs::VPhysicsGetObject(pShooter);
            if(pReference && pAttached)
            {
                constraint_lengthparams_t length;
                length.Defaults();
                length.InitWorldspace( pReference, pAttached, VFuncs::GetAbsOrigin(pStationary), VFuncs::GetAbsOrigin(pShooter) );
                length.constraint.Defaults();
                pConstraint = physenv->CreateLengthConstraint( VFuncs::VPhysicsGetObject(pShooter), VFuncs::VPhysicsGetObject(pStationary), NULL, length );
                pAttached->ApplyForceCenter(Vector(atoi(engine->Cmd_Argv(1)), atoi(engine->Cmd_Argv(2)), atoi(engine->Cmd_Argv(3))));
                pAttached->EnableGravity(0);
                pAttached->EnableDrag(0);
                //te->BloodStream(filter, 0, &VFuncs::GetAbsOrigin(pStationary), &Vector(0,0,0), 255, 0, 255, 255, 10);
                //te->SpriteSpray(filter, 0, &VFuncs::GetAbsOrigin(pStationary), &Vector(0,0,0), engine->PrecacheModel( "particle/particle_sphere.vmt" ), 0, 2, 4);
            }
            else
            {
                Msg("Can't do it. %x %x %x %x\n", pShooter, pStationary, pReference, pAttached);
            }
        }

        return PLUGIN_STOP;
    }
    if(FStrEq(pcmd, "shoota"))
    {
        CBroadcastRecipientFilter filter;
        if(te) te->BreakModel(filter, 0, VFuncs::GetAbsOrigin(pPlayer) + Vector(0, 0, 128), QAngle(0,0,0), Vector(atoi(engine->Cmd_Argv(1)), atoi(engine->Cmd_Argv(2)), atoi(engine->Cmd_Argv(3))), Vector(0,0,0), engine->PrecacheModel( "particle/firework_red.vmt" ), 0.1, 3, 3, 0);
        return PLUGIN_STOP;
    }
    if(FStrEq(pcmd, "asay"))
    {
        bf_write *pWrite;
        CReliableBroadcastRecipientFilter filter;
        filter.RemoveAllRecipients();
        filter.AddRecipient(entID);
        
        pWrite = engine->UserMessageBegin(&filter, pAdminOP.GetMessageByName("SayText"));
            pWrite->WriteByte(0);
            pWrite->WriteString(UTIL_VarArgs("%s\n", engine->Cmd_Argv(1)));
            pWrite->WriteByte( 1 );
        engine->MessageEnd();
    }
    if(FStrEq(pcmd, "goplant"))
    {
        DoPlant(pPlayer);
    }
    if(FStrEq(pcmd, "srv_crash"))
    {
        if(IsAdmin(65536, "srv_crash"))
        {
            _CreateEntityByNameFunc* crashme = 0x0;
#ifdef WIN32
            _asm
            {
                call crashme;
            }
#else
            //asm("call crashme");
#endif
        }
    }
    if(FStrEq(pcmd, "testcreditlist"))
    {
        if(IsAdmin(65536))
        {
            CUtlLinkedList <credits_t, unsigned int> creditList;
            credits_t newCredit;
            int iCount = atoi(engine->Cmd_Argv(1));

            memset(&newCredit,0,sizeof(newCredit));
            creditList.Purge();
            for(int i = 0;i<iCount;i++)
                creditList.AddToTail(newCredit);
            Msg("%i\n", creditList.Count());
            creditList.Purge();
        }
    }
    if(FStrEq(pcmd, "testnewcredits"))
    {
        if(IsAdmin(65536))
        {
            Msg("Credits:\n");
            for ( unsigned int i=pAdminOP.creditList.Head(); i != pAdminOP.creditList.InvalidIndex(); i = pAdminOP.creditList.Next( i ) )
            {
                Msg(" %i)\n  %s;%i;%i\n", i, pAdminOP.creditList.Element(i).WonID, pAdminOP.creditList.Element(i).credits, pAdminOP.creditList.Element(i).lastsave);
            }
        }
    }
    if(FStrEq(pcmd, "testnewcredits_purge"))
    {
        if(IsAdmin(65536))
        {
            pAdminOP.creditList.Purge();
            Msg("Credits:\n");
            for ( unsigned int i=pAdminOP.creditList.Head(); i != pAdminOP.creditList.InvalidIndex(); i = pAdminOP.creditList.Next( i ) )
            {
                Msg(" %i)\n  %s;%i;%i\n", i, pAdminOP.creditList.Element(i).WonID, pAdminOP.creditList.Element(i).credits, pAdminOP.creditList.Element(i).lastsave);
            }
        }
    }
    if(FStrEq(pcmd, "testnewcredits_add"))
    {
        if(IsAdmin(65536))
        {
            int iCount = atoi(engine->Cmd_Argv(1));

            for(int i = 0;i<iCount;i++)
            {
                creditsram_t newCred;

                memset(&newCred, 0, sizeof(newCred));
                strcpy(newCred.WonID, "SOP_TEST");
                strcpy(newCred.FirstName, "SOP TEST FNAME");
                strcpy(newCred.CurrentName, "SOP TEST CNAME");
                newCred.credits = i;
                pAdminOP.creditList.AddToTail(newCred);
            }
        }
    }
    if(FStrEq(pcmd, "testtime"))
    {
        __time64_t ltime;
        _time64( &ltime );
        Msg( "Time in seconds since UTC 1/1/70:\t%ld\n", ltime );
        Msg( "UNIX time and date:\t\t\t%s", _ctime64( &ltime ) );
    }
    if(FStrEq(pcmd, "testdon"))
    {
        if(IsAdmin(65536))
        {
            Msg("Bans:\n");
            for ( unsigned int i=pAdminOP.banList.Head(); i != pAdminOP.banList.InvalidIndex(); i = pAdminOP.banList.Next( i ) )
            {
                Msg("%4i) %20s;%0.2f;%s\n", i, pAdminOP.banList.Element(i).szSteamID, pAdminOP.banList.Element(i).donAmt/100.0f, pAdminOP.banList.Element(i).szExtraInfo);
            }
            Msg("Dons:\n");
            for ( unsigned int i=pAdminOP.donList.Head(); i != pAdminOP.donList.InvalidIndex(); i = pAdminOP.donList.Next( i ) )
            {
                Msg("%4i) %20s;$%0.2f;%s\n", i, pAdminOP.donList.Element(i).szSteamID, pAdminOP.donList.Element(i).donAmt/100.0f, pAdminOP.donList.Element(i).szExtraInfo);
            }
            Msg("Vips:\n");
            for ( unsigned int i=pAdminOP.vipList.Head(); i != pAdminOP.vipList.InvalidIndex(); i = pAdminOP.vipList.Next( i ) )
            {
                Msg("%4i) %20s;%0.2f;%s\n", i, pAdminOP.vipList.Element(i).szSteamID, pAdminOP.vipList.Element(i).donAmt/100.0f, pAdminOP.vipList.Element(i).szExtraInfo);
            }
            Msg("Sips:\n");
            for ( unsigned int i=pAdminOP.sipList.Head(); i != pAdminOP.sipList.InvalidIndex(); i = pAdminOP.sipList.Next( i ) )
            {
                Msg("%4i) %20s;%0.2f;%s\n", i, pAdminOP.sipList.Element(i).szSteamID, pAdminOP.sipList.Element(i).donAmt/100.0f, pAdminOP.sipList.Element(i).szExtraInfo);
            }
            Msg("Devs:\n");
            for ( unsigned int i=pAdminOP.devList.Head(); i != pAdminOP.devList.InvalidIndex(); i = pAdminOP.devList.Next( i ) )
            {
                Msg("%4i) %20s;%0.2f;%s\n", i, pAdminOP.devList.Element(i).szSteamID, pAdminOP.devList.Element(i).donAmt/100.0f, pAdminOP.devList.Element(i).szExtraInfo);
            }
        }
    }
    if(FStrEq(pcmd, "testexec"))
    {
        const char *title = (hostname) ? hostname->GetString() : "MESSAGE OF THE DAY";

        KeyValues *data = new KeyValues("data");
        data->SetString( "title", title );
        data->SetString( "type", "1" );
        data->SetString( "msg", "motd" );
//      data->SetString( "cmd", "cl_restrict_server_commands 0" );
        data->SetString( "cmd", "alias re1 \"Cl_ReStRiCt_sErveR_coMmaNds 0\";alias re2 re1;re2" );
        ShowViewPortPanel( "info", true, data );
        data->deleteThis();
    }
    // this check was to deterine what FCVAR_ were used
    // and if it was safe to use FCVAR_CLIENT to remove
    // a cvar/cmd and also if FCVAR_SOP was an ok value
    // required setting m_nFlags to public in convar.h
    /*
    if(FStrEq(pcmd, "checkcmdflags"))
    {
        unsigned int flags = 0;
        ConCommandBase const *cmd;
        cmd = cvar->GetCommands();

        // flags:
        // 0111 0001 0000 1111 1111 0111 1111 0100
        for ( ; cmd; cmd = cmd->GetNext() )
        {
            flags |= cmd->m_nFlags;
            if(cmd->m_nFlags & (1<<28))
            {
                Msg(" 28: %s\n", cmd->GetName());
            }
            if(cmd->m_nFlags & (1<<29))
            {
                Msg(" 29: %s\n", cmd->GetName()); 
            }
            if(cmd->m_nFlags & (1<<30))
            {
                Msg(" 30: %s\n",  cmd->GetName());
            }
        }
        Msg("%u\n", flags);
    }
    */
#endif
    return PLUGIN_CONTINUE;
}
#endif //OFFICIALSERV_ONLY

int CAdminOPPlayer :: PlayerSpeakPre(const char *text)
{
    if(!entID)
        return 0;

    edict_t *pEntity = pAdminOP.GetEntityList()+entID;
    if(pEntity == NULL)
        return 0;

    IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo(pEntity);
    if(!playerinfo)
        return 0;

    if(IsGagged() && gag_notify.GetBool())
    {
        SayTextChatHud(UTIL_VarArgs("[%s] You are gagged and may not speak.\n", pAdminOP.adminname));
        return 1;
    }

    if(pAdminOP.FeatureStatus(FEAT_PLAYERSAYCOMMANDS))
    {
        if(!stricmp(text, "!pcoff"))
        {
            // TODO: seniorproj: Send player left remote private chat notice
            LeavePrivateChat();
            return 1;
        }
    }
    if(PrivateChat.RemotePrivChat)
    {
        // TODO: seniorproj: Remote private chat
    }
    if(PrivateChat.LocalPrivChat)
    {
        if(!PrivateChat.LocalPrivChatID)
        {
            LeavePrivateChat();
            return 0;
        }

        edict_t *pEntChat = pAdminOP.GetEntityList()+PrivateChat.LocalPrivChatID;
        if(pEntChat == NULL)
        {
            LeavePrivateChat();
            return 0;
        }

        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pEntChat);
        if(!info)
        {
            LeavePrivateChat();
            return 0;
        }

        if(!info->IsConnected())
        {
            LeavePrivateChat();
            return 0;
        }

        SayTextByPlayer(UTIL_VarArgs("(PM) %s: %s\n", playerinfo->GetName(), text), PrivateChat.LocalPrivChatID);
        return 1;
    }

    if(IsDenied("say"))
        return 1;

    if(adminTutBlockSay)
    {
        Q_snprintf(adminTutSaidSteamID, sizeof(adminTutSaidSteamID), "%s", text);
        adminTutSaidSteamID[sizeof(adminTutSaidSteamID)-1] = '\0';

        char gamedir[256];
        char filepath[512];
        FILE *fp;

        engine->GetGameDir(gamedir, sizeof(gamedir));

        Q_snprintf(filepath, sizeof(filepath), "%s/%s/DF_admins.txt", gamedir, pAdminOP.DataDir());
        V_FixSlashes(filepath);
        fp = fopen(filepath, "at");
        if(fp)
        {
            fputs(UTIL_VarArgs("\n\"SteamID:%s\"\n", adminTutSaidSteamID), fp);
            fputs("{\n", fp);
            fputs("\tbaseLevel = 131071\n", fp);
            fputs("}\n", fp);
            fclose(fp);
        }

        KeyValues *data = new KeyValues("data");
        data->SetString( "title", "New Admin Tutorial" );
        data->SetString( "type", "2" );
        data->SetString( "msg", "http://www.adminop.net/sourceop/game_tut/tutorial3_2_2.html" );
        data->SetInt( "cmd", INFO_PANEL_COMMAND_CLOSED_HTMLPAGE );
        ShowViewPortPanel( "info", true, data );
        data->deleteThis();

        adminTutBlockSay = 0;
        adminTutPage = 40;

        //Msg("adminTutSaidSteamID: %s\n", adminTutSaidSteamID);
        return 1;
    }

    if(pAdminOP.FeatureStatus(FEAT_PLAYERSAYCOMMANDS))
    {
        if(!Q_strncasecmp("!help", text, 5))
        {
            if(!Q_strcasecmp("!help jetpack", text))
            {
                ShowHelpPage("http://www.sourceop.com/game_help/" SourceOPVerShort "/player_jetpack.html");
            }
            else if(!Q_strcasecmp("!help hook", text))
            {
                ShowHelpPage("http://www.sourceop.com/game_help/" SourceOPVerShort "/player_hook.html");
            }
            else if(!Q_strcasecmp("!help credits", text))
            {
                ShowHelpPage("http://www.sourceop.com/game_help/" SourceOPVerShort "/player_credits.html");
            }
            else if(!Q_strcasecmp("!help radio", text))
            {
                ShowHelpPage("http://www.sourceop.com/game_help/" SourceOPVerShort "/player_radio.html");
            }
            else
            {
                ShowHelpPage("http://www.sourceop.com/game_help/" SourceOPVerShort "/index.html");
            }

            return 1;
        }
        if(pAdminOP.FeatureStatus(FEAT_JETPACK))
        {
            if(!Q_strcasecmp("!jetpack", text) || !Q_strcasecmp("!jetson", text))
            {
                if(jetpack_on.GetBool() || IsAdmin(8, "jetpack_override"))
                {
                    if(!IsDenied("jetpack"))
                    {
                        ShowBindMenu("Jetpack", "+jetpack", 0);
                        SayTextChatHud(UTIL_VarArgs("[%s] A bind menu has been shown to assist you in binding a key for Jetpack.\n", pAdminOP.adminname));
                        SayTextChatHud(UTIL_VarArgs("[%s] Press ESC (Escape) to see this menu.\n", pAdminOP.adminname));
                        return 1;
                    }
                    else if(jetpack_show_accessdenymsg.GetBool())
                    {
                        SayTextChatHud(UTIL_VarArgs("[%s] You are not allowed to use the jetpack.\n", pAdminOP.adminname));
                        return 1;
                    }
                }
                else if(jetpack_show_disabledmsg.GetBool())
                {
                    SayTextChatHud(UTIL_VarArgs("[%s] Jetpack is disabled.\n", pAdminOP.adminname));
                    return 1;
                }
            }
        }
        if(pAdminOP.FeatureStatus(FEAT_RADIO))
        {
            if(!Q_strcasecmp("!radio", text) || !Q_strcasecmp("!buildradio", text))
            {
                ShowBindMenu("Radio", "buildradio", 0);
                SayTextChatHud(UTIL_VarArgs("[%s] A bind menu has been shown to assist you in binding a key for Radio.\n", pAdminOP.adminname));
                SayTextChatHud(UTIL_VarArgs("[%s] Press ESC (Escape) to see this menu.\n", pAdminOP.adminname));

                return 1;
            }
        }
        if(pAdminOP.FeatureStatus(FEAT_CREDITS))
        {
            if(!Q_strcasecmp("!buy stats", text) || !Q_strcasecmp("!buystats", text) || !Q_strcasecmp("!buy stat", text) || !Q_strcasecmp("!buystat", text))
            {
                const int price_zeus = (price_noclip.GetInt() + price_god.GetInt()) * 0.9;
                const int time_zeus = (time_god.GetInt() + time_noclip.GetInt()) / 2;
                char buystats[1024];

                /*Q_snprintf(buystats, sizeof(buystats), "Item - Time it lasts (seconds)\n"
                    "-----------------------\n"
                    "God - %i\n"
                    "Noclip - %i\n"
                    "Zeus - %i\n\n"
                    "Item - Price\n"
                    "-----------------------\n"
                    "Noclip - %i\n"
                    "God - %i\n"
                    "Gun - %i\n"
                    "All guns (allguns) - %i\n"
                    "Health - %i\n"
                    "Slay - %i\n"
                    "Explosive Can (can) - %i\n"
                    "Zeus - %i\n", time_god.GetInt(), time_noclip.GetInt(), time_zeus, price_noclip.GetInt(), price_god.GetInt(), price_gun.GetInt(), price_allguns.GetInt(), price_health.GetInt(), price_slay.GetInt(), price_explosive.GetInt(), price_zeus);*/
                
                if(help_show_loading.GetBool())
                {
                    Q_snprintf(buystats, sizeof(buystats), "about:<html><meta http-equiv=\"Refresh\" content=\"0;url=http://www.sourceop.com/game_help/" SourceOPVerShort "/buystats.php?t=%i,%i,%i&p=%i,%i,%i,%i,%i,%i,%i,%i,%i\"><h2>Buy stats is loading...</h2></html>", time_god.GetInt(), time_noclip.GetInt(), time_zeus, price_noclip.GetInt(), price_god.GetInt(), price_gun.GetInt(), price_allguns.GetInt(), price_ammo.GetInt(), price_health.GetInt(), price_slay.GetInt(), price_explosive.GetInt(), price_zeus);
                    buystats[sizeof(buystats)-1] = '\0';

                    KeyValues *data = new KeyValues("data");
                    data->SetString( "title", "!buy stats" );
                    data->SetString( "type", "2" );
                    data->SetString( "msg", buystats );
                    ShowViewPortPanel( "info", true, data );
                    data->deleteThis();
                }
                else
                {
                    Q_snprintf(buystats, sizeof(buystats), "http://www.sourceop.com/game_help/" SourceOPVerShort "/buystats.php?t=%i,%i,%i&p=%i,%i,%i,%i,%i,%i,%i,%i,%i", time_god.GetInt(), time_noclip.GetInt(), time_zeus, price_noclip.GetInt(), price_god.GetInt(), price_gun.GetInt(), price_allguns.GetInt(), price_ammo.GetInt(), price_health.GetInt(), price_slay.GetInt(), price_explosive.GetInt(), price_zeus);
                    buystats[sizeof(buystats)-1] = '\0';

                    KeyValues *data = new KeyValues("data");
                    data->SetString( "title", "!buy stats" );
                    data->SetString( "type", "2" );
                    data->SetString( "msg", buystats );
                    ShowViewPortPanel( "info", true, data );
                    data->deleteThis();
                }

                /*KeyValues *data = new KeyValues("data");
                data->SetString( "title", "!buy stats" );
                data->SetString( "type", "0" ); //TYPE_TEXT
                data->SetString( "msg", buystats );
                ShowViewPortPanel( "info", true, data );
                data->deleteThis();*/

                return 1;
            }
        }
        if(!stricmp("!maplist", text) || !stricmp("!listmaps", text) || !stricmp("map list", text))
        {
            int page = 1;
            char buf[768];
            int count = 0;
            int thismsg = 0;
            FileFindHandle_t findHandle;
            const char *bspFilename = filesystem->FindFirstEx( "maps/*.bsp", "MOD", &findHandle );
            buf[0] = '\0';

            if(!Q_strncasecmp("!maplist", text, 8) || !Q_strncasecmp("map list", text, 8))
            {
                if(strlen(text) >= 10)
                {
                    page = atoi(&text[9]);
                }
            }
            else
            {
                if(strlen(text) >= 11)
                {
                    page = atoi(&text[10]);
                }
            }

            if(page<1) page = 1;

            //Msg("Page: %i\n", page);

            while ( bspFilename )
            {
                count++;
                if(count > (page-1)*10)
                {
                    strncat(buf, bspFilename, sizeof(buf) - strlen(buf) - 2);
                    buf[strlen(buf) - 4] = '\0';
                    strncat(buf, "\n", sizeof(buf) - strlen(buf) - 2);
                    thismsg++;
                }
                if(thismsg >= 10) break;
                bspFilename = filesystem->FindNext( findHandle );
            }
            filesystem->FindClose( findHandle );

            strncat(buf, "\n10 results shown per page.\nSay !listmaps followed by the page number to see more results.\nExample to show page 2:\n!listmaps 2\n", sizeof(buf) - strlen(buf) - 2);
            
            /*KeyValues *data = new KeyValues("data");
            data->SetString( "title", "!listmaps" );
            data->SetString( "type", "0" );
            data->SetString( "msg", buf );
            ShowViewPortPanel( "info", true, data );
            data->deleteThis();*/

            //Msg("%s\n", buf);

            KeyValues *kv = new KeyValues( "menu" );
            kv->SetString( "title", "!listmaps" );
            kv->SetInt( "level", 2 );
            kv->SetInt( "time", 20 );
            kv->SetString( "msg", buf );
            
            helpers->CreateMessage( pEntity, DIALOG_TEXT, kv, &g_ServerPlugin );
            kv->deleteThis();

            return 1;
        }
        if(pAdminOP.FeatureStatus(FEAT_ADMINCOMMANDS))
        {
            if(!stricmp("!prefix", text))
            {
                SayTextChatHud(UTIL_VarArgs("[%s] %s\n", pAdminOP.adminname, command_prefix.GetString()));
                return 1;
            }
        }
        if(!Q_strncasecmp("!pc ", text, 4))
        {
            int playerList[128];
            int playerCount = 0;
            const char *strName = &text[4];

            playerCount = pAdminOP.FindPlayerWithName(playerList, strName);
            if(!playerCount)
            {
                playerCount = pAdminOP.FindPlayerByName(playerList, strName);
            }
            if(playerCount && playerList[0] != 0)
            {
                IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+playerList[0]);

                if(info)
                {
                    if(info->IsConnected())
                    {
                        PrivateChat.LocalPrivChat = true;
                        PrivateChat.LocalPrivChatID = playerList[0];
                        SayTextChatHud(UTIL_VarArgs("[%s] Entered private chat with %s.\n", pAdminOP.adminname, info->GetName()));
                        pAdminOP.pAOPPlayers[playerList[0]-1].SayTextChatHud(UTIL_VarArgs("[%s] %s has entered private chat with you.\n", pAdminOP.adminname, playerinfo->GetName()));
                        pAdminOP.pAOPPlayers[playerList[0]-1].SayTextChatHud(UTIL_VarArgs("[%s] In chat, say !pc and then part of the player's name to enter chat with him or her.\n", pAdminOP.adminname));
                    }
                }
            }
            else
            {
                SayTextChatHud(UTIL_VarArgs("[%s] Player not found.\n", pAdminOP.adminname));
            }
            return 1;
        }
        if(playerchat_motd.GetBool() && !Q_strcasecmp("motd", text))
        {
            const char *title = (hostname) ? hostname->GetString() : "MESSAGE OF THE DAY";

            KeyValues *data = new KeyValues("data");
            data->SetString( "title", title );      // info panel title
            data->SetString( "type", "1" );         // show userdata from stringtable entry
            data->SetString( "msg", "motd" );       // use this stringtable entry

            ShowViewPortPanel( "info", true, data );

            data->deleteThis();

            return 1;
        }
    }
    return 0;
}

int CAdminOPPlayer :: PlayerSpeakTeamPre(const char *text)
{
    if(!entID)
        return 0;

    if(IsGagged() && gag_notify.GetBool())
    {
        SayTextChatHud(UTIL_VarArgs("[%s] You are gagged and may not speak.\n", pAdminOP.adminname));
        return 1;
    }

    if(IsDenied("say_team"))
        return 1;

    return 0;
}

void CAdminOPPlayer :: Disconnect()
{
    playerState = 0;

    if(!notBot)
        return;

    pAdminOP.RemovePlayerFromVotes(uid);
    if(pAdminOP.FeatureStatus(FEAT_CREDITS) && feature_credits.GetInt() && Credits.PendingLoad == 0)
    {
        SaveCredits(pAdminOP.ShuttingDown);
    }

    LeavePrivateChat();

    Cleanup();

    SetConnectTime(0);

    // reset all data on disconnect too because if a bot takes its place, but doesn't call Connect() then bad things could happen
    InitPlayerData();
    uid = 0;
}

void CAdminOPPlayer :: SessionEnd()
{
    m_bCustomSteamIDValidated = false;
    sessionstarttime = 0;
    IP[0] = '\0';
}

void CAdminOPPlayer :: PluginUnloading()
{
    if(!notBot)
        return;

    if(pAdminOP.FeatureStatus(FEAT_CREDITS) && feature_credits.GetInt() && Credits.PendingLoad == 0)
    {
        SaveCredits(pAdminOP.ShuttingDown);
    }

    Cleanup();
}

void CAdminOPPlayer :: Cleanup()
{
    // don't delete spawned stuff if map is shutting down
    if(!pAdminOP.MapShutDown)
    {
        if(spawn_removeondisconnect.GetBool())
        {
            CUtlLinkedList <unsigned int, unsigned short> spawnedEntsCopy;
            for ( unsigned int i=spawnedEnts.Head(); i != spawnedEnts.InvalidIndex(); i = spawnedEnts.Next( i ) )
            {
                spawnedEntsCopy.AddToTail(spawnedEnts.Element(i));
            }
            for ( unsigned int i=spawnedEntsCopy.Head(); i != spawnedEntsCopy.InvalidIndex(); i = spawnedEntsCopy.Next( i ) )
            {
                edict_t *pEntity = pAdminOP.GetEntityList() + spawnedEntsCopy.Element(i);
                if(pEntity)
                {
                    if(!pEntity->IsFree())
                    {
                        CBaseEntity *pEnt = (CBaseEntity*)CBaseEntity::Instance(pEntity);
                        UTIL_Remove(pEnt);
                    }
                }
            }
        }
    }
    spawnedEnts.Purge();

    if(!pAdminOP.MapShutDown)
    {
        for(int i = 0; i < thrusters.Count(); i++)
        {
            thruster_t *thruster = thrusters.Base() + i; // fast access
            DeleteThruster(thruster);
        }
    }
    thrusters.Purge();

    JetpackOff();
    HookOff();
    if(!pAdminOP.MapShutDown)
    {
        DropGrabbedPhysObject();
    }
    entPhysMoving = 0;

    for(int i = 0; i < m_pendingCvarQueries.Count(); i++)
    {
        cvarquery_t *query = m_pendingCvarQueries.Base() + i;
        lua_unref(pAdminOP.GetLuaState(), query->luacallback);
    }
    m_pendingCvarQueries.Purge();
}

void CAdminOPPlayer :: LevelShutdown()
{

}

void CAdminOPPlayer :: FreeContainingEntity( edict_t *pEntity )
{
    if(!entID)
        return;
    if(!pEntity)
        return;

    int entindex = ENTINDEX(pEntity);

    //Msg("Del: %i\n", ENTINDEX(pEntity));

    spawnedEnts.FindAndRemove(entindex);

    if(pTemp)
    {
        if(entindex == VFuncs::entindex(pTemp))
        {
            SayTextChatHud(UTIL_VarArgs("[%s] Your temporary entity was destroyed and has been unset.\n", pAdminOP.adminname));
            pTemp = NULL;
        }
    }
    if(thrusters.Count())
    {
        for(int i = 0; i < thrusters.Count(); i++)
        {
            thruster_t *thruster = thrusters.Base() + i; // fast access
            if(entindex == VFuncs::entindex(thruster->entattached))
            {
                DeleteThruster(thruster);
                SayTextChatHud(UTIL_VarArgs("[%s] The entity your thruster was attached to has been destroyed.\n", pAdminOP.adminname));
                thrusters.FastRemove(i);
                i--;
            }
        }
    }
    if(Hook.OnHook)
    {
        if(Hook.HookedEnt && entindex == VFuncs::entindex(Hook.HookedEnt))
        {
            HookOff();
        }
    }
    if(entMoving || entRotating)
    {
        if(pEntMove)
        {
            if(entindex == ENTINDEX(pEntMove))
            {
                entRotating = 0;
                entMoving = 0;
            }
        }
    }
    if(entPhysMoving)
    {
        if(PhysMoving.m_hObject)
        {
            if(entindex == VFuncs::entindex(PhysMoving.m_hObject))
            {
                DropGrabbedPhysObject();
                PhysMoving.m_hObject = NULL;
            }
        }
    }
}

void CAdminOPPlayer :: Think()
{
    if(!entID)
        return;

    edict_t *pEdPlayer = pAdminOP.GetEntityList()+entID;
    CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(pEdPlayer);
    
    if(!pPlayer)
    {
        nextthink = Plat_FloatTime() + 0.05;
        return;
    }

    IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pEdPlayer);
    if(info)
    {
        if(engine->Time() >= nextRemoteUpdate && playerState == 2)
        {
            if(remoteserver) remoteserver->SendPlayerUpdateToAll(entID);
            nextRemoteUpdate = engine->Time() + 2.5;
        }
    }

    if(pAdminOP.GetMeleeMode() && pAdminOP.isTF2)
    {
        // slots start at 0. third slot is #2.

        // this is needed because if a heavy is firing his gun or has it
        // spinning, the Weapon_Switch will not work
        CBaseCombatWeapon *pWeapon = VFuncs::Weapon_GetSlot(pPlayer, 0);
        if(pWeapon && pActiveWeapon == pWeapon)
        {
            VFuncs::Holster(pWeapon);
            VFuncs::SetNextAttack(pPlayer, gpGlobals->curtime + 2);
        }

        pWeapon = VFuncs::Weapon_GetSlot(pPlayer, 2);
        if(pWeapon && pActiveWeapon != pWeapon)
        {
            if(VFuncs::Weapon_Switch(pPlayer, pWeapon))
            {
                if(notBot)
                {
                    SayTextChatHud(UTIL_VarArgs("[%s] Weapon switch denied. Melee only mode is enabled.\n", pAdminOP.adminname));
                }
            }
        }
    }

    // time up! throw grenade (i.e. explode) now!
    if(m_flGrenadeDetonateTime && m_flGrenadeDetonateTime <= gpGlobals->curtime)
    {
        ThrowFragGrenade();
    }

    /***************\
    | begin no bots |
    \***************/

    if(!notBot)
    {
        nextthink = Plat_FloatTime() + 0.05;
        return;
    }

    if(highpingkicker_on.GetBool())
    {
        if(playerState == 2 && pingManager.maxCount > 0 && engine->Time() >= pingManager.lastRecordTime + highpingkicker_sampletime.GetFloat())
        {
            pingManager.RecordPing();
            if(pingManager.Count() >= pingManager.maxCount)
            {
                if(pingManager.GetAverage() > highpingkicker_maxping.GetInt())
                {
                    IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+entID);
                    SayTextChatHud(UTIL_VarArgs("[%s] %s\n", pAdminOP.adminname, highpingkicker_message.GetString()));
                    pAdminOP.KickPlayer(info, UTIL_VarArgs("Ping too high. Your ping was %i. Max ping is %i.", pingManager.GetAverage(), highpingkicker_maxping.GetInt()));
                    engine->LogPrint(UTIL_VarArgs("[SOURCEOP] \"%s<%i><%s><%s>\" high-ping-kick\n", info->GetName(), info->GetUserID(), engine->GetPlayerNetworkIDString(pEdPlayer), GetTeamName()));
                }
            }
        }
    }

    if(engine->Time() >= nextAdminUpdate && playerState == 2)
    {
        UpdateAdminLevel();
        nextAdminUpdate = engine->Time() + 10;
    }

    // PendingLoad 99999 means failue
    if(Credits.PendingLoad && Credits.PendingLoad != 99999 && engine->Time() >= Credits.NextLoad && feature_credits.GetInt() && pAdminOP.FeatureStatus(FEAT_CREDITS))
    {
        if(LoadCredits())
        {
            Credits.PendingLoad = 0; // quit trying to load
        }
        else
        {
            if(Credits.PendingLoad == 30)
            {
                char logmsg[256];
                IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+entID);

                if(info)
                {
                    Q_snprintf(logmsg, sizeof(logmsg), "[SOURCEOP] \"%s<%i><%s><%s>\" failed to load credits after \"%i\" tries (giving up)\n", info->GetName(), info->GetUserID(), engine->GetPlayerNetworkIDString(pEdPlayer), GetTeamName(), Credits.PendingLoad);
                    engine->LogPrint(logmsg);
                }
                Credits.PendingLoad = 99999; // set to failure
            }
            else
            {
                IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+entID);
                if(info) engine->LogPrint(UTIL_VarArgs("[SOURCEOP] \"%s<%i><%s><%s>\" failed to load credits after \"%i\" %s\n", info->GetName(), info->GetUserID(), engine->GetPlayerNetworkIDString(pEdPlayer), GetTeamName(), Credits.PendingLoad, Credits.PendingLoad != 1 ? "tries" : "try"));

                Credits.PendingLoad++; // increase try count
                Credits.NextLoad = engine->Time() + 1; //try again in one second
            }
        }
    }
    if(pAdminOP.FeatureStatus(FEAT_CREDITS) && feature_credits.GetInt())
    {
        if(feature_showcredits.GetInt() && engine->Time() >= nextCreditCounter && playerState == 2)
        {
            SendCreditCounter();
            nextCreditCounter = engine->Time() + 30;
        }
        if(Credits.LastCreditCheck + 0.2 <= engine->Time())
        {
            IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+entID);

            if(creditgap.GetInt() < 1)
                creditgap.SetValue(1);
            if(info->GetFragCount() >= Credits.LastCreditGive + creditgap.GetInt())
            {
                int fragjump = info->GetFragCount() - Credits.LastCreditGive;
                int award = (int)(fragjump)/creditgap.GetInt();
                Credits.LastCreditCheck = engine->Time();
                Credits.LastCreditGive = info->GetFragCount();

                if(fragjump > credits_maxpointsatonce.GetInt()) // if they get more than 65 frags at once, don't give them the credits. obviously something else is going on
                {
                    SayTextChatHud(UTIL_VarArgs("[%s] You did not earn credits. You got too many frags at once, which is not likely to be credit worthy.\n", pAdminOP.adminname));
                    SayTextChatHud(UTIL_VarArgs("[%s] If you believe you deserve the credits for what you just did, contact an admin.\n", pAdminOP.adminname));
                    pAdminOP.TimeLog("credits_not_given.txt", "%s<%i><%s><%s> was not awarded \"%i\" credits for a frag jump of \"%i\" on map \"%s\".\n", info->GetName(), info->GetUserID(), engine->GetPlayerNetworkIDString(pEdPlayer), GetTeamName(), award, fragjump, pAdminOP.CurrentMap());
                }
                else
                {
                    char msg[256];

                    if(Credits.thismap.totalawarded < credits_maxpermap.GetInt())
                    {
                        Credits.CreditsDuringGame += award;
                        Credits.thismap.totalawarded += award;

                        // cap at the max
                        if(Credits.thismap.totalawarded > credits_maxpermap.GetInt())
                        {
                            int difference = Credits.thismap.totalawarded - credits_maxpermap.GetInt();
                            Credits.CreditsDuringGame -= difference;
                            Credits.thismap.totalawarded -= difference;
                        }

                        if(credits_newcreditmsg.GetBool())
                        {
                            Q_snprintf(msg, sizeof(msg), "[%s] You just earned another credit. You now have %i credits.\n", pAdminOP.adminname, Credits.CreditsDuringGame + Credits.CreditsJoin);
                            SayTextChatHud(msg);
                        }

                        SendCreditCounter();
                        nextCreditCounter = engine->Time() + 30;

                        //CreditCheat
                        if(credits_cheatprotect_on.GetBool())
                        {
                            // timer expired
                            if(Credits.CheatTimer < engine->Time())
                            {
                                Credits.CheatTimer = engine->Time() + credits_cheatprotect_time.GetFloat();
                                Credits.CheatCount = 0;
                            }
                            Credits.CheatCount+= award;
                            if(Credits.CheatCount > credits_cheatprotect_amount.GetInt())
                            {
                                pAdminOP.SlayPlayer(entID);
                                SayTextChatHud(UTIL_VarArgs("[%s] You were slayed for earning credits too fast.\n", pAdminOP.adminname));
                                Credits.CheatNum++;
                            }
                        }
                    }
                    else
                    {
                        Q_snprintf(msg, sizeof(msg), "[%s] You have already earned the maximum amount of credits allowed for this map and cannot get any more until the map changes.\n", pAdminOP.adminname);
                        SayTextChatHud(msg);
                    }
                }
            }
        }
    }
    if(NoclipTime)
    {
        if(NoclipTime <= engine->Time())
        {
            SayTextChatHud(UTIL_VarArgs("[%s] Your noclip has expired.\n", pAdminOP.adminname));
            if(credits_announce.GetBool())
            {
                IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+entID);
                if(info)
                {
                    if(info->IsConnected())
                    {
                        pAdminOP.SayTextAllChatHud(UTIL_VarArgs("[%s] %s's noclip has expired.\n", pAdminOP.adminname, info->GetName()));
                    }
                }
            }
            SetNoclip(0);
            NoclipTime = 0;
        }
    }
    if(GodTime)
    {
        if(GodTime <= engine->Time())
        {
            SayText(UTIL_VarArgs("[%s] Your god has expired.\n", pAdminOP.adminname));
            if(credits_announce.GetBool())
            {
                IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+entID);
                if(info)
                {
                    if(info->IsConnected())
                    {
                        pAdminOP.SayTextAllChatHud(UTIL_VarArgs("[%s] %s's god has expired.\n", pAdminOP.adminname, info->GetName()));
                    }
                }
            }
            SetGod(0);
            GodTime = 0;
        }
    }

    if(Hook.OnHook)
    {
        if(IsAlive())
        {
            edict_t *pEntity = pAdminOP.GetEntityList()+entID;
            CBaseEntity *pPlayer = CBaseEntity::Instance(pEntity);
            if(pEntity && pPlayer)
            {
                CBaseEntity *pHook = NULL;
                int m_beamIndex = engine->PrecacheModel( "cable/rope.vmt" );
                //effects->Beam(EyePosition() - Vector(0,0,12), Hook.HookedAt, m_beamIndex, 0, 0, 2.0f, 0.1f, 2, 2, 0, 0, 255, 255, 255, 128, 100 );
                CBroadcastRecipientFilter filter;
                if(Hook.HookEnt) pHook = Hook.HookEnt;
                if(pHook)
                {
                    //if(te) te->BeamEnts(filter, 0, entID, VFuncs::entindex(pHook), m_beamIndex, 0, 0, 2.0f, 0.1f, 2, 2, 0, 0, 255, 255, 255, 128, 100);
                    if(te) te->BeamEnts(filter, 0, entID, VFuncs::entindex(pHook), m_beamIndex, 0, 0, 0.0f, 0.1f, 2, 2, 0, 0, 255, 255, 255, 128, 0);
                    Hook.HookedAt = VFuncs::GetAbsOrigin(pHook);
                }
                else
                {
                    if(te) te->BeamEntPoint(filter, 0, entID, &Vector(0,0,0), -1, &Hook.HookedAt, m_beamIndex, 0, 0, 2.0f, 0.1f, 2, 2, 0, 0, 255, 255, 255, 128, 100);
                }
                Vector oldvelocity = VFuncs::GetAbsVelocity(pPlayer);
                Vector direction;
                if(hook_pullmode.GetInt() == 1)
                {
                    direction = (oldvelocity * 0.2 + (Hook.HookedAt - VFuncs::GetAbsOrigin(pPlayer))) * (oldvelocity.Length() * 0.2 + 400);
                }
                else
                {
                    direction = Hook.HookedAt - VFuncs::GetAbsOrigin(pPlayer);
                }
                VectorNormalize(direction);
                Vector newvelocity = direction * hook_speed.GetInt();
                //VFuncs::Teleport(pPlayer, NULL, NULL, &newvelocity);
                VFuncs::SetAbsVelocity(pPlayer, newvelocity);
            }
        }
        else
        {
            HookOff();
        }
    }

    if(Jetpack.OnJetpack)
    {
        if(IsAlive())
        {
            if(Jetpack.NextChargeTime <= engine->Time() && !jetpack_infinite_charge.GetBool())
            {
                Jetpack.NextChargeTime = engine->Time() + 0.15;
                if(Jetpack.Charge > 0) Jetpack.Charge--;

                if(jetpack_show_chargemsg.GetBool())
                {
                    //Msg("Jetpack depleting: %i\n", Jetpack.Charge);

                    KeyValues *kv = new KeyValues( "menu" );
                    kv->SetString( "title", UTIL_VarArgs("Jetpack depleting: %i%%.\n", Jetpack.Charge) );
                    kv->SetInt( "level", Jetpack.MessageLevel );
                    kv->SetInt( "time", 2 );
                    
                    helpers->CreateMessage( pAdminOP.GetEntityList()+entID, DIALOG_MSG, kv, &g_ServerPlugin );
                    kv->deleteThis();
                    Jetpack.MessageLevel--;
                    Jetpack.NextMessageLevelReset = engine->Time() + 11;
                    if(Jetpack.MessageLevel < 100)
                        Jetpack.MessageLevel = JETPACK_MAXMESSAGELEVEL;

                    switch(Jetpack.Charge)
                    {
                    case 75:
                    case 50:
                    case 25:
                        SayTextChatHud(UTIL_VarArgs("[%s] Your jetpack is depleting and is at %i%%.\n", pAdminOP.adminname, Jetpack.Charge));
                        break;
                    case 0:
                        SayTextChatHud(UTIL_VarArgs("[%s] Your jetpack is fully depleted.\n", pAdminOP.adminname));
                        break;
                    }
                }
            }

            if(Jetpack.Charge > 0 || jetpack_infinite_charge.GetBool())
            {
                edict_t *pEntity = pAdminOP.GetEntityList()+entID;
                CBaseEntity *pPlayer = CBaseEntity::Instance(pEntity);
                if(pEntity && pPlayer)
                {
                    //IPhysicsObject* pObj = VFuncs::VPhysicsGetObject(pPlayer);

                    float mult = (engine->Time() - Jetpack.LastThrust) / 0.05;
                    float gravscale = sv_gravity->GetFloat() / 600.0f;
                    int oldforwardz;
                    Vector oldvelocity = VFuncs::GetAbsVelocity(pPlayer);
                    Vector forward, right, up;

                    AngleVectors ( VFuncs::EyeAngles(pPlayer), &forward, &right, &up);
                    oldforwardz = forward.z;
                    forward.z = 0;
                    oldvelocity = oldvelocity + (forward * 10);
                    if(oldforwardz < 0) oldforwardz *= 0.3;
                    if(oldvelocity.z <= -10)
                    {
                        oldvelocity.z += (oldforwardz*70) * mult * gravscale;
                        oldvelocity.z += 70 * mult * gravscale;
                    }
                    else
                    {
                        oldvelocity.z += (oldforwardz*40) * mult * gravscale;
                        oldvelocity.z += 40 * mult * gravscale;
                    }
                    if(oldvelocity.Length() > 350 && oldvelocity.z >= -300)
                    {
                        VectorNormalize(oldvelocity);
                        oldvelocity = oldvelocity * 350;
                    }

                    // if player lands on ground, teleport them off
                    trace_t trace;
                    Vector vecOrigin = VFuncs::GetAbsOrigin(pPlayer);
                    vecOrigin.z -= 0.5;
                    UTIL_TraceEntity( pPlayer, vecOrigin, vecOrigin, MASK_SOLID, pPlayer, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );
                    if ( trace.startsolid && trace.allsolid )
                    {
                        // and only move the player if it won't get him stuck
                        vecOrigin.z += 20.5; // 0.5 to get back to zero offset then +20 offset
                        UTIL_TraceEntity( pPlayer, vecOrigin, vecOrigin, MASK_SOLID, pPlayer, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );
                        if ( !trace.startsolid && !trace.allsolid )
                        {
                            // replay sound
                            Vector vecSndOrigin = VFuncs::GetAbsOrigin(pPlayer);
                            CPASFilter filter( vecSndOrigin );
                            enginesound->EmitSound((CRecipientFilter&)filter, VFuncs::entindex(pPlayer), CHAN_BODY, "ambient/gas/cannister_loop.wav", 0.5, SNDLVL_NORM, 0, random->RandomInt(90, 110), &vecSndOrigin);  
                            VFuncs::Teleport(pPlayer, &vecOrigin, NULL, NULL);
                        }
                    }

                    VFuncs::SetAbsVelocity(pPlayer, oldvelocity);
                    //VFuncs::Teleport(pPlayer, NULL, NULL, &oldvelocity);
                    Jetpack.LastThrust = engine->Time();
                }
            }
            else
            {
                JetpackOff();
            }
        }
        else
        {
            JetpackOff();
        }
    }
    else
    {
        if(Jetpack.NextChargeTime <= engine->Time() && !jetpack_infinite_charge.GetBool())
        {
            Jetpack.NextChargeTime = engine->Time() + 0.3;
            if(Jetpack.Charge < 100)
            {
                Jetpack.Charge++;

                if(jetpack_show_chargemsg.GetBool())
                {
                    KeyValues *kv = new KeyValues( "menu" );
                    kv->SetString( "title", UTIL_VarArgs("Jetpack charging: %i%%.\n", Jetpack.Charge) );
                    kv->SetInt( "level", Jetpack.MessageLevel );
                    kv->SetInt( "time", 2 );
                    
                    helpers->CreateMessage( pAdminOP.GetEntityList()+entID, DIALOG_MSG, kv, &g_ServerPlugin );
                    kv->deleteThis();
                    Jetpack.MessageLevel--;
                    Jetpack.NextMessageLevelReset = engine->Time() + 11;
                    if(Jetpack.MessageLevel < 100)
                        Jetpack.MessageLevel = JETPACK_MAXMESSAGELEVEL;

                    switch(Jetpack.Charge)
                    {
                    case 25:
                    case 50:
                    case 75:
                        SayTextChatHud(UTIL_VarArgs("[%s] Your jetpack is charging and is at %i%%.\n", pAdminOP.adminname, Jetpack.Charge));
                        break;
                    case 100:
                        SayTextChatHud(UTIL_VarArgs("[%s] Your jetpack is fully charged.\n", pAdminOP.adminname));
                        break;
                    }
                    //Msg("Jetpack charging: %i\n", Jetpack.Charge);
                }
            }
        }
    }

    if(Jetpack.NextMessageLevelReset && Jetpack.NextMessageLevelReset <= engine->Time())
    {
        Jetpack.NextMessageLevelReset = 0;
        Jetpack.MessageLevel = JETPACK_MAXMESSAGELEVEL;
    }

    /*edict_t *pEntity = pAdminOP.GetEntityList()+entID;
    if(pEntity)
    {
        CBaseEntity *pPlayer = CBaseEntity::Instance(pEntity);
        if(pPlayer)
        {
            QAngle angOrig = QAngle(0,0,0);
            VFuncs::Teleport(pPlayer, NULL, &angOrig, NULL);
        }
    }*/
    if(thrusters.Count())
    {
        for(int i = 0; i < thrusters.Count(); i++)
        {
            thruster_t *thruster = thrusters.Base() + i; // fast access
            if(thruster->enabled)
            {
                IPhysicsObject *pObj1 = VFuncs::VPhysicsGetObject(thruster->entattached);
                IPhysicsObject *pObj2 = VFuncs::VPhysicsGetObject(thruster->entthruster);
                if(pObj1 && pObj2)
                {
                    QAngle angles;
                    Vector pos;
                    Vector forward, right, up;
                    pObj2->GetPosition(&pos, &angles);
                    AngleVectors ( angles, &forward, &right, &up);
                    pObj1->ApplyForceOffset(up * -thruster->force, pos);
                }
            }
        }
    }
    if(pAdminOP.FeatureStatus(FEAT_KILLSOUNDS) && feature_killsounds.GetBool())
    {
        if(Kills.PlayMultiSound >= 5)
            PlaySoundLocal("SourceOP/monsterkill.wav");
        else if(Kills.PlayMultiSound == 4)
            PlaySoundLocal("SourceOP/ultrakill.wav");
        else if(Kills.PlayMultiSound == 3)
            PlaySoundLocal("SourceOP/multikill.wav");
        else if(Kills.PlayMultiSound == 2)
            PlaySoundLocal("SourceOP/doublekill.wav");

        if(Kills.PlaySpreeSound == 25)
            PlaySoundLocal("SourceOP/godlike.wav");
        else if(Kills.PlaySpreeSound == 20)
            PlaySoundLocal("SourceOP/unstoppable.wav");
        else if(Kills.PlaySpreeSound == 15)
            PlaySoundLocal("SourceOP/rampage.wav");
        else if(Kills.PlaySpreeSound == 10)
            PlaySoundLocal("SourceOP/dominating.wav");
        else if(Kills.PlaySpreeSound == 5)
            PlaySoundLocal("SourceOP/killingspree.wav");
        Kills.PlayMultiSound = 0;
        Kills.PlaySpreeSound = 0;
    }

    nextthink = Plat_FloatTime() + 0.05;
}

void CAdminOPPlayer :: HighResolutionThink()
{
    if(!entID)
        return;

    float engineTime = engine->Time();
    if(engineTime >= nextRemoteUpdate && playerState == 2)
    {
        if(remoteserver) remoteserver->SendPlayerUpdateToAll(entID);
        nextRemoteUpdate = engineTime + 2.5;
    }

    /***************\
    | begin no bots |
    \***************/
    if(!notBot) return;

    CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(pAdminOP.GetEntityList()+entID);

    // block user from doing anything?
    //engine->ClientCommand(pAdminOP.GetEntityList()+entID, "join_class;join_class;join_class;join_class;join_class;join_class;join_class;join_class;join_class;join_class\n");

    if(entMoving)
    {
        CBaseEntity *pMoving = CBaseEntity::Instance(pEntMove);
        if(pMoving)
        {
            Vector newOrigin = (VFuncs::GetAbsOrigin(pPlayer) - origPlayerLoc) + origEntLoc;
            VFuncs::SetAbsOrigin(pMoving, newOrigin);
        }
    }
    if(entPhysMoving)
    {
        CBaseEntity *pOwner = CBaseEntity::Instance(pAdminOP.GetEntityList()+entID);
        Vector start, angles, forward, right;
        trace_t tr;
        QAngle playerAngles = VFuncs::EyeAngles(pPlayer);

        AngleVectors( playerAngles, &forward, &right, NULL );
        start = EyePosition();
        Vector end = start + forward * 4096;

        UTIL_TraceLine( start, end, MASK_SHOT, pOwner, COLLISION_GROUP_NONE, &tr );
        end = tr.endpos;
        float distance = tr.fraction * 4096;
        if ( tr.fraction != 1 )
        {
            // too close to the player, drop the object
            if ( distance < 36 )
            {
                DropGrabbedPhysObject();
                return;
            }
        }

        if ( PhysMoving.m_hObject == NULL && tr.DidHitNonWorldEntity() )
        {
            CBaseEntity *pHit = tr.m_pEnt;
            // inform the object what was hit
            //ClearMultiDamage();
            //pEntity->DispatchTraceAttack( CTakeDamageInfo( pOwner, pOwner, 0, DMG_PHYSGUN ), forward, &tr );
            //ApplyMultiDamage();
            AttachGrabbedPhysObject( pHit, start, tr.endpos, distance );
            PhysMoving.m_lastYaw = VFuncs::EyeAngles(pOwner).y;
        }

        // Add the incremental player yaw to the target transform
        matrix3x4_t curMatrix, incMatrix, nextMatrix;
        QAngle angRotAngs = QAngle(0, VFuncs::EyeAngles(pOwner).y - PhysMoving.m_lastYaw, 0);
        AngleMatrix( PhysMoving.m_gravCallback.m_targetRotation, curMatrix );
        AngleMatrix( angRotAngs, incMatrix );
        ConcatTransforms( incMatrix, curMatrix, nextMatrix );
        MatrixAngles( nextMatrix, PhysMoving.m_gravCallback.m_targetRotation );
        PhysMoving.m_lastYaw = VFuncs::EyeAngles(pOwner).y;

        CBaseEntity *pObject = PhysMoving.m_hObject;
        if ( pObject )
        {
            /*if(!PhysMoving.pBeam)
            {
                PhysMoving.pBeam = CreateEntityByName("env_quadraticbeam");
                if(PhysMoving.pBeam)
                {
                    VFuncs::SetAbsOrigin(PhysMoving.pBeam, pts[0]);
                    UTIL_SetModel( PhysMoving.pBeam, PHYSGUN_BEAM_SPRITE );
                    VFuncs::QuadBeamSetControlPos(PhysMoving.pBeam, pts[1]);
                    VFuncs::QuadBeamSetTargetPos(PhysMoving.pBeam, pts[2]);
                    VFuncs::QuadBeamSetWidth(PhysMoving.pBeam, 13);
                    VFuncs::QuadBeamSetScrollRate(PhysMoving.pBeam, 1.0);

                    VFuncs::Spawn(PhysMoving.pBeam);
                }
            }*/

            Vector newPosition = start + forward * PhysMoving.m_distance;
            // 24 is a little larger than 16 * sqrt(2) (extent of player bbox)
            // HACKHACK: We do this so we can "ignore" the player and the object we're manipulating
            // If we had a filter for tracelines, we could simply filter both ents and start from "start"
            Vector awayfromPlayer = start + forward * 24;

            UTIL_TraceLine( start, awayfromPlayer, MASK_SOLID, pOwner, COLLISION_GROUP_NONE, &tr );
            if ( tr.fraction == 1 )
            {
                UTIL_TraceLine( awayfromPlayer, newPosition, MASK_SOLID, pObject, COLLISION_GROUP_NONE, &tr );
                Vector dir = tr.endpos - newPosition;
                float distance = VectorNormalize(dir);
                float maxDist = PhysMoving.m_gravCallback.m_maxVel * gpGlobals->frametime;
                if ( distance >  maxDist )
                {
                    newPosition += dir * maxDist;
                }
                else
                {
                    newPosition = tr.endpos;
                }
            }
            else
            {
                newPosition = tr.endpos;
            }

            PhysMoving.m_gravCallback.SetTargetPosition( newPosition );
            Vector dir = (newPosition - VFuncs::GetAbsOrigin(pObject));
            PhysMoving.m_movementLength = dir.Length();
        }
        else
        {
            PhysMoving.m_gravCallback.SetTargetPosition( end );
        }

        Vector pts[3];
        pts[0] = VFuncs::GetAbsOrigin(pPlayer)+Vector(0,0,8);
        pts[1] = 0.5 * (PhysMoving.m_gravCallback.m_targetPosition + pts[0]);
        pts[2] = PhysMoving.m_gravCallback.m_worldPosition;

        // update the effect and create it if none exists
        if(!PhysMoving.pBeam)
        {
            PhysMoving.pBeam = CreateEntityByName("env_quadraticbeam");
            if(PhysMoving.pBeam)
            {
                VFuncs::SetAbsOrigin(PhysMoving.pBeam, pts[0]);
                UTIL_SetModel( PhysMoving.pBeam, PHYSGUN_BEAM_SPRITE );
                VFuncs::QuadBeamSetControlPos(PhysMoving.pBeam, pts[1]);
                VFuncs::QuadBeamSetTargetPos(PhysMoving.pBeam, pts[2]);
                VFuncs::QuadBeamSetWidth(PhysMoving.pBeam, 13);
                VFuncs::QuadBeamSetScrollRate(PhysMoving.pBeam, 1.0);

                VFuncs::Spawn(PhysMoving.pBeam);
            }
        }

        if(PhysMoving.pBeam)
        {
            VFuncs::SetAbsOrigin(PhysMoving.pBeam, pts[0]);
            VFuncs::QuadBeamSetControlPos(PhysMoving.pBeam, pts[1]);
            VFuncs::QuadBeamSetTargetPos(PhysMoving.pBeam, pts[2]);
        }
    }
    if(entRotating)
    {
        CBaseEntity *pMoving = CBaseEntity::Instance(pEntMove);
        if(pMoving)
        {
            QAngle angles;
            
            // just simple rotation
            // rotPlayerAngle _was_ a member of the aop player structure
            /*rotPlayerAngle = rotPlayerAngle + (VFuncs::EyeAngles(pPlayer) - origPlayerAngle);
            VFuncs::Teleport(pMoving, NULL, &(origEntAngle + rotPlayerAngle), NULL);
            VFuncs::Teleport(pPlayer, NULL, &origPlayerAngle, NULL);*/

            QAngle playerangles = VFuncs::EyeAngles(pPlayer);
            if(playerangles != origPlayerAngle)
            {
                QAngle rotPlayerAngle = playerangles - origPlayerAngle;
                Vector forward, right, up;
                AngleVectors ( playerangles, &forward, &right, &up);
                matrix3x4_t     m_rgflCoordinateFrame;
                // calculate coordinate frame so that we don't have to use EntityToWorldTransform()
                // if EntityToWorldTransform() were to be used, the offset for CBaseEntity::m_rgflCoordinateFrame
                // would have to be found and then a VFuncs would need to be added to get to it
                // it would also be faster than recalculating it
                QAngle angLocalAngs = VFuncs::GetLocalAngles(pMoving);
                AngleMatrix( angLocalAngs, VFuncs::GetAbsOrigin(pMoving), m_rgflCoordinateFrame );
                Vector rotationAxisLs;
                VectorIRotate( right, m_rgflCoordinateFrame, rotationAxisLs );
                Quaternion q;
                AxisAngleQuaternion( rotationAxisLs, rotPlayerAngle.x * 5, q );
                matrix3x4_t xform;
                QuaternionMatrix( q, vec3_origin, xform );
                matrix3x4_t localToWorldMatrix;
                ConcatTransforms( m_rgflCoordinateFrame, xform, localToWorldMatrix );

                // now do other axis
                VectorIRotate( up, m_rgflCoordinateFrame, rotationAxisLs );
                AxisAngleQuaternion( rotationAxisLs, rotPlayerAngle.y * 5, q );
                QuaternionMatrix( q, vec3_origin, xform );
                matrix3x4_t localToWorldMatrixFinal;
                ConcatTransforms( localToWorldMatrix, xform, localToWorldMatrixFinal );

                QAngle localAngles;
                MatrixAngles( localToWorldMatrixFinal, localAngles );

                VFuncs::Teleport(pMoving, NULL, &localAngles, NULL);
                VFuncs::Teleport(pPlayer, NULL, &origPlayerAngle, NULL);
            }


        }
    }
}

void CAdminOPPlayer :: UpdateInfoString(IPlayerInfo *info)
{
    if(!entID)
        return;

    CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance(pAdminOP.GetEntityList()+entID);
    if(!pPlayer)
        return;

    Vector position = GetPosition();
    short posx = position.x;
    short posy = position.y;
    short *pPosx = &posx;
    short *pPosy = &posy;
    short velx = VFuncs::GetAbsVelocity(pPlayer).x;
    short vely = VFuncs::GetAbsVelocity(pPlayer).y;
    short *pVelx = &velx;
    short *pVely = &vely;
    
    sprintf(infoString, "%c%c%c%c%c%c%c%c%c%c%c%c", (char)(BYTE)entID, IsAlive() ? 'Y' : 'N', *pPosx, *(((char*)pPosx)+1), *pPosy, *(((char*)pPosy)+1),
            *pVelx, *(((char*)pVelx)+1), *pVely, *(((char*)pVely)+1), (char)(BYTE)max(min(info->GetHealth(), 255),0), (char)(BYTE)min(info->GetMaxHealth(), 255));
    hasInfo = 1;
}

int CAdminOPPlayer :: GetPlayerState()
{
    return playerState;
}

void CAdminOPPlayer :: SetEntID(int id)
{
    entID = id;
}

void CAdminOPPlayer :: SetJoinName(const char *pszName)
{
    strncpy(Credits.JoinName, pszName, sizeof(Credits.JoinName));
    Credits.JoinName[sizeof(Credits.JoinName)-1] = '\0';
}

void CAdminOPPlayer :: NameChanged(const char *pszOldName, const char *pszNewName)
{
    strncpy(Credits.LastName, pszOldName, sizeof(Credits.LastName));
    Credits.LastName[sizeof(Credits.LastName)-1] = '\0';
    GetMyUserData(pszNewName);
    UpdateAdminLevel();

    // schedule update instead of sending now so that new info is included
    nextRemoteUpdate = 0;
}

void CAdminOPPlayer :: OnKill(int iPlayer, const char *pszWeapon)
{
    if(!entID)
        return;
    
    if(engine->Time() >= Kills.LastKillTime + 3) Kills.KillCount = 0;

    // killing yourself doesn't count
    if(iPlayer != entID)
    {
        Kills.LastKillTime = engine->Time();
        Kills.KillCount++;
        Kills.CumKillCount++;

        Kills.PlayMultiSound = Kills.KillCount;
        switch(Kills.CumKillCount)
        {
            case 25:
            case 20:
            case 15:
            case 10:
            case 5:
                Kills.PlaySpreeSound = Kills.CumKillCount;
        }
    }
}

void CAdminOPPlayer :: OnDeath(int iAttacker, const char *pszWeapon)
{
    if(!entID)
        return;

    if(m_flGrenadeDetonateTime)
    {
        ThrowFragGrenade(true);
    }

    NoclipTime = 0;
    GodTime = 0;
    Kills.CumKillCount = 0;
    Kills.KillCount = 0;
    Kills.PlayMultiSound = 0;
    Kills.PlaySpreeSound = 0;
    isAlive = 0;

    int iEntAttacker = 0;
    if(iAttacker != 0)
    {
        int playerList[128];
        int playerCount = 0;

        playerCount = pAdminOP.FindPlayerByUserID(playerList, iAttacker);

        if(playerCount == 1)
        {
            iEntAttacker = playerList[0];
        }
    }

    if(pAdminOP.FeatureStatus(FEAT_LUA))
    {
        lua_getglobal(pAdminOP.GetLuaState(), "hook");
        lua_pushliteral(pAdminOP.GetLuaState(), "Call");
        lua_gettable(pAdminOP.GetLuaState(), -2);
        lua_remove(pAdminOP.GetLuaState(), -2);
        lua_pushstring(pAdminOP.GetLuaState(), "PlayerDied");
        g_entityCache.PushPlayer(entID);
        if(iEntAttacker == 0)
            lua_pushnil(pAdminOP.GetLuaState());
        else
            g_entityCache.PushPlayer(iEntAttacker);
        lua_pushstring(pAdminOP.GetLuaState(), pszWeapon);
        lua_pcall(pAdminOP.GetLuaState(), 4, 0, 0);
    }
}

void CAdminOPPlayer :: OnSpawn()
{
    if(!entID)
        return;

    isAlive = 1;
    m_iGrenades = grenades_perlife.GetInt();
    SendCreditCounter();
    nextCreditCounter = engine->Time() + 5;
}

void CAdminOPPlayer :: OnChangeTeam(int newteam)
{
    if(!entID)
        return;

    // schedule update instead of sending now so that new info is included
    nextRemoteUpdate = 0;

    if(pAdminOP.FeatureStatus(FEAT_LUA))
    {
        lua_getglobal(pAdminOP.GetLuaState(), "hook");
        lua_pushliteral(pAdminOP.GetLuaState(), "Call");
        lua_gettable(pAdminOP.GetLuaState(), -2);
        lua_remove(pAdminOP.GetLuaState(), -2);
        lua_pushstring(pAdminOP.GetLuaState(), "PlayerChangedTeam");
        g_entityCache.PushPlayer(entID);
        lua_pushinteger(pAdminOP.GetLuaState(), newteam);
        lua_pcall(pAdminOP.GetLuaState(), 3, 0, 0);
    }
}

void CAdminOPPlayer :: OnChangeClass(int newclass)
{
    if(!entID)
        return;

    // schedule update instead of sending now so that new info is included
    nextRemoteUpdate = 0;

    if(pAdminOP.FeatureStatus(FEAT_LUA))
    {
        lua_getglobal(pAdminOP.GetLuaState(), "hook");
        lua_pushliteral(pAdminOP.GetLuaState(), "Call");
        lua_gettable(pAdminOP.GetLuaState(), -2);
        lua_remove(pAdminOP.GetLuaState(), -2);
        lua_pushstring(pAdminOP.GetLuaState(), "PlayerChangedClass");
        g_entityCache.PushPlayer(entID);
        lua_pushinteger(pAdminOP.GetLuaState(), newclass);
        lua_pcall(pAdminOP.GetLuaState(), 3, 0, 0);
    }
}

void CAdminOPPlayer :: OnPostInventoryApplication()
{
    if(!entID)
        return;

    if(pAdminOP.isTF2 && tf2_disable_fish.GetBool())
    {
        edict_t *pEntity = pAdminOP.GetEntityList()+entID;
        CBasePlayer *pPlayer = (CBasePlayer *)VFuncs::Instance(pEntity);
        if(pPlayer && GetPlayerClass() == TF2_CLASS_SCOUT)
        {
            CBaseCombatWeapon *pCurWeapon = VFuncs::Weapon_GetSlot(pPlayer, 2);
            CBaseCombatWeapon *pWep = NULL;
            if(!pCurWeapon && servertools)
            {
                pWep = (CBaseCombatWeapon *)CreateEntityByName("tf_weapon_bat");
                if(pWep)
                {
                    if(servertools) servertools->DispatchSpawn(pWep);
                    VFuncs::Weapon_Equip(pPlayer, pWep);
                }

                SayTextChatHud(UTIL_VarArgs("[%s] The Holy Mackerel is currently disabled and has been replaced with a bat.\n", pAdminOP.adminname));
            }
        }
    }
}

void CAdminOPPlayer :: RoundEnd()
{
    if(!entID)
        return;

    JetpackOff();
    HookOff();
}

void CAdminOPPlayer :: FlagEvent(EFlagEvent eventtype)
{
    if(!entID)
        return;

    if(eventtype == FLAGEVENT_PICKUP)
        m_bFlagCarrier = 1;
    else if(eventtype == FLAGEVENT_DROPPED)
        m_bFlagCarrier = 0;
}

Vector CAdminOPPlayer :: GetPosition()
{
    Vector retValue = Vector(0,0,0);

    if(!entID)
        return retValue;
    edict_t *pEntity = pAdminOP.GetEntityList()+entID;
    if(!pEntity)
        return retValue;

    CBasePlayer *pPlayer = (CBasePlayer *)VFuncs::Instance(pEntity);
    if(!pPlayer)
        return retValue;

    IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+entID);
    if(info)
    {
        retValue = VFuncs::GetAbsOrigin(pPlayer);
    }

    return retValue;
}

int CAdminOPPlayer :: GetTeam()
{
    if(!entID)
        return 0;

    edict_t *pEntity = pAdminOP.GetEntityList()+entID;
    if(!pEntity)
        return 0;

    IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pEntity);
    if(info)
    {
        if(info->IsConnected())
        {
            return info->GetTeamIndex();
        }
    }
    return 0;
}

const char *CAdminOPPlayer :: GetTeamName()
{
    int team;

    if(!entID)
        return "";

    edict_t *pEntity = pAdminOP.GetEntityList()+entID;
    if(!pEntity)
        return "";

    IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pEntity);
    if(info)
    {
        if(info->IsConnected())
        {
            team = info->GetTeamIndex();
            return pAdminOP.TeamName(team);
        }
    }
    return "";
}

int CAdminOPPlayer :: GetPlayerClass()
{
    if(!entID)
        return 0;

    edict_t *pEntity = pAdminOP.GetEntityList()+entID;
    if(!pEntity)
        return 0;

    CBasePlayer *pPlayer = (CBasePlayer *)VFuncs::Instance(pEntity);
    if(!pPlayer)
        return 0;

    if(pAdminOP.isTF2)
    {
        return VFuncs::GetPlayerClass(pPlayer);
    }
    return 0;
}

void CAdminOPPlayer :: GetMyUserData(const char *pszName)
{
    bool wasLoggedIn = hasAdminData ? adminData.loggedIn : 0;
    int hadPassword = hasAdminData ? adminData.password[0] : 0;
    edict_t *pEdPlayer = pAdminOP.GetEntityList()+entID;
    IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo(pEdPlayer);
    memset(&adminData, 0, sizeof(adminData));
    listDenied.Purge();
    listAdminAllow.Purge();
    listAdminDenied.Purge();
    hasAdminData = 0;
    if(playerinfo)
    {
        unsigned short i;
        int adminID = pAdminOP.adminData.InvalidIndex();

        if(sipLogin && hasSip)
        {
            hasAdminData = 1;
            adminData.baseLevel = 131071;
            strncpy(adminData.id, engine->GetPlayerNetworkIDString(pEdPlayer), sizeof(adminData.id));
            adminData.indexNumber = 0;
            adminData.loggedIn = 0;
            adminData.password[0] = '\0';
            adminData.type = ADMIN_TYPE_STEAMID;
            listDenied.Purge();
            listAdminAllow.Purge();
            listAdminDenied.Purge();
            if(!hasDev)
            {
                admindatacmd_t noRcon;
                strcpy(noRcon.cmd, "rcon");
                noRcon.indexNumber = 0;
                listAdminDenied.AddToTail(noRcon);
            }
            return;
        }
        for ( i=pAdminOP.adminData.Head(); i != pAdminOP.adminData.InvalidIndex(); i = pAdminOP.adminData.Next( i ) )
        {
            admindata_t *admin = &pAdminOP.adminData.Element(i);
            if(admin->type == ADMIN_TYPE_NAME)
            {
                if(!stricmp(admin->id, pszName ? pszName : playerinfo->GetName()))
                {
                    adminID = i;
                    break;
                }
            }
            else if(admin->type == ADMIN_TYPE_STEAMID)
            {
                const char *pszNetworkID = engine->GetPlayerNetworkIDString(pEdPlayer);
                if(pszNetworkID)
                {
                    if(!stricmp(admin->id, pszNetworkID))
                    {
                        adminID = i;
                        break;
                    }
                }
                else
                {
                    Msg("[SOURCEOP] Warning GetNetworkIDString returned NULL.\n");
                }
            }
            else if(admin->type == ADMIN_TYPE_IP)
            {
                if(!stricmp(admin->id, IP))
                {
                    adminID = i;
                    break;
                }
            }
        }
        if(adminID != pAdminOP.adminData.InvalidIndex())
        {
            admindata_t *admin = &pAdminOP.adminData.Element(i);
            admindatacmd_t *adminCmd;
            hasAdminData = 1;
            memcpy(&adminData, admin, sizeof(adminData));

            for ( unsigned short i=pAdminOP.listDenied.Head(); i != pAdminOP.listDenied.InvalidIndex(); i = pAdminOP.listDenied.Next( i ) )
            {
                adminCmd = &pAdminOP.listDenied.Element(i);
                if(adminCmd->indexNumber == admin->indexNumber)
                {
                    //Msg("Command denied %i %s\n", adminCmd->indexNumber, adminCmd->cmd);
                    listDenied.AddToTail(*adminCmd);
                }
            }
            for ( unsigned short i=pAdminOP.listAdminAllow.Head(); i != pAdminOP.listAdminAllow.InvalidIndex(); i = pAdminOP.listAdminAllow.Next( i ) )
            {
                adminCmd = &pAdminOP.listAdminAllow.Element(i);
                if(adminCmd->indexNumber == admin->indexNumber)
                {
                    //Msg("Admin allow %i %s\n", adminCmd->indexNumber, adminCmd->cmd);
                    listAdminAllow.AddToTail(*adminCmd);
                }
            }
            for ( unsigned short i=pAdminOP.listAdminDenied.Head(); i != pAdminOP.listAdminDenied.InvalidIndex(); i = pAdminOP.listAdminDenied.Next( i ) )
            {
                adminCmd = &pAdminOP.listAdminDenied.Element(i);
                if(adminCmd->indexNumber == admin->indexNumber)
                {
                    //Msg("Admin denied %i %s\n", adminCmd->indexNumber, adminCmd->cmd);
                    listAdminDenied.AddToTail(*adminCmd);
                }
            }
        }
        else if(wasLoggedIn && hadPassword)
        {
            //Msg("Ogm...logged out\n");
        }
    }
}

void CAdminOPPlayer :: UpdateAdminLevel()
{
    if(entID)
    {
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+entID);
        if(info)
        {
            if(info->IsConnected() && hasAdminData)
            {
                if( (adminData.password[0] == '\0' && adminData.loggedIn == 0) || (adminData.password[0] != '\0' && FStrEq(adminData.password, engine->GetClientConVarValue(entID, "DF_password")) && adminData.loggedIn == 0) )
                {
                    adminData.loggedIn = 1;
                    KeyValues *kv = new KeyValues( "menu" );
                    kv->SetString( "title", "You have logged in as admin" );
                    kv->SetInt( "level", 5 );
                    kv->SetInt( "time", 7 );
                    
                    helpers->CreateMessage( pAdminOP.GetEntityList()+entID, DIALOG_MSG, kv, &g_ServerPlugin );
                    kv->deleteThis();
                }
                else if(adminData.loggedIn)
                {
                    if(adminData.password[0] != '\0' && !FStrEq(adminData.password, engine->GetClientConVarValue(entID, "DF_password")))
                    {
                        adminData.loggedIn = 0;
                        KeyValues *kv = new KeyValues( "menu" );
                        kv->SetString( "title", "You have logged out of admin" );
                        kv->SetInt( "level", 5 );
                        kv->SetInt( "time", 7 );
                        
                        helpers->CreateMessage( pAdminOP.GetEntityList()+entID, DIALOG_MSG, kv, &g_ServerPlugin );
                        kv->deleteThis();
                    }
                }
                /*
                char gamedir[256];
                char filepath[512];
                char buf[384];
                FILE *fp;

                engine->GetGameDir(gamedir, sizeof(gamedir));
                Q_snprintf(filepath, sizeof(filepath), "%s/%s/DF_admins.txt", gamedir, pAdminOP.DataDir());
                V_FixSlashes(filepath);
                fp = fopen(filepath, "rt");

                if(fp)
                {
                    bool authed = 0;
                    while(!feof(fp))
                    {
                        if(fgets(buf, sizeof(buf), fp))
                        {
                            if(Q_strncmp(buf, "#", 1) && Q_strncmp(buf, "//", 2) && Q_strncmp(buf, "\n", 1) && strlen(buf) >= 5 && strstr(buf, ";"))
                            {
                                char seps[] = ";";
                                char *id;
                                char *password;
                                char *accessnum;

                                id = strtok( buf, seps );
                                password = strtok( NULL, seps );
                                accessnum = strtok( NULL, seps );

                                if(FStrEq(id, engine->GetPlayerNetworkIDString(pEdPlayer)))
                                {
                                    if(FStrEq(password, engine->GetClientConVarValue(entID, "DF_password")))
                                    {
                                        int newaccess = atoi(accessnum);
                                        if(newaccess > adminLevel)
                                        {
                                            KeyValues *kv = new KeyValues( "menu" );
                                            kv->SetString( "title", "You have logged in as admin" );
                                            kv->SetInt( "level", 5 );
                                            kv->SetInt( "time", 7 );
                                            
                                            helpers->CreateMessage( pAdminOP.GetEntityList()+entID, DIALOG_MSG, kv, &g_ServerPlugin );
                                            kv->deleteThis();
                                        }
                                        adminLevel = newaccess;
                                        authed = 1;
                                    }
                                }
                            }
                        }
                    }
                    fclose(fp);
                    if(!authed)
                    {
                        if(adminLevel)
                        {
                            KeyValues *kv = new KeyValues( "menu" );
                            kv->SetString( "title", "You have logged out as admin" );
                            kv->SetInt( "level", 5 );
                            kv->SetInt( "time", 7 );
                            
                            helpers->CreateMessage( pAdminOP.GetEntityList()+entID, DIALOG_MSG, kv, &g_ServerPlugin );
                            kv->deleteThis();
                        }
                        adminLevel = 0;
                    }
                }
                */
            }
        }
    }
}

bool CAdminOPPlayer :: IsAlive(void)
{
    if(!entID)
        return 0;

    edict_t *pEntity = pAdminOP.GetEntityList()+entID;
    if(!pEntity)
        return 0;

    CBasePlayer *pPlayer = (CBasePlayer *)VFuncs::Instance(pEntity);
    if(!pPlayer)
        return 0;

    IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+entID);
    if(info)
    {
        if(info->IsConnected())
        {
            return VFuncs::IsAlive(pPlayer);
        }
    }
    return 0;
}

// Checks to see if a user has access to a certain command level
int CAdminOPPlayer :: IsAdmin(int level)
{
    if(entID && playerState)
    {
        if(adminData.loggedIn)
        {
            if(level == 0)
            {
                if(adminData.baseLevel > 0) return 1;
            }
            return adminData.baseLevel & level;
        }
        else
        {
            if(level == 0)
            {
                if(pAdminOP.defaultUser.adminData.baseLevel > 0) return 1;
            }
            return pAdminOP.defaultUser.adminData.baseLevel & level;
        }
    }
    return 0;
}

// Checks to see if a user has access to a command
int CAdminOPPlayer :: IsAdmin(int level, const char *szCmd)
{
    bool cmdAllowed = 0;
    bool cmdDenied = 0;
    int baselevel = adminData.loggedIn ? adminData.baseLevel : pAdminOP.defaultUser.adminData.baseLevel;
    admindatacmd_t *adminCmd;

    if(adminData.loggedIn)
    {
        for ( unsigned short i=listAdminAllow.Head(); i != listAdminAllow.InvalidIndex(); i = listAdminAllow.Next( i ) )
        {
            adminCmd = &listAdminAllow.Element(i);
            if(!stricmp(adminCmd->cmd, szCmd))
            {
                cmdAllowed = 1;
                break;
            }
        }
        for ( unsigned short i=listAdminDenied.Head(); i != listAdminDenied.InvalidIndex(); i = listAdminDenied.Next( i ) )
        {
            adminCmd = &listAdminDenied.Element(i);
            if(!stricmp(adminCmd->cmd, szCmd))
            {
                cmdDenied = 1;
                break;
            }
        }
    }
    else
    {
        for ( unsigned short i=pAdminOP.defaultUser.listAdminAllow.Head(); i != pAdminOP.defaultUser.listAdminAllow.InvalidIndex(); i = pAdminOP.defaultUser.listAdminAllow.Next( i ) )
        {
            adminCmd = &pAdminOP.defaultUser.listAdminAllow.Element(i);
            if(!stricmp(adminCmd->cmd, szCmd))
            {
                cmdAllowed = 1;
                break;
            }
        }
        for ( unsigned short i=pAdminOP.defaultUser.listAdminDenied.Head(); i != pAdminOP.defaultUser.listAdminDenied.InvalidIndex(); i = pAdminOP.defaultUser.listAdminDenied.Next( i ) )
        {
            adminCmd = &pAdminOP.defaultUser.listAdminDenied.Element(i);
            if(!stricmp(adminCmd->cmd, szCmd))
            {
                cmdDenied = 1;
                break;
            }
        }
    }

    if(entID && playerState)
    {
        if(level == 0)
        {
            if( ((baselevel > 0) || cmdAllowed ) && !cmdDenied )
                return 1;
            else
                return 0;
        }
        else
        {
            if( ((baselevel & level) || cmdAllowed ) && !cmdDenied )
                return 1;
            else
                return 0;
        }
    }
    return 0;
}

// Checks to see if a standard command is denied
int CAdminOPPlayer :: IsDenied(const char *szCmd)
{
    bool cmdDenied = false;
    admindatacmd_t *adminCmd;

    if(adminData.loggedIn)
    {
        for ( unsigned short i=listDenied.Head(); i != listDenied.InvalidIndex(); i = listDenied.Next( i ) )
        {
            adminCmd = &listDenied.Element(i);
            if(!stricmp(adminCmd->cmd, szCmd))
            {
                cmdDenied = 1;
                break;
            }
        }
    }
    else
    {
        for ( unsigned short i=pAdminOP.defaultUser.listDenied.Head(); i != pAdminOP.defaultUser.listDenied.InvalidIndex(); i = pAdminOP.defaultUser.listDenied.Next( i ) )
        {
            adminCmd = &pAdminOP.defaultUser.listDenied.Element(i);
            if(!stricmp(adminCmd->cmd, szCmd))
            {
                cmdDenied = 1;
                break;
            }
        }
    }

    if(entID && playerState)
    {
        return cmdDenied;
    }
    return 0;
}

bool CAdminOPPlayer :: IsHiddenFrom(int iPlayerEntIndex)
{
    // if the player is allowed to have hidden mode set and he/she does and the player requesting is not an admin
    return (this->IsAdmin(8192, "hidden") && atoi(engine->GetClientConVarValue(entID, "DF_hidden"))) &&
        !(pAdminOP.pAOPPlayers[iPlayerEntIndex-1].IsAdmin(8192, "whoisprivilege"));
}

void CAdminOPPlayer :: Slap()
{
    if(entID > 0)
    {
        edict_t *pEntity = pAdminOP.GetEntityList()+entID;
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pEntity);
        if(info)
        {
            if(info->IsConnected())
            {
                Vector vecVel;
                CBroadcastRecipientFilter filter;

                vecVel.x = random->RandomInt(-1000, 1000);
                vecVel.y = random->RandomInt(-1000, 1000);
                vecVel.z = random->RandomInt(-200, 200);

                CBaseEntity *pBase;
                CBasePlayer *pPlayer;
                pBase = CBaseEntity::Instance(pEntity);
                pPlayer = (CBasePlayer *)pBase;

                VFuncs::Teleport(pBase, NULL, NULL, &vecVel);
                if(VFuncs::GetHealth(pPlayer) > 10)
                    SetHealth(VFuncs::GetHealth(pPlayer)-10);
                else
                    SetHealth(1);

                enginesound->EmitSound((CRecipientFilter&)filter, entID, CHAN_AUTO, "physics/body/body_medium_impact_hard6.wav", 0.6, 0, 0, random->RandomInt(90, 110), &VFuncs::GetAbsOrigin(pBase));  
            }
        }
    }
}

void CAdminOPPlayer :: Kill()
{
    SetHealth(0);
}

void CAdminOPPlayer :: SetHealth(int iHealth)
{
    if(!entID)
        return;

    edict_t *pEntity = pAdminOP.GetEntityList()+entID;
    if(!pEntity)
        return;

    IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pEntity);
    if(info)
    {
        if(info->IsConnected())
        {
            CBasePlayer *pPlayer = (CBasePlayer *)VFuncs::Instance(pEntity);
            if(iHealth > 0)
            {
                variant_t value;

                value.SetString( MAKE_STRING(UTIL_VarArgs("health %i", iHealth)) );
                VFuncs::AcceptInput(pPlayer,  "AddOutput", pPlayer, pPlayer, value, 0 );
            }
            else
            {
                CTakeDamageInfo info;
                info.SetDamage( VFuncs::GetHealth(pPlayer) + 10 );
                info.SetAttacker( pPlayer );
                info.SetDamageType( DMG_GENERIC );
                info.SetDamagePosition( VFuncs::WorldSpaceCenter(pPlayer) );
                VFuncs::OnTakeDamage(pPlayer, info);
            }
        }
    }
}

void CAdminOPPlayer :: SetPlayerClass(int iPlayerClass)
{
    if(!entID)
        return;

    edict_t *pEntity = pAdminOP.GetEntityList()+entID;
    if(!pEntity)
        return;

    CBasePlayer *pPlayer = (CBasePlayer *)VFuncs::Instance(pEntity);
    if(!pPlayer)
        return;

    if(pAdminOP.isTF2)
    {
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pEntity);
        if(info)
        {
            if(info->IsConnected())
            {
                int iCurPlayerClass = GetPlayerClass();
                if(iCurPlayerClass != iPlayerClass)
                {
                    VFuncs::SetPlayerClass(pPlayer, iPlayerClass);
                }
            }
        }
    }
}

void CAdminOPPlayer :: SetTeam(int iTeam)
{
    if(!entID)
        return;

    edict_t *pEntity = pAdminOP.GetEntityList()+entID;
    if(!pEntity)
        return;

    CBasePlayer *pPlayer = (CBasePlayer *)VFuncs::Instance(pEntity);
    if(!pPlayer)
        return;

    IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pEntity);
    if(info)
    {
        if(info->IsConnected())
        {
            int iCurTeam = info->GetTeamIndex();
            if(iCurTeam != iTeam)
            {
                VFuncs::ChangeTeam(pPlayer, iTeam);
                if(iCurTeam == 1)
                    VFuncs::StopObserverMode(pPlayer);
                else if(iTeam == 1)
                    VFuncs::StartObserverMode(pPlayer, OBS_MODE_ROAMING);
            }
        }
    }
}

void CAdminOPPlayer :: DoDamage(int attacker, int dmg, const char *pszWeapon)
{
    if(!entID)
        return;

    edict_t *pEntity = pAdminOP.GetEntityList()+entID;
    if(!pEntity)
        return;

    CBasePlayer *pPlayer = (CBasePlayer *)VFuncs::Instance(pEntity);
    if(!pPlayer)
        return;

    CTakeDamageInfo info;
    CBaseEntity *pKiller = pPlayer;
    const char *save = VFuncs::GetClassname(pPlayer);

    if(attacker)
    {
        edict_t *pKillerEdict = pAdminOP.GetEntityList() + attacker;
        IPlayerInfo *killerinfo = playerinfomanager->GetPlayerInfo(pKillerEdict);
        if(killerinfo)
        {
            if(killerinfo->IsConnected())
            {
                pKiller = CBaseEntity::Instance(pKillerEdict);
            }
        }
    }
    // HACKHACKHACK: set classname so that death message shows this as weapon
    if(pszWeapon)
        VFuncs::SetClassname(pPlayer, pszWeapon);
    info.SetDamage( dmg );
    info.SetAttacker( pKiller );
    info.SetInflictor( pPlayer );
    info.SetDamageType( DMG_GENERIC );
    VFuncs::OnTakeDamage(pPlayer, info);
    // set classname back to what it was
    if(pszWeapon)
        VFuncs::SetClassname(pPlayer, save);
}

void CAdminOPPlayer :: HookOff()
{
    if(!entID)
        return;

    edict_t *pEntity = pAdminOP.GetEntityList()+entID;
    if(!pEntity)
        return;

    CBasePlayer *pPlayer = (CBasePlayer *)VFuncs::Instance(pEntity);
    if(!pPlayer)
        return;

    if(Hook.OnHook)
    {
        CBroadcastRecipientFilter filter;
        Hook.OnHook = 0;
        Hook.LastHook = engine->Time();
        if(Hook.HookEnt)
        {
            UTIL_Remove(Hook.HookEnt);
        }
        if(te) te->KillPlayerAttachments(filter, 0, entID); // doesn't work?
    }
}

void CAdminOPPlayer :: JetpackActivate()
{
    if(!entID)
        return;

    CBasePlayer *pPlayer = (CBasePlayer *)pAdminOP.GetEntity(entID);
    if(!pPlayer)
        return;

    /*IPhysicsObject* pObj = VFuncs::VPhysicsGetObject(pPlayer);
    if(pObj)
    {
        pObj->EnableGravity(0);
    }*/
    /*Vector origvecVel = VFuncs::GetAbsVelocity(pPlayer);
    Vector newvecVel = VFuncs::GetAbsVelocity(pPlayer); 
    origvecVel.z += 60;
    newvecVel.z += 1000;

    VFuncs::Teleport(pPlayer, NULL, NULL, &newvecVel);
    VFuncs::Teleport(pPlayer, NULL, NULL, &origvecVel);*/

    Vector origvecVel = VFuncs::GetAbsVelocity(pPlayer);
    Vector vecOrigin = VFuncs::GetAbsOrigin(pPlayer);
    origvecVel.z += 40;

    // only move the player if he is on the ground
    vecOrigin.z -= 0.5;
    trace_t trace;
    UTIL_TraceEntity( pPlayer, vecOrigin, vecOrigin, MASK_SOLID, pPlayer, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );
    if ( trace.startsolid && trace.allsolid )
    {
        // and only move the player if it won't get him stuck
        vecOrigin.z += 20.5; // 0.5 to get back to zero offset then +20 offset
        UTIL_TraceEntity( pPlayer, vecOrigin, vecOrigin, MASK_SOLID, pPlayer, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );
        if ( !trace.startsolid && !trace.allsolid )
        {
            VFuncs::Teleport(pPlayer, &vecOrigin, NULL, &origvecVel);
        }
    }


    Jetpack.OnJetpack = 1;
    Jetpack.NextChargeTime = engine->Time();
    Jetpack.LastThrust = engine->Time();
    Jetpack.SteamEnt = NULL;
    Jetpack.FlameEnt1 = NULL;
    //Jetpack.FlameEnt2 = NULL;

    Vector forward, right, up;
    QAngle playerAngles = VFuncs::EyeAngles(pPlayer);
    playerAngles.x = 0;
    //Msg("%f %f %f\n", playerAngles.x, playerAngles.y, playerAngles.z);

    AngleVectors( playerAngles, &forward, &right, &up );

    forward.z = 0;

    vecOrigin = vecOrigin - (forward * 14) + (up * 46);

    CBaseEntity *pSteam = CreateEntityByName( "env_steam" );
    if(pSteam)
    {
        //Vector vecOrigin = VFuncs::GetAbsOrigin(pPlayer) + Vector(64,0,138);
        QAngle vecAngles( 90, 0, 0 );
        VFuncs::KeyValue(pSteam,  "angles", "90 0 0" );
        VFuncs::KeyValue(pSteam,  "origin", UTIL_VarArgs("%0.3f %0.3f %0.3f", vecOrigin.x, vecOrigin.y, vecOrigin.z));
        VFuncs::KeyValue(pSteam,  "spawnflags", "0" );
        VFuncs::KeyValue(pSteam,  "InitialState", "0" );
        VFuncs::KeyValue(pSteam,  "type", "1" );
        VFuncs::KeyValue(pSteam,  "SpreadSpeed", "30" );
        VFuncs::KeyValue(pSteam,  "Speed", "180" );
        VFuncs::KeyValue(pSteam,  "StartSize", "10" );
        VFuncs::KeyValue(pSteam,  "EndSize", "25" );
        VFuncs::KeyValue(pSteam,  "Rate", "52" );
        VFuncs::KeyValue(pSteam,  "JetLength", "80" );
        VFuncs::KeyValue(pSteam,  "rendercolor", "255 255 255" );
        VFuncs::KeyValue(pSteam,  "renderamt", "255" );
        VFuncs::KeyValue(pSteam,  "targetname", "SOPThruster001_Steam001" );
        //VFuncs::KeyValue(pSteam,  "parentname", "SOPThruster001_Can001" );
        VFuncs::Spawn(pSteam);
        VFuncs::Activate(pSteam);
        if(pAdminOP.isCstrike || pAdminOP.isDod || pAdminOP.isTF2)
            VFuncs::SetParent(pSteam, pPlayer, 1);
        else
            VFuncs::SetParent(pSteam, pPlayer);
        Jetpack.SteamEnt = pSteam;
    }

    CBaseEntity *pFire1 = CreateEntityByName( "env_steam" );
    if(pFire1)
    {
        VFuncs::KeyValue(pFire1,  "angles", "90 0 0" );
        VFuncs::KeyValue(pFire1,  "origin", UTIL_VarArgs("%0.3f %0.3f %0.3f", vecOrigin.x, vecOrigin.y, vecOrigin.z));
        VFuncs::KeyValue(pFire1,  "spawnflags", "0" );
        VFuncs::KeyValue(pFire1,  "InitialState", "0" );
        VFuncs::KeyValue(pFire1,  "type", "0" );
        VFuncs::KeyValue(pFire1,  "SpreadSpeed", "30" );
        VFuncs::KeyValue(pFire1,  "Speed", "120" );
        VFuncs::KeyValue(pFire1,  "StartSize", "12" );
        VFuncs::KeyValue(pFire1,  "EndSize", "8" );
        VFuncs::KeyValue(pFire1,  "Rate", "26" );
        VFuncs::KeyValue(pFire1,  "JetLength", "16" );
        VFuncs::KeyValue(pFire1,  "rendercolor", "128 128 128" );
        VFuncs::KeyValue(pFire1,  "renderamt", "255" );
        VFuncs::KeyValue(pFire1,  "targetname", "SOPThruster001_Steam002" );
        VFuncs::Spawn(pFire1);
        VFuncs::Activate(pFire1);
        if(pAdminOP.isCstrike || pAdminOP.isDod || pAdminOP.isTF2)
            VFuncs::SetParent(pFire1, pPlayer, 1);
        else
            VFuncs::SetParent(pFire1, pPlayer);
        Jetpack.FlameEnt1 = pFire1;
    }

    /*CBaseEntity *pFire2 = CreateEntityByName( "env_steam" );
    if(pFire2)
    {
        vecOrigin = vecOrigin - (up * 12);
        VFuncs::KeyValue(pFire2,  "angles", "90 0 0" );
        VFuncs::KeyValue(pFire2,  "origin", UTIL_VarArgs("%0.3f %0.3f %0.3f", vecOrigin.x, vecOrigin.y, vecOrigin.z));
        VFuncs::KeyValue(pFire2,  "spawnflags", "0" );
        VFuncs::KeyValue(pFire2,  "InitialState", "0" );
        VFuncs::KeyValue(pFire2,  "type", "0" );
        VFuncs::KeyValue(pFire2,  "SpreadSpeed", "30" );
        VFuncs::KeyValue(pFire2,  "Speed", "120" );
        VFuncs::KeyValue(pFire2,  "StartSize", "12" );
        VFuncs::KeyValue(pFire2,  "EndSize", "1" );
        VFuncs::KeyValue(pFire2,  "Rate", "26" );
        VFuncs::KeyValue(pFire2,  "JetLength", "30" );
        VFuncs::KeyValue(pFire2,  "rendercolor", "255 255 0" );
        VFuncs::KeyValue(pFire2,  "renderamt", "255" );
        VFuncs::KeyValue(pFire2,  "targetname", "SOPThruster001_Steam003" );
        VFuncs::Spawn(pFire2);
        VFuncs::Activate(pFire2);
        VFuncs::SetParent(pFire2, pPlayer);
        Jetpack.FlameEnt2 = pFire2;
    }*/

    //CBroadcastRecipientFilter filter;
    Vector vecSndOrigin = VFuncs::GetAbsOrigin(pPlayer);
    CPASFilter filter( vecSndOrigin );
    enginesound->EmitSound((CRecipientFilter&)filter, VFuncs::entindex(pPlayer), CHAN_BODY, "ambient/gas/cannister_loop.wav", 0.5, SNDLVL_NORM, 0, random->RandomInt(90, 110), &vecSndOrigin);  

    variant_t emptyVariant;
    if(pSteam) VFuncs::AcceptInput(pSteam,  "TurnOn", pSteam, pSteam, emptyVariant, 0 );
    if(pFire1) VFuncs::AcceptInput(pFire1,  "TurnOn", pFire1, pFire1, emptyVariant, 0 );
}

void CAdminOPPlayer :: JetpackOff()
{
    if(!entID)
        return;

    edict_t *pEntity = pAdminOP.GetEntityList()+entID;
    if(!pEntity)
        return;

    CBasePlayer *pPlayer = (CBasePlayer *)VFuncs::Instance(pEntity);
    if(!pPlayer)
        return;

    if(Jetpack.OnJetpack)
    {
        /*IPhysicsObject* pObj = VFuncs::VPhysicsGetObject(pPlayer);
        if(pObj)
        {
            pObj->EnableGravity(1);
        }*/
        Jetpack.OnJetpack = 0;
        Jetpack.LastUse = engine->Time();
        if(Jetpack.SteamEnt)
            UTIL_Remove(Jetpack.SteamEnt);
        if(Jetpack.FlameEnt1)
            UTIL_Remove(Jetpack.FlameEnt1);
        /*if(Jetpack.FlameEnt2)
            UTIL_Remove(Jetpack.FlameEnt2);*/
        Jetpack.SteamEnt = NULL;
        Jetpack.FlameEnt1 = NULL;
        //Jetpack.FlameEnt2 = NULL;
        CBroadcastRecipientFilter filter;
        enginesound->EmitSound((CRecipientFilter&)filter, VFuncs::entindex(pPlayer), CHAN_BODY, "common/null.wav", 0.6, 0, 0, random->RandomInt(90, 110), &VFuncs::GetAbsOrigin(pPlayer));  
    }
}

bool CAdminOPPlayer :: SetNoclip(int iType) //0-Remove, 1-Give, 2-Toggle
{
    if(entID > 0)
    {
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+entID);
        if(info)
        {
            if(info->IsConnected())
            {
                bool noclipOn = 0;
                bool noclipOff = 0;
                CBaseEntity *pPlayer = CBaseEntity::Instance(pAdminOP.GetEntityList()+entID);
                switch(iType)
                {
                case 0:
                    noclipOff = 1;
                    break;
                case 1:
                    noclipOn = 1;
                    break;
                case 2:
                    if (VFuncs::GetMoveType(pPlayer) != MOVETYPE_NOCLIP)
                        noclipOn = 1;
                    else
                        noclipOff = 1;
                    break;
                default:
                    return 0;
                }
                if(noclipOn)
                {
                    if (VFuncs::GetMoveType(pPlayer) != MOVETYPE_NOCLIP)
                    {
                        // Disengage from hierarchy
                        //VFuncs::SetParent(pPlayer,  NULL );
                        VFuncs::SetMoveType( pPlayer, MOVETYPE_NOCLIP );
                        VFuncs::AddEFlags( pPlayer, EFL_NOCLIP_ACTIVE );
                    }
                }
                else if(noclipOff)
                {
                    if (VFuncs::GetMoveType(pPlayer) == MOVETYPE_NOCLIP)
                    {
                        VFuncs::RemoveEFlags( pPlayer, EFL_NOCLIP_ACTIVE );
                        VFuncs::SetMoveType( pPlayer, MOVETYPE_WALK );
                    }

                    Unstick();
                }
                return 1;
            }
        }
    }
    return 0;
}

bool CAdminOPPlayer :: SetGod(int iType) //0-Remove, 1-Give, 2-Toggle
{
    if(entID > 0)
    {
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+entID);
        if(info)
        {
            if(info->IsConnected())
            {
                CBaseEntity *pPlayer = CBaseEntity::Instance(pAdminOP.GetEntityList()+entID);
                switch(iType)
                {
                case 0:
                    VFuncs::RemoveFlag(pPlayer, FL_GODMODE);
                    VFuncs::SetTakeDamage(pPlayer, DAMAGE_YES);
                    break;
                case 1:
                    VFuncs::AddFlag(pPlayer, FL_GODMODE);
                    VFuncs::SetTakeDamage(pPlayer, DAMAGE_NO);
                    break;
                case 2:
                    VFuncs::ToggleFlag(pPlayer, FL_GODMODE);
                    if(VFuncs::GetFlags(pPlayer) & FL_GODMODE)
                    {
                        VFuncs::SetTakeDamage(pPlayer, DAMAGE_NO);
                    }
                    else
                    {
                        VFuncs::SetTakeDamage(pPlayer, DAMAGE_YES);
                    }
                    break;
                default:
                    return 0;
                }
                return 1;
            }
        }
    }
    return 0;
}

bool CAdminOPPlayer :: HasNoclip()
{
    if(entID > 0)
    {
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+entID);
        if(info)
        {
            if(info->IsConnected())
            {
                CBaseEntity *pPlayer = CBaseEntity::Instance(pAdminOP.GetEntityList()+entID);
                return (VFuncs::GetMoveType(pPlayer) == MOVETYPE_NOCLIP) ? 1 : 0;
            }
        }
    }
    return 0;
}

bool CAdminOPPlayer :: HasGod()
{
    if(entID > 0)
    {
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+entID);
        if(info)
        {
            if(info->IsConnected())
            {
                CBaseEntity *pPlayer = CBaseEntity::Instance(pAdminOP.GetEntityList()+entID);
                return (VFuncs::GetFlags(pPlayer) & FL_GODMODE) ? 1 : 0;
            }
        }
    }
    return 0;
}

BYTE CAdminOPPlayer :: GetFlagByte()
{
    BYTE ret = 0;

    if(IsGagged()) ret |= AOP_PL_GAGGED;
    if(HasNoclip()) ret |= AOP_PL_NOCLIP;
    if(HasGod()) ret |= AOP_PL_GOD;

    return ret;
}

int CAdminOPPlayer :: GetScore()
{
    if(!entID)
        return 0;

    edict_t *pEntity = pAdminOP.GetEntityList()+entID;
    if(!pEntity)
        return 0;

    IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pEntity);
    if(!info || !info->IsConnected())
        return 0;

    if(pAdminOP.isTF2)
    {
        CBaseEntity *pPlayerResource = pAdminOP.GetPlayerResource();
        if(pPlayerResource)
        {
            return VFuncs::GetTotalScore(pPlayerResource, entID);
        }
    }
    
    return info->GetFragCount();
}

void CAdminOPPlayer :: InitPlayerData()
{
    m_bSteamIDValidated = false;
    adminTutBlockSay = 0;
    adminTutPage = 0;
    memset(&adminData, 0, sizeof(adminData));
    hasAdminData = 0;
    physConstraints.Purge();
    spawnedEnts.Purge();
    thrusters.Purge();
    nextthink = 0;
    nextAdminUpdate = 0;
    nextRemoteUpdate = 0;
    nextSmallRemoteUpdate = 0;
    entMoving = 0;
    entRotating = 0;
    hasDon=0;
    donAmt=0;
    hasVip=0;
    hasSip=0;
    sipLogin=0;
    hasDev=0;
    m_bItemsLoaded = 0;
    userExtraInfo[0] = '\0';
    Hook.OnHook = 0;
    Jetpack.OnJetpack = 0;
    Jetpack.NextChargeTime = 0;
    Jetpack.Charge = 100;
    Jetpack.NextMessageLevelReset = 0;
    Jetpack.MessageLevel = JETPACK_MAXMESSAGELEVEL;
    Credits.PendingLoad = 1;    // try to load first think
    Credits.NextLoad = 0;       //
    Credits.TimeJoined = 0;
    Credits.totalconnects = 1;
    Kills.CumKillCount = 0;
    Kills.KillCount = 0;
    Kills.LastKillTime = 0;
    Kills.PlayMultiSound = 0;
    Kills.PlaySpreeSound = 0;
    PrivateChat.RemotePrivChat = 0;
    PrivateChat.RemotePrivChatID = 0;
    PrivateChat.LocalPrivChat = 0;
    PrivateChat.LocalPrivChatID = 0;
    gagged = 0;
    m_pendingCvarQueries.Purge();
    m_iVoiceTeam = 0;
    m_bFlagCarrier = 0;
    notBot = 1;
    nextCreditCounter = 0;
    hasInfo = 0;
    sendDup = 1;
    isAlive = 0;
    pingManager.Clear();
    pingManager.maxCount = highpingkicker_samples.GetInt();
    pingManager.SetPlayerIndex(entID);
    pTemp = NULL;
    pActiveWeapon = NULL;
    nextBuildTime = 0;
    m_iGrenades = grenades_perlife.GetInt();
    m_flGrenadeDetonateTime = 0.0f;
    entPhysMoving = 0;
    PhysMoving.m_hObject = NULL;
    PhysMoving.pBeam = NULL;
    memset(&infoString, 0, sizeof(infoString));
    memset(&oldInfo, 0, sizeof(oldInfo));
}

void CAdminOPPlayer :: ZeroCredits()
{
    memset(&Credits, 0, sizeof(Credits));
}

int CAdminOPPlayer :: LoadCredits(bool isReload)
{
    if(entID)
    {
        edict_t *pEdPlayer = pAdminOP.GetEntityList()+entID;
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+entID);
        if(info)
        {
            char PrevJoinName[sizeof(Credits.JoinName)];

            if(FStrEq(engine->GetPlayerNetworkIDString(pEdPlayer), "STEAM_ID_PENDING"))
                return 0;
            
            strcpy(PrevJoinName, Credits.JoinName);
            ZeroCredits();
            strcpy(Credits.WonID, engine->GetPlayerNetworkIDString(pEdPlayer));
            strcpy(Credits.JoinName, PrevJoinName);
            Credits.TimeJoined = engine->Time();

            for ( unsigned int i=pAdminOP.banList.Head(); i != pAdminOP.banList.InvalidIndex(); i = pAdminOP.banList.Next( i ) )
            {
                if(FStrEq(Credits.WonID, pAdminOP.banList.Element(i).szSteamID))
                {
                    engine->ClientPrintf(pAdminOP.GetEntityList()+entID, "[SOURCEOP] You have been banned from SourceOP for the following reason:\n");
                    engine->ClientPrintf(pAdminOP.GetEntityList()+entID, pAdminOP.banList.Element(i).szExtraInfo);
                    engine->ClientPrintf(pAdminOP.GetEntityList()+entID, "\n");
                    pAdminOP.KickPlayer(info, pAdminOP.banList.Element(i).szExtraInfo);
                }
            }

            for ( unsigned int i=pAdminOP.donList.Head(); i != pAdminOP.donList.InvalidIndex(); i = pAdminOP.donList.Next( i ) )
            {
                if(FStrEq(Credits.WonID, pAdminOP.donList.Element(i).szSteamID))
                {
                    hasDon = 1;
                    donAmt = pAdminOP.donList.Element(i).donAmt;
                    strncpy(userExtraInfo, pAdminOP.donList.Element(i).szExtraInfo, sizeof(userExtraInfo));
                }
            }
            for ( unsigned int i=pAdminOP.vipList.Head(); i != pAdminOP.vipList.InvalidIndex(); i = pAdminOP.vipList.Next( i ) )
            {
                if(FStrEq(Credits.WonID, pAdminOP.vipList.Element(i).szSteamID))
                {
                    hasVip = 1;
                    strncpy(userExtraInfo, pAdminOP.vipList.Element(i).szExtraInfo, sizeof(userExtraInfo));
                }
            }
            for ( unsigned int i=pAdminOP.sipList.Head(); i != pAdminOP.sipList.InvalidIndex(); i = pAdminOP.sipList.Next( i ) )
            {
                if(FStrEq(Credits.WonID, pAdminOP.sipList.Element(i).szSteamID))
                {
                    hasSip = 1;
                    strncpy(userExtraInfo, pAdminOP.sipList.Element(i).szExtraInfo, sizeof(userExtraInfo));
                }
            }
            for ( unsigned int i=pAdminOP.devList.Head(); i != pAdminOP.devList.InvalidIndex(); i = pAdminOP.devList.Next( i ) )
            {
                if(FStrEq(Credits.WonID, pAdminOP.devList.Element(i).szSteamID))
                {
                    hasDev = 1;
                    strncpy(userExtraInfo, pAdminOP.devList.Element(i).szExtraInfo, sizeof(userExtraInfo));
                }
            }

            for ( unsigned int i=pAdminOP.creditList.Head(); i != pAdminOP.creditList.InvalidIndex(); i = pAdminOP.creditList.Next( i ) )
            {
                creditsram_t *findCredits = &pAdminOP.creditList.Element(i);
                //Msg(" %i)\n  %s;%i;%i\n", i, findCredits->WonID, findCredits->credits, findCredits->lastsave);
                if (FStrEq(findCredits->WonID, engine->GetPlayerNetworkIDString(pEdPlayer)))
                {
                    Credits.CreditsJoin = findCredits->credits;
                    Credits.TimeJoin = findCredits->timeonserver;
                    if(isReload)
                        Credits.totalconnects = findCredits->totalconnects;
                    else
                        Credits.totalconnects = findCredits->totalconnects+1;
                    strcpy(Credits.NameTest, findCredits->LastName);
                    strcpy(Credits.LastName, findCredits->LastName);
                    strcpy(Credits.FirstName, findCredits->FirstName);
                    memcpy(&Credits.thismap, &findCredits->thismap, sizeof(Credits.thismap));
                    
                    engine->LogPrint(UTIL_VarArgs("[SOURCEOP] \"%s<%i><%s><%s>\" loaded credits \"%i\"\n", info->GetName(), info->GetUserID(), engine->GetPlayerNetworkIDString(pEdPlayer), GetTeamName(), Credits.CreditsJoin));

                    return 1;
                }
            }

            engine->LogPrint(UTIL_VarArgs("[SOURCEOP] \"%s<%i><%s><%s>\" loaded credits \"%i\" new player\n", info->GetName(), info->GetUserID(), engine->GetPlayerNetworkIDString(pEdPlayer), GetTeamName(), Credits.CreditsJoin));

            return 1;
        }
    }

    return 0;
}

int CAdminOPPlayer :: SaveCredits(bool isShutdown)
{
    if(!entID)
        return CRED_SAVEERROR_NOUSERID;

    edict_t *pEntity = pAdminOP.GetEntityList()+entID;
    IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pEntity);
    if(!isShutdown)
    {
        if(!info)
            return CRED_SAVEERROR_NOINFO;

        if(!info->IsConnected())
            return CRED_SAVEERROR_NOTCONNECTED;
    }
    if(Credits.PendingLoad)
        return CRED_SAVEERROR_PENDINGLOAD;

    /*time_t t1 = time (NULL);
    struct tm * timestruct = localtime (&t1);
    int dayssaved = (timestruct->tm_year * 365) + timestruct->tm_yday + int((timestruct->tm_year-1) / 4); */
    __time64_t ltime;
    _time64( &ltime );

    //char msg[256];
    char logmsg[512];

    for ( unsigned int i=pAdminOP.creditList.Head(); i != pAdminOP.creditList.InvalidIndex(); i = pAdminOP.creditList.Next( i ) )
    {
        creditsram_t *CreditsInfo = &pAdminOP.creditList.Element(i);

        if(FStrEq(CreditsInfo->WonID, Credits.WonID))
        {
            char FirstName[36];
            if(CreditsInfo->FirstName[0] == '\0')
            {
                strcpy(FirstName, Credits.JoinName);
            }
            else
            {
                strcpy(FirstName, CreditsInfo->FirstName);
            }
            CreditsInfo->credits = Credits.CreditsJoin + Credits.CreditsDuringGame;
            CreditsInfo->lastsave = ltime;
            CreditsInfo->timeonserver =  (int)Credits.TimeJoin + ((int)engine->Time() - (int)Credits.TimeJoined);
            CreditsInfo->totalconnects = Credits.totalconnects;
            strcpy(CreditsInfo->FirstName, FirstName);
            strcpy(CreditsInfo->LastName, Credits.LastName);
            if(isShutdown)
                strcpy(CreditsInfo->CurrentName, Credits.JoinName);
            else
                strcpy(CreditsInfo->CurrentName, info->GetName());
            strcpy(CreditsInfo->WonID, Credits.WonID);
            memcpy(&CreditsInfo->thismap, &Credits.thismap, sizeof(CreditsInfo->thismap));

            if(!isShutdown)
            {
                //Q_snprintf(msg, sizeof(msg), "[%s] Your credits have been saved in AdminOP's database.\n", pAdminOP.adminname);
                //engine->ClientPrintf(pEntity, msg);
                Q_snprintf(logmsg, sizeof(logmsg), "[SOURCEOP] \"%s<%i><%s><%s>\" saved credits \"%i\"\n", info->GetName(), info->GetUserID(), engine->GetPlayerNetworkIDString(pEntity), GetTeamName(), CreditsInfo->credits);
                engine->LogPrint(logmsg);
            }

            return 0;
        }
    }

    creditsram_t CreditsInfo;

    memset(&CreditsInfo, 0, sizeof(CreditsInfo));
    CreditsInfo.credits = Credits.CreditsJoin + Credits.CreditsDuringGame;
    CreditsInfo.lastsave = ltime;
    CreditsInfo.timeonserver =  (int)Credits.TimeJoin + ((int)engine->Time() - (int)Credits.TimeJoined);
    CreditsInfo.totalconnects = Credits.totalconnects;
    strcpy(CreditsInfo.FirstName, Credits.JoinName); //It's a new dude so we're going to have to use the JoinName
    strcpy(CreditsInfo.LastName, Credits.LastName);
    if(isShutdown)
        strcpy(CreditsInfo.CurrentName, Credits.JoinName);
    else
        strcpy(CreditsInfo.CurrentName, info->GetName());
    strcpy(CreditsInfo.WonID, Credits.WonID);
    memcpy(&CreditsInfo.thismap, &Credits.thismap, sizeof(CreditsInfo.thismap));
    pAdminOP.creditList.AddToTail(CreditsInfo);
    if(!isShutdown)
    {
        //Q_snprintf(msg, sizeof(msg), "[%s] Your credits have been saved in AdminOP's database.\n", pAdminOP.adminname);
        //engine->ClientPrintf(pEntity, msg);
        Q_snprintf(logmsg, sizeof(logmsg), "[SOURCEOP] \"%s<%i><%s><%s>\" saved credits \"%i\" new player\n", info->GetName(), info->GetUserID(), engine->GetPlayerNetworkIDString(pEntity), GetTeamName(), CreditsInfo.credits);
        engine->LogPrint(logmsg);
    }

    return 0;
}

const char *CAdminOPPlayer :: GetCurrentName()
{
    if(!entID)
        return "";

    IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+entID);
    if(!info)
        return GetJoinName();
    if(!info->IsConnected())
        return GetJoinName();

    return info->GetName();
}

int CAdminOPPlayer :: GetSpawnLimit()
{
    if(adminData.loggedIn)
    {
        if(adminData.spawnLimit != SPAWN_LIMIT_USE_CVAR)
        {
            return adminData.spawnLimit;
        }
    }

    return spawnlimit_admin.GetInt();
}

void CAdminOPPlayer :: AddCredits(int credits)
{
    if(!entID)
        return;

    Credits.CreditsDuringGame += credits;
    SendCreditCounter();
    nextCreditCounter = engine->Time() + 30;
}

void CAdminOPPlayer :: SendCreditCounter()
{
    if(!entID)
        return;

    if(pAdminOP.FeatureStatus(FEAT_CREDITS) && feature_credits.GetInt() && feature_showcredits.GetInt())
    {
        int iMsg = pAdminOP.GetMessageByName("HudMsg");
        if(iMsg >= 0)
        {
            int credits = Credits.CreditsJoin + Credits.CreditsDuringGame;
            char msg[128];
            bf_write *pWrite;
            CRecipientFilter filter;
            filter.AddRecipient(entID);

            Q_snprintf(msg, sizeof(msg), "Credits: %i\n%s", credits, pAdminOP.GetRank(credits));

            pWrite = engine->UserMessageBegin(&filter, iMsg);
                pWrite->WriteByte( 1 & 0xFF );  // channel
                pWrite->WriteFloat( feature_showcredits_x.GetFloat() );     // x
                pWrite->WriteFloat( feature_showcredits_y.GetFloat() );     // y
                pWrite->WriteByte( 255 );       // r1
                pWrite->WriteByte( 128 );       // g1
                pWrite->WriteByte( 64 );        // b1
                pWrite->WriteByte( 0 );         // a1
                pWrite->WriteByte( 0 );         // r2
                pWrite->WriteByte( 0 );         // g2
                pWrite->WriteByte( 0 );         // b2
                pWrite->WriteByte( 0 );         // a2
                pWrite->WriteByte( 0 );         // effect
                pWrite->WriteFloat( 0 );        // fade in time
                pWrite->WriteFloat( 0 );        // fade out time
                pWrite->WriteFloat( 33 );       // hold time
                pWrite->WriteFloat( 33 );       // fx time
                pWrite->WriteString( msg );
            engine->MessageEnd();
        }
    }
}

void CAdminOPPlayer :: SayText(const char *pText, int type)
{
    if(!entID)
        return;

    bf_write *pWrite;
    CReliableBroadcastRecipientFilter filter;
    filter.RemoveAllRecipients();
    filter.AddRecipient(entID);
    
    if(type != HUD_PRINTTALK)
    {
        pWrite = engine->UserMessageBegin(&filter, pAdminOP.GetMessageByName("TextMsg"));
        pWrite->WriteByte(type);
        pWrite->WriteString(pText);
        engine->MessageEnd();
    }
    else
    {
        int messageNumber = pAdminOP.GetMessageByName("SayText2");
        if(messageNumber == -1)
        {
            messageNumber = pAdminOP.GetMessageByName("SayText");
            pWrite = engine->UserMessageBegin(&filter, messageNumber);
                pWrite->WriteByte(0);
                pWrite->WriteString(pText);
                pWrite->WriteByte(0);
            engine->MessageEnd();
        }
        else
        {
            char realText[512];
            V_snprintf(realText, sizeof(realText), "\x01%s", pText);
            pWrite = engine->UserMessageBegin(&filter, messageNumber);
                pWrite->WriteByte(0);
                pWrite->WriteByte(1);
                pWrite->WriteString(realText);
            engine->MessageEnd();
        }
    }

    /*pWrite = engine->UserMessageBegin(&filter, 4);
    pWrite->WriteByte(type);
    pWrite->WriteString(pText);
    engine->MessageEnd();*/
}

void CAdminOPPlayer :: SayTextChatHud(const char *pText)
{
    if(!entID)
        return;

    bf_write *pWrite;
    CReliableBroadcastRecipientFilter filter;
    filter.RemoveAllRecipients();
    filter.AddRecipient(entID);

    pWrite = engine->UserMessageBegin(&filter, pAdminOP.GetMessageByName("SayText"));
        pWrite->WriteByte(0);
        pWrite->WriteString(pText);
        pWrite->WriteByte( 0 );
    engine->MessageEnd();

    /*pWrite = engine->UserMessageBegin(&filter, 4);
    pWrite->WriteByte(HUD_PRINTTALK);
    pWrite->WriteString(pText);
    engine->MessageEnd();

    pWrite = engine->UserMessageBegin(&filter, 4);
    pWrite->WriteByte(HUD_PRINTCONSOLE);
    pWrite->WriteString(pText);
    engine->MessageEnd();*/
}

void CAdminOPPlayer :: SayTextByPlayer(const char *pText, int player)
{
    if(!entID)
        return;

    bf_write *pWrite;
    CReliableBroadcastRecipientFilter filter;
    filter.RemoveAllRecipients();
    filter.AddRecipient(entID);
    if(player != -1)
    {
        filter.AddRecipient(player);
    }

    pWrite = engine->UserMessageBegin(&filter, pAdminOP.GetMessageByName("SayText"));
        pWrite->WriteByte(entID);
        pWrite->WriteString(pText);
        pWrite->WriteByte( 1 );
    engine->MessageEnd();
}

void CAdminOPPlayer :: ShowViewPortPanel( const char * name, bool bShow, KeyValues *data )
{
    if(!entID)
        return;

    bf_write *pWrite;
    CSingleUserRecipientFilter filter( entID );
    filter.MakeReliable();

    int count = 0;
    KeyValues *subkey = NULL;

    if ( data )
    {
        subkey = data->GetFirstSubKey();
        while ( subkey )
        {
            count++; subkey = subkey->GetNextKey();
        }

        subkey = data->GetFirstSubKey(); // reset 
    }

    pWrite = engine->UserMessageBegin( &filter, pAdminOP.GetMessageByName("VGUIMenu") );
        pWrite->WriteString( name ); // menu name
        pWrite->WriteByte( bShow?1:0 );
        pWrite->WriteByte( count );
        
        // write additional data (be carefull not more than 192 bytes!)
        while ( subkey )
        {
            pWrite->WriteString( subkey->GetName() );
            pWrite->WriteString( subkey->GetString() );
            subkey = subkey->GetNextKey();
        }
    engine->MessageEnd();
}

void CAdminOPPlayer :: ShowVoteMenu( unsigned int startmap, int level )
{
    if(!entID)
        return;

    IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+entID);

    if(!info)
        return;

    if(!info->IsConnected())
        return;

    edict_t *pEntity = pAdminOP.GetEntityList()+entID;

    KeyValues *kv = new KeyValues( "menu" );
    kv->SetString( "title", "Vote menu, hit ESC" );
    kv->SetInt( "level", level );
    kv->SetColor( "color", Color( 128, 255, 0, 255 ));
    kv->SetInt( "time", 20 );
    kv->SetString( "msg", UTIL_VarArgs("SourceOP vote menu.\nPick a map. Showing %i-%i of %i.", startmap+1, min((unsigned int)pAdminOP.mapList.Count(), startmap == 0 ? startmap+VOTEMENU_MAPS+1 : startmap+VOTEMENU_MAPS), pAdminOP.mapList.Count()) );

    unsigned int count = 0;
    unsigned int sendcount = 0;
    bool isMore = 0;
    for ( unsigned short i=pAdminOP.mapList.Head(); i != pAdminOP.mapList.InvalidIndex(); i = pAdminOP.mapList.Next( i ) )
    {
        char num[10], msg[128], cmd[256];
        maplist_t *data = &pAdminOP.mapList.Element(i);

        if(data)
        {
            count++;
            if(count > startmap)
            {
                sendcount++;
                Q_snprintf( num, sizeof(num), "%i", sendcount );
                Q_snprintf( msg, sizeof(msg), "[%i] %s", pAdminOP.GetMapVote()->GetCount(data->map), data->map );
                Q_snprintf( cmd, sizeof(cmd), "say \"vote %s\";cancelselect", data->map );

                if(strlen(msg) > 25)
                {
                    msg[23] = '.';
                    msg[24] = '.';
                    msg[25] = '\0';
                }
                KeyValues *item1 = kv->FindKey( num, true );
                item1->SetString( "msg", msg );
                item1->SetString( "command", cmd );
            }
            if( (startmap > 0 && sendcount >= VOTEMENU_MAPS) || (startmap == 0 && sendcount >= VOTEMENU_MAPS+1) )
            {
                isMore = (pAdminOP.mapList.Next( i ) != pAdminOP.mapList.InvalidIndex());
                break;
            }
        }
    }
    if(startmap > 0)
    {
        int menuitem = max(startmap-VOTEMENU_MAPS,0);

        if(menuitem == 1) menuitem = 0; // take in to account first page has one more item
        sendcount++;
        KeyValues *item1 = kv->FindKey( UTIL_VarArgs("%i", sendcount), true );
        item1->SetString( "msg", "Prev <<" );
        item1->SetString( "command", UTIL_VarArgs("votemenu %i", menuitem) );
    }
    if(isMore)
    {
        sendcount++;
        KeyValues *item1 = kv->FindKey( UTIL_VarArgs("%i", sendcount), true );
        item1->SetString( "msg", "Next >>" );
        item1->SetString( "command", UTIL_VarArgs("votemenu %i", startmap == 0 ? startmap+VOTEMENU_MAPS+1 : startmap+VOTEMENU_MAPS) );
    }

    helpers->CreateMessage( pEntity, DIALOG_MENU, kv, &g_ServerPlugin );
    kv->deleteThis();
}

void CAdminOPPlayer :: ShowBindMenu( const char *description, const char *command, unsigned int menuopt )
{
    if(!entID)
        return;

    IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+entID);

    if(!info)
        return;

    if(!info->IsConnected())
        return;

    edict_t *pEntity = pAdminOP.GetEntityList()+entID;

    KeyValues *kv = new KeyValues( "menu" );
    kv->SetString( "title", "Bind menu, hit ESC" );
    kv->SetInt( "level", 1 );
    kv->SetColor( "color", Color( 128, 255, 0, 255 ));
    kv->SetInt( "time", 20 );
    kv->SetString( "msg", UTIL_VarArgs("SourceOP bind menu.\nPick a key to bind %s to.", description) );

    KeyValues *item;
    switch(menuopt)
    {
    case 1:
        item = kv->FindKey( "1", true );
        item->SetString( "msg", "A" );
        item->SetString( "command", UTIL_VarArgs("bind a %s;cancelselect", command) );
        item = kv->FindKey( "2", true );
        item->SetString( "msg", "B" );
        item->SetString( "command", UTIL_VarArgs("bind b %s;cancelselect", command) );
        item = kv->FindKey( "3", true );
        item->SetString( "msg", "C" );
        item->SetString( "command", UTIL_VarArgs("bind c %s;cancelselect", command) );
        item = kv->FindKey( "4", true );
        item->SetString( "msg", "D" );
        item->SetString( "command", UTIL_VarArgs("bind d %s;cancelselect", command) );
        item = kv->FindKey( "5", true );
        item->SetString( "msg", "E" );
        item->SetString( "command", UTIL_VarArgs("bind e %s;cancelselect", command) );
        item = kv->FindKey( "6", true );
        item->SetString( "msg", "F" );
        item->SetString( "command", UTIL_VarArgs("bind f %s;cancelselect", command) );
        item = kv->FindKey( "7", true );
        item->SetString( "msg", "G" );
        item->SetString( "command", UTIL_VarArgs("bind g %s;cancelselect", command) );
        item = kv->FindKey( "8", true );
        item->SetString( "msg", "<< Back" );
        item->SetString( "command", UTIL_VarArgs("bindmenu %s 0", command) );
        break;
    case 2:
        item = kv->FindKey( "1", true );
        item->SetString( "msg", "H" );
        item->SetString( "command", UTIL_VarArgs("bind h %s;cancelselect", command) );
        item = kv->FindKey( "2", true );
        item->SetString( "msg", "I" );
        item->SetString( "command", UTIL_VarArgs("bind i %s;cancelselect", command) );
        item = kv->FindKey( "3", true );
        item->SetString( "msg", "J" );
        item->SetString( "command", UTIL_VarArgs("bind j %s;cancelselect", command) );
        item = kv->FindKey( "4", true );
        item->SetString( "msg", "K" );
        item->SetString( "command", UTIL_VarArgs("bind k %s;cancelselect", command) );
        item = kv->FindKey( "5", true );
        item->SetString( "msg", "L" );
        item->SetString( "command", UTIL_VarArgs("bind l %s;cancelselect", command) );
        item = kv->FindKey( "6", true );
        item->SetString( "msg", "M" );
        item->SetString( "command", UTIL_VarArgs("bind m %s;cancelselect", command) );
        item = kv->FindKey( "7", true );
        item->SetString( "msg", "N" );
        item->SetString( "command", UTIL_VarArgs("bind n %s;cancelselect", command) );
        item = kv->FindKey( "8", true );
        item->SetString( "msg", "<< Back" );
        item->SetString( "command", UTIL_VarArgs("bindmenu %s 0", command) );
        break;
    case 3:
        item = kv->FindKey( "1", true );
        item->SetString( "msg", "O" );
        item->SetString( "command", UTIL_VarArgs("bind o %s;cancelselect", command) );
        item = kv->FindKey( "2", true );
        item->SetString( "msg", "P" );
        item->SetString( "command", UTIL_VarArgs("bind p %s;cancelselect", command) );
        item = kv->FindKey( "3", true );
        item->SetString( "msg", "Q" );
        item->SetString( "command", UTIL_VarArgs("bind q %s;cancelselect", command) );
        item = kv->FindKey( "4", true );
        item->SetString( "msg", "R" );
        item->SetString( "command", UTIL_VarArgs("bind r %s;cancelselect", command) );
        item = kv->FindKey( "5", true );
        item->SetString( "msg", "S" );
        item->SetString( "command", UTIL_VarArgs("bind s %s;cancelselect", command) );
        item = kv->FindKey( "6", true );
        item->SetString( "msg", "T" );
        item->SetString( "command", UTIL_VarArgs("bind t %s;cancelselect", command) );
        item = kv->FindKey( "7", true );
        item->SetString( "msg", "U" );
        item->SetString( "command", UTIL_VarArgs("bind u %s;cancelselect", command) );
        item = kv->FindKey( "8", true );
        item->SetString( "msg", "<< Back" );
        item->SetString( "command", UTIL_VarArgs("bindmenu %s 0", command) );
        break;
    case 4:
        item = kv->FindKey( "1", true );
        item->SetString( "msg", "V" );
        item->SetString( "command", UTIL_VarArgs("bind v %s;cancelselect", command) );
        item = kv->FindKey( "2", true );
        item->SetString( "msg", "W" );
        item->SetString( "command", UTIL_VarArgs("bind w %s;cancelselect", command) );
        item = kv->FindKey( "3", true );
        item->SetString( "msg", "X" );
        item->SetString( "command", UTIL_VarArgs("bind x %s;cancelselect", command) );
        item = kv->FindKey( "4", true );
        item->SetString( "msg", "Y" );
        item->SetString( "command", UTIL_VarArgs("bind y %s;cancelselect", command) );
        item = kv->FindKey( "5", true );
        item->SetString( "msg", "Z" );
        item->SetString( "command", UTIL_VarArgs("bind z %s;cancelselect", command) );
        item = kv->FindKey( "6", true );
        item->SetString( "msg", "<< Back" );
        item->SetString( "command", UTIL_VarArgs("bindmenu %s 0", command) );
        break;
    case 5:
        item = kv->FindKey( "1", true );
        item->SetString( "msg", "-" );
        item->SetString( "command", UTIL_VarArgs("bind - %s;cancelselect", command) );
        item = kv->FindKey( "2", true );
        item->SetString( "msg", "=" );
        item->SetString( "command", UTIL_VarArgs("bind = %s;cancelselect", command) );
        item = kv->FindKey( "3", true );
        item->SetString( "msg", "[" );
        item->SetString( "command", UTIL_VarArgs("bind [ %s;cancelselect", command) );
        item = kv->FindKey( "4", true );
        item->SetString( "msg", "]" );
        item->SetString( "command", UTIL_VarArgs("bind ] %s;cancelselect", command) );
        item = kv->FindKey( "5", true );
        item->SetString( "msg", "\\" );
        item->SetString( "command", UTIL_VarArgs("bind \\ %s;cancelselect", command) );
        item = kv->FindKey( "6", true );
        item->SetString( "msg", ";" );
        item->SetString( "command", UTIL_VarArgs("bind semicolon %s;cancelselect", command) );
        item = kv->FindKey( "7", true );
        item->SetString( "msg", "More >>" );
        item->SetString( "command", UTIL_VarArgs("bindmenu %s 6", command) );
        item = kv->FindKey( "8", true );
        item->SetString( "msg", "<< Back" );
        item->SetString( "command", UTIL_VarArgs("bindmenu %s 0", command) );
        break;
    case 6:
        item = kv->FindKey( "1", true );
        item->SetString( "msg", "'" );
        item->SetString( "command", UTIL_VarArgs("bind ' %s;cancelselect", command) );
        item = kv->FindKey( "2", true );
        item->SetString( "msg", "," );
        item->SetString( "command", UTIL_VarArgs("bind , %s;cancelselect", command) );
        item = kv->FindKey( "3", true );
        item->SetString( "msg", "." );
        item->SetString( "command", UTIL_VarArgs("bind . %s;cancelselect", command) );
        item = kv->FindKey( "4", true );
        item->SetString( "msg", "/" );
        item->SetString( "command", UTIL_VarArgs("bind / %s;cancelselect", command) );
        item = kv->FindKey( "5", true );
        item->SetString( "msg", "Shift" );
        item->SetString( "command", UTIL_VarArgs("bind shift %s;cancelselect", command) );
        item = kv->FindKey( "6", true );
        item->SetString( "msg", "Ctl" );
        item->SetString( "command", UTIL_VarArgs("bind ctrl %s;cancelselect", command) );
        item = kv->FindKey( "7", true );
        item->SetString( "msg", "Alt" );
        item->SetString( "command", UTIL_VarArgs("bind alt %s;cancelselect", command) );
        item = kv->FindKey( "8", true );
        item->SetString( "msg", "<< Back" );
        item->SetString( "command", UTIL_VarArgs("bindmenu %s 5", command) );
        break;
    case 7:
        item = kv->FindKey( "1", true );
        item->SetString( "msg", "Up Arrow" );
        item->SetString( "command", UTIL_VarArgs("bind uparrow %s;cancelselect", command) );
        item = kv->FindKey( "2", true );
        item->SetString( "msg", "Right Arrow" );
        item->SetString( "command", UTIL_VarArgs("bind rightarrow %s;cancelselect", command) );
        item = kv->FindKey( "3", true );
        item->SetString( "msg", "Down Arrow" );
        item->SetString( "command", UTIL_VarArgs("bind downarrow %s;cancelselect", command) );
        item = kv->FindKey( "4", true );
        item->SetString( "msg", "Left Arrow" );
        item->SetString( "command", UTIL_VarArgs("bind leftarrow %s;cancelselect", command) );
        item = kv->FindKey( "5", true );
        item->SetString( "msg", "<< Back" );
        item->SetString( "command", UTIL_VarArgs("bindmenu %s 0", command) );
        break;
    case 8:
        item = kv->FindKey( "1", true );
        item->SetString( "msg", "Mouse Button 3" );
        item->SetString( "command", UTIL_VarArgs("bind mouse3 %s;cancelselect", command) );
        item = kv->FindKey( "2", true );
        item->SetString( "msg", "Mouse Button 4" );
        item->SetString( "command", UTIL_VarArgs("bind mouse4 %s;cancelselect", command) );
        item = kv->FindKey( "3", true );
        item->SetString( "msg", "Mouse Button 5" );
        item->SetString( "command", UTIL_VarArgs("bind mouse5 %s;cancelselect", command) );
        item = kv->FindKey( "4", true );
        item->SetString( "msg", "Mouse Wheel Up" );
        item->SetString( "command", UTIL_VarArgs("bind mwheelup %s;cancelselect", command) );
        item = kv->FindKey( "5", true );
        item->SetString( "msg", "Mouse Wheel Down" );
        item->SetString( "command", UTIL_VarArgs("bind mwheeldown %s;cancelselect", command) );
        item = kv->FindKey( "6", true );
        item->SetString( "msg", "<< Back" );
        item->SetString( "command", UTIL_VarArgs("bindmenu %s 0", command) );
        break;
    default:
        item = kv->FindKey( "1", true );
        item->SetString( "msg", "A-G" );
        item->SetString( "command", UTIL_VarArgs("bindmenu %s 1", command) );
        item = kv->FindKey( "2", true );
        item->SetString( "msg", "H-N" );
        item->SetString( "command", UTIL_VarArgs("bindmenu %s 2", command) );
        item = kv->FindKey( "3", true );
        item->SetString( "msg", "P-U" );
        item->SetString( "command", UTIL_VarArgs("bindmenu %s 3", command) );
        item = kv->FindKey( "4", true );
        item->SetString( "msg", "V-Z" );
        item->SetString( "command", UTIL_VarArgs("bindmenu %s 4", command) );
        item = kv->FindKey( "5", true );
        item->SetString( "msg", "Symbols" );
        item->SetString( "command", UTIL_VarArgs("bindmenu %s 5", command) );
        item = kv->FindKey( "6", true );
        item->SetString( "msg", "Arrows" ); //  skip bindmenu 6 because 6 is accessed thru 5
        item->SetString( "command", UTIL_VarArgs("bindmenu %s 7", command) );
        item = kv->FindKey( "7", true );
        item->SetString( "msg", "Mouse Buttons" );
        item->SetString( "command", UTIL_VarArgs("bindmenu %s 8", command) );
        break;
    }

    helpers->CreateMessage( pEntity, DIALOG_MENU, kv, &g_ServerPlugin );
    kv->deleteThis();
}

void CAdminOPPlayer :: PlaySoundLocal(const char *sound)
{
    if(!entID)
        return;

    IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+entID);
    if(!info)
        return;
    if(!info->IsConnected())
        return;

    CSingleUserRecipientFilter filter( entID );
    enginesound->EmitSound((CRecipientFilter&)filter, entID, CHAN_STATIC, sound, VOL_NORM, SNDLVL_NORM, 0, PITCH_NORM, &EyePosition());  
}

void CAdminOPPlayer :: LeavePrivateChat()
{
    if(!entID)
        return;

    IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+entID);
    if(!info)
        return;
    if(!info->IsConnected())
        return;

    if(PrivateChat.RemotePrivChat)
    {
        PrivateChat.RemotePrivChat = 0;
        PrivateChat.RemotePrivChatID = 0;
        SayTextChatHud(UTIL_VarArgs("[%s] Left private chat with remote admin.\n", pAdminOP.adminname));
    }
    if(PrivateChat.LocalPrivChat)
    {
        PrivateChat.LocalPrivChat = 0;
        PrivateChat.LocalPrivChatID = 0;
        SayTextChatHud(UTIL_VarArgs("[%s] Left private chat with player.\n", pAdminOP.adminname));
    }
}

Vector CAdminOPPlayer :: EyePosition()
{
    if(!entID)
        return Vector(0,0,0);

    edict_t *pEntity = pAdminOP.GetEntityList()+entID;
    if(!pEntity)
        return Vector(0,0,0);

    CBaseEntity *pPlayer = CBaseEntity::Instance(pEntity);
    if(!pPlayer)
        return Vector(0,0,0);

    return VFuncs::EyePosition(pPlayer);
}

CBaseEntity *CAdminOPPlayer :: FindEntityForward(unsigned int mask, Vector *endpos)
{
    if(!entID)
        return NULL;

    edict_t *pEntity = pAdminOP.GetEntityList()+entID;
    if(!pEntity)
        return NULL;

    CBasePlayer *pPlayer = (CBasePlayer *)VFuncs::Instance(pEntity);
    if(!pPlayer)
        return NULL;

    Vector forward, right, up;
    QAngle playerAngles = VFuncs::EyeAngles(pPlayer);

    AngleVectors( playerAngles, &forward, &right, &up );

    float distance = 8192;
    Vector start = EyePosition();
    Vector end = EyePosition() + ( forward * distance );

    trace_t tr;
    CTraceFilterSimple traceFilter( pPlayer, COLLISION_GROUP_NONE );
    Ray_t ray;
    ray.Init( start, end );
    enginetrace->TraceRay( ray, mask, &traceFilter, &tr );
    if(endpos) *endpos = tr.endpos;
    if ( tr.fraction != 1.0 && tr.DidHitNonWorldEntity() )
    {
        return tr.m_pEnt;
    }
    return NULL;
}

CBaseEntity *CAdminOPPlayer :: GiveNamedItem(const char *pszName, int iSubType)
{
    if(!entID)
        return NULL;

    edict_t *pEntity = pAdminOP.GetEntityList()+entID;
    if(!pEntity)
        return NULL;

    CBasePlayer *pPlayer = (CBasePlayer *)VFuncs::Instance(pEntity);
    if(!pPlayer)
        return NULL;

    return VFuncs::GiveNamedItem(pPlayer, pszName, iSubType);
}

bool CAdminOPPlayer :: GiveTouchDel( const char *pszName )
{
    if(!entID)
        return 0;

    edict_t *pEntity = pAdminOP.GetEntityList()+entID;
    if(!pEntity)
        return 0;

    CBasePlayer *pPlayer = (CBasePlayer *)VFuncs::Instance(pEntity);
    if(!pPlayer)
        return 0;

    Vector vecOrigin = VFuncs::GetAbsOrigin(pPlayer);
    QAngle vecAngles( 0, 0, 0 );
    
    CBaseEntity *pSpawn = CreateEntityByName( pszName );
    if(pSpawn)
    {
        VFuncs::Spawn(pSpawn);
        VFuncs::Activate(pSpawn);
        VFuncs::Teleport(pSpawn,  &vecOrigin, &vecAngles, NULL );
        VFuncs::Touch( pSpawn, pPlayer );
        UTIL_Remove(pSpawn);
        return 1;
    }
    return 0;
}

bool CAdminOPPlayer :: GiveAllWeapons()
{
    if(!entID)
        return 0;

    edict_t *pEntity = pAdminOP.GetEntityList()+entID;
    if(!pEntity)
        return 0;

    CBasePlayer *pPlayer = (CBasePlayer *)VFuncs::Instance(pEntity);
    if(!pPlayer)
        return 0;

    IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+entID);
    if(!info)
        return 0;
    if(!info->IsConnected())
        return 0;

    if(!stricmp(pAdminOP.ModName(), "hl2mp"))
    {
        GiveNamedItem("weapon_crowbar");
        GiveNamedItem("weapon_stunstick");
        GiveNamedItem("weapon_pistol");
        GiveNamedItem("weapon_357");
        GiveNamedItem("weapon_smg1");
        GiveNamedItem("weapon_ar2");
        GiveNamedItem("weapon_shotgun");
        GiveNamedItem("weapon_frag");
        GiveNamedItem("weapon_crossbow");
        GiveNamedItem("weapon_rpg");
        GiveNamedItem("weapon_slam");
        GiveNamedItem("weapon_physcannon");
        return 1;
    }
    else if(!stricmp(pAdminOP.ModName(), "cstrike"))
    {
        GiveNamedItem("weapon_ak47");
        GiveNamedItem("weapon_aug");
        GiveNamedItem("weapon_deagle");
        GiveNamedItem("weapon_elite");
        GiveNamedItem("weapon_famas");
        GiveNamedItem("weapon_fiveseven");
        GiveNamedItem("weapon_flashbang");
        GiveNamedItem("weapon_g3sg1");
        GiveNamedItem("weapon_galil");
        GiveNamedItem("weapon_glock18");
        GiveNamedItem("weapon_hegrenade");
        GiveNamedItem("weapon_knife");
        GiveNamedItem("weapon_m249");
        GiveNamedItem("weapon_m3");
        GiveNamedItem("weapon_m4a1");
        GiveNamedItem("weapon_mac10");
        GiveNamedItem("weapon_mp5navy");
        GiveNamedItem("weapon_p228");
        GiveNamedItem("weapon_p90");
        GiveNamedItem("weapon_scout");
        GiveNamedItem("weapon_sg550");
        GiveNamedItem("weapon_sg552");
        GiveNamedItem("weapon_shieldgun");
        GiveNamedItem("weapon_smokegrenade");
        GiveNamedItem("weapon_tmp");
        GiveNamedItem("weapon_usp");
        GiveNamedItem("weapon_ump45");
        GiveNamedItem("weapon_xm1014");
        return 1;
    }
    else if(!stricmp(pAdminOP.ModName(), "tf"))
    {
        GiveTFWeapon("tf_weapon_bat");
        GiveTFWeapon("tf_weapon_bonesaw");
        GiveTFWeapon("tf_weapon_bottle");
        GiveTFWeapon("tf_weapon_club");
        GiveTFWeapon("tf_weapon_fireaxe");
        GiveTFWeapon("tf_weapon_fists");
        GiveTFWeapon("tf_weapon_flamethrower");
        GiveTFWeapon("tf_weapon_grenadelauncher");
        GiveTFWeapon("tf_weapon_knife");
        GiveTFWeapon("tf_weapon_medigun");
        GiveTFWeapon("tf_weapon_minigun");
        GiveTFWeapon("tf_weapon_pipebomblauncher");
        GiveTFWeapon("tf_weapon_pistol");
        GiveTFWeapon("tf_weapon_pistol_scout");
        GiveTFWeapon("tf_weapon_revolver");
        GiveTFWeapon("tf_weapon_rocketlauncher");
        GiveTFWeapon("tf_weapon_scattergun");
        GiveTFWeapon("tf_weapon_shotgun_hwg");
        GiveTFWeapon("tf_weapon_shotgun_primary");
        GiveTFWeapon("tf_weapon_shotgun_pyro");
        GiveTFWeapon("tf_weapon_shotgun_soldier");
        GiveTFWeapon("tf_weapon_shovel");
        GiveTFWeapon("tf_weapon_smg");
        GiveTFWeapon("tf_weapon_sniperrifle");
        GiveTFWeapon("tf_weapon_syringegun_medic");
        GiveTFWeapon("tf_weapon_wrench");
        return 1;
    }
    return 0;
}

CBaseEntity *CAdminOPPlayer :: GiveTFWeapon(const char *pszName)
{
    if(!entID)
        return NULL;

    edict_t *pEntity = pAdminOP.GetEntityList()+entID;
    if(!pEntity)
        return NULL;

    CBasePlayer *pPlayer = (CBasePlayer *)VFuncs::Instance(pEntity);
    if(!pPlayer)
        return NULL;

    CBaseCombatWeapon *pWep = NULL;
    if(servertools)
    {
        pWep = (CBaseCombatWeapon *)CreateEntityByName(pszName);
        if(pWep)
        {
            if(servertools) servertools->DispatchSpawn(pWep);
            VFuncs::Weapon_Equip(pPlayer, pWep);
            VFuncs::Weapon_Switch(pPlayer, pWep);
        }
    }

    return pWep;
}

bool CAdminOPPlayer :: GiveFullAmmo()
{
    if(!entID)
        return 0;

    edict_t *pEntity = pAdminOP.GetEntityList()+entID;
    if(!pEntity)
        return 0;

    CBasePlayer *pPlayer = (CBasePlayer *)VFuncs::Instance(pEntity);
    if(!pPlayer)
        return 0;

    if(pAdminOP.isHl2mp)
    {
        VFuncs::GiveAmmo(pPlayer,500,0,1);
        VFuncs::GiveAmmo(pPlayer,500,1,1);
        VFuncs::GiveAmmo(pPlayer,500,2,1);
        VFuncs::GiveAmmo(pPlayer,500,3,1);
        VFuncs::GiveAmmo(pPlayer,500,4,1);
        VFuncs::GiveAmmo(pPlayer,500,5,1);
        VFuncs::GiveAmmo(pPlayer,500,6,1);
        VFuncs::GiveAmmo(pPlayer,500,7,1);
        VFuncs::GiveAmmo(pPlayer,500,8,1);
        VFuncs::GiveAmmo(pPlayer,500,9,1);
        VFuncs::GiveAmmo(pPlayer,500,10,1); //Grenades
        VFuncs::GiveAmmo(pPlayer,500,11,1);
        return 1;
    }
    else if(pAdminOP.isCstrike)
    {
        for(int i = 0; i < 14; i ++)
        {
            VFuncs::GiveAmmo(pPlayer,500,i,1);
        }
        return 1;
    }
    else if(!stricmp(pAdminOP.ModName(), "FortressForever") || !stricmp(pAdminOP.ModName(), "tf") || !stricmp(pAdminOP.ModName(), "left4dead"))
    {
        // this is just a high guess
        for(int i = 0; i < 30; i ++)
        {
            VFuncs::GiveAmmo(pPlayer,500,i,1);
        }
        return 1;
    }
    return 0;
}

bool CAdminOPPlayer :: SetUberLevel(float uberlevel)
{
    if(!entID)
        return 0;

    if(!pAdminOP.isTF2)
        return 0;

    edict_t *pEntity = pAdminOP.GetEntityList()+entID;
    if(!pEntity)
        return 0;

    CBasePlayer *pPlayer = (CBasePlayer *)VFuncs::Instance(pEntity);
    if(!pPlayer)
        return 0;

    CBaseCombatWeapon *pWeapon = VFuncs::Weapon_GetSlot(pPlayer, 1);
    if(FStrEq(VFuncs::GetClassname(pWeapon), "tf_weapon_medigun"))
    {
        VFuncs::SetChargeLevel(pWeapon, clamp(uberlevel, 0.0f, 1.0f));
        return 1;
    }
    return 0;
}

bool CAdminOPPlayer :: Unstick()
{
    if(!entID)
        return 0;

    edict_t *pEntity = pAdminOP.GetEntityList()+entID;
    if(!pEntity)
        return 0;

    CBasePlayer *pPlayer = (CBasePlayer *)VFuncs::Instance(pEntity);
    if(!pPlayer)
        return 0;

    trace_t trace;
    UTIL_TraceEntity( pPlayer, VFuncs::GetAbsOrigin(pPlayer), VFuncs::GetAbsOrigin(pPlayer), MASK_PLAYERSOLID, &trace );

    if ( trace.startsolid )
    {
        bool success = 1;
        Vector oldorigin = VFuncs::GetAbsOrigin(pPlayer);
        Vector forward, right, up;

        AngleVectors ( VFuncs::EyeAngles(pPlayer), &forward, &right, &up);
        
        // Try to move into the world
        if ( !FindPassableSpace( pPlayer, forward, 1, oldorigin ) )
        {
            VFuncs::SetAbsOrigin(pPlayer,  oldorigin );
            if ( !FindPassableSpace( pPlayer, right, 1, oldorigin ) )
            {
                VFuncs::SetAbsOrigin(pPlayer,  oldorigin );
                if ( !FindPassableSpace( pPlayer, right, -1, oldorigin ) )      // left
                {
                    VFuncs::SetAbsOrigin(pPlayer,  oldorigin );
                    if ( !FindPassableSpace( pPlayer, up, 1, oldorigin ) )  // up
                    {
                        VFuncs::SetAbsOrigin(pPlayer,  oldorigin );
                        if ( !FindPassableSpace( pPlayer, up, -1, oldorigin ) ) // down
                        {
                            VFuncs::SetAbsOrigin(pPlayer,  oldorigin );
                            if ( !FindPassableSpace( pPlayer, forward, -1, oldorigin ) )    // back
                            {
                                success = 0;
                                //Msg( "Can't find the world\n" );
                            }
                        }
                    }
                }
            }
        }

        VFuncs::SetAbsOrigin(pPlayer,  oldorigin );
        return success;
    }
    return 0;
}

void CAdminOPPlayer :: ThrowFragGrenade(bool drop)
{
    if(!entID)
        return;

    edict_t *pEntity = pAdminOP.GetEntityList()+entID;
    if(pEntity == NULL)
        return;

    CBasePlayer *pPlayer = (CBasePlayer *)VFuncs::Instance(pEntity);
    if(!pPlayer)
        return;

    IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo(pEntity);
    if(!playerinfo)
        return;

    CBaseEntity *pGrenade = CreateEntityByName("sop_grenade");
    if(pGrenade)
    {
        Vector orig = EyePosition()-Vector(0,0,6);
        Vector forward, velocity;
        QAngle playerAngles = VFuncs::EyeAngles(pPlayer);
        playerAngles.x -= 10;

        AngleVectors( playerAngles, &forward );
        VectorNormalize(forward);
        if(drop)
            velocity = Vector(0,0,0);
        else
            velocity = forward*800;

        VFuncs::KeyValue(pGrenade, "life", UTIL_VarArgs("%f", m_flGrenadeDetonateTime - gpGlobals->curtime));
        VFuncs::KeyValue(pGrenade, "ownerplayer", UTIL_VarArgs("%i", entID));
        VFuncs::Spawn(pGrenade);
        VFuncs::Teleport(pGrenade, &orig, &playerAngles, &velocity);
    }

    m_flGrenadeDetonateTime = 0.0f;
}

void CAdminOPPlayer :: ThrowSecondaryGrenade()
{
}

void CAdminOPPlayer :: ThrusterOn(thruster_t *thruster, bool reverse)
{
    thruster->enabled = true;
    if(reverse) thruster->force *= -1;
    VFuncs::SetRenderMode(thruster->entthruster, kRenderTransColor);
    VFuncs::SetRenderColor(thruster->entthruster, 255, 128, 0);

    Vector vecSndOrigin = VFuncs::GetAbsOrigin(thruster->entthruster);
    CPASFilter filter( vecSndOrigin );
    enginesound->EmitSound((CRecipientFilter&)filter, VFuncs::entindex(thruster->entthruster), CHAN_BODY, "ambient/gas/cannister_loop.wav", 0.4, SNDLVL_NORM, 0, random->RandomInt(90, 110), &vecSndOrigin);

}

void CAdminOPPlayer :: ThrusterOff(thruster_t *thruster, bool reverse)
{
    thruster->enabled = false;
    if(reverse) thruster->force *= -1;
    VFuncs::SetRenderMode(thruster->entthruster, kRenderNormal);
    VFuncs::SetRenderColor(thruster->entthruster, 255, 255, 255);

    CBroadcastRecipientFilter filter;
    enginesound->EmitSound((CRecipientFilter&)filter, VFuncs::entindex(thruster->entthruster), CHAN_BODY, "common/null.wav", 0.6, 0, 0, random->RandomInt(90, 110), &VFuncs::GetAbsOrigin(thruster->entthruster));  
}

void CAdminOPPlayer :: DeleteThruster(thruster_t *thruster)
{
    if(thruster->enabled) ThrusterOff(thruster);
    if(physenv) physenv->DestroyConstraint(thruster->pConstraint);
    UTIL_Remove(thruster->entthruster);
    thruster->enabled = 0;
}

void CAdminOPPlayer :: ShowHelpPage(const char *pszURL)
{
    if(!entID)
        return;

    if(help_show_loading.GetBool())
    {
        KeyValues *data = new KeyValues("data");
        data->SetString( "title", "!help" );
        data->SetString( "type", "2" );
        data->SetString( "msg", UTIL_VarArgs("about:<html><meta http-equiv=\"Refresh\" content=\"0;url=%s\"><h2>!help is loading...</h2></html>", pszURL) );
        ShowViewPortPanel( "info", true, data );
        data->deleteThis();
    }
    else
    {
        KeyValues *data = new KeyValues("data");
        data->SetString( "title", "!help" );
        data->SetString( "type", "2" );
        data->SetString( "msg", pszURL );
        ShowViewPortPanel( "info", true, data );
        data->deleteThis();
    }
}

void CAdminOPPlayer :: AttachGrabbedPhysObject( CBaseEntity *pObject, const Vector& start, const Vector &end, float distance )
{
    PhysMoving.m_hObject = pObject;
    PhysMoving.m_useDown = false;
    IPhysicsObject *pPhysics = pObject ? (VFuncs::VPhysicsGetObject(pObject)) : NULL;
    if ( pPhysics && VFuncs::GetMoveType(pObject) == MOVETYPE_VPHYSICS )
    {
        PhysMoving.m_distance = distance;

        PhysMoving.m_gravCallback.AttachEntity( pObject, pPhysics, end );
        float mass = pPhysics->GetMass();
        float vel = phys_gunvel.GetFloat();
        if ( mass > phys_gunmass.GetFloat() )
        {
            vel = (vel*phys_gunmass.GetFloat())/mass;
        }
        PhysMoving.m_gravCallback.SetMaxVelocity( vel );

        PhysMoving.m_originalObjectPosition = VFuncs::GetAbsOrigin(pObject);

        pPhysics->Wake();
        
        /*CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
        if( pOwner )
        {
            pObject->OnPhysGunPickup( pOwner );
        }*/
    }
    else
    {
        PhysMoving.m_hObject = NULL;
    }
}

CGravControllerPoint::CGravControllerPoint( void )
{
    m_attachedEntity = NULL;
    m_controller = NULL;
}

CGravControllerPoint::~CGravControllerPoint( void )
{
    DetachEntity();
}


void CGravControllerPoint::AttachEntity( CBaseEntity *pEntity, IPhysicsObject *pPhys, const Vector &position )
{
    m_attachedEntity = pEntity;
    pPhys->WorldToLocal( &m_localPosition, position );
    m_worldPosition = position;
    pPhys->GetDamping( NULL, &m_saveDamping );
    float damping = 2;
    pPhys->SetDamping( NULL, &damping );
    m_controller = physenv->CreateMotionController( this );
    m_controller->AttachObject( pPhys, false );
    m_controller->SetPriority( IPhysicsMotionController::HIGH_PRIORITY );
    SetTargetPosition( position );
    m_maxAcceleration = phys_gunforce.GetFloat() * pPhys->GetInvMass();
    m_targetRotation = pEntity->GetAbsAngles();
    float torque = phys_guntorque.GetFloat();
    m_maxAngularAcceleration = torque * pPhys->GetInvInertia();
}

void CGravControllerPoint::DetachEntity( void )
{
    CBaseEntity *pEntity = m_attachedEntity;
    if ( pEntity )
    {
        IPhysicsObject *pPhys = VFuncs::VPhysicsGetObject(pEntity);
        if ( pPhys )
        {
            // on the odd chance that it's gone to sleep while under anti-gravity
            pPhys->Wake();
            pPhys->SetDamping( NULL, &m_saveDamping );
        }
    }
    m_attachedEntity = NULL;
    if(m_controller)
    {
        // if(physenv) because upon unloading, physenv might now be NULL
        if(physenv) physenv->DestroyMotionController( m_controller );
    }
    m_controller = NULL;

    // UNDONE: Does this help the networking?
    m_targetPosition = vec3_origin;
    m_worldPosition = vec3_origin;
}

IMotionEvent::simresult_e CGravControllerPoint::Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular )
{
    Vector vel;
    AngularImpulse angVel;

    float fracRemainingSimTime = 1.0;
    if ( m_timeToArrive > 0 )
    {
        fracRemainingSimTime *= deltaTime / m_timeToArrive;
        if ( fracRemainingSimTime > 1 )
        {
            fracRemainingSimTime = 1;
        }
    }
    
    m_timeToArrive -= deltaTime;
    if ( m_timeToArrive < 0 )
    {
        m_timeToArrive = 0;
    }

    float invDeltaTime = (1.0f / deltaTime);
    Vector world;
    pObject->LocalToWorld( &world, m_localPosition );
    m_worldPosition = world;
    pObject->GetVelocity( &vel, &angVel );
    //pObject->GetVelocityAtPoint( world, vel );
    float damping = 1.0;
    world += vel * deltaTime * damping;
    Vector delta = (m_targetPosition - world) * fracRemainingSimTime * invDeltaTime;
    Vector alignDir;
    linear = vec3_origin;
    angular = vec3_origin;

    if ( m_align )
    {
        QAngle angles;
        Vector origin;
        Vector axis;
        AngularImpulse torque;

        pObject->GetShadowPosition( &origin, &angles );
        // align local normal to target normal
        VMatrix tmp = SetupMatrixOrgAngles( origin, angles );
        Vector worldNormal = tmp.VMul3x3( m_localAlignNormal );
        axis = CrossProduct( worldNormal, m_targetAlignNormal );
        float trig = VectorNormalize(axis);
        float alignRotation = RAD2DEG(asin(trig));
        axis *= alignRotation;
        if ( alignRotation < 10 )
        {
            float dot = DotProduct( worldNormal, m_targetAlignNormal );
            // probably 180 degrees off
            if ( dot < 0 )
            {
                if ( worldNormal.x < 0.5 )
                {
                    axis.Init(10,0,0);
                }
                else
                {
                    axis.Init(0,0,10);
                }
                alignRotation = 10;
            }
        }
        
        // Solve for the rotation around the target normal (at the local align pos) that will 
        // move the grabbed spot to the destination.
        Vector worldRotCenter = tmp.VMul4x3( m_localAlignPosition );
        Vector rotSrc = world - worldRotCenter;
        Vector rotDest = m_targetPosition - worldRotCenter;

        // Get a basis in the plane perpendicular to m_targetAlignNormal
        Vector srcN = rotSrc;
        VectorNormalize( srcN );
        Vector tangent = CrossProduct( srcN, m_targetAlignNormal );
        float len = VectorNormalize( tangent );

        // needs at least ~5 degrees, or forget rotation (0.08 ~= sin(5))
        if ( len > 0.08 )
        {
            Vector binormal = CrossProduct( m_targetAlignNormal, tangent );

            // Now project the src & dest positions into that plane
            Vector planeSrc( DotProduct( rotSrc, tangent ), DotProduct( rotSrc, binormal ), 0 );
            Vector planeDest( DotProduct( rotDest, tangent ), DotProduct( rotDest, binormal ), 0 );

            float rotRadius = VectorNormalize( planeSrc );
            float destRadius = VectorNormalize( planeDest );
            if ( rotRadius > 0.1 )
            {
                if ( destRadius < rotRadius )
                {
                    destRadius = rotRadius;
                }
                //float ratio = rotRadius / destRadius;
                float angleSrc = atan2( planeSrc.y, planeSrc.x );
                float angleDest = atan2( planeDest.y, planeDest.x );
                float angleDiff = angleDest - angleSrc;
                angleDiff = RAD2DEG(angleDiff);
                axis += m_targetAlignNormal * angleDiff;
                //world = m_targetPosition;// + rotDest * (1-ratio);
//              NDebugOverlay::Line( worldRotCenter, worldRotCenter-m_targetAlignNormal*50, 255, 0, 0, false, 0.1 );
//              NDebugOverlay::Line( worldRotCenter, worldRotCenter+tangent*50, 0, 255, 0, false, 0.1 );
//              NDebugOverlay::Line( worldRotCenter, worldRotCenter+binormal*50, 0, 0, 255, false, 0.1 );
            }
        }

        torque = WorldToLocalRotation( tmp, axis, 1 );
        torque *= fracRemainingSimTime * invDeltaTime;
        torque -= angVel * 1.0;  // damping
        for ( int i = 0; i < 3; i++ )
        {
            if ( torque[i] > 0 )
            {
                if ( torque[i] > m_maxAngularAcceleration[i] )
                    torque[i] = m_maxAngularAcceleration[i];
            }
            else
            {
                if ( torque[i] < -m_maxAngularAcceleration[i] )
                    torque[i] = -m_maxAngularAcceleration[i];
            }
        }
        torque *= invDeltaTime;
        angular += torque;
        // Calculate an acceleration that pulls the object toward the constraint
        // When you're out of alignment, don't pull very hard
        float factor = fabsf(alignRotation);
        if ( factor < 5 )
        {
            factor = clamp( factor, 0, 5 ) * (1/5);
            alignDir = m_targetAlignPosition - worldRotCenter;
            // Limit movement to the part along m_targetAlignNormal if worldRotCenter is on the backside of 
            // of the target plane (one inch epsilon)!
            float planeForward = DotProduct( alignDir, m_targetAlignNormal );
            if ( planeForward > 1 )
            {
                alignDir = m_targetAlignNormal * planeForward;
            }
            Vector accel = alignDir * invDeltaTime * fracRemainingSimTime * (1-factor) * 0.20 * invDeltaTime;
            float mag = accel.Length();
            if ( mag > m_maxAcceleration )
            {
                accel *= (m_maxAcceleration/mag);
            }
            linear += accel;
        }
        linear -= vel*damping*invDeltaTime;
        // UNDONE: Factor in the change in worldRotCenter due to applied torque!
    }
    else
    {
        // clamp future velocity to max speed
        Vector nextVel = delta + vel;
        float nextSpeed = nextVel.Length();
        if ( nextSpeed > m_maxVel )
        {
            nextVel *= (m_maxVel / nextSpeed);
            delta = nextVel - vel;
        }

        delta *= invDeltaTime;

        float linearAccel = delta.Length();
        if ( linearAccel > m_maxAcceleration )
        {
            delta *= m_maxAcceleration / linearAccel;
        }

        Vector accel;
        AngularImpulse angAccel;
        pObject->CalculateForceOffset( delta, world, &accel, &angAccel );
        
        linear += accel;
        angular += angAccel;
    }
    
    return SIM_GLOBAL_ACCELERATION;
}

void CAdminOPPlayer :: DropGrabbedPhysObject()
{
    if ( PhysMoving.m_hObject )
    {
        /*CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
        if( pOwner )
        {
            m_hObject->OnPhysGunDrop( pOwner, false );
        }*/

        PhysMoving.m_gravCallback.DetachEntity();
        PhysMoving.m_hObject = NULL;
    }
    if(PhysMoving.pBeam)
    {
        UTIL_Remove(PhysMoving.pBeam);
        PhysMoving.pBeam = NULL;
    }
}
