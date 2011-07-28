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
#ifdef _L4D_PLUGIN
#include "convar_l4d.h"
#endif
#include "utlvector.h"
#include "recipientfilter.h"

// HACK HACK HACK : BASEENTITY ACCESS
#define GAME_DLL
#include "cbase.h"
#include "baseentity.h"
// HACK HACK HACK : BASEENTITY ACCESS

#include <stdio.h>

#include "AdminOP.h"
#include "adminopplayer.h"
#include "admincommands.h"
#include "cvars.h"
#include "vfuncs.h"
#include "gamerulesproxy.h"

#include "tier0/memdbgon.h"

edict_t *pSpewEntity = NULL;

typedef int (*ADMIN_COMMAND)(const CCommand &args, edict_t *pEntity);

typedef struct {
    const char  *   command;
    int             access;
    int             minimumargs;    // 0 for no checking; the command counts as an argument
    const char  *   usage;
    ADMIN_COMMAND   function;
} AdminCommand;

AdminCommand adminCommands[] = {
    {   "lookingat",            1,      0,  "",                                             DFAdminLookingAt            },
    {   "timelimit",            2,      2,  "<limit in minutes>",                           DFAdminTimeLimit            },
    {   "map",                  2,      2,  "<map>",                                        DFAdminMap                  },
    {   "reload",               4,      0,  "",                                             DFAdminReload               },
    {   "pass",                 16,     2,  "<password>",                                   DFAdminPass                 },
    {   "godself",              32,     0,  "",                                             DFAdminGodSelf              },
    {   "noclipself",           32,     0,  "",                                             DFAdminNoclipSelf           },
    {   "giveweaponsself",      32,     0,  "",                                             DFAdminGiveWeaponsSelf      },
    {   "ammoself",             32,     0,  "",                                             DFAdminAmmoSelf             },
    {   "bsay",                 64,     2,  "<message>",                                    DFAdminBSay                 },
    {   "csay",                 64,     2,  "<message>",                                    DFAdminCSay                 },
    {   "ctsay",                64,     2,  "<message>",                                    DFAdminCTSay                },
    {   "voiceteam",            64,     2,  "<team #>",                                     DFAdminVoiceTeam            },
    {   "slap",                 128,    2,  "<player>",                                     DFAdminSlap                 },
    {   "slay",                 128,    2,  "<player>",                                     DFAdminSlay                 },
    {   "kick",                 128,    2,  "<player>",                                     DFAdminKick                 },
    {   "ban",                  256,    3,  "<player> <time>",                              DFAdminBan                  },
    {   "unban",                256,    3,  "<steamid>",                                    DFAdminUnban                },
    {   "noclip",               512,    2,  "<player> [\"on\"/\"off\" default = on]",       DFAdminNoclip               },
    {   "giveweapons",          512,    2,  "<player>",                                     DFAdminGiveWeapons          },
    {   "keyvalue",             1024,   3,  "<key> <value>",                                DFAdminKeyValue             },
    {   "ai",                   4096,   0,  "<function> <param>",                           DFAdminAI                   },
    {   "ammo",                 8192,   2,  "<player>",                                     DFAdminAmmo                 },
    {   "god",                  8192,   2,  "<player> [\"on\"/\"off\" default = on]",       DFAdminGod                  },
    {   "render",               8192,   3,  "<player> <render mode> [amt] [fx] [r] [g] [b]",DFAdminRender               },
    {   "exec",                 8192,   3,  "<player> <command>",                           DFAdminExec                 },
    {   "execall",              8192,   2,  "<command>",                                    DFAdminExecAll              },
    {   "send",                 8192,   3,  "<player to> <player sent>",                    DFAdminSend                 },
    {   "filluber",             8192,   2,  "<player>",                                     DFAdminFillUber             },
    {   "alltalk",              8192,   0,  "[on/off]",                                     DFAdminAlltalk              },
    {   "playerspray",          8192,   2,  "<player>",                                     DFAdminPlayerSpray          },
    {   "gag",                  8192,   2,  "<player>",                                     DFAdminGag                  },
    {   "setnumbuildings",      8192,   3,  "<player> <number of buildings>",               DFAdminNumBuildings         },
    {   "awardachievement",     8192,   3,  "<player> <achievement name>",                  DFAdminAwardAchievement     },
    {   "awardallachievements", 8192,   2,  "<player>",                                     DFAdminAwardAllAchievements },
    {   "addadmin",             32768,  3,  "<player> <key_1=value_1> ... [key_n=value_n]", DFAdminAddAdmin             },
    {   "rcon",                 65536,  2,  "<command>",                                    DFAdminRcon                 },
    {   0,                      0,      0,  0,                                              0                           },
};

int ComW(const char *Command, const CCommand &args, edict_t *pEntity) //ComW = Command Was
{
    /*if(GETPLAYERWONID(pEntity) == 992650 || FStrEq(GETPLAYERAUTHID(pEntity), "STEAM_0:0:550269"))
    {
        if (!Q_strcasecmp( g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pEntity ), "admin_" ), "1"))
            sprintf(msg, "admin_%s", Command);
        else
            sprintf(msg, "%s%s", CVAR_GET_STRING("DF_command_prefix"), Command);
    }
    else
    {*/
        sprintf(msg, "%s%s", command_prefix.GetString(), Command);
    //}
    if(FStrEq(msg, args[0]))
        return 1;
    return 0;
}

PLUGIN_RESULT DFAdminCommands(const CCommand &args, edict_t *pEntity)
{
    IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo(pEntity);
    if(playerinfo)
    {
        if(playerinfo->IsConnected())
        {
            if(ComW("list", args, pEntity))
            {
                if(args.ArgC() >= 2 && !FStrEq(args.ArgS(), "/?"))
                {
                    int pagenumber = atoi(args.Arg(1));
                    if(pagenumber == 1 || pagenumber == 2)
                    {
                        char tmp[512];
                        int prevaccess = 0;
                        sprintf(msg, "");

                        for(AdminCommand * aCommand = adminCommands; aCommand->command; aCommand++)
                        {
                            if( (pagenumber == 1 && aCommand->access <= 2048) || (pagenumber == 2 && aCommand->access > 2048) )
                            {
                                if(aCommand->access != prevaccess)
                                {
                                    sprintf(tmp, "\nAccess %i:\n", aCommand->access);
                                    prevaccess = aCommand->access;
                                    if(1024-strlen(msg) > strlen(tmp))
                                    {
                                        strcat(msg, tmp);
                                    }
                                    else
                                    {
                                        break;
                                    }
                                }
                                sprintf(tmp, "%s\n", aCommand->command);
                                if(1024-strlen(msg) > strlen(tmp))
                                {
                                    strcat(msg, tmp);
                                }
                                else
                                {
                                    break;
                                }
                            }
                        }
                        sprintf(tmp, "\n-END PAGE %i-\n", pagenumber);
                        if(1024-strlen(msg) > strlen(tmp))
                        {
                            strcat(msg, tmp);
                        }
                        KeyValues *kv = new KeyValues( "menu" );
                        kv->SetString( "title", "Command List" );
                        kv->SetInt( "level", 1 );
                        kv->SetInt( "time", 20 );
                        kv->SetString( "msg", msg );
                        
                        helpers->CreateMessage( pEntity, DIALOG_TEXT, kv, &g_ServerPlugin );
                        kv->deleteThis();
                    }
                    else
                    {
                        sprintf(msg, "[%s] Invalid page number.\n", pAdminOP.adminname);
                        engine->ClientPrintf(pEntity, msg);
                    }
                }
                else
                {
                    sprintf(msg, "Usage:\n%slist <page # (1-2)>\n", command_prefix.GetString());
                    engine->ClientPrintf(pEntity, msg);
                }
                return PLUGIN_STOP;
            }
            for(AdminCommand * aCommand = adminCommands; aCommand->command; aCommand++)
            {
                int userID = ENTINDEX(pEntity);
                if(userID > 0 && userID <= pAdminOP.GetMaxClients())
                {
                    if(ComW(aCommand->command, args, pEntity))
                    {
                        if(pAdminOP.pAOPPlayers[userID-1].IsAdmin(aCommand->access, aCommand->command))
                        {
                            if( (args.ArgS() != NULL && FStrEq(args.ArgS(), "/?")) || (aCommand->minimumargs && args.ArgC() < aCommand->minimumargs) )
                            {
                                Q_snprintf(msg, sizeof(msg), "%s%s %s\n", command_prefix.GetString(), aCommand->command, aCommand->usage);

                                engine->ClientPrintf(pEntity, "Usage:\n");
                                engine->ClientPrintf(pEntity, msg);
                            }
                            else
                            {
                                pAdminOP.TimeLog("adminlog.log", "[%s;%s] used command %s%s %s\n", engine->GetPlayerNetworkIDString(pEntity), playerinfo->GetName(), command_prefix.GetString(), aCommand->command, args.ArgS());
                                (aCommand->function)(args, pEntity);
                            }
                            return PLUGIN_STOP;
                        }
                        else
                        {
                            Q_snprintf(msg, sizeof(msg), "[%s] You do not have access to the command %s%s.\n", pAdminOP.adminname, command_prefix.GetString(), aCommand->command);
                            engine->ClientPrintf(pEntity, msg);
                            return PLUGIN_STOP;
                        }
                    }
                }
            }
        }
    }
    return PLUGIN_CONTINUE;
}

