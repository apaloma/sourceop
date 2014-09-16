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

// HACK HACK HACK : BASEENTITY ACCESS
#define GAME_DLL
#include "cbase.h"
#include "baseentity.h"
#include "player.h"
// HACK HACK HACK : BASEENTITY ACCESS
#include "toolframework/itoolentity.h"

#include "AdminOP.h"
#include "recipientfilter.h"
#include "vfuncs.h"

#include <ctype.h>

#include "tier0/memdbgon.h"

int offs[NUM_OFFSETS];
int m_MoveType_off = 0;
int m_MoveCollide_off = 0;
int m_SolidType_off = 0;
int m_SolidFlags_off = 0;

int g_offIClassname = 0;
int g_offSimulationTime = 0;
int g_offIHealth = 0;
int g_offIMaxHealth = 0;
int g_offVecAbsOrigin = 0;
int g_offAngRotation = 0;
int g_offVecAbsVelocity = 0;
int g_offVecAngVelocity = 0;
int g_offVecLocalOrigin = 0;
int g_offFriction = 0;
int g_offGravity = 0;
int g_offModelName = 0;
int g_offFFlags = 0;
int g_offFEFlags = 0;
int g_offCollision = 0;
int g_offOwnerEntity = 0;
int g_offCollisionGroup = 0;
int g_offPPhysicsObject = 0;
int g_offVecMins = 0;
int g_offVecMaxs = 0;
int g_offSolidType = 0;
int g_useSolidTypeInt = 0;
int g_offSolidFlags = 0;
int g_offFnCommandCallback = 0;
int g_offEntityFactoryDictionary = 0;
int g_useOldEntityDictionary = 0;
int g_useEntFactDebugPrints = 0;
int g_offMovetype = 0;
int g_offMovecollide = 0;
int g_offSimulatedEveryTick = 0;
int g_offAnimatedEveryTick = 0;
int g_offTakeDamage = 0;
int g_offSkin = 0;
int g_offSequence = 0;
int g_offDeathTime = 0;
int g_offPlayerClass = 0;
int g_offDesiredPlayerClass = 0;
int g_offNumBuildings = 0;
int g_offPlayerState = 0;
int g_offPlayerCond = 0;
int g_offPlayerCondsBase = 0;
int g_offUber = 0;
int g_offKritz = 0;
int g_offRageDraining;
int g_offMedigunChargeLevel = 0;
int g_offActiveWeapon = 0;
int g_offNextAttack = 0;
int g_offRenderFX = 0;
int g_offRenderMode = 0;
int g_offEffects = 0;
int g_offRender = 0;
int g_offThrower = 0;
int g_offButtons = 0;
int g_offCombineBallRadius = 0;

// items
int g_offItem = 0;

// gamerules
int g_offPlayingMannVsMachine = 0;

// loaded from datadesc
int g_offNetwork = 0;
int g_offTarget = 0;
int g_offName = 0;
int g_offWaterLevel = 0;
int g_offAbsRotation = 0;

// loaded from player datadesc
int g_offImpulse = 0;

bool g_foundBeamFuncs = 1;
int g_offBeamTargetPosition = 0;
int g_offBeamControlPosition = 0;
int g_offBeamScrollRate = 0;
int g_offBeamWidth = 0;

int g_offSpriteAttachedEntity = 0;
int g_offSpriteAttachment = 0;

int g_offTotalScore;

bool g_suppressEntindexError = 0;

void VFuncs::LoadOffsets(void)
{
    int slashpos;
    char gamedir[256];
    FILE *fp;
    char filepath[512];

    engine->GetGameDir(gamedir, sizeof(gamedir));

    Q_snprintf(filepath, sizeof(filepath), "%s/%s/DF_funcoffs.txt", gamedir, pAdminOP.DataDir());
    V_FixSlashes(filepath);
    fp = fopen(filepath, "rt");

    for(slashpos = strlen(gamedir) - 1; slashpos >= 0 && gamedir[slashpos] != '\\' && gamedir[slashpos] != '/'; slashpos--);
    slashpos++;

    if(fp)
    {
        char tmp[256];
        char thisgame[256];
        bool inblock = 0;
        while(!feof(fp))
        {
            if(fgets(tmp,sizeof(tmp),fp))
            {
                char *tmp2;
                tmp2 = strtrim(tmp, NULL);
                if((tmp2[0] != '#') && (strncmp(tmp2, "//", 2)) && (strncmp(tmp2, "\n", 1)) && (strlen(tmp2)))
                {
                    if(!inblock)
                    {
                        if(!strcmp(tmp2, "{"))
                            inblock = 1;
                        else
                            strncpy(thisgame, tmp2, sizeof(thisgame));
                    }
                    else
                    {
                        if(!strcmp(tmp2, "}"))
                        {
                            inblock = 0;
                        }
                        else if(!stricmp(thisgame, "default") || !stricmp(thisgame, &gamedir[slashpos]))
                        {
                            char *pszFunc, *pszOffset;
                            int iOffset;

                            pszFunc = strtok(tmp2, "=");
                            if(!pszFunc) Error("DF_funcoffs.txt has bad format.");
                            pszFunc = strtrim(pszFunc);

                            pszOffset = strtok(NULL, "=");
                            if(!pszOffset) Error("DF_funcoffs.txt has bad format.");
                            pszOffset = strtrim(pszOffset);
                            iOffset = atoi(pszOffset);
                            if(iOffset == 0 && pszOffset[0] != '0') Error("DF_funcoffs.txt has invalid offset value for \"%s\".", pszFunc);
#ifndef __linux__
                            if(strstr(pszFunc, "_from_") == NULL && strstr(pszFunc, "_win") == NULL && strstr(pszFunc, "_platind") == NULL && strstr(pszFunc, "_use_") == NULL && iOffset != -1)
                                iOffset--;
#endif

                            //Msg("%s: %s = %i\n", thisgame, pszFunc, iOffset);
                            if(!strcmp(pszFunc, "CBaseEntity::GetRefEHandle"))                          offs[OFFSET_GETREFEHANDLE] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseEntity::GetCollideable"))                    offs[OFFSET_GETCOLLIDEABLE] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseEntity::GetNetworkable"))                    offs[OFFSET_GETNETWORKABLE] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseEntity::GetModelIndex"))                     offs[OFFSET_GETMODELINDEX] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseEntity::GetModelName"))                      offs[OFFSET_GETMODELNAME] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseEntity::SetModelIndex"))                     offs[OFFSET_SETMODELINDEX] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseEntity::GetDataDescMap"))                    offs[OFFSET_GETDATADESCMAP] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseEntity::GetClassName"))                      offs[OFFSET_GETCLASSNAME] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseEntity::UpdateTransmitState"))               offs[OFFSET_UPDATETRANSMITSTATE] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseEntity::Spawn"))                             offs[OFFSET_SPAWN] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseEntity::Precache"))                          offs[OFFSET_PRECACHE] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseEntity::SetModel"))                          offs[OFFSET_SETMODEL] = iOffset;
#ifndef __linux__
                            else if(!strcmp(pszFunc, "CBaseEntity::SetModel_win"))                      offs[OFFSET_SETMODEL] = iOffset;
#endif
                            else if(!strcmp(pszFunc, "CBaseEntity::PostConstructor"))                   offs[OFFSET_POSTCONSTRUCTOR] = iOffset;
#ifndef __linux__
                            else if(!strcmp(pszFunc, "CBaseEntity::KeyValue_win"))                      offs[OFFSET_KEYVALUE] = iOffset;
#else
                            else if(!strcmp(pszFunc, "CBaseEntity::KeyValue_lin"))                      offs[OFFSET_KEYVALUE] = iOffset;
#endif
                            else if(!strcmp(pszFunc, "CBaseEntity::Activate"))                          offs[OFFSET_ACTIVATE] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseEntity::SetParent"))                         offs[OFFSET_SETPARENT] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseEntity::AcceptInput"))                       offs[OFFSET_ACCEPTINPUT] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseEntity::Think"))                             offs[OFFSET_THINK] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseEntity::PassesDamageFilter"))                offs[OFFSET_PASSESDAMAGEFILTER] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseEntity::TraceAttack"))                       offs[OFFSET_TRACEATTACK] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseEntity::OnTakeDamage"))                      offs[OFFSET_ONTAKEDAMAGE] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseEntity::IsAlive"))                           offs[OFFSET_ISALIVE] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseEntity::Event_Killed"))                      offs[OFFSET_EVENTKILLED] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseEntity::IsNPC"))                             offs[OFFSET_ISNPC] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseEntity::IsPlayer"))                          offs[OFFSET_ISPLAYER] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseEntity::IsBaseCombatWeapon"))                offs[OFFSET_ISBASECOMBATWEAPON] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseEntity::ChangeTeam"))                        offs[OFFSET_CHANGETEAM] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseEntity::Use"))                               offs[OFFSET_USE] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseEntity::StartTouch"))                        offs[OFFSET_STARTTOUCH] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseEntity::Touch"))                             offs[OFFSET_TOUCH] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseEntity::UpdateOnRemove"))                    offs[OFFSET_UPDATEONREMOVE] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseEntity::Teleport"))                          offs[OFFSET_TELEPORT] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseEntity::FireBullets"))                       offs[OFFSET_FIREBULLETS] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseEntity::SetDamage"))                         offs[OFFSET_SETDAMAGE] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseEntity::EyePosition"))                       offs[OFFSET_EYEPOSITION] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseEntity::EyeAngles"))                         offs[OFFSET_EYEANGLES] = iOffset;
#ifndef __linux__
                            else if(!strcmp(pszFunc, "CBaseEntity::FVisible_win"))                      offs[OFFSET_FVISIBLE] = iOffset;
#else
                            else if(!strcmp(pszFunc, "CBaseEntity::FVisible_lin"))                      offs[OFFSET_FVISIBLE] = iOffset;
#endif
                            else if(!strcmp(pszFunc, "CBaseEntity::WorldSpaceCenter"))                  offs[OFFSET_WORLDSPACECENTER] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseEntity::GetSoundEmissionOrigin"))            offs[OFFSET_GETSOUNDEMISSIONORIGIN] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseEntity::CreateVPhysics"))                    offs[OFFSET_CREATEVPHYSICS] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseEntity::VPhysicsDestroyObject"))             offs[OFFSET_VPHYSICSDESTROYOBJECT] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseEntity::VPhysicsGetObjectList"))             offs[OFFSET_VPHYSICSGETOBJECTLIST] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseAnimating::StudioFrameAdvance"))             offs[OFFSET_STUDIOFRAMEADVANCE] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseAnimating::Extinguish"))                     offs[OFFSET_EXTINGUISH] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseCombatCharacter::GiveAmmo"))                 offs[OFFSET_GIVEAMMO] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseCombatCharacter::Weapon_Equip"))             offs[OFFSET_WEAPONEQUIP] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseCombatCharacter::Weapon_Switch"))            offs[OFFSET_WEAPONSWITCH] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseCombatCharacter::Weapon_GetSlot"))           offs[OFFSET_WEAPONGETSLOT] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseCombatCharacter::RemovePlayerItem"))         offs[OFFSET_REMOVEPLAYERITEM] = iOffset;
#ifndef __linux__
                            else if(!strcmp(pszFunc, "CBaseCombatWeapon::Holster_win"))                 offs[OFFSET_HOLSTER] = iOffset;
#else
                            else if(!strcmp(pszFunc, "CBaseCombatWeapon::Holster_lin"))                 offs[OFFSET_HOLSTER] = iOffset;
#endif
                            else if(!strcmp(pszFunc, "CBaseGrenade::SetDamageRadius"))                  offs[OFFSET_SETDAMAGERADIUS] = iOffset;
#ifndef __linux__
                            else if(!strcmp(pszFunc, "CBaseCombatWeapon::PrimaryAttack_win"))           offs[OFFSET_PRIMARYATTACK] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseCombatWeapon::SecondaryAttack_win"))         offs[OFFSET_SECONDARYATTACK] = iOffset;
#else
                            else if(!strcmp(pszFunc, "CBaseCombatWeapon::PrimaryAttack_lin"))           offs[OFFSET_PRIMARYATTACK] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseCombatWeapon::SecondaryAttack_lin"))         offs[OFFSET_SECONDARYATTACK] = iOffset;
#endif
                            else if(!strcmp(pszFunc, "CBasePlayer::ForceRespawn"))                      offs[OFFSET_FORCERESPAWN] = iOffset;
                            else if(!strcmp(pszFunc, "CBasePlayer::StartObserverMode"))                 offs[OFFSET_STARTOBSERVERMODE] = iOffset;
                            else if(!strcmp(pszFunc, "CBasePlayer::StopObserverMode"))                  offs[OFFSET_STOPOBSERVERMODE] = iOffset;
                            else if(!strcmp(pszFunc, "CBasePlayer::ItemPostFrame"))                     offs[OFFSET_ITEMPOSTFRAME] = iOffset;
                            else if(!strcmp(pszFunc, "CBasePlayer::GiveNamedItem"))                     offs[OFFSET_GIVENAMEDITEM] = iOffset;
                            else if(!strcmp(pszFunc, "CBasePlayer::CanHearAndReadChatFrom"))            offs[OFFSET_CANHEARANDREADCHATFROM] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseMultiplayerPlayer::CalculateTeamBalanceScore")) offs[OFFSET_CALCULATETEAMBALANCESCORE] = iOffset;
#ifndef __linux__
                            else if(!strcmp(pszFunc, "CTFPlayer::GiveNamedItem_win"))                   offs[OFFSET_GIVENAMEDSCRIPTITEM] = iOffset;
#else
                            else if(!strcmp(pszFunc, "CTFPlayer::GiveNamedItem_lin"))                   offs[OFFSET_GIVENAMEDSCRIPTITEM] = iOffset;
