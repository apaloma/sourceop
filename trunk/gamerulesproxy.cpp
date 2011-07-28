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

// HACK HACK HACK : BASEENTITY ACCESS
#define GAME_DLL
#include "fixdebug.h"
#include "cbase.h"
#include "baseentity.h"
#include "player.h"
// HACK HACK HACK : BASEENTITY ACCESS

#include "AdminOP.h"
#include "sourcehooks.h"
#include "cvars.h"
#include "vfuncs.h"

#include "mapcycletracker.h"
#include "gamerulesproxy.h"

#include "tier0/memdbgon.h"

SH_DECL_MANUALHOOK0_void(CMultiplayRules_ChangeLevel, 0, 0, 0);
SH_DECL_MANUALHOOK0_void(CMultiplayRules_GoToIntermission, 0, 0, 0);
SH_DECL_MANUALHOOK0_void(CTeamplayRules_HandleSwitchTeams, 0, 0, 0);
SH_DECL_MANUALHOOK0_void(CTeamplayRules_HandleScrambleTeams, 0, 0, 0);

CSOPGameRulesProxy::CSOPGameRulesProxy()
{
    SH_MANUALHOOK_RECONFIGURE(CMultiplayRules_ChangeLevel, offs[OFFSET_CHANGELEVEL], 0, 0);
    SH_MANUALHOOK_RECONFIGURE(CMultiplayRules_GoToIntermission, offs[OFFSET_GOTOINTERMISSION], 0, 0);
    SH_MANUALHOOK_RECONFIGURE(CTeamplayRules_HandleSwitchTeams, offs[OFFSET_HANDLESWITCHTEAMS], 0, 0);
    SH_MANUALHOOK_RECONFIGURE(CTeamplayRules_HandleScrambleTeams, offs[OFFSET_HANDLESCRAMBLETEAMS], 0, 0);
    pGameRules = NULL;
}

void CSOPGameRulesProxy::Init()
{
    SendTableProxyFn proxyfn;
    proxyfn = pAdminOP.GetPropDataTableProxyFn("DT_TFGameRulesProxy", "tf_gamerules_data");
    if(!proxyfn)
        proxyfn = pAdminOP.GetPropDataTableProxyFn("DT_CSGameRulesProxy", "cs_gamerules_data");
    if(!proxyfn)
        proxyfn = pAdminOP.GetPropDataTableProxyFn("DT_HL2MPGameRulesProxy", "hl2mp_gamerules_data");
    if(proxyfn)
    {
        // the proxy function will call SetAllRecipients on
        // this class. We do not use it other than the fact
        // that the proxy function needs it to call that func.
        CSendProxyRecipients recp;

        // get the game rules from the proxy function
        pGameRules = proxyfn(NULL, NULL, NULL, &recp, 0);

        SetupHooks();

        return;
    }
    else
    {
        CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Unable to load game rules.\n");
        engine->LogPrint("[SOURCEOP] \"Console<0><Console><Console>\" could not load game rules\n");
    }
}

void CSOPGameRulesProxy::SetupHooks()
{
    this->hookId1 = SH_ADD_MANUALVPHOOK(CMultiplayRules_ChangeLevel, pGameRules, SH_MEMBER(this, &CSOPGameRulesProxy::OnChangeLevel), false);
    this->hookId2 = SH_ADD_MANUALVPHOOK(CMultiplayRules_GoToIntermission, pGameRules, SH_MEMBER(this, &CSOPGameRulesProxy::OnGoToIntermission), false);
    this->hookId3 = SH_ADD_MANUALVPHOOK(CTeamplayRules_HandleSwitchTeams, pGameRules, SH_MEMBER(this, &CSOPGameRulesProxy::OnHandleSwitchTeams), false);
    this->hookId4 = SH_ADD_MANUALVPHOOK(CTeamplayRules_HandleScrambleTeams, pGameRules, SH_MEMBER(this, &CSOPGameRulesProxy::OnHandleScrambleTeams), false);
}