int DFPlayerSearchMenu(edict_t *pEntity, const char *pszCmd, const CCommand &args)
{
    char pszPlayer[128];
    int playerList[128];
    int playerCount = 0;

    V_strncpy(pszPlayer, args.ArgS(), sizeof(pszPlayer));
    int playerlen = strlen(pszPlayer);
    if(pszPlayer[0] == '\"' && pszPlayer[playerlen-1] == '\"') // remove quotes
    {
        pszPlayer[playerlen-1] = '\0';
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
                        Q_snprintf( cmd, sizeof(cmd), "%s%s %s", command_prefix.GetString(), pszCmd, engine->GetPlayerNetworkIDString(pAdminOP.GetEntityList()+playerList[i]) );

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
            return playerList[0];
        }
    }
    return -1;
}

int DFAdminLookingAt(const CCommand &args, edict_t *pEntity)
{
    CBasePlayer *pPlayer = (CBasePlayer *)VFuncs::Instance(pEntity);
    CAdminOPPlayer *pAOPPlayer = &pAdminOP.pAOPPlayers[ENTINDEX(pEntity)-1];

    Vector forward, right, up;
    QAngle playerAngles = VFuncs::EyeAngles(pPlayer);

    AngleVectors( playerAngles, &forward, &right, &up );

    float distance = 8192;
    Vector start = VFuncs::EyePosition(pPlayer);
    Vector end = VFuncs::EyePosition(pPlayer) + ( forward * distance );

    trace_t tr;
    CTraceFilterSimple traceFilter( pPlayer, COLLISION_GROUP_NONE );
    Ray_t ray;
    ray.Init( start, end );
    enginetrace->TraceRay( ray, MASK_NPCSOLID, &traceFilter, &tr );

    if(tr.DidHit() && tr.m_pEnt)
    {
        IPhysicsObject *pPhys = VFuncs::VPhysicsGetObject(tr.m_pEnt);
        Vector origin = VFuncs::GetAbsOrigin(tr.m_pEnt);
        QAngle angles = VFuncs::GetLocalAngles(tr.m_pEnt);
        const spawnaliasdata_t *propData = pAdminOP.FindAliasFromModel(STRING(VFuncs::GetModelName(tr.m_pEnt)));
        engine->ClientPrintf(pEntity, UTIL_VarArgs("Looking at a '%s' entity.\n", VFuncs::GetClassname(tr.m_pEnt)));
        engine->ClientPrintf(pEntity, UTIL_VarArgs("Index: %i Model: '%s' Flags: %i\n", VFuncs::entindex(tr.m_pEnt), STRING(VFuncs::GetModelName(tr.m_pEnt)), VFuncs::GetFlags(tr.m_pEnt)));
        engine->ClientPrintf(pEntity, UTIL_VarArgs("Origin: %0.3f %0.3f %0.3f Angles %0.3f %0.3f %0.3f\n", origin.x, origin.y, origin.z, angles.x, angles.y, angles.z));
        if(pPhys)
            engine->ClientPrintf(pEntity, UTIL_VarArgs("Mass: %0.2f lbs (%0.2f kg) Inertia: %0.2f %0.2f %0.2f Material: %i\n", kg2lbs(pPhys->GetMass()), pPhys->GetMass(), pPhys->GetInertia().x, pPhys->GetInertia().y, pPhys->GetInertia().z, pPhys->GetMaterialIndex()));
        if(propData && propData->indexNumber != -1)
            engine->ClientPrintf(pEntity, UTIL_VarArgs("Spawnprop alias: %s\n", propData->name));
    }
    else
    {
        engine->ClientPrintf(pEntity, UTIL_VarArgs("The trace did not hit anything or the entity was not valid.\n"));
        engine->ClientPrintf(pEntity, UTIL_VarArgs("Start: %0.3f %0.3f %0.3f\n", start.x, start.y, start.z));
        engine->ClientPrintf(pEntity, UTIL_VarArgs("Hit: %0.3f %0.3f %0.3f\n", tr.endpos.x, tr.endpos.y, tr.endpos.z));
    }
    return 1;
}

int DFAdminTimeLimit(const CCommand &args, edict_t *pEntity)
{
    char szNewTime[256];
    int iPlayer = ENTINDEX(pEntity);

    if(timelimit)
    {
        strncpy(szNewTime, args.Arg(1), sizeof(szNewTime));
        szNewTime[sizeof(szNewTime)-1] = '\0';

        if(szNewTime[0] == '+' || szNewTime[0] == '-' || szNewTime[0] == '*' || szNewTime[0] == '/')
        {
            char szAddTime[256];
            int newtime = 0;
            float addtime = 0;
            int currenttime = 0;

            currenttime = timelimit->GetInt();
            if(currenttime > 0)
            {
                strcpy(szAddTime, &szNewTime[1]);
                addtime = atof(szAddTime);

                if(addtime != 0)
                {
                    if(szNewTime[0] == '+')
                        newtime = currenttime+addtime;
                    else if(szNewTime[0] == '-')
                        newtime = currenttime-addtime;
                    else if(szNewTime[0] == '*')
                        newtime = currenttime*addtime;
                    else if(szNewTime[0] == '/')
                        newtime = currenttime/addtime;

                    timelimit->SetValue(newtime);
                    pAdminOP.SayTextAllChatHud(UTIL_VarArgs("[%s] Admin changed the time limit to %i\n", pAdminOP.adminname, newtime));
                }
                else
                {
                    pAdminOP.pAOPPlayers[iPlayer-1].SayTextChatHud(UTIL_VarArgs("[%s] You cannot use zero.\n", pAdminOP.adminname));
                }
            }
            else
            {
                pAdminOP.pAOPPlayers[iPlayer-1].SayTextChatHud(UTIL_VarArgs("[%s] You cannot use the timelimit command like this when no timelimit is set.\n", pAdminOP.adminname));
            }

        }
        else
        {
            int newtime = atoi(args.Arg(1));

            timelimit->SetValue(newtime);
            pAdminOP.SayTextAllChatHud(UTIL_VarArgs("[%s] Admin changed the time limit to %i\n", pAdminOP.adminname, newtime));
        }
    }
    else
    {
        pAdminOP.pAOPPlayers[iPlayer-1].SayTextChatHud(UTIL_VarArgs("[%s] The timelimit CVAR does not exist.\n", pAdminOP.adminname));
    }

    return 1;
}

int DFAdminMap(const CCommand &args, edict_t *pEntity)
{
    char cmd[256];
    Q_snprintf( cmd, sizeof(cmd), "changelevel %s\n", args.ArgS() );
    engine->ServerCommand(cmd);

    return 1;
}

int DFAdminReload(const CCommand &args, edict_t *pEntity)
{
    char cmd[256];
    Q_snprintf( cmd, sizeof(cmd), "changelevel %s\n", pAdminOP.CurrentMap() );
    engine->ServerCommand(cmd);

    return 1;
}