#endif

                            else if(!strcmp(pszFunc, "CGameRules::PlayerRelationship"))                 offs[OFFSET_PLAYERRELATIONSHIP] = iOffset;
                            else if(!strcmp(pszFunc, "CGameRules::PlayerCanHearChat"))                  offs[OFFSET_PLAYERCANHEARCHAT] = iOffset;
                            else if(!strcmp(pszFunc, "CGameRules::GetTeamIndex"))                       offs[OFFSET_GETTEAMINDEX] = iOffset;
                            else if(!strcmp(pszFunc, "CGameRules::GetIndexedTeamName"))                 offs[OFFSET_GETINDEXEDTEAMNAME] = iOffset;
                            else if(!strcmp(pszFunc, "CGameRules::IsValidTeam"))                        offs[OFFSET_ISVALIDTEAM] = iOffset;
                            else if(!strcmp(pszFunc, "CGameRules::MarkAchievement"))                    offs[OFFSET_MARKACHIEVEMENT] = iOffset;
                            else if(!strcmp(pszFunc, "CMultiplayRules::GetNextLevelName"))              offs[OFFSET_GETNEXTLEVELNAME] = iOffset;
                            else if(!strcmp(pszFunc, "CMultiplayRules::ChangeLevel"))                   offs[OFFSET_CHANGELEVEL] = iOffset;
                            else if(!strcmp(pszFunc, "CMultiplayRules::GoToIntermission"))              offs[OFFSET_GOTOINTERMISSION] = iOffset;
                            else if(!strcmp(pszFunc, "CTeamplayRules::SetStalemate"))                   offs[OFFSET_SETSTALEMATE] = iOffset;
                            else if(!strcmp(pszFunc, "CTeamplayRules::SetSwitchTeams"))                 offs[OFFSET_SETSWITCHTEAMS] = iOffset;
                            else if(!strcmp(pszFunc, "CTeamplayRules::HandleSwitchTeams"))              offs[OFFSET_HANDLESWITCHTEAMS] = iOffset;
                            else if(!strcmp(pszFunc, "CTeamplayRules::SetScrambleTeams"))               offs[OFFSET_SETSCRAMBLETEAMS] = iOffset;
                            else if(!strcmp(pszFunc, "CTeamplayRules::HandleScrambleTeams"))            offs[OFFSET_HANDLESCRAMBLETEAMS] = iOffset;

                            else if(!strcmp(pszFunc, "IServerNetworkable::GetEntityHandle_platind"))    offs[OFFSET_GETENTITYHANDLE] = iOffset;
                            else if(!strcmp(pszFunc, "IServerNetworkable::GetEdict_platind"))           offs[OFFSET_GETEDICT] = iOffset;
                            else if(!strcmp(pszFunc, "IServerNetworkable::Release_platind"))            offs[OFFSET_RELEASE] = iOffset;
                            else if(!strcmp(pszFunc, "IServerNetworkable::GetBaseEntity_platind"))      offs[OFFSET_GETBASEENTITY] = iOffset;

#ifndef __linux__
                            else if(!strcmp(pszFunc, "CBaseClient::Connect_win"))                       offs[OFFSET_CONNECT] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseClient::Reconnect_win"))                     offs[OFFSET_RECONNECT] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseClient::Disconnect_win"))                    offs[OFFSET_DISCONNECT] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseClient::GetPlayerSlot_win"))
                            {
                                offs[OFFSET_GETPLAYERSLOT] = iOffset;
                                offs[OFFSET_IGETPLAYERSLOT] = offs[OFFSET_GETPLAYERSLOT];
                            }
                            else if(!strcmp(pszFunc, "CBaseClient::GetNetworkID_win"))                  offs[OFFSET_GETNETWORKID] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseClient::GetClientName_win"))                 offs[OFFSET_GETCLIENTNAME] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseClient::GetNetChannel_win"))                 offs[OFFSET_GETNETCHANNEL] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseClient::GetServer_win"))                     offs[OFFSET_GETSERVER] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseClient::ExecuteStringCommand_win"))          offs[OFFSET_EXECSTRINGCMD] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseClient::FillUserInfo_win"))                  offs[OFFSET_FILLUSERINFO] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseClient::SendServerInfo_win"))                offs[OFFSET_SENDSERVERINFO] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseClient::SetUserCVar_win"))                   offs[OFFSET_SETUSERCVAR] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseClient::vtable2_from_vtable3"))              offs[OFFSET_VTABLE2FROM3] = iOffset;
#else
                            else if(!strcmp(pszFunc, "CBaseClient::Connect_lin"))                       offs[OFFSET_CONNECT] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseClient::Reconnect_lin"))                     offs[OFFSET_RECONNECT] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseClient::Disconnect_lin"))                    offs[OFFSET_DISCONNECT] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseClient::IGetPlayerSlot_lin"))                offs[OFFSET_IGETPLAYERSLOT] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseClient::GetPlayerSlot_lin"))                 offs[OFFSET_GETPLAYERSLOT] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseClient::GetNetworkID_lin"))                  offs[OFFSET_GETNETWORKID] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseClient::GetClientName_lin"))                 offs[OFFSET_GETCLIENTNAME] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseClient::GetNetChannel_lin"))                 offs[OFFSET_GETNETCHANNEL] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseClient::GetServer_lin"))                     offs[OFFSET_GETSERVER] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseClient::ExecuteStringCommand_lin"))          offs[OFFSET_EXECSTRINGCMD] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseClient::FillUserInfo_lin"))                  offs[OFFSET_FILLUSERINFO] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseClient::SendServerInfo_lin"))                offs[OFFSET_SENDSERVERINFO] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseClient::SetUserCVar_lin"))                   offs[OFFSET_SETUSERCVAR] = iOffset;
#endif
                            else if(!strcmp(pszFunc, "CBaseServer::GetNumClients"))                     offs[OFFSET_GETNUMCLIENTS] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseServer::GetMaxClients"))                     offs[OFFSET_GETMAXCLIENTS] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseServer::RejectConnection"))                  offs[OFFSET_REJECTCONNECTION] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseServer::ConnectClient"))                     offs[OFFSET_CONNECTCLIENT] = iOffset;
                            else if(!strcmp(pszFunc, "CBaseServer::GetFreeClient"))                     offs[OFFSET_GETFREECLIENT] = iOffset;

                            else if(!strcmp(pszFunc, "m_fnCommandCallback_from_ConCommand"))            g_offFnCommandCallback = iOffset;
                            else if(!strcmp(pszFunc, "CEntityDictionary_from_dumpentityfactories"))     g_offEntityFactoryDictionary = iOffset;
                            else if(!strcmp(pszFunc, "CEntityDictionary_use_oldstyle"))                 g_useOldEntityDictionary = iOffset;
                            else if(!strcmp(pszFunc, "CEntityDictionary_use_debugprints"))              g_useEntFactDebugPrints = iOffset;
#ifndef __linux__
                            else if(!strcmp(pszFunc, "m_MoveType_from_CBaseEntity_fallback_win"))       m_MoveType_off = iOffset;
                            else if(!strcmp(pszFunc, "m_MoveCollide_from_CBaseEntity_fallback_win"))    m_MoveCollide_off = iOffset;
#else
                            else if(!strcmp(pszFunc, "m_MoveType_from_CBaseEntity_fallback_lin"))       m_MoveType_off = iOffset;
                            else if(!strcmp(pszFunc, "m_MoveCollide_from_CBaseEntity_fallback_lin"))    m_MoveCollide_off = iOffset;