void CSOPGameRulesProxy::RemoveHooks()
{
    SH_REMOVE_HOOK_ID(this->hookId1);
    SH_REMOVE_HOOK_ID(this->hookId2);
    SH_REMOVE_HOOK_ID(this->hookId3);
    SH_REMOVE_HOOK_ID(this->hookId4);
}

void CSOPGameRulesProxy::LevelShutdown()
{
    RemoveHooks();
}

void CSOPGameRulesProxy::PluginUnloading()
{
    RemoveHooks();
}

int CSOPGameRulesProxy::PlayerRelationship(CBaseEntity *pPlayer, CBaseEntity *pTarget)
{
    return VFuncs::PlayerRelationship(pGameRules, pPlayer, pTarget);
}

bool CSOPGameRulesProxy::PlayerCanHearChat(CBasePlayer *pListener, CBasePlayer *pSpeaker)
{
    return VFuncs::PlayerCanHearChat(pGameRules, pListener, pSpeaker);
}

int CSOPGameRulesProxy::GetTeamIndex(const char *pTeamName)
{
    if(pTeamName && *pTeamName != 0 )
    {
        for(int i = MIN_TEAM_NUM; i <= MAX_TEAM_NUM; i++)
        {
            if(!stricmp(pAdminOP.TeamName(i), pTeamName))
            {
                return i;
            }
        }
    }
    return -1;
    //return VFuncs::GetTeamIndex(pGameRules, pTeamName);
}

const char *CSOPGameRulesProxy::GetIndexedTeamName(int teamIndex)
{
    return pAdminOP.TeamName(teamIndex);
    //return VFuncs::GetIndexedTeamName(pGameRules, teamIndex);
}

bool CSOPGameRulesProxy::IsValidTeam(const char *pTeamName)
{
    return GetTeamIndex(pTeamName) != -1;
    //return VFuncs::GetTeamIndex(pGameRules, pTeamName);
}

void CSOPGameRulesProxy::MarkAchievement(IRecipientFilter &filter, char const *pchAchievementName)
{
    VFuncs::MarkAchievement(pGameRules, filter, pchAchievementName);
}

void CSOPGameRulesProxy::GetNextLevelName(char *szName, int bufsize)
{
    VFuncs::GetNextLevelName(pGameRules, szName, bufsize, false);
}

void CSOPGameRulesProxy::ChangeLevel()
{
    VFuncs::ChangeLevel(pGameRules);
}

void CSOPGameRulesProxy::AdvanceMapCycle()
{
    if(pAdminOP.isTF2)
    {
        engine->ServerCommand("skip_next_map\n");
        if(pAdminOP.MapCycleTracker())
        {
            pAdminOP.MapCycleTracker()->AdvanceCycle(true);
        }
    }
    else
    {
        pAdminOP.BlockNextChangeLevel();
        ChangeLevel();
        pAdminOP.UnblockNextChangeLevel();
    }
}

void CSOPGameRulesProxy::SetStalemate(int iReason, bool bForceMapReset, bool bUnknown)
{
    VFuncs::SetStalemate(pGameRules, iReason, bForceMapReset, bUnknown);
}

void CSOPGameRulesProxy::SetSwitchTeams(bool bSwitch)
{
    VFuncs::SetSwitchTeams(pGameRules, bSwitch);
}

void CSOPGameRulesProxy::HandleSwitchTeams()
{
    VFuncs::HandleSwitchTeams(pGameRules);
}

void CSOPGameRulesProxy::SetScrambleTeams(bool bScramble)
{
    VFuncs::SetScrambleTeams(pGameRules, bScramble);
}

void CSOPGameRulesProxy::OnChangeLevel()
{
    if(pAdminOP.MapCycleTracker())
    {
        pAdminOP.MapCycleTracker()->AdvanceCycle(false);
    }
}

void CSOPGameRulesProxy::OnGoToIntermission()
{
    pAdminOP.OnGoToIntermission();
}

void CSOPGameRulesProxy::OnHandleSwitchTeams()
{
    pAdminOP.OnHandleSwitchTeams();
}

void CSOPGameRulesProxy::OnHandleScrambleTeams()
{
    pAdminOP.OnHandleScrambleTeams();
}