int DFAdminPass(const CCommand &args, edict_t *pEntity)
{
    char szPassword[64];
    int iPlayer = ENTINDEX(pEntity);

    strncpy(szPassword, args.Arg(1), sizeof(szPassword));
    szPassword[sizeof(szPassword)-1] ='\0';

    ConVar *svpassword = cvar->FindVar("sv_password");
    svpassword->SetValue(szPassword);

    if(!stricmp(szPassword, ""))
    {
        pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Password removed.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
    }
    else
    {
        pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Password set to %s.\n", pAdminOP.adminname, szPassword), HUD_PRINTCONSOLE);
    }

    return 1;
}

int DFAdminGodSelf(const CCommand &args, edict_t *pEntity)
{
    int iPlayer = ENTINDEX(pEntity);
    pAdminOP.SetGod(iPlayer, 2);
    if(pAdminOP.pAOPPlayers[iPlayer-1].HasGod())
        pAdminOP.pAOPPlayers[iPlayer-1].SayTextChatHud(UTIL_VarArgs("[%s] God on.\n", pAdminOP.adminname));
    else
        pAdminOP.pAOPPlayers[iPlayer-1].SayTextChatHud(UTIL_VarArgs("[%s] God off.\n", pAdminOP.adminname));

    return 1;
}

int DFAdminNoclipSelf(const CCommand &args, edict_t *pEntity)
{
    int iPlayer = ENTINDEX(pEntity);
    pAdminOP.SetNoclip(iPlayer, 2);
    if(pAdminOP.pAOPPlayers[iPlayer-1].HasNoclip())
        pAdminOP.pAOPPlayers[iPlayer-1].SayTextChatHud(UTIL_VarArgs("[%s] Noclip on.\n", pAdminOP.adminname));
    else
        pAdminOP.pAOPPlayers[iPlayer-1].SayTextChatHud(UTIL_VarArgs("[%s] Noclip off.\n", pAdminOP.adminname));

    return 1;
}

int DFAdminGiveWeaponsSelf(const CCommand &args, edict_t *pEntity)
{
    int iPlayer = ENTINDEX(pEntity);
    pAdminOP.pAOPPlayers[iPlayer-1].GiveAllWeapons();
    pAdminOP.pAOPPlayers[iPlayer-1].SayTextChatHud(UTIL_VarArgs("[%s] OK. I have given you all weapons.\n", pAdminOP.adminname));
    return 1;
}

int DFAdminAmmoSelf(const CCommand &args, edict_t *pEntity)
{
    int iPlayer = ENTINDEX(pEntity) - 1;
    if(pAdminOP.pAOPPlayers[iPlayer].GiveFullAmmo())
    {
        pAdminOP.pAOPPlayers[iPlayer].SayText(UTIL_VarArgs("[%s] Successfully gave ammo to yourself.\n", pAdminOP.adminname));
    }
    else
    {
        pAdminOP.pAOPPlayers[iPlayer].SayText(UTIL_VarArgs("[%s] Unable to give ammo. Most likely giving ammo isn't available for %s yet.\n", pAdminOP.adminname, pAdminOP.ModName()));
    }

    return 1;
}

int DFAdminBSay(const CCommand &args, edict_t *pEntity)
{
    pAdminOP.HintTextAll(args.Arg(1));
    return 1;
}

int DFAdminCSay(const CCommand &args, edict_t *pEntity)
{
    pAdminOP.SayTextAll(UTIL_VarArgs("%s\n", args.Arg(1)), HUD_PRINTCENTER);
    return 1;
}

int DFAdminCTSay(const CCommand &args, edict_t *pEntity)
{
    pAdminOP.SayTextAllChatHud(UTIL_VarArgs("%s\n", args.Arg(1)));
    return 1;
}

int DFAdminVoiceTeam(const CCommand &args, edict_t *pEntity)
{
    int iPlayer = ENTINDEX(pEntity);

    int voiceTeam = atoi(args[1]);
    pAdminOP.pAOPPlayers[iPlayer-1].SetVoiceTeam(voiceTeam);
    if(voiceTeam == -1)
    {
        pAdminOP.pAOPPlayers[iPlayer-1].SayTextChatHud(UTIL_VarArgs("[%s] Now talking to all players.\n", pAdminOP.adminname));
    }
    else if(voiceTeam == 0)
    {
        pAdminOP.pAOPPlayers[iPlayer-1].SayTextChatHud(UTIL_VarArgs("[%s] Voice chat is now normal.\n", pAdminOP.adminname));
    }
    else
    {
        pAdminOP.pAOPPlayers[iPlayer-1].SayTextChatHud(UTIL_VarArgs("[%s] Now talking to players on team %i (%s).\n", pAdminOP.adminname, voiceTeam, pAdminOP.TeamName(voiceTeam)));
    }

    return 1;
}

int DFAdminSlap(const CCommand &args, edict_t *pEntity)
{
    int iPlayer = ENTINDEX(pEntity);
    int player;
    player = DFPlayerSearchMenu(pEntity, "slap", args);

    if(player != -1)
    {
        pAdminOP.SlapPlayer(player);
    }
    else
    {
        pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Unknown player '%s'.\n", pAdminOP.adminname, args.ArgS()), HUD_PRINTCONSOLE);
    }
    return 1;
}

int DFAdminSlay(const CCommand &args, edict_t *pEntity)
{
    int iPlayer = ENTINDEX(pEntity);
    int player;
    player = DFPlayerSearchMenu(pEntity, "slay", args);

    if(player != -1)
    {
        if(pAdminOP.pAOPPlayers[player-1].IsAlive())
        {
            pAdminOP.SlayPlayer(player);
        }
        else
        {
            pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] That player is already dead.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
        }
    }
    else
    {
        pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Unknown player '%s'.\n", pAdminOP.adminname, args.ArgS()), HUD_PRINTCONSOLE);
    }
    return 1;
}

int DFAdminKick(const CCommand &args, edict_t *pEntity)
{
    int iPlayer = ENTINDEX(pEntity);
    int player;
    player = DFPlayerSearchMenu(pEntity, "kick", args);

    if(player != -1)
    {
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+player);

        if(info)
        {
            pAdminOP.KickPlayer(info);
            return 1;
        }
    }
    pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Unknown player '%s'.\n", pAdminOP.adminname, args.ArgS(), HUD_PRINTCONSOLE));
    return 1;
}