#endif
                            else if(!strcmp(pszFunc, "m_nSolidType_from_CollisionProp_fallback"))       m_SolidType_off = iOffset;
                            else if(!strcmp(pszFunc, "m_nSolidType_use_integer"))                       g_useSolidTypeInt = iOffset;
                            else if(!strcmp(pszFunc, "m_usSolidFlags_from_CCollisionProp_fallback"))    m_SolidFlags_off = iOffset;

                        }
                    }
                }
            }
        }
        fclose(fp);
    }

    /*for(int i = 0; i < NUM_OFFSETS; i++)
    {
        Msg("offs[%i] = %i\n", i, offs[i]);
    }*/

    // m_flSimulationTime
    g_offSimulationTime = pAdminOP.GetPropOffset("DT_BaseEntity", "m_flSimulationTime");
    if(!g_offSimulationTime)
    {
        Warning("[SOURCEOP] No offset for m_flSimulationTime.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] No offset for m_flSimulationTime.\n");
    }

    // m_iHealth
    // get m_iHealth from a table that isn't DT_PlayerResource
    g_offIHealth = pAdminOP.GetPropOffsetNotTable("DT_PlayerResource", "m_iHealth");
    if(!g_offIHealth)
    {
        Warning("[SOURCEOP] No offset for m_iHealth.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] No offset for m_iHealth.\n");
    }

    // m_iMaxHealth
    // get m_iMaxHealth from a table that isn't DT_PlayerResource
    g_offIMaxHealth = pAdminOP.GetPropOffsetNotTable("DT_PlayerResource", "m_iMaxHealth");
    if(!g_offIMaxHealth)
    {
        if(!pAdminOP.isHl2mp)
        {
            Warning("[SOURCEOP] No offset for m_iMaxHealth.\n");
            pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] No offset for m_iMaxHealth.\n");
        }
    }

    // m_angRotation
    g_offAngRotation = pAdminOP.GetPropOffset("DT_BaseEntity", "m_angRotation");
    if(!g_offAngRotation)
    {
        Warning("[SOURCEOP] Using hardcoded offset for m_angRotation.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] Using hardcoded offset for m_angRotation.\n");
        g_offAngRotation = 808;
    }

    g_offFriction = pAdminOP.GetPropOffsetAnyTable("m_flFriction");
    if(!g_offFriction)
    {
        Warning("[SOURCEOP] No offset for m_flFriction.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] No offset for m_flFriction.\n");
    }

    // m_fFlags
    g_offFFlags = pAdminOP.GetPropOffsetNotTable("DT_EffectData", "m_fFlags");
    if(!g_offFFlags)
    {
        Warning("[SOURCEOP] Using hardcoded offset for m_fFlags.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] Using hardcoded offset for m_fFlags.\n");
        g_offFFlags = 172;
    }

    // this could use the datadesc
    if(g_offFFlags)
    {
        g_offFEFlags = g_offFFlags - 4;
    }

    // m_Collision
    g_offCollision = pAdminOP.GetPropOffset("DT_BaseEntity", "m_Collision");
    if(!g_offCollision)
    {
        Warning("[SOURCEOP] Using hardcoded offset for m_Collision.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] Using hardcoded offset for m_Collision.\n");
        g_offCollision = 236;
    }

    g_offOwnerEntity = pAdminOP.GetPropOffset("DT_BaseEntity", "m_hOwnerEntity");
    if(!g_offOwnerEntity)
    {
        Warning("[SOURCEOP] No offset for m_hOwnerEntity.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] No offset for m_hOwnerEntity.\n");
    }
    
    // m_CollisionGroup
    g_offCollisionGroup = pAdminOP.GetPropOffset("DT_BaseEntity", "m_CollisionGroup");

    // m_vecMins
    g_offVecMins = pAdminOP.GetPropOffset("DT_CollisionProperty", "m_vecMins");
    if(!g_offVecMins)
    {
        Warning("[SOURCEOP] Using hardcoded offset for m_vecMins.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] Using hardcoded offset for m_vecMins.\n");
        g_offVecMins = 8;
    }

    // m_vecMaxs
    g_offVecMaxs = pAdminOP.GetPropOffset("DT_CollisionProperty", "m_vecMaxs");
    if(!g_offVecMaxs)
    {
        Warning("[SOURCEOP] Using hardcoded offset for m_vecMaxs.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] Using hardcoded offset for m_vecMaxs.\n");
        g_offVecMaxs = 20;
    }

    // m_nSolidType
    g_offSolidType = pAdminOP.GetPropOffset("DT_CollisionProperty", "m_nSolidType");
    if(!g_offSolidType) g_offSolidType = pAdminOP.GetPropOffset("DT_CollisionProperty", "solid");
    if(!g_offSolidType)
    {
        if(m_SolidType_off)
        {
            g_offSolidType = m_SolidType_off;
            CAdminOP::ColorMsg(CONCOLOR_MAGENTA, "Using fallback offset for m_nSolidType\n");
        }
        else
        {
            Warning("[SOURCEOP] m_SolidType offset wasn't found. Set manually in DF_funcoffs.txt\n");
            pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] m_SolidType offset wasn't found. Set manually in DF_funcoffs.txt\n");
        }
    }

    // m_usSolidFlags
    g_offSolidFlags = pAdminOP.GetPropOffset("DT_CollisionProperty", "m_usSolidFlags");
    if(!g_offSolidFlags) g_offSolidFlags = pAdminOP.GetPropOffset("DT_CollisionProperty", "solidflags");
    if(!g_offSolidFlags)
    {
        if(m_SolidFlags_off)
        {
            g_offSolidFlags = m_SolidFlags_off;
            CAdminOP::ColorMsg(CONCOLOR_MAGENTA, "Using fallback offset for m_usSolidFlags\n");
        }
        else
        {
            Warning("[SOURCEOP] m_solidFlags offset wasn't found. Set manually in DF_funcoffs.txt\n");
            pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] m_solidFlags offset wasn't found. Set manually in DF_funcoffs.txt\n");
        }
    }

    // m_fnCommandCallback
    if(!g_offFnCommandCallback)
    {
        Warning("[SOURCEOP] Using hardcoded offset for m_fnCommandCallback.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] Using hardcoded offset for m_fnCommandCallback.\n");
        g_offFnCommandCallback = 24;
    }

    // CEntityFactoryDictionary
    if(!g_offEntityFactoryDictionary)
    {
        Warning("[SOURCEOP] Using hardcoded offset for CEntityFactoryDictionary.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] Using hardcoded offset for CEntityFactoryDictionary.\n");
        g_offEntityFactoryDictionary = 20;
    }

    // m_MoveType
    g_offMovetype = pAdminOP.GetPropOffsetAnyTable("movetype");
    if(!g_offMovetype)
    {
        if(m_MoveType_off)
        {
            g_offMovetype = m_MoveType_off;
        }
        else
        {
            Warning("[SOURCEOP] m_MoveType offset wasn't found. Set manually in DF_funcoffs.txt\n");
            pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] m_MoveType offset wasn't found. Set manually in DF_funcoffs.txt\n");
        }
    }

    // m_MoveCollide
    g_offMovecollide = pAdminOP.GetPropOffsetAnyTable("movecollide");
    if(!g_offMovecollide)
    {
        if(m_MoveCollide_off)
        {
            g_offMovecollide = m_MoveCollide_off;
        }
        else
        {
            Warning("[SOURCEOP] m_MoveCollide offset wasn't found. Set manually in DF_funcoffs.txt\n");
            pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] m_MoveCollide offset wasn't found. Set manually in DF_funcoffs.txt\n");
        }
    }

    // m_vecOrigin
    g_offVecLocalOrigin = pAdminOP.GetPropOffset("DT_BaseEntity", "m_vecOrigin");
    if(!g_offVecLocalOrigin)
    {
        Warning("[SOURCEOP] No offset for m_vecOrigin.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] No offset for m_vecOrigin.\n");
    }

    g_offSimulatedEveryTick = pAdminOP.GetPropOffset("DT_BaseEntity", "m_bSimulatedEveryTick");
    g_offAnimatedEveryTick = pAdminOP.GetPropOffset("DT_BaseEntity", "m_bAnimatedEveryTick");

    g_offSkin = pAdminOP.GetPropOffset("DT_BaseAnimating", "m_nSkin");
    if(!g_offSkin)
    {
        Warning("[SOURCEOP] No offset for m_nSkin.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] No offset for m_nSkin.\n");
    }

    g_offSequence = pAdminOP.GetPropOffset("DT_BaseAnimating", "m_nSequence");
    if(!g_offSequence)
    {
        Warning("[SOURCEOP] No offset for m_nSequence.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] No offset for m_nSequence.\n");
    }

    g_offDeathTime = pAdminOP.GetPropOffset("DT_LocalPlayerExclusive", "m_flDeathTime");
    if(!g_offDeathTime)
    {
        Warning("[SOURCEOP] No offset for m_flDeathTime.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] No offset for m_flDeathTime.\n");
    }

    int offClass = pAdminOP.GetPropOffset("DT_TFPlayerClassShared", "m_iClass");
    int offPlayerClass = pAdminOP.GetPropOffset("DT_TFPlayer", "m_PlayerClass");
    g_offPlayerClass = offClass + offPlayerClass;
    if(pAdminOP.isTF2 && (offClass < 4 || offPlayerClass < 4))
    {
        g_offPlayerClass = 0;
        Warning("[SOURCEOP] No offset for m_PlayerClass.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] No offset for m_PlayerClass.\n");
    }

    int offTFPlayerShared = pAdminOP.GetPropOffset("DT_TFPlayer", "m_Shared");
    int offDesiredPlayerClass = pAdminOP.GetPropOffset("DT_TFPlayerShared", "m_iDesiredPlayerClass");
    g_offDesiredPlayerClass = offTFPlayerShared + offDesiredPlayerClass;
    if(pAdminOP.isTF2 && (offTFPlayerShared < 4 || offDesiredPlayerClass < 4))
    {
        g_offDesiredPlayerClass = 0;
        Warning("[SOURCEOP] No offset for m_iDesiredPlayerClass.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] No offset for m_iDesiredPlayerClass.\n");
    }

    int offTFPlayerClass = pAdminOP.GetPropOffset("DT_TFPlayer", "m_PlayerClass");
    int offTFCustomVisibleToSelf = pAdminOP.GetPropOffset("DT_TFPlayerClassShared", "m_bCustomModelVisibleToSelf");
    g_offNumBuildings = offTFPlayerClass + offTFCustomVisibleToSelf + 67;
    if(pAdminOP.isTF2 && (offTFPlayerClass < 4 || offTFCustomVisibleToSelf < 4))
    {
        g_offNumBuildings = 0;
        Warning("[SOURCEOP] No offset for m_iObjectCount.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] No offset for m_iObjectCount.\n");
    }

    int offPlayerState = pAdminOP.GetPropOffset("DT_TFPlayerShared", "m_nPlayerState");
    g_offPlayerState = offTFPlayerShared + offPlayerState;
    if(pAdminOP.isTF2 && (offTFPlayerShared < 4 || offPlayerState < 4))
    {
        g_offPlayerState = 0;
        Warning("[SOURCEOP] No offset for m_nPlayerState.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] No offset for m_nPlayerState.\n");
    }

    int offPlayerCond = pAdminOP.GetPropOffset("DT_TFPlayerShared", "m_nPlayerCond");
    g_offPlayerCond = offTFPlayerShared + offPlayerCond;
    if(pAdminOP.isTF2 && (offTFPlayerShared < 4 || offPlayerCond < 4))
    {
        g_offPlayerCond = 0;
        Warning("[SOURCEOP] No offset for m_nPlayerCond.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] No offset for m_nPlayerCond.\n");
    }

    g_offPlayerCondsBase = offTFPlayerShared + offPlayerCond + 6;
    if(pAdminOP.isTF2 && (offTFPlayerShared < 4 || offPlayerCond < 4))
    {
        g_offPlayerCondsBase = 0;
        Warning("[SOURCEOP] No offset for g_offPlayerCondsBase.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] No offset for g_offPlayerCondsBase.\n");
    }

    g_offUber = offTFPlayerShared + offPlayerState + 30; // -2 ?!?
    if(pAdminOP.isTF2 && (offTFPlayerShared < 4 || offPlayerState < 4))
    {
        g_offKritz = 0;
        g_offUber = 0;
        Warning("[SOURCEOP] No offset for g_offUber.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] No offset for g_offUber.\n");
    }
    else
    {
        g_offKritz = g_offUber + 24;
    }

    int offRageDraining = pAdminOP.GetPropOffset("DT_TFPlayerSharedLocal", "m_bRageDraining");
    g_offRageDraining = offTFPlayerShared + offRageDraining;
    if(pAdminOP.isTF2 && (offTFPlayerShared < 4 || offRageDraining < 4))
    {
        g_offRageDraining = 0;
        Warning("[SOURCEOP] No offset for m_bRageDraining.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] No offset for m_bRageDraining.\n");
    }

    g_offMedigunChargeLevel = pAdminOP.GetPropOffset("DT_LocalTFWeaponMedigunData", "m_flChargeLevel");
    if(!g_offMedigunChargeLevel && pAdminOP.isTF2)
    {
        Warning("[SOURCEOP] No offset for m_flChargeLevel.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] No offset for m_flChargeLevel.\n");
    }

    g_offActiveWeapon = pAdminOP.GetPropOffset("DT_BaseCombatCharacter", "m_hActiveWeapon");
    if(!g_offActiveWeapon)
    {
        Warning("[SOURCEOP] No offset for m_hActiveWeapon.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] No offset for m_hActiveWeapon.\n");
    }

    g_offNextAttack = pAdminOP.GetPropOffset("DT_BCCLocalPlayerExclusive", "m_flNextAttack");
    if(!g_offNextAttack)
    {
        Warning("[SOURCEOP] No offset for m_flNextAttack.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] No offset for m_flNextAttack.\n");
    }

    g_offRenderFX = pAdminOP.GetPropOffsetAnyTable("m_nRenderFX");
    if(!g_offRenderFX)
    {
        Warning("[SOURCEOP] No offset for m_nRenderFX.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] No offset for m_nRenderFX.\n");
    }

    g_offRenderMode = pAdminOP.GetPropOffsetAnyTable("m_nRenderMode");
    if(!g_offRenderMode)
    {
        Warning("[SOURCEOP] No offset for m_nRenderMode.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] No offset for m_nRenderMode.\n");
    }

    g_offEffects = pAdminOP.GetPropOffsetAnyTable("m_fEffects");
    if(!g_offEffects)
    {
        Warning("[SOURCEOP] No offset for m_fEffects.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] No offset for m_fEffects.\n");
    }

    g_offRender = pAdminOP.GetPropOffsetAnyTable("m_clrRender");
    if(!g_offRender)
    {
        Warning("[SOURCEOP] No offset for m_clrRender.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] No offset for m_clrRender.\n");
    }

    g_offThrower = pAdminOP.GetPropOffset("DT_BaseGrenade", "m_hThrower");
    if(!g_offThrower)
    {
        Warning("[SOURCEOP] No offset for m_hThrower.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] No offset for m_hThrower.\n");
    }

    // m_nButtons
    g_offButtons = pAdminOP.GetPropOffset("DT_LocalPlayerExclusive", "m_fOnTarget");
    if(!g_offButtons)
    {
        Warning("[SOURCEOP] Didn't find offset for m_nButtons.\n"); 
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] Didn't find offset for m_nButtons.\n"); 
    }
    else
    {
        g_offButtons -= 24;
    }

    g_offCombineBallRadius = pAdminOP.GetPropOffset("DT_PropCombineBall", "m_flRadius");

    g_offItem = pAdminOP.GetPropOffset("DT_AttributeContainer", "m_Item");

    g_offPlayingMannVsMachine = pAdminOP.GetPropOffset("DT_TFGameRules", "m_bPlayingMannVsMachine");

    // m_targetPosition
    g_offBeamTargetPosition = pAdminOP.GetPropOffsetAnyTable("m_targetPosition");
    if(!g_offBeamTargetPosition)
    {
        g_foundBeamFuncs = 0;
        Warning("[SOURCEOP] Using hardcoded offset for m_targetPosition.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] Using hardcoded offset for m_targetPosition.\n");
        g_offBeamTargetPosition = 848;
    }

    // m_controlPosition
    g_offBeamControlPosition = pAdminOP.GetPropOffsetAnyTable("m_controlPosition");
    if(!g_offBeamControlPosition)
    {
        g_foundBeamFuncs = 0;
        Warning("[SOURCEOP] Using hardcoded offset for m_controlPosition.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] Using hardcoded offset for m_controlPosition.\n");
        g_offBeamControlPosition = 860;
    }

    // m_scrollRate
    g_offBeamScrollRate = pAdminOP.GetPropOffsetAnyTable("m_scrollRate");
    if(!g_offBeamScrollRate)
    {
        g_foundBeamFuncs = 0;
        Warning("[SOURCEOP] Using hardcoded offset for m_scrollRate.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] Using hardcoded offset for m_scrollRate.\n");
        g_offBeamScrollRate = 872;
    }

    // m_flWidth
    g_offBeamWidth = pAdminOP.GetPropOffset("DT_QuadraticBeam", "m_flWidth");
    if(!g_offBeamWidth)
    {
        Warning("[SOURCEOP] Guessing offset for m_flWidth.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] Guessing offset for m_flWidth.\n");
        g_offBeamWidth = g_offBeamScrollRate + 4;
    }

    // m_hAttachedToEntity
    g_offSpriteAttachedEntity = pAdminOP.GetPropOffset("DT_Sprite", "m_hAttachedToEntity");
    if(!g_offSpriteAttachedEntity)
    {
        Warning("[SOURCEOP] No offset for m_hAttachedToEntity.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] No offset for m_hAttachedToEntity.\n");
    }

    // m_nAttachment
    g_offSpriteAttachment = pAdminOP.GetPropOffset("DT_Sprite", "m_nAttachment");
    if(!g_offSpriteAttachment)
    {
        Warning("[SOURCEOP] No offset for m_nAttachment.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] No offset for m_nAttachment.\n");
    }

    // m_iTotalScore
    g_offTotalScore = pAdminOP.GetPropOffset("DT_TFPlayerResource", "m_iTotalScore");
    if(!g_offTotalScore && pAdminOP.isTF2)
    {
        Warning("[SOURCEOP] No offset for m_iTotalScore.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] No offset for m_iTotalScore.\n");
    }
}

void VFuncs::LoadDataDescOffsets(void)
{
    // m_Network
    g_offNetwork = pAdminOP.GetDataDescFieldAnyClass("m_Network");
    if(!g_offNetwork)
    {
        Warning("[SOURCEOP] Didn't find offset for m_Network.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] Didn't find offset for m_Network.\n");
    }

    // m_iClassname
    g_offIClassname = pAdminOP.GetDataDescFieldAnyClass("m_iClassname");
    if(!g_offIClassname)
    {
        Warning("[SOURCEOP] Didn't find offset for m_iClassname.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] Didn't find offset for m_iClassname.\n");
    }

    // m_target
    g_offTarget = pAdminOP.GetDataDescFieldAnyClass("m_target");
    if(!g_offTarget)
    {
        Warning("[SOURCEOP] Didn't find offset for m_target.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] Didn't find offset for m_target.\n");
    }

    // m_takedamage
    g_offTakeDamage = pAdminOP.GetDataDescFieldAnyClass("m_takedamage");
    if(!g_offTakeDamage)
    {
        Warning("[SOURCEOP] Didn't find offset for m_takedamage.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] Didn't find offset for m_takedamage.\n");
    }

    // m_iName
    g_offName = pAdminOP.GetDataDescFieldAnyClass("m_iName");
    if(!g_offName)
    {
        Warning("[SOURCEOP] Didn't find offset for m_iName.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] Didn't find offset for m_iName.\n");
    }

    // m_pPhysicsObject
    g_offPPhysicsObject = pAdminOP.GetDataDescFieldAnyClass("m_pPhysicsObject");
    if(!g_offPPhysicsObject)
    {
        Warning("[SOURCEOP] Didn't find offset for m_pPhysicsObject.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] Didn't find offset for m_pPhysicsObject.\n");
    }

    // m_nWaterLevel
    g_offWaterLevel = pAdminOP.GetDataDescFieldAnyClass("m_nWaterLevel");
    if(!g_offWaterLevel)
    {
        Warning("[SOURCEOP] Didn't find offset for m_nWaterLevel.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] Didn't find offset for m_nWaterLevel.\n");
    }

    // m_ModelName
    g_offModelName = pAdminOP.GetDataDescFieldAnyClass("m_ModelName");
    if(!g_offModelName)
    {
        Warning("[SOURCEOP] Didn't find offset for m_ModelName.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] Didn't find offset for m_ModelName.\n");
    }

    // m_vecAbsVelocity
    g_offVecAbsVelocity = pAdminOP.GetDataDescFieldAnyClass("m_vecAbsVelocity");
    if(!g_offVecAbsVelocity)
    {
        Warning("[SOURCEOP] Didn't find offset for m_vecAbsVelocity.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] Didn't find offset for m_vecAbsVelocity.\n");
    }

    // m_vecAngVelocity
    g_offVecAngVelocity = pAdminOP.GetDataDescFieldAnyClass("m_vecAngVelocity");
    if(!g_offVecAngVelocity)
    {
        Warning("[SOURCEOP] Didn't find offset for m_vecAngVelocity.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] Didn't find offset for m_vecAngVelocity.\n");
    }

    // m_flGravity
    g_offGravity = pAdminOP.GetDataDescFieldAnyClass("m_flGravity");
    if(!g_offGravity)
    {
        Warning("[SOURCEOP] Didn't find offset for m_flGravity.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] Didn't find offset for m_flGravity.\n");
    }

    // m_vecAbsOrigin
    g_offVecAbsOrigin = pAdminOP.GetDataDescFieldAnyClass("m_vecAbsOrigin");
    if(!g_offVecAbsOrigin)
    {
        Warning("[SOURCEOP] Didn't find offset for m_vecAbsOrigin.\n");
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] Didn't find offset for m_vecAbsOrigin.\n");
    }

    // m_angAbsRotation
    g_offAbsRotation = pAdminOP.GetDataDescFieldAnyClass("m_angAbsRotation");
    if(!g_offAbsRotation)
    {
        Warning("[SOURCEOP] Didn't find offset for m_angAbsRotation.\n"); 
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] Didn't find offset for m_angAbsRotation.\n"); 
    }
}