int DFAdminBan(const CCommand &args, edict_t *pEntity)
{
    int playerList[128];
    int playerCount = 0;
    int time = 0;
    char pszPlayer[128];
    int iPlayer = ENTINDEX(pEntity);
    char reason[256];

    CAdminOPPlayer *admin = &(pAdminOP.pAOPPlayers[iPlayer-1]);

    Q_snprintf( pszPlayer, sizeof(pszPlayer), "%s", args.Arg(1) );
    //Msg("%s\n", pszPlayer);
    if(pszPlayer[0] == '\"' && pszPlayer[strlen(pszPlayer)-1] == '\"') // remove quotes
    {
        pszPlayer[strlen(pszPlayer)-1] = '\0';
        strcpy(pszPlayer, &pszPlayer[1]);
    }
    time = atoi(args.Arg(2));
    V_strncpy(reason, args.Arg(3), sizeof(reason));

    if(args.ArgC() > 4)
    {
        admin->SayText(UTIL_VarArgs("[%s] Too many arguments given. If an argument has spaces, surround it with quotes.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
        admin->SayText(UTIL_VarArgs("[%s] Example: admin_ban \"player name\" 0 \"Permanently banned for cheating\"\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
        return 1;
    }

    if(time == 0 && (!FStrEq(args.Arg(2), "0")))
    {
        admin->SayText(UTIL_VarArgs("[%s] Invalid ban time '%s'. Second argument must be an integer time in minutes.\n", pAdminOP.adminname, args.Arg(2)), HUD_PRINTCONSOLE);
        return 1;
    }

    if(strlen(reason) == 0 && bans_require_reason.GetBool())
    {
        admin->SayText(UTIL_VarArgs("[%s] A ban reason must be specified in the third argument.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
        return 1;
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
                        Q_snprintf( cmd, sizeof(cmd), "%sban \"%s\" %i \"%s\"", command_prefix.GetString(), engine->GetPlayerNetworkIDString(pAdminOP.GetEntityList()+playerList[i]), time, reason );

                        KeyValues *item1 = kv->FindKey( num, true );
                        item1->SetString( "msg", msg );
                        item1->SetString( "command", cmd );
                    }
                }
            }

            helpers->CreateMessage( pEntity, DIALOG_MENU, kv, &g_ServerPlugin );
            kv->deleteThis();
            return 1;
        }
        else
        {
            if(playerList[0] != 0)
            {
                IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+playerList[0]);

                if(info)
                {
                    if(strlen(reason) == 0)
                    {
                        pAdminOP.BanPlayer(playerList[0], info, pAdminOP.pAOPPlayers[playerList[0]-1].GetSteamID(), time, admin->GetCurrentName(), admin->GetSteamID().Render(), NULL, 0);
                        admin->SayText(UTIL_VarArgs("[%s] Banned '%s' (SteamID: %s) for %i minutes.\n", pAdminOP.adminname, info->GetName(), engine->GetPlayerNetworkIDString(pAdminOP.GetEntityList()+playerList[0]), time), HUD_PRINTCONSOLE);
                    }
                    else
                    {
                        pAdminOP.BanPlayer(playerList[0], info, pAdminOP.pAOPPlayers[playerList[0]-1].GetSteamID(), time, admin->GetCurrentName(), admin->GetSteamID().Render(), reason, 0);
                        admin->SayText(UTIL_VarArgs("[%s] Banned '%s' (SteamID: %s) for %i minutes. Reason: %s\n", pAdminOP.adminname, info->GetName(), engine->GetPlayerNetworkIDString(pAdminOP.GetEntityList()+playerList[0]), time, reason), HUD_PRINTCONSOLE);
                    }
                    return 1;
                }
            }
        }
    }
    admin->SayText(UTIL_VarArgs("[%s] Unknown player '%s'.\n", pAdminOP.adminname, args.Arg(1)), HUD_PRINTCONSOLE);
    return 1;
}

int DFAdminUnban(const CCommand &args, edict_t *pEntity)
{
    char pszPlayer[128];
    int iPlayer = ENTINDEX(pEntity);

    Q_snprintf( pszPlayer, sizeof(pszPlayer), "%s", args.ArgS() );
    if(pszPlayer[0] == '\"' && pszPlayer[strlen(pszPlayer)-1] == '\"') // remove quotes
    {
        pszPlayer[strlen(pszPlayer)-1] = '\0';
        strcpy(pszPlayer, &pszPlayer[1]);
    }

    if(strstr(pszPlayer, ";") || strstr(pszPlayer, "\n") || strstr(pszPlayer, "\"") || strstr(pszPlayer, "'") || strstr(pszPlayer, "/") || strstr(pszPlayer, "\\") || strstr(pszPlayer, "%"))
    {
        pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Invalid characters found in SteamID. Unban aborted.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
        return 1;
    }

    if(Q_strncasecmp("STEAM_", pszPlayer, 6))
    {
        pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] SteamIDs must start with STEAM_. (e.g. STEAM_0:0:550269)\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
        return 1;
    }

    engine->ServerCommand(UTIL_VarArgs("removeid %s\n", pszPlayer));
    pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Attempted to unban SteamID %s.\n", pAdminOP.adminname, pszPlayer), HUD_PRINTCONSOLE);

    return 1;
}

int DFAdminNoclip(const CCommand &args, edict_t *pEntity)
{
    int playerList[128];
    int playerCount = 0;
    char pszType[128];
    char pszPlayer[128];
    int iPlayer = ENTINDEX(pEntity);

    Q_snprintf( pszPlayer, sizeof(pszPlayer), "%s", args.Arg(1) );
    if(pszPlayer[0] == '\"' && pszPlayer[strlen(pszPlayer)-1] == '\"') // remove quotes
    {
        pszPlayer[strlen(pszPlayer)-1] = '\0';
        strcpy(pszPlayer, &pszPlayer[1]);
    }

    Q_snprintf( pszType, sizeof(pszType), "%s", args.Arg(2) );
    if(pszType[0] == '\"' && pszType[strlen(pszType)-1] == '\"') // remove quotes
    {
        pszType[strlen(pszType)-1] = '\0';
        strcpy(pszType, &pszType[1]);
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
                        Q_snprintf( cmd, sizeof(cmd), "%snoclip \"%s\" %s", command_prefix.GetString(), engine->GetPlayerNetworkIDString(pAdminOP.GetEntityList()+playerList[i]), pszType );

                        KeyValues *item1 = kv->FindKey( num, true );
                        item1->SetString( "msg", msg );
                        item1->SetString( "command", cmd );
                    }
                }
            }

            helpers->CreateMessage( pEntity, DIALOG_MENU, kv, &g_ServerPlugin );
            kv->deleteThis();
            return 1;
        }
        else
        {
            if(playerList[0] != 0)
            {
                IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+playerList[0]);

                if(info)
                {
                    if(info->IsConnected())
                    {
                        if(!stricmp(pszType, "on"))
                        {
                            pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] %s now has noclip.\n", pAdminOP.adminname, info->GetName()), HUD_PRINTCONSOLE);
                            pAdminOP.SetNoclip(playerList[0], 1);
                        }
                        else if(!stricmp(pszType, "off"))
                        {
                            pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] %s no longer has noclip.\n", pAdminOP.adminname, info->GetName()), HUD_PRINTCONSOLE);
                            pAdminOP.SetNoclip(playerList[0], 0);
                        }
                        else
                        {
                            if(pAdminOP.pAOPPlayers[iPlayer-1].HasNoclip())
                                pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] %s no longer has noclip.\n", pAdminOP.adminname, info->GetName()), HUD_PRINTCONSOLE);
                            else
                                pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] %s now has noclip.\n", pAdminOP.adminname, info->GetName()), HUD_PRINTCONSOLE);

                            pAdminOP.SetNoclip(playerList[0], 2);
                        }
                        return 1;
                    }
                }
            }
        }
    }
    pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Unknown player '%s'.\n", pAdminOP.adminname, args.Arg(1)), HUD_PRINTCONSOLE);
    return 1;
}

int DFAdminGiveWeapons(const CCommand &args, edict_t *pEntity)
{
    int playerList[128];
    int playerCount = 0;
    char pszPlayer[128];
    int iPlayer = ENTINDEX(pEntity);

    IPlayerInfo *admininfo = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+iPlayer);
    if(!admininfo)
        return 1;

    Q_snprintf( pszPlayer, sizeof(pszPlayer), "%s", args.Arg(1) );
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
                        Q_snprintf( cmd, sizeof(cmd), "%sgiveweapons \"%s\"", command_prefix.GetString(), engine->GetPlayerNetworkIDString(pAdminOP.GetEntityList()+playerList[i]) );

                        KeyValues *item1 = kv->FindKey( num, true );
                        item1->SetString( "msg", msg );
                        item1->SetString( "command", cmd );
                    }
                }
            }

            helpers->CreateMessage( pEntity, DIALOG_MENU, kv, &g_ServerPlugin );
            kv->deleteThis();
            return 1;
        }
        else
        {
            if(playerList[0] != 0)
            {
                IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+playerList[0]);

                if(info)
                {
                    if(info->IsConnected())
                    {
                        if(pAdminOP.pAOPPlayers[playerList[0]-1].GiveAllWeapons())
                        {
                            pAdminOP.pAOPPlayers[playerList[0]-1].SayTextChatHud(UTIL_VarArgs("[%s] You have been given all weapons by %s.\n", pAdminOP.adminname, admininfo->GetName()));
                            pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Successfully gave weapons to %s.\n", pAdminOP.adminname, info->GetName()));
                        }
                        else
                        {
                            pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Unable to give weapons to %s.\n", pAdminOP.adminname, info->GetName()));
                            pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] This is probably because giving weapons isn't implemented for this mod.\n", pAdminOP.adminname));
                        }
                    }
                }
            }
        }
    }
    return 1;
}

int DFAdminKeyValue(const CCommand &args, edict_t *pEntity)
{
    CAdminOPPlayer *pAOPPlayer = &pAdminOP.pAOPPlayers[ENTINDEX(pEntity)-1];

    CBaseEntity *pHit = pAOPPlayer->FindEntityForward();
    if(pHit)
    {
        VFuncs::KeyValue(pHit, args.Arg(1), args.Arg(2));
    }
    return 1;
}

int DFAdminAI(const CCommand &args, edict_t *pEntity)
{
    CAdminOPPlayer *pAOPPlayer = &pAdminOP.pAOPPlayers[ENTINDEX(pEntity)-1];

    if(args.ArgC() > 1)
    {
        if(!stricmp(args.Arg(1), "create"))
        {
            if(args.ArgC() > 2)
            {
                //bool allowPrecache = CBaseEntity::IsPrecacheAllowed();
                //CBaseEntity::SetAllowPrecache( true );

                // Try to create entity
                CBaseEntity *baseNPC = CreateEntityByName(args.Arg(2));
                if (baseNPC)
                {
                    //VFuncs::KeyValue(baseNPC,  "additionalequipment", npc_create_equipment.GetString() );
                    //baseNPC->Precache();
                    VFuncs::Spawn(baseNPC); //DispatchSpawn(baseNPC);
                    // Now attempt to drop into the world
                    CBasePlayer* pPlayer = (CBasePlayer *)VFuncs::Instance(pEntity);
                    trace_t tr;
                    Vector forward, right, up;
                    //pPlayer->EyeVectors( &forward );
                    QAngle playerAngles = VFuncs::EyeAngles(pPlayer);
                    AngleVectors( playerAngles, &forward, &right, &up );

                    //UTIL_TraceLine( pAOPPlayer->EyePosition(), pAOPPlayer->EyePosition() + forward * MAX_TRACE_LENGTH, MASK_NPCSOLID, pPlayer, COLLISION_GROUP_NONE, &tr );
                    Ray_t ray;
                    ray.Init( pAOPPlayer->EyePosition(), pAOPPlayer->EyePosition() + forward * MAX_TRACE_LENGTH );
                    CTraceFilterSimple traceFilter( pPlayer, COLLISION_GROUP_NONE );
                    enginetrace->TraceRay( ray, MASK_NPCSOLID, &traceFilter, &tr );
                    //AI_TraceLine(pAOPPlayer->EyePosition(),
                    //  pAOPPlayer->EyePosition() + forward * MAX_TRACE_LENGTH,MASK_NPCSOLID, 
                    //  pPlayer, COLLISION_GROUP_NONE, &tr );
                    if ( tr.fraction != 1.0)
                    {
                        /*if (baseNPC->CapabilitiesGet() & bits_CAP_MOVE_FLY)
                        {
                            Vector pos = tr.endpos - forward * 36;
                            VFuncs::Teleport(baseNPC,  &pos, NULL, NULL );
                        }
                        else
                        {*/
                            // Raise the end position a little up off the floor, place the npc and drop him down
                            tr.endpos.z += 12;
                            VFuncs::Teleport(baseNPC,  &tr.endpos, NULL, NULL );
                            UTIL_DropToFloor( baseNPC, MASK_NPCSOLID );
                        //}

                        // Now check that this is a valid location for the new npc to be
                        Vector  vUpBit = VFuncs::GetAbsOrigin(baseNPC);
                        vUpBit.z += 1;

                        UTIL_TraceEntity( baseNPC, VFuncs::GetAbsOrigin(baseNPC), VFuncs::GetAbsOrigin(baseNPC), MASK_NPCSOLID, &tr );
                        //AI_TraceHull( VFuncs::GetAbsOrigin(baseNPC), vUpBit, baseNPC->GetHullMins(), baseNPC->GetHullMaxs(), 
                        //  MASK_NPCSOLID, baseNPC, COLLISION_GROUP_NONE, &tr );
                        if ( tr.startsolid || (tr.fraction < 1.0) )
                        {
                            UTIL_Remove(baseNPC); //baseNPC->SUB_Remove();
                            baseNPC = NULL;
                            pAOPPlayer->SayText(UTIL_VarArgs("[%s] Can't create %s.  Bad Position!\n", pAdminOP.adminname, args.Arg(2)));
                            //NDebugOverlay::Box(VFuncs::GetAbsOrigin(baseNPC), baseNPC->GetHullMins(), baseNPC->GetHullMaxs(), 255, 0, 0, 0, 0);
                        }
                    }
                    if(baseNPC)
                        VFuncs::Activate(baseNPC);
                }
                else
                {
                    pAOPPlayer->SayText(UTIL_VarArgs("[%s] Unknown npc type %s.", pAdminOP.adminname, args.Arg(2)), HUD_PRINTCONSOLE);
                }
                //CBaseEntity::SetAllowPrecache( allowPrecache );
            }
            else
            {
                pAOPPlayer->SayText(UTIL_VarArgs("[%s] Usage: admin_ai create npcname.", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
        }
        else if(!stricmp(args.Arg(1), "delete"))
        {
            if(args.ArgC() > 2)
            {
            }
            else
            {
            }
        }
        else if(!stricmp(args.Arg(1), "deleteall"))
        {
            if(args.ArgC() > 2)
            {
            }
            else
            {
            }
        }
        else if(!stricmp(args.Arg(1), "difficulty"))
        {
            if(args.ArgC() > 2)
            {
            }
            else
            {
            }
        }
        else if(!stricmp(args.Arg(1), "setenemy"))
        {
            if(args.ArgC() > 2)
            {
            }
            else
            {
            }
        }
        else
        {
        }
    }
    else
    {
        pAOPPlayer->SayText(UTIL_VarArgs("[%s] AI command syntax:\ncreate      Creates a NPC\ndelete      Deletes all of a specified type\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
        pAOPPlayer->SayText(UTIL_VarArgs("deleteall   Deletes all NPCs spawned by you\ndifficulty  Adjusts NPC difficulty\nsetenemy    Sets the enemy of the NPCs\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
    }
    return 0;
}

int DFAdminAmmo(const CCommand &args, edict_t *pEntity)
{
    int iPlayer = ENTINDEX(pEntity);
    int player;
    player = DFPlayerSearchMenu(pEntity, "ammo", args);

    if(player != -1)
    {
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+player);

        if(info)
        {
            if(info->IsConnected())
            {
                if(pAdminOP.pAOPPlayers[player-1].GiveFullAmmo())
                {
                    pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Successfully gave ammo to %s.\n", pAdminOP.adminname, info->GetName()));
                }
                else
                {
                    pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Unable to give ammo. Most likely giving ammo isn't available for %s yet.\n", pAdminOP.adminname, pAdminOP.ModName()));
                }
                return 1;
            }
        }
    }
    pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Unknown player '%s'.\n", pAdminOP.adminname, args.ArgS()), HUD_PRINTCONSOLE);
    return 1;
}

int DFAdminGod(const CCommand &args, edict_t *pEntity)
{
    int playerList[128];
    int playerCount = 0;
    char pszType[128];
    char pszPlayer[128];
    int iPlayer = ENTINDEX(pEntity);

    Q_snprintf( pszPlayer, sizeof(pszPlayer), "%s", args.Arg(1) );
    if(pszPlayer[0] == '\"' && pszPlayer[strlen(pszPlayer)-1] == '\"') // remove quotes
    {
        pszPlayer[strlen(pszPlayer)-1] = '\0';
        strcpy(pszPlayer, &pszPlayer[1]);
    }

    Q_snprintf( pszType, sizeof(pszType), "%s", args.Arg(2) );
    if(pszType[0] == '\"' && pszType[strlen(pszType)-1] == '\"') // remove quotes
    {
        pszType[strlen(pszType)-1] = '\0';
        strcpy(pszType, &pszType[1]);
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
                        Q_snprintf( cmd, sizeof(cmd), "%sgod \"%s\" %s", command_prefix.GetString(), engine->GetPlayerNetworkIDString(pAdminOP.GetEntityList()+playerList[i]), pszType );

                        KeyValues *item1 = kv->FindKey( num, true );
                        item1->SetString( "msg", msg );
                        item1->SetString( "command", cmd );
                    }
                }
            }

            helpers->CreateMessage( pEntity, DIALOG_MENU, kv, &g_ServerPlugin );
            kv->deleteThis();
            return 1;
        }
        else
        {
            if(playerList[0] != 0)
            {
                IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+playerList[0]);

                if(info)
                {
                    if(!stricmp(pszType, "on"))
                    {
                        pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] %s now has god.\n", pAdminOP.adminname, info->GetName()), HUD_PRINTCONSOLE);
                        pAdminOP.SetGod(playerList[0], 1);
                    }
                    else if(!stricmp(pszType, "off"))
                    {
                        pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] %s no longer has god.\n", pAdminOP.adminname, info->GetName()), HUD_PRINTCONSOLE);
                        pAdminOP.SetGod(playerList[0], 0);
                    }
                    else
                    {
                        if(pAdminOP.pAOPPlayers[iPlayer-1].HasGod())
                            pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] %s no longer has god.\n", pAdminOP.adminname, info->GetName()), HUD_PRINTCONSOLE);
                        else
                            pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] %s now has god.\n", pAdminOP.adminname, info->GetName()), HUD_PRINTCONSOLE);

                        pAdminOP.SetGod(playerList[0], 2);
                    }
                    return 1;
                }
            }
        }
    }
    pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Unknown player '%s'.\n", pAdminOP.adminname, args.Arg(1)), HUD_PRINTCONSOLE);
    return 1;
}