void VFuncs::LoadPlayerDataDescOffsets(void)
{
    // m_nImpulse
    g_offImpulse = pAdminOP.GetDataDescFieldAnyClass("m_nImpulse");
    if(!g_offImpulse)
    {
        Warning("[SOURCEOP] Didn't find offset for m_nImpulse.\n"); 
        pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] Didn't find offset for m_nImpulse.\n"); 
    }
}

CREATE_VFUNC0(CBaseEntity, GetRefEHandle, offs[OFFSET_GETREFEHANDLE], 0, 0, CBaseHandle&)
CREATE_VFUNC0(CBaseEntity, GetCollideable, offs[OFFSET_GETCOLLIDEABLE], 0, 0, ICollideable*)
CREATE_VFUNC0(CBaseEntity, GetNetworkable, offs[OFFSET_GETNETWORKABLE], 0, 0, IServerNetworkable*)
CREATE_VFUNC0(CBaseEntity, GetModelIndex, offs[OFFSET_GETMODELINDEX], 0, 0, int)
CREATE_VFUNC0(CBaseEntity, GetModelName, offs[OFFSET_GETMODELNAME], 0, 0, string_t)
CREATE_VFUNC1_void(CBaseEntity, SetModelIndex, offs[OFFSET_SETMODELINDEX], 0, 0, int, index)
CREATE_VFUNC0(CBaseEntity, GetDataDescMap, offs[OFFSET_GETDATADESCMAP], 0, 0, datamap_t *);
const char * VFuncs::GetClassName( CBaseEntity *pThisPtr )
{
    if(offs[OFFSET_GETCLASSNAME] == -1) return "unknown";

    void **this_ptr = *(void ***)&pThisPtr;
    void **vtable = *(void ***)pThisPtr;
    void *func = vtable[offs[OFFSET_GETCLASSNAME]]; 
 
    union {const char * (VfuncEmptyClass::*mfpnew)( void );
    #ifndef __linux__
            void *addr; } u;    u.addr = func;
    #else // GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 
            struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
    #endif
 
    return (const char *) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)( );
 
}
//CREATE_VFUNC0(CBaseEntity, GetClassName, offs[OFFSET_GETCLASSNAME], 0, 0, const char *)
CREATE_VFUNC0(CBaseEntity, UpdateTransmitState, offs[OFFSET_UPDATETRANSMITSTATE], 0, 0, int)
CREATE_VFUNC0_void(CBaseEntity, Spawn, offs[OFFSET_SPAWN], 0, 0)
CREATE_VFUNC0_void(CBaseEntity, Precache, offs[OFFSET_PRECACHE], 0, 0)
CREATE_VFUNC1_void(CBaseEntity, SetModel, offs[OFFSET_SETMODEL], 0, 0, const char *, ModelName)
CREATE_VFUNC2(CBaseEntity, KeyValue, offs[OFFSET_KEYVALUE], 0, 0, bool, const char *, szKeyName, const char *, szValue)
CREATE_VFUNC0_void(CBaseEntity, Activate, offs[OFFSET_ACTIVATE], 0, 0)
CREATE_VFUNC5(CBaseEntity, AcceptInput, offs[OFFSET_ACCEPTINPUT], 0, 0, bool, const char *, szInputName, CBaseEntity *, pActivator, CBaseEntity *, pCaller, variant_t, Value, int, outputID)
CREATE_VFUNC1(CBaseEntity, PassesDamageFilter, offs[OFFSET_PASSESDAMAGEFILTER], 0, 0, bool, const CTakeDamageInfo &, info)
CREATE_VFUNC4_void(CBaseEntity, TraceAttack, offs[OFFSET_TRACEATTACK], 0, 0, const CTakeDamageInfo &, info, const Vector &, vecDir, trace_t *, ptr, void *, pAccumulator)
CREATE_VFUNC1(CBaseEntity, OnTakeDamage, offs[OFFSET_ONTAKEDAMAGE], 0, 0, int, const CTakeDamageInfo &, info)
CREATE_VFUNC0(CBaseEntity, IsAlive, offs[OFFSET_ISALIVE], 0, 0, bool)
CREATE_VFUNC1_void(CBaseEntity, Event_Killed, offs[OFFSET_EVENTKILLED], 0, 0, const CTakeDamageInfo &, info)
CREATE_VFUNC0(CBaseEntity, IsNPC, offs[OFFSET_ISNPC], 0, 0, bool)
CREATE_VFUNC0(CBaseEntity, IsPlayer, offs[OFFSET_ISPLAYER], 0, 0, bool)
CREATE_VFUNC0(CBaseEntity, IsBaseCombatWeapon, offs[OFFSET_ISBASECOMBATWEAPON], 0, 0, bool)
CREATE_VFUNC1_void(CBaseEntity, ChangeTeam, offs[OFFSET_CHANGETEAM], 0, 0, int, iTeamNum)
CREATE_VFUNC4_void(CBaseEntity, Use, offs[OFFSET_USE], 0, 0, CBaseEntity *, pActivator, CBaseEntity *, pCaller, USE_TYPE, useType, float, value)
CREATE_VFUNC1_void(CBaseEntity, Touch, offs[OFFSET_TOUCH], 0, 0, CBaseEntity *, pOther)
CREATE_VFUNC0_void(CBaseEntity, UpdateOnRemove, offs[OFFSET_UPDATEONREMOVE], 0, 0)
CREATE_VFUNC3_void(CBaseEntity, Teleport, offs[OFFSET_TELEPORT], 0, 0, const Vector *, newPosition, const QAngle *, newAngles, const Vector *, newVelocity)
CREATE_VFUNC1_void(CBaseEntity, FireBullets, offs[OFFSET_FIREBULLETS], 0, 0, const FireBulletsInfo_t &, info)
CREATE_VFUNC1_void(CBaseEntity, SetDamage, offs[OFFSET_SETDAMAGE], 0, 0, float, flDamage)
CREATE_VFUNC0(CBaseEntity, EyePosition, offs[OFFSET_EYEPOSITION], 0, 0, Vector)
CREATE_VFUNC0(CBaseEntity, EyeAngles, offs[OFFSET_EYEANGLES], 0, 0, const QAngle &)
CREATE_VFUNC3(CBaseEntity, FVisible, offs[OFFSET_FVISIBLE], 0, 0, bool, CBaseEntity *, pEntity, int, traceMask, CBaseEntity **, ppBlocker)
CREATE_VFUNC0(CBaseEntity, WorldSpaceCenter, offs[OFFSET_WORLDSPACECENTER], 0, 0, const Vector &)
CREATE_VFUNC0(CBaseEntity, GetSoundEmissionOrigin, offs[OFFSET_GETSOUNDEMISSIONORIGIN], 0, 0, Vector)
CREATE_VFUNC0(CBaseEntity, CreateVPhysics, offs[OFFSET_CREATEVPHYSICS], 0, 0, bool)
CREATE_VFUNC0_void(CBaseEntity, VPhysicsDestroyObject, offs[OFFSET_VPHYSICSDESTROYOBJECT], 0, 0)
CREATE_VFUNC2(CBaseEntity, VPhysicsGetObjectList, offs[OFFSET_VPHYSICSGETOBJECTLIST], 0, 0, int, IPhysicsObject **, pList, int, listMax)
CREATE_VFUNC0_void(CBaseAnimating, StudioFrameAdvance, offs[OFFSET_STUDIOFRAMEADVANCE], 0, 0)
CREATE_VFUNC0_void(CBaseAnimating, Extinguish, offs[OFFSET_EXTINGUISH], 0, 0)
CREATE_VFUNC3(CBasePlayer, GiveAmmo, offs[OFFSET_GIVEAMMO], 0, 0, int, int, iCount, int, iAmmoIndex, bool, bSuppressSound)
CREATE_VFUNC1_void(CBasePlayer, Weapon_Equip, offs[OFFSET_WEAPONEQUIP], 0, 0, CBaseCombatWeapon *, pWeapon)
CREATE_VFUNC2(CBasePlayer, Weapon_Switch, offs[OFFSET_WEAPONSWITCH], 0, 0, bool, CBaseCombatWeapon *, pWeapon, int, viewmodelindex)
CREATE_VFUNC1(CBasePlayer, Weapon_GetSlot, offs[OFFSET_WEAPONGETSLOT], 0, 0, CBaseCombatWeapon *, int, slot)
CREATE_VFUNC1(CBasePlayer, RemovePlayerItem, offs[OFFSET_REMOVEPLAYERITEM], 0, 0, bool, CBaseCombatWeapon *, pItem)
CREATE_VFUNC1(CBaseCombatWeapon, Holster, offs[OFFSET_HOLSTER], 0, 0, bool, CBaseCombatWeapon *, pSwitchingTo)
CREATE_VFUNC1_void(CBaseEntity, SetDamageRadius, offs[OFFSET_SETDAMAGERADIUS], 0, 0, float, flDamageRadius)
CREATE_VFUNC0_void(CBasePlayer, ForceRespawn, offs[OFFSET_FORCERESPAWN], 0, 0)
CREATE_VFUNC1(CBasePlayer, StartObserverMode, offs[OFFSET_STARTOBSERVERMODE], 0, 0, bool, int, mode)
CREATE_VFUNC0_void(CBasePlayer, StopObserverMode, offs[OFFSET_STOPOBSERVERMODE], 0, 0)
CREATE_VFUNC2(CBasePlayer, GiveNamedItem, offs[OFFSET_GIVENAMEDITEM], 0, 0, CBaseEntity *, const char *, pszName, int, iSubType)

CREATE_VFUNC2(void, PlayerRelationship, offs[OFFSET_PLAYERRELATIONSHIP], 0, 0, int, CBaseEntity *, pPlayer, CBaseEntity *, pTarget)
CREATE_VFUNC2(void, PlayerCanHearChat, offs[OFFSET_PLAYERCANHEARCHAT], 0, 0, bool, CBasePlayer *, pListener, CBasePlayer *, pSpeaker)
CREATE_VFUNC1(void, GetTeamIndex, offs[OFFSET_GETTEAMINDEX], 0, 0, int, const char *, pTeamName)
CREATE_VFUNC1(void, GetIndexedTeamName, offs[OFFSET_GETINDEXEDTEAMNAME], 0, 0, const char *, int, teamIndex)
CREATE_VFUNC1(void, IsValidTeam, offs[OFFSET_ISVALIDTEAM], 0, 0, bool, const char *, pTeamName)
CREATE_VFUNC2_void(void, MarkAchievement, offs[OFFSET_MARKACHIEVEMENT], 0, 0, IRecipientFilter &, filter, char const *, pchAchievementName)
CREATE_VFUNC3(void, GetNextLevelName, offs[OFFSET_GETNEXTLEVELNAME], 0, 0, int, char *, szName, size_t, bufsize, bool, unknown)
CREATE_VFUNC0_void(void, ChangeLevel, offs[OFFSET_CHANGELEVEL], 0, 0)
CREATE_VFUNC3_void(void, SetStalemate, offs[OFFSET_SETSTALEMATE], 0, 0, int, iReason, bool, bForceMapReset, bool, bUnknown)
CREATE_VFUNC1_void(void, SetSwitchTeams, offs[OFFSET_SETSWITCHTEAMS], 0, 0, bool, bSwitch);
CREATE_VFUNC0_void(void, HandleSwitchTeams, offs[OFFSET_HANDLESWITCHTEAMS], 0, 0);
CREATE_VFUNC1_void(void, SetScrambleTeams, offs[OFFSET_SETSCRAMBLETEAMS], 0, 0, bool, bScramble);
CREATE_VFUNC0_void(void, HandleScrambleTeams, offs[OFFSET_HANDLESCRAMBLETEAMS], 0, 0);
bool VFuncs::IsMannVsMachineMode( void *pThisPtr )
{
    DECLARE_VAR_RVAL(bool, m_bPlayingMannVsMachine, pThisPtr, g_offPlayingMannVsMachine, false);
    return *m_bPlayingMannVsMachine;
}


CREATE_VFUNC0(IServerNetworkable, GetEntityHandle, offs[OFFSET_GETENTITYHANDLE], 0, 0, IHandleEntity *)
CREATE_VFUNC0(IServerNetworkable, GetEdict, offs[OFFSET_GETEDICT], 0, 0, edict_t *)
CREATE_VFUNC0_void(IServerNetworkable, Release, offs[OFFSET_RELEASE], 0, 0)
CREATE_VFUNC0(IServerNetworkable, GetBaseEntity, offs[OFFSET_GETBASEENTITY], 0, 0, CBaseEntity *)

#ifndef __linux__
#define CGAMESERVER_VTBL_OFFS 1
#else
#define CGAMESERVER_VTBL_OFFS 0
#endif
CREATE_VFUNC0_void(void, Reconnect, offs[OFFSET_RECONNECT], CGAMESERVER_VTBL_OFFS, CGAMESERVER_VTBL_OFFS)
//CREATE_VFUNC0_void(void, Disconnect, 12)
void VFuncs::Disconnect( void *pThisPtr, const char *fmt, ... )
{
    void **this_ptr = (*(void ***)&pThisPtr)+CGAMESERVER_VTBL_OFFS;
    void **vtable = *(void ***)((int *)pThisPtr+CGAMESERVER_VTBL_OFFS);
    void *func = vtable[offs[OFFSET_DISCONNECT]]; 
 
    union {void (VfuncEmptyClass::*mfpnew)( const char *fmt, ... );
    #ifndef __linux__
            void *addr; } u;    u.addr = func;
    #else // GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 
            struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
    #endif
 
    static char string[1024];
    va_list argptr; 
    va_start(argptr, fmt);
    vsprintf(string, fmt, argptr);
    va_end(argptr);

    (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)( string ); 
}
CREATE_VFUNC0(void, GetPlayerSlot, offs[OFFSET_GETPLAYERSLOT], CGAMESERVER_VTBL_OFFS, CGAMESERVER_VTBL_OFFS, short);
CREATE_VFUNC0(void, GetNetworkID, offs[OFFSET_GETNETWORKID], CGAMESERVER_VTBL_OFFS, CGAMESERVER_VTBL_OFFS, USERID_t);
CREATE_VFUNC0(void, GetClientName, offs[OFFSET_GETCLIENTNAME], CGAMESERVER_VTBL_OFFS, CGAMESERVER_VTBL_OFFS, const char*);
CREATE_VFUNC0(void, GetNetChannel, offs[OFFSET_GETNETCHANNEL], CGAMESERVER_VTBL_OFFS, CGAMESERVER_VTBL_OFFS, void*);
CREATE_VFUNC0(void, GetServer, offs[OFFSET_GETSERVER], CGAMESERVER_VTBL_OFFS, CGAMESERVER_VTBL_OFFS, void *);
CREATE_VFUNC2_void(void, SetUserCVar, offs[OFFSET_SETUSERCVAR], 0, 0, const char *, cvar, const char *, value)
CREATE_VFUNC0(void, IGetPlayerSlot, offs[OFFSET_IGETPLAYERSLOT], 0, 0, short);

CREATE_VFUNC0(void, GetNumClients, offs[OFFSET_GETNUMCLIENTS], 0, 0, int);
CREATE_VFUNC0(void, GetMaxClients, offs[OFFSET_GETMAXCLIENTS], 0, 0, int);

void VFuncs::RejectConnection( void *pThisPtr, const netadr_t &net, int challenge, const char *string )
{
    void **this_ptr = *(void ***)&pThisPtr;
    void **vtable = *(void ***)((int *)pThisPtr);
    void *func = vtable[offs[OFFSET_REJECTCONNECTION]]; 
 
    union {void (VfuncEmptyClass::*mfpnew)( const netadr_t &net, int challenge, const char *string );
    #ifndef __linux__
            void *addr; } u;    u.addr = func;
    #else // GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 
            struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
    #endif

    (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)( net, challenge, string ); 
}


/*
Vector VFuncs::EyePosition( CBaseEntity *pThisPtr )
{
    void **this_ptr = *(void ***)&pThisPtr;
    void **vtable = *(void ***)pThisPtr;
    void *func = vtable[118]; 
 
    union {Vector (VfuncEmptyClass::*mfpnew)( void );
    #ifndef __linux__
            void *addr; } u;    u.addr = func;
    #else // GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 
            struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
    #endif
 
    return (Vector) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)( );
 
}
*/

// SetParent is special case
// old version sdk didnt have SetParent as VFunc
void VFuncs::SetParent( CBaseEntity *pThisPtr, CBaseEntity* pNewParent, int iAttachment )
{
    if(offs[OFFSET_SETPARENT] != -1)
    {
        void **this_ptr = *(void ***)&pThisPtr;
        void **vtable = *(void ***)pThisPtr;
        void *func = vtable[offs[OFFSET_SETPARENT]]; 
     
        union {void (VfuncEmptyClass::*mfpnew)( CBaseEntity*, int );
        #ifndef __linux__
                void *addr; } u;    u.addr = func;
        #else // GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 
                struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
        #endif
     
        (void) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)( pNewParent, iAttachment );
    }
    else
    {
        // TODO: Add workaround method
    }
}

CBaseEntity *VFuncs::Instance( edict_t *pent )
{
    if ( !pent )
    {
        pent = INDEXENT(0);
    }

    // FIXME: this really doesn't have to be networkable
    IServerNetworkable *pNet = pent ? pent->GetNetworkable() : NULL;
    if ( pNet && pent->GetUnknown() )
    {
        return VFuncs::GetBaseEntity(pNet);
    }
    return NULL;
}

void VFuncs::SetClassname( CBaseEntity *pThisPtr, const char *className )
{
    DECLARE_VAR(string_t, m_iClassname, pThisPtr, g_offIClassname);
    *m_iClassname = MAKE_STRING(className);
}

static string_t g_NullString = NULL_STRING;
FORCEINLINE bool NamesMatch( const char *pszQuery, string_t nameToMatch )
{
    if ( nameToMatch == NULL_STRING )
        return (*pszQuery == 0 || *pszQuery == '*');

    const char *pszNameToMatch = STRING(nameToMatch);

    // If the pointers are identical, we're identical
    if ( pszNameToMatch == pszQuery )
        return true;

    while ( *pszNameToMatch && *pszQuery )
    {
        char cName = *pszNameToMatch;
        char cQuery = *pszQuery;
        if ( cName != cQuery && tolower(cName) != tolower(cQuery) ) // people almost always use lowercase, so assume that first
            break;
        ++pszNameToMatch;
        ++pszQuery;
    }

    if ( *pszQuery == 0 && *pszNameToMatch == 0 )
        return true;

    // @TODO (toml 03-18-03): Perhaps support real wildcards. Right now, only thing supported is trailing *
    if ( *pszQuery == '*' )
        return true;

    return false;
}

bool VFuncs::ClassMatches( CBaseEntity *pThisPtr, const char *pszClassOrWildcard )
{
    string_t* m_iClassname;
    if(g_offIClassname)
        m_iClassname = (string_t*)(((unsigned int)pThisPtr)+(g_offIClassname));
    else
        m_iClassname = &g_NullString;

    if ( IDENT_STRINGS(m_iClassname, pszClassOrWildcard ) )
        return true;
    return NamesMatch( pszClassOrWildcard, *m_iClassname );
}

char const *VFuncs::GetClassname( CBaseEntity *pThisPtr )
{
    DECLARE_VAR_RVAL(string_t, m_iClassname, pThisPtr, g_offIClassname, "");
    return STRING(*m_iClassname);
}

bool VFuncs::NameMatches( CBaseEntity *pThisPtr, const char *pszNameOrWildcard )
{
    string_t* m_iName;
    if(g_offName)
        m_iName = (string_t*)(((unsigned int)pThisPtr)+(g_offName));
    else
        m_iName = &g_NullString;

    if ( IDENT_STRINGS(m_iName, pszNameOrWildcard ) )
        return true;

    // NameMatchesComplex
    if ( !Q_stricmp( "!player", pszNameOrWildcard) )
        return VFuncs::IsPlayer(pThisPtr);
    return NamesMatch( pszNameOrWildcard, *m_iName );
}

int VFuncs::entindex( CBaseEntity *pThisPtr )
{
    edict_t *pEnt = servergameents->BaseEntityToEdict(pThisPtr);
    if(pEnt)
    {
        return ENTINDEX(pEnt);
    }
    else
    {
        if(!g_suppressEntindexError)
        {
            CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] VFuncs::entindex got a null pEnt for ent %s (%s).\n", VFuncs::GetClassname(pThisPtr), VFuncs::GetClassName(pThisPtr));
            pAdminOP.TimeLog("SourceOPErrors.log", "[SOURCEOP] VFuncs::entindex got a null pEnt for ent %s (%s).\n", VFuncs::GetClassname(pThisPtr), VFuncs::GetClassName(pThisPtr));
        }
        return 0;
    }
}

void VFuncs::SetSimulationTime(CBaseEntity *pThisPtr, float st)
{
    DECLARE_VAR(float, m_flSimulationTime, pThisPtr, g_offSimulationTime);
    *m_flSimulationTime = st;
}

int VFuncs::GetMaxHealth( CBaseEntity *pThisPtr )
{
    DECLARE_VAR_RVAL(int, m_iMaxHealth, pThisPtr, g_offIMaxHealth, 0);
    return *m_iMaxHealth;
}

int VFuncs::GetHealth( CBaseEntity *pThisPtr )
{
    DECLARE_VAR_RVAL(int, m_iHealth, pThisPtr, g_offIHealth, 0);
    return *m_iHealth;
}

void VFuncs::SetHealth( CBaseEntity *pThisPtr, int amt )
{
    DECLARE_VAR(int, m_iHealth, pThisPtr, g_offIHealth);
    *m_iHealth = amt;

    SetEdictStateChanged(servergameents->BaseEntityToEdict(pThisPtr), g_offIHealth);
}

Vector VFuncs::GetAbsOrigin( const CBaseEntity *pThisPtr )
{
    DECLARE_VAR_RVAL(Vector, m_vecAbsOrigin, pThisPtr, g_offVecAbsOrigin, Vector(0,0,0));
    return *m_vecAbsOrigin;
}

void VFuncs::SetAbsOrigin( CBaseEntity *pThisPtr, const Vector& absOrigin )
{
    VFuncs::Teleport(pThisPtr, &absOrigin, NULL, NULL);
}

void VFuncs::SetAbsAngles( CBaseEntity *pThisPtr, const QAngle& absAngles )
{
    VFuncs::Teleport(pThisPtr, NULL, &absAngles, NULL);
}

Vector VFuncs::GetAbsVelocity( const CBaseEntity *pThisPtr )
{
    DECLARE_VAR_RVAL(Vector, m_vecAbsVelocity, pThisPtr, g_offVecAbsVelocity, Vector(0,0,0));
    return *m_vecAbsVelocity;
}

void VFuncs::SetAbsVelocity( CBaseEntity *pThisPtr, const Vector &vecAbsVelocity )
{
    if ( VFuncs::GetAbsVelocity(pThisPtr) == vecAbsVelocity )
        return;

    VFuncs::Teleport(pThisPtr, NULL, NULL, &vecAbsVelocity);
    return;
}

Vector VFuncs::GetLocalOrigin(CBaseEntity *pThisPtr)
{
    DECLARE_VAR_RVAL(Vector, m_vecOrigin, pThisPtr, g_offVecLocalOrigin, Vector(0,0,0));
    return *m_vecOrigin;
}

void VFuncs::SetLocalOrigin(CBaseEntity *pThisPtr, const Vector& origin)
{
    DECLARE_VAR(Vector, m_vecOrigin, pThisPtr, g_offVecLocalOrigin);

    //InvalidatePhysicsRecursive( POSITION_CHANGED );
    *m_vecOrigin = origin;
    VFuncs::SetSimulationTime(pThisPtr, gpGlobals->curtime);
}

QAngle VFuncs::GetLocalAngles( CBaseEntity *pThisPtr )
{
    DECLARE_VAR_RVAL(QAngle, m_angRotation, pThisPtr, g_offAngRotation, QAngle(0,0,0));
    return *m_angRotation;
}

void VFuncs::SetLocalAngles(CBaseEntity *pThisPtr, const QAngle& angles)
{
    DECLARE_VAR(QAngle, m_angRotation, pThisPtr, g_offAngRotation);

    //InvalidatePhysicsRecursive( POSITION_CHANGED );
    *m_angRotation = angles;
    VFuncs::SetSimulationTime(pThisPtr, gpGlobals->curtime);
}

QAngle VFuncs::GetLocalAngularVelocity( CBaseEntity *pThisPtr )
{
    DECLARE_VAR_RVAL(QAngle, m_vecAngVelocity, pThisPtr, g_offVecAngVelocity, QAngle(0,0,0));
    return *m_vecAngVelocity;
}

void VFuncs::SetLocalAngularVelocity( CBaseEntity *pThisPtr, const QAngle &vecAngVelocity )
{
    DECLARE_VAR(QAngle, m_vecAngVelocity, pThisPtr, g_offVecAngVelocity);
    if (*m_vecAngVelocity != vecAngVelocity)
    {
//      InvalidatePhysicsRecursive( EFL_DIRTY_ABSANGVELOCITY );
        *m_vecAngVelocity = vecAngVelocity;
    }
}

void VFuncs::SetGravity( CBaseEntity *pThisPtr, float gravity )
{
    DECLARE_VAR(float, m_flGravity, pThisPtr, g_offGravity);
    *m_flGravity = gravity;
}

void VFuncs::SetFriction( CBaseEntity *pThisPtr, float flFriction )
{
    DECLARE_VAR(float, m_flFriction, pThisPtr, g_offFriction);
    *m_flFriction = flFriction;
}

void VFuncs::SetModelName( CBaseEntity *pThisPtr, string_t name )
{
    DECLARE_VAR(string_t, m_ModelName, pThisPtr, g_offModelName);
    *m_ModelName = name;
    pThisPtr->DispatchUpdateTransmitState();
}