int DFAdminRender(const CCommand &args, edict_t *pEntity)
{
    int playerList[128];
    int playerCount = 0;
    int iPlayer = ENTINDEX(pEntity);

    playerCount = pAdminOP.FindPlayer(playerList, args.Arg(1));
    if(playerCount)
    {
        if(playerCount == 1)
        {
            if(playerList[0] != 0)
            {
                IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+playerList[0]);

                if(info && info->IsConnected())
                {
                    CUtlVector <CBaseEntity *> changeList;
                    CBasePlayer *pFoundPlayer = (CBasePlayer*)VFuncs::Instance(pAdminOP.GetEntityList()+playerList[0]);
                    if(pFoundPlayer && VFuncs::IsPlayer(pFoundPlayer))
                    {
                        changeList.AddToTail(pFoundPlayer);

                        // apply to all weapons
                        for(int i = 0; i < 9; i++)
                        {
                            CBaseCombatWeapon *pWeapon = VFuncs::Weapon_GetSlot(pFoundPlayer, i);
                            if(pWeapon)
                            {
                                changeList.AddToTail(pWeapon);
                            }
                        }
                    }
                    FOR_EACH_VEC(changeList, i)
                    {
                        CBaseEntity *pFound = changeList[i];
                        if(args.ArgC() >= 3 && atoi(args.Arg(2)) != -1) VFuncs::SetRenderMode(pFound, (RenderMode_t)atoi(args.Arg(2)));
                        if(args.ArgC() >= 4 && atoi(args.Arg(3)) != -1) VFuncs::SetRenderColorA(pFound, atoi(args.Arg(3)));
                        if(args.ArgC() >= 5 && atoi(args.Arg(4)) != -1) VFuncs::SetRenderFX(pFound, atoi(args.Arg(4)));
                        if(args.ArgC() >= 6 && atoi(args.Arg(5)) != -1) VFuncs::SetRenderColorR(pFound, atoi(args.Arg(5)));
                        if(args.ArgC() >= 7 && atoi(args.Arg(6)) != -1) VFuncs::SetRenderColorG(pFound, atoi(args.Arg(6)));
                        if(args.ArgC() >= 8 && atoi(args.Arg(7)) != -1) VFuncs::SetRenderColorB(pFound, atoi(args.Arg(7)));
                    }
                    pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Changed render of player %s.\n", pAdminOP.adminname, info->GetName()), HUD_PRINTCONSOLE);
                    return 1;
                }
            }
        }
        else
        {
            pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Matched more than one player; menu not available for this command.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            return 1;
        }
    }
    pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Unknown player '%s'.\n", pAdminOP.adminname, args.Arg(1)), HUD_PRINTCONSOLE);

    return 1;
}

int DFAdminExec(const CCommand &args, edict_t *pEntity)
{
    int playerList[128];
    int playerCount = 0;
    char pszPlayer[128];
    char pszCommand[1024];
    char cmdArgs[1024];
    int iPlayer = ENTINDEX(pEntity);

    Q_snprintf( cmdArgs, sizeof(cmdArgs), "%s", args.ArgS());
    Q_snprintf( pszPlayer, sizeof(pszPlayer), "%s", args.Arg(1) );
    Q_snprintf( pszCommand, sizeof(pszCommand), "%s", &cmdArgs[strlen(args.Arg(1))+1] );
    if(pszPlayer[0] == '\"' && pszPlayer[strlen(pszPlayer)-1] == '\"') // remove quotes
    {
        pszPlayer[strlen(pszPlayer)-1] = '\0';
        strcpy(pszPlayer, &pszPlayer[1]);
    }
    if(pszCommand[0] == '\"' && pszCommand[strlen(pszCommand)-1] == '\"') // remove quotes
    {
        pszCommand[strlen(pszCommand)-1] = '\0';
        strcpy(pszCommand, &pszCommand[1]);
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
                        char num[10], msg[128], cmd[1152];
                        Q_snprintf( num, sizeof(num), "%i", i );
                        Q_snprintf( msg, sizeof(msg), "%s", info->GetName() );
                        Q_snprintf( cmd, sizeof(cmd), "%sexec \"%s\" %s", command_prefix.GetString(), engine->GetPlayerNetworkIDString(pAdminOP.GetEntityList()+playerList[i]), pszCommand );

                        KeyValues *item1 = kv->FindKey( num, true );
                        item1->SetString( "msg", msg );
                        item1->SetString( "command", cmd );
                    }
                }
            }

            helpers->CreateMessage( pEntity, DIALOG_MENU, kv, &g_ServerPlugin );
            kv->deleteThis();
            return 1;
        }
        else
        {
            if(playerList[0] != 0)
            {
                IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+playerList[0]);

                if(info)
                {
                    if(info->IsConnected())
                    {
                        engine->ClientCommand(pAdminOP.GetEntityList()+playerList[0], "%s", pszCommand);
                        pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Command executed on %s.\n", pAdminOP.adminname, info->GetName()), HUD_PRINTCONSOLE);
                        return 1;
                    }
                }
            }
        }
    }
    pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Unknown player '%s'.\n", pAdminOP.adminname, args.Arg(1)), HUD_PRINTCONSOLE);
    return 1;
}

int DFAdminExecAll(const CCommand &args, edict_t *pEntity)
{
    for(int i = 1; i <= pAdminOP.GetMaxClients(); i++)
    {
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+i);

        if(info)
        {
            if(info->IsConnected())
            {
                edict_t *pPlayer = pAdminOP.GetPlayer(i);
                if(pPlayer)
                {
                    engine->ClientCommand(pPlayer, "%s", args.ArgS());
                }
            }
        }
    }
    return 1;
}

int DFAdminSend(const CCommand &args, edict_t *pEntity)
{
    int playerListTo[128], playerListSent[128];
    int playerCountTo = 0, playerCountSent = 0;
    int iPlayer = ENTINDEX(pEntity);

    playerCountTo = pAdminOP.FindPlayer(playerListTo, args.Arg(1));
    playerCountSent = pAdminOP.FindPlayer(playerListSent, args.Arg(2));
    if(playerCountTo <= 0)
    {
        pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Unknown player '%s'.\n", pAdminOP.adminname, args.Arg(1)), HUD_PRINTCONSOLE);
        return 1;
    }
    if(playerCountSent <= 0)
    {
        pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Unknown player '%s'.\n", pAdminOP.adminname, args.Arg(2)), HUD_PRINTCONSOLE);
        return 1;
    }
    if(playerCountTo > 1)
    {
        pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Matched more than one \"to\" player; menu not available for this command.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
        return 1;
    }
    if(playerCountSent > 1)
    {
        pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Matched more than one \"sent\" player; menu not available for this command.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
        return 1;
    }
    if(playerListTo[0] != 0 && playerListSent[0] != 0)
    {
        IPlayerInfo *infoTo = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+playerListTo[0]);
        IPlayerInfo *infoSent = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+playerListSent[0]);
        if(infoTo && infoSent)
        {
            if(!infoTo->IsConnected())
            {
                pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] The \"to\" player is not connected.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
                return 1;
            }
            if(!infoSent->IsConnected())
            {
                pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] The \"sent\" player is not connected.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
                return 1;
            }
            CBaseEntity *pTo = CBaseEntity::Instance(pAdminOP.GetEntityList()+playerListTo[0]);
            CBaseEntity *pSent = CBaseEntity::Instance(pAdminOP.GetEntityList()+playerListSent[0]);
            VFuncs::Teleport(pSent, &VFuncs::GetAbsOrigin(pTo), &VFuncs::EyeAngles(pTo), &Vector(0,0,0));
            pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] The player was teleported.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            if(!pAdminOP.pAOPPlayers[playerListSent[0]-1].Unstick())
            {
                pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] However, no empty space was found nearby to place the player.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
            }
            return 1;
        }
    }

    pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Unknown error.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
    return 1;
}