Vector CBaseEntity::GetSoundEmissionOrigin() const
{
    // cast away constness
    return VFuncs::GetSoundEmissionOrigin((CBaseEntity *)this);
}

int VFuncs::GetFlags( CBaseEntity *pThisPtr )
{
    DECLARE_VAR_RVAL(int, m_fFlags, pThisPtr, g_offFFlags, 0);
    return *m_fFlags;
}

void VFuncs::AddFlag( CBaseEntity *pThisPtr, int flags )
{
    DECLARE_VAR(int, m_fFlags, pThisPtr, g_offFFlags);
    *m_fFlags = *m_fFlags | flags;
}

void VFuncs::RemoveFlag( CBaseEntity *pThisPtr, int flagsToRemove )
{
    DECLARE_VAR(int, m_fFlags, pThisPtr, g_offFFlags);
    *m_fFlags = *m_fFlags & ~flagsToRemove;
    //CHANGE_FLAGS( m_fFlags, m_fFlags & ~flagsToRemove );
}

void VFuncs::ToggleFlag( CBaseEntity *pThisPtr, int flagToToggle )
{
    DECLARE_VAR(int, m_fFlags, pThisPtr, g_offFFlags);
    *m_fFlags = *m_fFlags ^ flagToToggle;
}

bool VFuncs::IsMarkedForDeletion( CBaseEntity *pThisPtr )
{
    return VFuncs::GetEFlags(pThisPtr) & EFL_KILLME;
}

int VFuncs::GetEFlags( CBaseEntity *pThisPtr )
{
    DECLARE_VAR_RVAL(int, m_iEFlags, pThisPtr, g_offFEFlags, 0);
    return *m_iEFlags;
}

void VFuncs::SetEFlags( CBaseEntity *pThisPtr, int iEFlags )
{
    DECLARE_VAR(int, m_iEFlags, pThisPtr, g_offFEFlags);
    *m_iEFlags = iEFlags;

    /*if ( iEFlags & ( EFL_FORCE_CHECK_TRANSMIT | EFL_IN_SKYBOX ) )
        DispatchUpdateTransmitState();
    
    // Make sure the EFL_DIRTY_PVS_INFORMATION flag gets passed onto the edict.
    if ( (iEFlags & EFL_DIRTY_PVS_INFORMATION) && edict() )
        edict()->m_fStateFlags |= FL_EDICT_DIRTY_PVS_INFORMATION;*/
}

void VFuncs::AddEFlags( CBaseEntity *pThisPtr, int nEFlagMask )
{
    DECLARE_VAR(int, m_iEFlags, pThisPtr, g_offFEFlags);
    *m_iEFlags |= nEFlagMask;

    /*if ( nEFlagMask & ( EFL_FORCE_CHECK_TRANSMIT | EFL_IN_SKYBOX ) )
        DispatchUpdateTransmitState();

    // Make sure the EFL_DIRTY_PVS_INFORMATION flag gets passed onto the edict.
    if ( (nEFlagMask & EFL_DIRTY_PVS_INFORMATION) && edict() )
        edict()->m_fStateFlags |= FL_EDICT_DIRTY_PVS_INFORMATION;*/
}

void VFuncs::RemoveEFlags( CBaseEntity *pThisPtr, int nEFlagMask )
{
    DECLARE_VAR(int, m_iEFlags, pThisPtr, g_offFEFlags);
    *m_iEFlags &= ~nEFlagMask;
    
    /*if ( nEFlagMask & ( EFL_FORCE_CHECK_TRANSMIT | EFL_IN_SKYBOX ) )
        DispatchUpdateTransmitState();*/
}

bool VFuncs::IsEFlagSet( CBaseEntity *pThisPtr, int nEFlagMask )
{
    DECLARE_VAR_RVAL(int, m_iEFlags, pThisPtr, g_offFEFlags, false);
    return (*m_iEFlags & nEFlagMask) != 0;
}

CCollisionProperty *VFuncs::CollisionProp( CBaseEntity *pThisPtr )
{
    DECLARE_VAR_RVAL(CCollisionProperty, m_Collision, pThisPtr, g_offCollision, NULL);
    return m_Collision;
}

CServerNetworkProperty *VFuncs::NetworkProp( CBaseEntity *pThisPtr )
{
    DECLARE_VAR_RVAL(CServerNetworkProperty, m_Network, pThisPtr, g_offNetwork, NULL);
    return m_Network;
}

IPhysicsObject *VFuncs::VPhysicsGetObject( CBaseEntity *pThisPtr )
{
    DECLARE_VAR_RVAL(IPhysicsObject*, m_pPhysicsObject, pThisPtr, g_offPPhysicsObject, NULL);
    return *m_pPhysicsObject;
}

void VFuncs::VPhysicsSetObject( CBaseEntity *pThisPtr, IPhysicsObject *pPhysics )
{
    DECLARE_VAR(IPhysicsObject*, m_pPhysicsObject, pThisPtr, g_offPPhysicsObject);
    *m_pPhysicsObject = pPhysics;
}

MoveType_t VFuncs::GetMoveType( CBaseEntity *pThisPtr )
{
    DECLARE_VAR_RVAL(unsigned char, m_MoveType, pThisPtr, g_offMovetype, MOVETYPE_NONE);
    return (MoveType_t)*m_MoveType;
}

void VFuncs::SetMoveType( CBaseEntity *pThisPtr, MoveType_t val, MoveCollide_t moveCollide )
{
    if ( g_nServerToolsVersion >= 2 )
    {
        servertools->SetMoveType( pThisPtr, val, moveCollide );
        return;
    }

    if(_SetMoveType)
    {
        void **this_ptr = *(void ***)&pThisPtr;
        void *func = _SetMoveType; 
     
        union {void (VfuncEmptyClass::*mfpnew)( MoveType_t, MoveCollide_t);
        #ifndef __linux__
                void *addr; } u;    u.addr = func;
        #else // GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 
                struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
        #endif
     
        (void) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)( val, moveCollide );

        return;
    }

    // fallback method
    DECLARE_VAR(unsigned char, m_MoveType, pThisPtr, g_offMovetype);
    DECLARE_VAR(unsigned char, m_MoveCollide, pThisPtr, g_offMovecollide);
/*#ifdef _DEBUG
    // Make sure the move type + move collide are compatible...
    if ((val != MOVETYPE_FLY) && (val != MOVETYPE_FLYGRAVITY))
    {
        Assert( moveCollide == MOVECOLLIDE_DEFAULT );
    }

    if ( m_MoveType == MOVETYPE_VPHYSICS && val != m_MoveType )
    {
        if ( VPhysicsGetObject() && val != MOVETYPE_NONE )
        {
            // What am I supposed to do with the physics object if
            // you're changing away from MOVETYPE_VPHYSICS without making the object 
            // shadow?  This isn't likely to work, assert.
            // You probably meant to call VPhysicsInitShadow() instead of VPhysicsInitNormal()!
            Assert( VPhysicsGetObject()->GetShadowController() );
        }
    }
#endif*/

    if ( *m_MoveType == val )
    {
        *m_MoveCollide = moveCollide;
        return;
    }

    // This is needed to the removal of MOVETYPE_FOLLOW:
    // We can't transition from follow to a different movetype directly
    // or the leaf code will break.
    //Assert( !IsEffectActive( EF_BONEMERGE ) );
    *m_MoveType = val;
    *m_MoveCollide = moveCollide;

    pThisPtr->CollisionRulesChanged();

    switch( *m_MoveType )
    {
    case MOVETYPE_WALK:
        {
            VFuncs::SetSimulatedEveryTick( pThisPtr, true );
            VFuncs::SetAnimatedEveryTick( pThisPtr, true );
        }
        break;
    case MOVETYPE_STEP:
        {
            // This will probably go away once I remove the cvar that controls the test code
            VFuncs::SetSimulatedEveryTick( pThisPtr, true );
            VFuncs::SetAnimatedEveryTick( pThisPtr, false );
        }
        break;
    default:
        {
            VFuncs::SetSimulatedEveryTick( pThisPtr, true );
            VFuncs::SetAnimatedEveryTick( pThisPtr, false );
        }
    }

    // This will probably go away or be handled in a better way once I remove the cvar that controls the test code
    //CheckStepSimulationChanged();
    VFuncs::CheckHasGamePhysicsSimulation(pThisPtr);
}

void VFuncs::FollowEntity( CBaseEntity *pThisPtr, CBaseEntity *pBaseEntity, bool bBoneMerge )
{
    if (pBaseEntity)
    {
        VFuncs::SetParent( pThisPtr, pBaseEntity );
        VFuncs::SetMoveType( pThisPtr, MOVETYPE_NONE );
        
        if ( bBoneMerge )
            VFuncs::AddEffects( pThisPtr, EF_BONEMERGE );

        VFuncs::AddSolidFlags( pThisPtr, FSOLID_NOT_SOLID );
        VFuncs::SetLocalOrigin( pThisPtr, vec3_origin );
        VFuncs::SetLocalAngles( pThisPtr, vec3_angle );
    }
    else
    {
        VFuncs::StopFollowingEntity(pThisPtr);
    }
}

void VFuncs::StopFollowingEntity( CBaseEntity *pThisPtr )
{
    /*if( !IsFollowingEntity() )
    {
        Assert( IsEffectActive( EF_BONEMERGE ) == 0 );
        return;
    }*/

    VFuncs::SetParent( pThisPtr, NULL );
    VFuncs::RemoveEffects( pThisPtr, EF_BONEMERGE );
    VFuncs::RemoveSolidFlags( pThisPtr, FSOLID_NOT_SOLID );
    VFuncs::SetMoveType( pThisPtr, MOVETYPE_NONE );
    pThisPtr->CollisionRulesChanged();
}

void VFuncs::CheckHasGamePhysicsSimulation( CBaseEntity *pThisPtr )
{
    bool isSimulating = VFuncs::WillSimulateGamePhysics(pThisPtr);
    if ( isSimulating != VFuncs::IsEFlagSet(pThisPtr,EFL_NO_GAME_PHYSICS_SIMULATION) )
        return;
    if ( isSimulating )
    {
        VFuncs::RemoveEFlags( pThisPtr, EFL_NO_GAME_PHYSICS_SIMULATION );
    }
    else
    {
        VFuncs::AddEFlags( pThisPtr, EFL_NO_GAME_PHYSICS_SIMULATION );
    }
}

bool VFuncs::WillSimulateGamePhysics( CBaseEntity *pThisPtr )
{
    // players always simulate game physics
    if ( !VFuncs::IsPlayer(pThisPtr) )
    {
        MoveType_t movetype = VFuncs::GetMoveType(pThisPtr);
        
        if ( movetype == MOVETYPE_NONE || movetype == MOVETYPE_VPHYSICS )
            return false;

#if !defined( CLIENT_DLL )
        // MOVETYPE_PUSH not supported on the client
        //if ( movetype == MOVETYPE_PUSH && GetMoveDoneTime() <= 0 )
        //  return false;
#endif
    }

    return true;
}

int VFuncs::GetCollisionGroup( CBaseEntity *pThisPtr )
{
    DECLARE_VAR_RVAL(int, m_CollisionGroup, pThisPtr, g_offCollisionGroup, 0);
    return *m_CollisionGroup;
}

void VFuncs::SetCollisionGroup( CBaseEntity *pThisPtr, int collisionGroup )
{
    DECLARE_VAR(int, m_CollisionGroup, pThisPtr, g_offCollisionGroup);
    if ( (int)*m_CollisionGroup != collisionGroup )
    {
        *m_CollisionGroup = collisionGroup;
        pThisPtr->CollisionRulesChanged();
    }
}

void VFuncs::SetSimulatedEveryTick( CBaseEntity *pThisPtr, bool sim )
{
    DECLARE_VAR(bool, m_bSimulatedEveryTick, pThisPtr, g_offSimulatedEveryTick);
    if ( *m_bSimulatedEveryTick != sim )
    {
        *m_bSimulatedEveryTick = sim;
    }
}

void VFuncs::SetAnimatedEveryTick( CBaseEntity *pThisPtr, bool anim )
{
    DECLARE_VAR(bool, m_bAnimatedEveryTick, pThisPtr, g_offAnimatedEveryTick);
    if ( *m_bAnimatedEveryTick != anim )
    {
        *m_bAnimatedEveryTick = anim;
    }
}

char VFuncs::GetTakeDamage( CBaseEntity *pThisPtr )
{
    DECLARE_VAR_RVAL(char, m_takedamage, pThisPtr, g_offTakeDamage, DAMAGE_NO);
    return *m_takedamage;
}

void VFuncs::SetTakeDamage( CBaseEntity *pThisPtr, char takedamage )
{
    DECLARE_VAR(char, m_takedamage, pThisPtr, g_offTakeDamage);
    if( *m_takedamage != takedamage )
    {
        *m_takedamage = takedamage;
    }
}

int VFuncs::GetSkin( CBaseAnimating *pThisPtr )
{
    DECLARE_VAR_RVAL(int, m_nSkin, pThisPtr, g_offSkin, 0);
    return *m_nSkin;
}

void VFuncs::SetSkin( CBaseAnimating *pThisPtr, int nSkin )
{
    DECLARE_VAR(int, m_nSkin, pThisPtr, g_offSkin);
    if( (int)*m_nSkin != nSkin )
    {
        *m_nSkin = nSkin;
    }
}

int VFuncs::GetSequence( CBaseAnimating *pThisPtr )
{
    DECLARE_VAR_RVAL(int, m_nSequence, pThisPtr, g_offSequence, 0);
    return *m_nSequence;
}

void VFuncs::SetSequence( CBaseAnimating *pThisPtr, int nSequence )
{
    DECLARE_VAR(int, m_nSequence, pThisPtr, g_offSequence);
    if( (int)*m_nSequence != nSequence )
    {
        *m_nSequence = nSequence;
    }
}

void VFuncs::ResetSequence( CBaseAnimating *pThisPtr, int nSequence )
{
    if ( g_nServerToolsVersion >= 2 )
    {
        servertools->ResetSequence( pThisPtr, nSequence );
        return;
    }

    if(_ResetSequence)
    {
        void **this_ptr = *(void ***)&pThisPtr;
        void *func = _ResetSequence; 

        union {void (VfuncEmptyClass::*mfpnew)( int );
        #ifndef __linux__
                void *addr; } u;    u.addr = func;
        #else // GCC's member function pointers all contain a this pointer adjustor. You'd probably set it to 0 
                struct {void *addr; intptr_t adjustor;} s; } u; u.s.addr = func; u.s.adjustor = 0;
        #endif

        (void) (reinterpret_cast<VfuncEmptyClass*>(this_ptr)->*u.mfpnew)( nSequence );

        return;
    }
}

float VFuncs::GetDeathTime( CBasePlayer *pThisPtr )
{
    DECLARE_VAR_RVAL(float, m_flDeathTime, pThisPtr, g_offDeathTime, 0);
    return *m_flDeathTime;
}

void VFuncs::SetDeathTime( CBasePlayer *pThisPtr, float flDeathTime )
{
    DECLARE_VAR(float, m_flDeathTime, pThisPtr, g_offDeathTime);
    if( (float)*m_flDeathTime != flDeathTime )
    {
        *m_flDeathTime = flDeathTime;
    }
}

int VFuncs::GetPlayerClass( CBasePlayer *pThisPtr )
{
    DECLARE_VAR_RVAL(int, m_PlayerClass, pThisPtr, g_offPlayerClass, 0);
    return *m_PlayerClass;
}

void VFuncs::SetPlayerClass( CBasePlayer *pThisPtr, int PlayerClass )
{
    DECLARE_VAR(int, m_PlayerClass, pThisPtr, g_offPlayerClass);
    if( (int)*m_PlayerClass != PlayerClass )
    {
        *m_PlayerClass = PlayerClass;
    }
}

int VFuncs::GetDesiredPlayerClass( CBasePlayer *pThisPtr )
{
    DECLARE_VAR_RVAL(int, m_iDesiredPlayerClass, pThisPtr, g_offDesiredPlayerClass, 0);
    return *m_iDesiredPlayerClass;
}

void VFuncs::SetDesiredPlayerClass( CBasePlayer *pThisPtr, int PlayerClass )
{
    DECLARE_VAR(int, m_iDesiredPlayerClass, pThisPtr, g_offDesiredPlayerClass);
    if( (int)*m_iDesiredPlayerClass != PlayerClass )
    {
        *m_iDesiredPlayerClass = PlayerClass;
    }
}

int VFuncs::GetNumBuildings( CBasePlayer *pThisPtr )
{
    DECLARE_VAR_RVAL(int, m_iObjectCount, pThisPtr, g_offNumBuildings, 0);
    return *m_iObjectCount;
}

void VFuncs::SetNumBuildings( CBasePlayer *pThisPtr, int numBuildings )
{
    DECLARE_VAR(int, m_iObjectCount, pThisPtr, g_offNumBuildings);
    if( (int)*m_iObjectCount != numBuildings )
    {
        *m_iObjectCount = numBuildings;
    }
}

int VFuncs::GetPlayerState( CBasePlayer *pThisPtr )
{
    DECLARE_VAR_RVAL(int, m_nPlayerState, pThisPtr, g_offPlayerState, 0);
    return *m_nPlayerState;
}

void VFuncs::SetPlayerState( CBasePlayer *pThisPtr, int nPlayerState )
{
    DECLARE_VAR(int, m_nPlayerState, pThisPtr, g_offPlayerState);
    if( (int)*m_nPlayerState != nPlayerState )
    {
        *m_nPlayerState = nPlayerState;
    }
}

int VFuncs::GetPlayerCond( CBasePlayer *pThisPtr )
{
    DECLARE_VAR_RVAL(int, m_nPlayerCond, pThisPtr, g_offPlayerCond, 0);
    return *m_nPlayerCond;
}

void VFuncs::SetPlayerCond( CBasePlayer *pThisPtr, int nPlayerCond )
{
    DECLARE_VAR(int, m_nPlayerCond, pThisPtr, g_offPlayerCond);
    if( (int)*m_nPlayerCond != nPlayerCond )
    {
        *m_nPlayerCond = nPlayerCond;
    }
}

unsigned short VFuncs::GetUber( CBasePlayer *pThisPtr )
{
    DECLARE_VAR_RVAL(unsigned short, m_nUber, pThisPtr, g_offUber, 0);
    return *m_nUber;
}

void VFuncs::SetUber( CBasePlayer *pThisPtr, unsigned short nUber )
{
    DECLARE_VAR(unsigned short, m_nUber, pThisPtr, g_offUber);
    if( (unsigned short)*m_nUber != nUber )
    {
        *m_nUber = nUber;
    }
}

unsigned short VFuncs::GetKritz( CBasePlayer *pThisPtr )
{
    DECLARE_VAR_RVAL(unsigned short, m_nUber, pThisPtr, g_offKritz, 0);
    return *m_nUber;
}

void VFuncs::SetKritz( CBasePlayer *pThisPtr, unsigned short nUber )
{
    DECLARE_VAR(unsigned short, m_nUber, pThisPtr, g_offKritz);
    if( (unsigned short)*m_nUber != nUber )
    {
        *m_nUber = nUber;
    }
}

bool VFuncs::GetRageDraining( CBasePlayer *pThisPtr )
{
    DECLARE_VAR_RVAL(bool, m_bRageDraining, pThisPtr, g_offRageDraining, 0);
    return *m_bRageDraining;
}

void VFuncs::SetRageDraining( CBasePlayer *pThisPtr, bool bRageDraining)
{
    DECLARE_VAR(bool, m_bRageDraining, pThisPtr, g_offRageDraining);
    if(*m_bRageDraining != bRageDraining)
    {
        *m_bRageDraining = bRageDraining;
    }
}

float VFuncs::GetChargeLevel( CBaseCombatWeapon *pThisPtr )
{
    DECLARE_VAR_RVAL(float, m_flChargeLevel, pThisPtr, g_offMedigunChargeLevel, 0);
    return *m_flChargeLevel;
}

void VFuncs::SetChargeLevel( CBaseCombatWeapon *pThisPtr, float flChargeLevel )
{
    DECLARE_VAR(float, m_flChargeLevel, pThisPtr, g_offMedigunChargeLevel);
    if( (float)*m_flChargeLevel != flChargeLevel )
    {
        *m_flChargeLevel = flChargeLevel;
    }
}

CHandle<CBaseEntity> VFuncs::GetOwnerEntity( CBaseEntity *pThisPtr )
{
    DECLARE_VAR_RVAL(CHandle<CBaseEntity>, m_hOwnerEntity, pThisPtr, g_offOwnerEntity, 0);
    return *m_hOwnerEntity;
}

void VFuncs::SetOwnerEntity( CBaseEntity *pThisPtr, CBaseEntity *pOwner )
{
    DECLARE_VAR(CHandle<CBaseEntity>, m_hOwnerEntity, pThisPtr, g_offOwnerEntity);
    *m_hOwnerEntity = pOwner;
}

CHandle<CBaseCombatWeapon> VFuncs::GetActiveWeapon( CBasePlayer *pThisPtr )
{
    DECLARE_VAR_RVAL(CHandle<CBaseCombatWeapon>, m_hActiveWeapon, pThisPtr, g_offActiveWeapon, 0);
    return *m_hActiveWeapon;
}

float VFuncs::GetNextAttack( CBasePlayer *pThisPtr )
{
    DECLARE_VAR_RVAL(float, m_flNextAttack, pThisPtr, g_offNextAttack, 0);
    return *m_flNextAttack;
}

void VFuncs::SetNextAttack( CBasePlayer *pThisPtr, float flNextAttack )
{
    DECLARE_VAR(float, m_flNextAttack, pThisPtr, g_offNextAttack);
    if( (float)*m_flNextAttack != flNextAttack )
    {
        *m_flNextAttack = flNextAttack;
    }
}

RenderFx_t VFuncs::GetRenderFX( CBaseEntity *pThisPtr )
{
    DECLARE_VAR_RVAL(unsigned char, m_nRenderFX, pThisPtr, g_offRenderFX, kRenderFxNone);
    return (RenderFx_t)*m_nRenderFX;
}

void VFuncs::SetRenderFX( CBaseEntity *pThisPtr, unsigned char fx )
{
    DECLARE_VAR(unsigned char, m_nRenderFX, pThisPtr, g_offRenderFX);
    if( *m_nRenderFX != fx )
    {
        *m_nRenderFX = fx;
    }

    SetEdictStateChanged(servergameents->BaseEntityToEdict(pThisPtr), g_offRenderFX);
}

RenderMode_t VFuncs::GetRenderMode( CBaseEntity *pThisPtr )
{
    DECLARE_VAR_RVAL(unsigned char, m_nRenderMode, pThisPtr, g_offRenderMode, kRenderNormal);
    return (RenderMode_t)*m_nRenderMode;
}

void VFuncs::SetRenderMode( CBaseEntity *pThisPtr, unsigned char mode )
{
    DECLARE_VAR(unsigned char, m_nRenderMode, pThisPtr, g_offRenderMode);
    if( *m_nRenderMode != mode )
    {
        *m_nRenderMode = mode;
    }

    SetEdictStateChanged(servergameents->BaseEntityToEdict(pThisPtr), g_offRenderMode);
}

void VFuncs::SetEffects( CBaseEntity *pThisPtr, int nEffects )
{
    DECLARE_VAR(int, m_fEffects, pThisPtr, g_offEffects);
    if( nEffects != *m_fEffects )
    {
        *m_fEffects = nEffects;
        pThisPtr->DispatchUpdateTransmitState();
    }
}

void VFuncs::AddEffects( CBaseEntity *pThisPtr, int nEffects )
{
    DECLARE_VAR(int, m_fEffects, pThisPtr, g_offEffects);
    *m_fEffects |= nEffects;

    if( nEffects & EF_NODRAW )
    {
        pThisPtr->DispatchUpdateTransmitState();
    }
}

void VFuncs::RemoveEffects( CBaseEntity *pThisPtr, int nEffects )
{
    DECLARE_VAR(int, m_fEffects, pThisPtr, g_offEffects);
    *m_fEffects &= ~nEffects;

    if( nEffects & EF_NODRAW )
    {
        pThisPtr->DispatchUpdateTransmitState();
    }
}

color32 VFuncs::GetRenderColor( CBaseEntity *pThisPtr )
{
    color32 defaultret;
    defaultret.r = defaultret.g = defaultret.b = defaultret.a = 0;
    DECLARE_VAR_RVAL(color32, m_clrRender, pThisPtr, g_offRender, defaultret);
    return *m_clrRender;
}

void VFuncs::SetRenderColor( CBaseEntity *pThisPtr, byte r, byte g, byte b )
{
    DECLARE_VAR(color32, m_clrRender, pThisPtr, g_offRender);
    m_clrRender->r = r;
    m_clrRender->g = g;
    m_clrRender->b = b;

    SetEdictStateChanged(servergameents->BaseEntityToEdict(pThisPtr), g_offRender);
}

void VFuncs::SetRenderColor( CBaseEntity *pThisPtr, byte r, byte g, byte b, byte a )
{
    DECLARE_VAR(color32, m_clrRender, pThisPtr, g_offRender);
    m_clrRender->r = r;
    m_clrRender->g = g;
    m_clrRender->b = b;
    m_clrRender->a = a;

    SetEdictStateChanged(servergameents->BaseEntityToEdict(pThisPtr), g_offRender);
}

void VFuncs::SetRenderColorR( CBaseEntity *pThisPtr, byte r )
{
    DECLARE_VAR(color32, m_clrRender, pThisPtr, g_offRender);
    m_clrRender->r = r;

    SetEdictStateChanged(servergameents->BaseEntityToEdict(pThisPtr), g_offRender);
}

void VFuncs::SetRenderColorG( CBaseEntity *pThisPtr, byte g )
{
    DECLARE_VAR(color32, m_clrRender, pThisPtr, g_offRender);
    m_clrRender->g = g;

    SetEdictStateChanged(servergameents->BaseEntityToEdict(pThisPtr), g_offRender);
}

void VFuncs::SetRenderColorB( CBaseEntity *pThisPtr, byte b )
{
    DECLARE_VAR(color32, m_clrRender, pThisPtr, g_offRender);
    m_clrRender->b = b;

    SetEdictStateChanged(servergameents->BaseEntityToEdict(pThisPtr), g_offRender);
}

void VFuncs::SetRenderColorA( CBaseEntity *pThisPtr, byte a )
{
    DECLARE_VAR(color32, m_clrRender, pThisPtr, g_offRender);
    m_clrRender->a = a;

    SetEdictStateChanged(servergameents->BaseEntityToEdict(pThisPtr), g_offRender);
}

CBaseEntity *VFuncs::GetThrower( CBaseEntity *pThisPtr )
{
    DECLARE_VAR_RVAL(EHANDLE, m_hThrower, pThisPtr, g_offThrower, NULL);
    return m_hThrower->Get();
}

void VFuncs::SetThrower( CBaseEntity *pThisPtr, CBaseEntity *pThrower )
{
    DECLARE_VAR(EHANDLE, m_hThrower, pThisPtr, g_offThrower);
    m_hThrower->Set(pThrower);
}

void VFuncs::SetSolid( CBaseEntity *pThisPtr, SolidType_t val )
{
    CCollisionProperty *pCollProp = VFuncs::CollisionProp(pThisPtr);
    if(pCollProp)
        VFuncs::CCollProp_SetSolid( pCollProp, val );
}