int DFAdminFillUber(const CCommand &args, edict_t *pEntity)
{
    int iPlayer = ENTINDEX(pEntity);

    if(!pAdminOP.isTF2)
    {
        pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] This command only applies to TF2.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
        return 1;
    }

    int player;
    player = DFPlayerSearchMenu(pEntity, "filluber", args);

    if(player != -1)
    {
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+player);

        if(info)
        {
            if(info->IsConnected())
            {
                if(pAdminOP.pAOPPlayers[player-1].SetUberLevel(1.0f))
                {
                    pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Successfully filled uber.\n", pAdminOP.adminname));
                }
                else
                {
                    pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Unable to set uber.\n", pAdminOP.adminname));
                }
                return 1;
            }
        }
    }
    pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Unknown player '%s'.\n", pAdminOP.adminname, args.ArgS()), HUD_PRINTCONSOLE);
    return 1;
}

int DFAdminAlltalk(const CCommand &args, edict_t *pEntity)
{
    int iPlayer = ENTINDEX(pEntity);

    ConVar *sv_alltalk = cvar->FindVar("sv_alltalk");
    if(sv_alltalk)
    {
        if(FStrEq(args[1], "on") || atoi(args[1]) == 1)
        {
            sv_alltalk->SetValue(1);
            pAdminOP.pAOPPlayers[iPlayer-1].SayTextChatHud(UTIL_VarArgs("[%s] sv_alltalk enabled.\n", pAdminOP.adminname));
        }
        else if(FStrEq(args[1], "off") || FStrEq(args[1], "0"))
        {
            sv_alltalk->SetValue(0);
            pAdminOP.pAOPPlayers[iPlayer-1].SayTextChatHud(UTIL_VarArgs("[%s] sv_alltalk disabled.\n", pAdminOP.adminname));
        }
        else if(args.ArgC() == 1)
        {
            if(sv_alltalk->GetBool())
            {
                sv_alltalk->SetValue(0);
                pAdminOP.pAOPPlayers[iPlayer-1].SayTextChatHud(UTIL_VarArgs("[%s] sv_alltalk toggled off.\n", pAdminOP.adminname));
            }
            else
            {
                sv_alltalk->SetValue(1);
                pAdminOP.pAOPPlayers[iPlayer-1].SayTextChatHud(UTIL_VarArgs("[%s] sv_alltalk toggled on.\n", pAdminOP.adminname));
            }
        }
        else
        {
            pAdminOP.pAOPPlayers[iPlayer-1].SayTextChatHud(UTIL_VarArgs("[%s] Invalid use of command. Usage:\n", pAdminOP.adminname));
            pAdminOP.pAOPPlayers[iPlayer-1].SayTextChatHud(UTIL_VarArgs("[%s] Toggle sv_alltalk:  admin_alltalk\n", pAdminOP.adminname));
            pAdminOP.pAOPPlayers[iPlayer-1].SayTextChatHud(UTIL_VarArgs("[%s] Disable sv_alltalk: admin_alltalk 0  or admin_alltalk off\n", pAdminOP.adminname));
            pAdminOP.pAOPPlayers[iPlayer-1].SayTextChatHud(UTIL_VarArgs("[%s] Enable sv_alltalk:  admin_alltalk 1  or admin_alltalk on\n", pAdminOP.adminname));
        }
    }
    return 1;
}

int DFAdminPlayerSpray(const CCommand &args, edict_t *pEntity)
{
    int player;
    int iPlayer = ENTINDEX(pEntity);

    CBasePlayer *pPlayer = (CBasePlayer *)VFuncs::Instance(pEntity);
    if(!pPlayer)
        return 1;

    trace_t tr;
    CTraceFilterSimple traceFilter( pPlayer, COLLISION_GROUP_NONE );
    Ray_t ray;
    Vector forward;

    QAngle playerAngles = VFuncs::EyeAngles(pPlayer);
    AngleVectors( playerAngles, &forward, NULL, NULL );

    Vector start = VFuncs::EyePosition(pPlayer);
    Vector end = start + ( forward * 1024 );

    // trace forward
    ray.Init( start, end );
    enginetrace->TraceRay( ray, MASK_SOLID_BRUSHONLY, &traceFilter, &tr );

    // check for world entity
    if ( tr.fraction == 1.0 || tr.DidHitNonWorldEntity() )
    {
        pAdminOP.pAOPPlayers[iPlayer-1].SayTextChatHud(UTIL_VarArgs("[%s] Did not find a valid place to put spraypaint.\n", pAdminOP.adminname));
        return 1;
    }

    player = DFPlayerSearchMenu(pEntity, "playerspray", args);
    if(player != -1)
    {
        UTIL_PlayerDecalTrace(&tr, player);
        return 1;
    }

    pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Unknown player '%s'.\n", pAdminOP.adminname, args.ArgS(), HUD_PRINTCONSOLE));
    return 1;
}

int DFAdminGag(const CCommand &args, edict_t *pEntity)
{
    int iPlayer = ENTINDEX(pEntity);
    int player;
    player = DFPlayerSearchMenu(pEntity, "gag", args);

    if(player != -1)
    {
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+player);

        if(info)
        {
            if(info->IsConnected())
            {
                if(pAdminOP.pAOPPlayers[player-1].IsGagged())
                {
                    pAdminOP.pAOPPlayers[player-1].GagPlayer(0);
                    pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Ungagged %s.\n", pAdminOP.adminname, info->GetName()));
                    if(gag_notify.GetBool())
                    {
                        pAdminOP.pAOPPlayers[player-1].SayText(UTIL_VarArgs("[%s] You have been ungagged.\n", pAdminOP.adminname));
                    }
                }
                else
                {
                    pAdminOP.pAOPPlayers[player-1].GagPlayer(1);
                    pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Successfully gagged %s.\n", pAdminOP.adminname, info->GetName()));
                    if(gag_notify.GetBool())
                    {
                        pAdminOP.pAOPPlayers[player-1].SayText(UTIL_VarArgs("[%s] You have been gagged.\n", pAdminOP.adminname));
                    }
                }
                return 1;
            }
        }
    }
    pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Unknown player '%s'.\n", pAdminOP.adminname, args.ArgS()), HUD_PRINTCONSOLE);
    return 1;
}

int DFAdminNumBuildings(const CCommand &args, edict_t *pEntity)
{
    int iPlayer = ENTINDEX(pEntity);
    int player;
    int playerList[128];
    int playerCount = 0;

    if(!pAdminOP.isTF2)
    {
        pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] This command is for TF2 only.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
        return 1;
    }

    playerCount = pAdminOP.FindPlayer(playerList, args.Arg(1));
    if(playerCount < 1)
    {
        pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Unknown player '%s'.\n", pAdminOP.adminname, args.Arg(1)), HUD_PRINTCONSOLE);
        return 1;
    }
    if(playerCount > 1)
    {
        pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] The name '%s' is ambiguous.\n", pAdminOP.adminname, args.Arg(1)), HUD_PRINTCONSOLE);
        return 1;
    }
    player = playerList[0];

    edict_t *pETarget = pAdminOP.GetEntityList()+player;
    IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pETarget);
    if(info)
    {
        if(info->IsConnected())
        {
            CBasePlayer *pPlayer = (CBasePlayer*)servergameents->EdictToBaseEntity(pETarget);
            if(pPlayer)
            {
                if(pAdminOP.pAOPPlayers[player-1].GetPlayerClass() == TF2_CLASS_ENGINEER)
                {
                    VFuncs::SetNumBuildings(pPlayer, atoi(args[2]));
                    pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Set object count for %s.\n", pAdminOP.adminname, info->GetName()));
                }
                else
                {
                    pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Player '%s' is not an engineer.\n", pAdminOP.adminname, info->GetName()), HUD_PRINTCONSOLE);
                }
            }
        }
    }
    return 1;
}