void VFuncs::CCollProp_SetSolid( CCollisionProperty *pThisPtr, SolidType_t val )
{
    if(g_useSolidTypeInt)
    {
        DECLARE_VAR(SolidType_t, m_nSolidType, pThisPtr, g_offSolidType);
        if ( *m_nSolidType == val )
            return;
        *m_nSolidType = val;
    }
    else
    {
        DECLARE_VAR(unsigned char, m_nSolidType, pThisPtr, g_offSolidType);
        if ( *m_nSolidType == val )
            return;

#ifndef CLIENT_DLL
        //bool bWasNotSolid = IsSolid();
#endif
/*
        MarkSurroundingBoundsDirty();

        // OBB is not yet implemented
        if ( val == SOLID_BSP )
        {
            if ( GetOuter()->GetMoveParent() )
            {
                if ( GetOuter()->GetRootMoveParent()->GetSolid() != SOLID_BSP )
                {
                    // must be SOLID_VPHYSICS because parent might rotate
                    val = SOLID_VPHYSICS;
                }
            }
#ifndef CLIENT_DLL
            // UNDONE: This should be fine in the client DLL too.  Move GetAllChildren() into shared code.
            // If the root of the hierarchy is SOLID_BSP, then assume that the designer
            // wants the collisions to rotate with this hierarchy so that the player can
            // move while riding the hierarchy.
            if ( !GetOuter()->GetMoveParent() )
            {
                // NOTE: This assumes things don't change back from SOLID_BSP
                // NOTE: This is 100% true for HL2 - need to support removing the flag to support changing from SOLID_BSP
                CUtlVector<CBaseEntity *> list;
                GetAllChildren( GetOuter(), list );
                for ( int i = list.Count()-1; i>=0; --i )
                {
                    VFuncs::AddSolidFlags( list[i], FSOLID_ROOT_PARENT_ALIGNED );
                }
            }
#endif
        }
*/
        *m_nSolidType = val;
/*
#ifndef CLIENT_DLL
        m_pOuter->CollisionRulesChanged();

        UpdateServerPartitionMask( );

        if ( bWasNotSolid != IsSolid() )
        {
            CheckForUntouch();
        }
#endif
*/
    }
}

void VFuncs::ClearSolidFlags( CBaseEntity *pThisPtr )
{
    CCollisionProperty *pCollProp = VFuncs::CollisionProp(pThisPtr);
    if(pCollProp)
        VFuncs::CCollProp_SetSolidFlags( pCollProp, 0 );
}

void VFuncs::RemoveSolidFlags( CBaseEntity *pThisPtr, int flags )
{
    CCollisionProperty *pCollProp = VFuncs::CollisionProp(pThisPtr);
    if(pCollProp)
        VFuncs::CCollProp_SetSolidFlags( pCollProp, VFuncs::GetSolidFlags(pThisPtr) & ~flags );
}

void VFuncs::AddSolidFlags( CBaseEntity *pThisPtr, int flags )
{
    CCollisionProperty *pCollProp = VFuncs::CollisionProp(pThisPtr);
    if(pCollProp)
        VFuncs::CCollProp_SetSolidFlags( pCollProp, VFuncs::GetSolidFlags(pThisPtr) | flags );
}

int VFuncs::GetSolidFlags( CBaseEntity *pThisPtr )
{
    CCollisionProperty *pCollProp = VFuncs::CollisionProp(pThisPtr);
    if(pCollProp)
        return VFuncs::CCollProp_GetSolidFlags( pCollProp );
    return 0;
}

void VFuncs::SetSolidFlags( CBaseEntity *pThisPtr, int flags )
{
    CCollisionProperty *pCollProp = VFuncs::CollisionProp(pThisPtr);
    if(pCollProp)
        VFuncs::CCollProp_SetSolidFlags( pCollProp, flags );
}

string_t VFuncs::GetTarget( CBaseEntity *pThisPtr )
{
    DECLARE_VAR_RVAL(string_t, m_target, pThisPtr, g_offTarget, NULL_STRING);
    return *m_target;
}

string_t VFuncs::GetEntityName( CBaseEntity *pThisPtr )
{
    DECLARE_VAR_RVAL(string_t, m_iName, pThisPtr, g_offName, NULL_STRING);
    return *m_iName;
}

void VFuncs::SetName( CBaseEntity *pThisPtr, string_t newName)
{
    DECLARE_VAR(string_t, m_iName, pThisPtr, g_offName);
    *m_iName = newName;
}

int VFuncs::GetWaterLevel( CBaseEntity *pThisPtr )
{
    DECLARE_VAR_RVAL(int, m_nWaterLevel, pThisPtr, g_offWaterLevel, 0);
    return *m_nWaterLevel;
}

QAngle VFuncs::GetAbsAngles( CBaseEntity *pThisPtr )
{
    DECLARE_VAR_RVAL(QAngle, m_angAbsRotation, pThisPtr, g_offAbsRotation, VFuncs::GetLocalAngles(pThisPtr));
    return *m_angAbsRotation;
}

int VFuncs::GetImpulse( CBasePlayer *pThisPtr )
{
    DECLARE_VAR_RVAL(int, m_nImpulse, pThisPtr, g_offImpulse, 0);
    return *m_nImpulse;
}

void VFuncs::SetImpulse( CBasePlayer *pThisPtr, int impulse )
{
    DECLARE_VAR(int, m_nImpulse, pThisPtr, g_offImpulse);
    *m_nImpulse = impulse;
}

int VFuncs::GetButtons( CBasePlayer *pThisPtr )
{
    DECLARE_VAR_RVAL(int, m_nButtons, pThisPtr, g_offButtons, 0);
    return *m_nButtons;
}

void VFuncs::SetCombineBallRadius( CBaseEntity *pThisPtr, float flRadius )
{
    DECLARE_VAR(float, m_flRadius, pThisPtr, g_offCombineBallRadius);
    *m_flRadius = flRadius;
}

int VFuncs::CCollProp_GetSolidFlags( CCollisionProperty *pThisPtr )
{
    DECLARE_VAR_RVAL(int, m_usSolidFlags, pThisPtr, g_offSolidFlags, 0);
    return *m_usSolidFlags;
}

void VFuncs::CCollProp_SetSolidFlags( CCollisionProperty *pThisPtr, int flags )
{
    DECLARE_VAR(unsigned short, m_usSolidFlags, pThisPtr, g_offSolidFlags);
    int oldFlags = *m_usSolidFlags;
    *m_usSolidFlags = (unsigned short)(flags & 0xFFFF);
/*  if ( oldFlags == m_usSolidFlags )
        return;
    // These two flags, if changed, can produce different surrounding bounds
    if ( (oldFlags & (FSOLID_FORCE_WORLD_ALIGNED | FSOLID_USE_TRIGGER_BOUNDS)) != 
         (m_usSolidFlags & (FSOLID_FORCE_WORLD_ALIGNED | FSOLID_USE_TRIGGER_BOUNDS)) )
    {
        MarkSurroundingBoundsDirty();
    }

    if ( (oldFlags & FSOLID_NOT_SOLID) != (m_usSolidFlags & FSOLID_NOT_SOLID) )
    {
        m_pOuter->CollisionRulesChanged();
    }

#ifndef CLIENT_DLL
    if ( (oldFlags & (FSOLID_NOT_SOLID | FSOLID_TRIGGER)) != (m_usSolidFlags & (FSOLID_NOT_SOLID | FSOLID_TRIGGER)) )
    {
        UpdateServerPartitionMask( );
        CheckForUntouch();
    }
#endif
*/
}

void VFuncs::SetCollisionBounds( CBaseEntity *pThisPtr, const Vector& mins, const Vector &maxs )
{
    CCollisionProperty *pCollProp = VFuncs::CollisionProp(pThisPtr);
    if(pCollProp)
        VFuncs::CCollProp_SetCollisionBounds( pCollProp, mins, maxs );
}

void VFuncs::CCollProp_SetCollisionBounds( CCollisionProperty *pThisPtr, const Vector& mins, const Vector &maxs )
{
    DECLARE_VAR(Vector, m_vecMins, pThisPtr, g_offVecMins);
    DECLARE_VAR(Vector, m_vecMaxs, pThisPtr, g_offVecMaxs);
    DECLARE_VAR(float, m_flRadius, pThisPtr, g_offVecMaxs+12);
    if ( (*m_vecMins == mins) && (*m_vecMaxs == maxs) )
        return;
    
    *m_vecMins = mins;
    *m_vecMaxs = maxs;
    
    //ASSERT_COORD( mins );
    //ASSERT_COORD( maxs );

    Vector vecSize;
    VectorSubtract( maxs, mins, vecSize );
    *m_flRadius = vecSize.Length() * 0.5f;

    //MarkSurroundingBoundsDirty();
}

float VFuncs::CCollProp_BoundingRadius( CCollisionProperty *pThisPtr )
{
    DECLARE_VAR_RVAL(float, m_flRadius, pThisPtr, g_offVecMaxs+12, 0);

    return *m_flRadius;
}

Vector VFuncs::CCollProp_OBBCenter( CCollisionProperty *pThisPtr )
{
    DECLARE_VAR_RVAL(Vector, m_vecMins, pThisPtr, g_offVecMins, Vector(0,0,0));
    DECLARE_VAR_RVAL(Vector, m_vecMaxs, pThisPtr, g_offVecMaxs, Vector(0,0,0));

    Vector vecResult;
    VectorLerp( *m_vecMins, *m_vecMaxs, 0.5f, vecResult );

    return vecResult;
}

Vector VFuncs::CCollProp_GetMins( CCollisionProperty *pThisPtr )
{
    DECLARE_VAR_RVAL(Vector, m_vecMins, pThisPtr, g_offVecMins, Vector(0,0,0));
    return *m_vecMins;
}

Vector VFuncs::CCollProp_GetMaxs( CCollisionProperty *pThisPtr )
{
    DECLARE_VAR_RVAL(Vector, m_vecMaxs, pThisPtr, g_offVecMaxs, Vector(0,0,0));
    return *m_vecMaxs;
}

FnCommandCallback_t VFuncs::GetCommandCallback( ConCommand *pThisPtr )
{
    FnCommandCallback_t m_fnCommandCallback;
    if(!g_offFnCommandCallback) return NULL;
    m_fnCommandCallback = (FnCommandCallback_t)*(unsigned int*)(((unsigned char*)pThisPtr)+(g_offFnCommandCallback));
    return m_fnCommandCallback;
}

CEntityFactoryDictionary *VFuncs::GetEntityDictionary( ConCommand *pDumpEntityFactoriesCmd )
{
#ifdef WIN32
    char *pEntityFactoriesCallback = (char*)VFuncs::GetCommandCallback(pDumpEntityFactoriesCmd);
    return ( CEntityFactoryDictionary * )*(int *)( (pEntityFactoriesCallback) + g_offEntityFactoryDictionary );
#else
    char gamedir[256];
    char filepath[512];
    void *handle;

    engine->GetGameDir(gamedir, sizeof(gamedir));
    Q_snprintf(filepath, sizeof(filepath), "%s/bin/server_srv.so", gamedir);

    handle = dlopen(filepath, RTLD_LAZY | RTLD_NOLOAD);
    if(handle)
    {
        typedef IEntityFactoryDictionary* (__cdecl* EntFactDictFunc)( void );
        EntFactDictFunc EntFactDict;

        Msg(">");
        EntFactDict = (EntFactDictFunc)ResolveSymbol(handle, "_Z23EntityFactoryDictionaryv");
        CEntityFactoryDictionary *dict = ( CEntityFactoryDictionary * )EntFactDict();
        dlclose(handle);
        return dict;
    }
    else
    {
        Msg("..fail\n");
        return NULL;
    }
#endif
}

void VFuncs::QuadBeamSetTargetPos( CBaseEntity *pThisPtr, const Vector& pos )
{
    if(!g_foundBeamFuncs) return;
    DECLARE_VAR(Vector, m_targetPosition, pThisPtr, g_offBeamTargetPosition);
    if (*m_targetPosition != pos)
    {
        *m_targetPosition = pos;
    }
}

void VFuncs::QuadBeamSetControlPos( CBaseEntity *pThisPtr, const Vector& control )
{
    if(!g_foundBeamFuncs) return;
    DECLARE_VAR(Vector, m_controlPosition, pThisPtr, g_offBeamControlPosition);
    if (*m_controlPosition != control)
    {
        *m_controlPosition = control;
    }
}

void VFuncs::QuadBeamSetScrollRate( CBaseEntity *pThisPtr, float scrollRate )
{
    if(!g_foundBeamFuncs) return;
    DECLARE_VAR(float, m_scrollRate, pThisPtr, g_offBeamScrollRate);
    *m_scrollRate = scrollRate;
}

void VFuncs::QuadBeamSetWidth( CBaseEntity *pThisPtr, float width )
{
    if(!g_foundBeamFuncs) return;
    DECLARE_VAR(float, m_flWidth, pThisPtr, g_offBeamWidth);
    *m_flWidth = width;
}

void VFuncs::SpriteSetAttachment( CBaseEntity *pThisPtr, CBaseEntity *pEntity, int attachment )
{
    DECLARE_VAR(EHANDLE, m_hAttachedToEntity, pThisPtr, g_offSpriteAttachedEntity);
    DECLARE_VAR(int, m_nAttachment, pThisPtr, g_offSpriteAttachment);

    m_hAttachedToEntity->Set(pEntity);
    *m_nAttachment = attachment;
    VFuncs::FollowEntity(pThisPtr, pEntity);
}

int VFuncs::GetTotalScore( CBaseEntity *pPlayerResource, int player )
{
    DECLARE_VAR_RVAL(int, m_iTotalScore, pPlayerResource, g_offTotalScore, 0);

    return m_iTotalScore[player];
}

void VFuncs::SetEdictStateChanged(edict_t *pEdict, unsigned short offset)
{
    if (g_pSharedChangeInfo != NULL)
    {
        if (offset)
        {
            pEdict->StateChanged(offset);
        }
        else
        {
            pEdict->StateChanged();
        }
    }
    else
    {
        pEdict->m_fStateFlags |= FL_EDICT_CHANGED;
    }
}