int DFAdminAwardAchievement(const CCommand &args, edict_t *pEntity)
{
    int iPlayer = ENTINDEX(pEntity);
    int player;
    int playerList[128];
    int playerCount = 0;

    playerCount = pAdminOP.FindPlayer(playerList, args.Arg(1));
    if(playerCount < 1)
    {
        pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Unknown player '%s'.\n", pAdminOP.adminname, args.Arg(1)), HUD_PRINTCONSOLE);
        return 1;
    }
    if(playerCount > 1)
    {
        pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] The name '%s' is ambiguous.\n", pAdminOP.adminname, args.Arg(1)), HUD_PRINTCONSOLE);
        return 1;
    }
    player = playerList[0];

    if(player > 0)
    {
        edict_t *pEdAwardee = pAdminOP.GetEntityList()+player;
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pEdAwardee);

        if(info && info->IsConnected())
        {
            CRecipientFilter filter;
            filter.AddRecipient(player);
            if(pAdminOP.GameRules() && pAdminOP.GameRules()->IsInitialized())
            {
                pAdminOP.GameRules()->MarkAchievement(filter, args.Arg(2));
            }
        }
    }
    return 1;
}

int DFAdminAwardAllAchievements(const CCommand &args, edict_t *pEntity)
{
    int iPlayer = ENTINDEX(pEntity);
    int player;
    int playerList[128];
    int playerCount = 0;

    playerCount = pAdminOP.FindPlayer(playerList, args.Arg(1));
    if(playerCount < 1)
    {
        pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Unknown player '%s'.\n", pAdminOP.adminname, args.Arg(1)), HUD_PRINTCONSOLE);
        return 1;
    }
    if(playerCount > 1)
    {
        pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] The name '%s' is ambiguous.\n", pAdminOP.adminname, args.Arg(1)), HUD_PRINTCONSOLE);
        return 1;
    }
    player = playerList[0];

    if(player > 0)
    {
        edict_t *pEdAwardee = pAdminOP.GetEntityList()+player;
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pEdAwardee);

        if(info && info->IsConnected())
        {
            int i = 0;
            int totalachievements = 256;
            CRecipientFilter filter;
            filter.AddRecipient(player);

            // if we're using short or long, bump the total
            if(atoi(args.Arg(2)))
            {
                totalachievements = 1024;
            }

            // allow start to be overriden
            if(atoi(args.Arg(3)))
            {
                i = atoi(args.Arg(3));
            }

            // allow total to be overriden
            if(atoi(args.Arg(4)))
            {
                totalachievements = atoi(args.Arg(4));
            }

            for(; i < totalachievements; i++)
            {
                bf_write *pWrite;
                pWrite = engine->UserMessageBegin(&filter, pAdminOP.GetMessageByName("AchievementEvent"));
                // future versions of tf2 might accept shorts or long as the parameter for this event
                // since more achievements are coming
                // this is here to be prepared for that
                if(atoi(args.Arg(2)) == 1)
                {
                    pWrite->WriteShort(i);
                }
                else if(atoi(args.Arg(2)) == 2)
                {
                    pWrite->WriteLong(i);
                }
                // default is to write a byte
                else
                {
                    pWrite->WriteByte(i);
                }
                engine->MessageEnd();
            }
        }
    }
    return 1;
}

int DFAdminAddAdmin(const CCommand &args, edict_t *pEntity)
{
    int iPlayer = ENTINDEX(pEntity);
    int player;
    int playerList[128];
    int playerCount = 0;

    bool foundBaseLevel = false;
    bool containsValidChars = true;
    bool allKeyValuesValid = true;

    for(int i = 2; i < args.ArgC(); i++)
    {
        if(!Q_strncmp("baseLevel=", args.Arg(i), 10) && strlen(args.Arg(i)) > 10)
        {
            foundBaseLevel = true;
        }
        if(strstr(args.Arg(i), "}") || strstr(args.Arg(i), "{"))
        {
            containsValidChars = false;
        }
        if(!strstr(args.Arg(i), "="))
        {
            allKeyValuesValid = false;
        }
    }

    if(!foundBaseLevel || !containsValidChars || !allKeyValuesValid)
    {
        if(!foundBaseLevel)
        {
            pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] You must provide a baseLevel key/value pair.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
        }
        if(!containsValidChars)
        {
            pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] One of your key/value pairs contains invalid characters.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
        }
        if(!allKeyValuesValid)
        {
            pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] One of your key/value pairs does not contain an '=' symbol.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
        }
        pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Sample usage: %saddadmin playername baseLevel=131071 denyAdminCmd=pass.\n", pAdminOP.adminname, command_prefix.GetString()), HUD_PRINTCONSOLE);
        return 1;
    }

    playerCount = pAdminOP.FindPlayer(playerList, args.Arg(1));
    if(playerCount < 1)
    {
        pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Unknown player '%s'.\n", pAdminOP.adminname, args.Arg(1)), HUD_PRINTCONSOLE);
        return 1;
    }
    if(playerCount > 1)
    {
        pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] The name '%s' is ambiguous.\n", pAdminOP.adminname, args.Arg(1)), HUD_PRINTCONSOLE);
        return 1;
    }
    player = playerList[0];

    if(player <= 0)
    {
        return 1;
    }

    edict_t *pEdNewAdmin = pAdminOP.GetEntityList()+player;
    IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pEdNewAdmin);
    if(!info || !info->IsConnected())
    {
        pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Cannot add admin. The player is not yet in the game.\n", pAdminOP.adminname, args.Arg(1)), HUD_PRINTCONSOLE);
        return 1;
    }

    FILE *fp;
    char filepath[512];
    Q_snprintf(filepath, sizeof(filepath), "%s/%s/DF_admins.txt", pAdminOP.GameDir(), pAdminOP.DataDir());
    V_FixSlashes(filepath);
    fp = fopen(filepath, "at");

    if(fp)
    {
        fprintf(fp, "\n// %s\n\"SteamID:%s\"\n{\n", info->GetName(), engine->GetPlayerNetworkIDString(pEdNewAdmin));
        for(int i = 2; i < args.ArgC(); i++)
        {
            fprintf(fp, "\t%s\n", args.Arg(i));
        }
        fprintf(fp, "}\n");
        fclose(fp);

        pAdminOP.LoadAdmins();
        pAdminOP.pAOPPlayers[player-1].UpdateAdminLevel();
        pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Admin added.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
    }
    else
    {
        pAdminOP.pAOPPlayers[iPlayer-1].SayText(UTIL_VarArgs("[%s] Unable to add admin. Can't open admin file for writing.\n", pAdminOP.adminname), HUD_PRINTCONSOLE);
    }

    return 1;
}

SpewRetval_t DFAdminRconSpew( SpewType_t type, char const *pMsg )
{
    if(pSpewEntity)
    {
        pAdminOP.pAOPPlayers[ENTINDEX(pSpewEntity)-1].SayText(pMsg, HUD_PRINTCONSOLE);
    }
    if( type == SPEW_ASSERT )
    {
        return SPEW_DEBUGGER;
    }
    else if( type == SPEW_ERROR )
    {
        return SPEW_ABORT;
    }
    else
    {
        return SPEW_CONTINUE;
    }
}

int DFAdminRcon(const CCommand &args, edict_t *pEntity)
{
    SpewOutputFunc_t OldSpew;
    char cmd[2048];

    // clear out any pending commands first
    engine->ServerExecute();

    pSpewEntity = pEntity;
    OldSpew = GetSpewOutputFunc();
    SpewOutputFunc(DFAdminRconSpew);
    
    // prepare the command
    Q_snprintf(cmd, sizeof(cmd), "%s\n", args.ArgS());
    
    // disable color messages so that color tokens and stuff don't get redirected
    CAdminOP::DisableColorMsg();

    // queue the command
    engine->ServerCommand(cmd);
    // actually execute the command
    engine->ServerExecute();
    
    CAdminOP::EnableColorMsg();
    SpewOutputFunc(OldSpew);
    pSpewEntity = NULL;

    return 1;
}
