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

#ifndef SOPGAMERULESPROXY_H
#define SOPGAMERULESPROXY_H

class IRecipientFilter;

class CSOPGameRulesProxy {
public:
    CSOPGameRulesProxy();
    void Init();
    inline bool IsInitialized() { return pGameRules != NULL; }
    inline void *GetGameRulesObject() { return pGameRules; }
    void LevelShutdown();
    void PluginUnloading();

    // CGameRules
    int PlayerRelationship(CBaseEntity *pPlayer, CBaseEntity *pTarget);
    bool PlayerCanHearChat(CBasePlayer *pListener, CBasePlayer *pSpeaker);
    int GetTeamIndex(const char *pTeamName);
    const char *GetIndexedTeamName(int teamIndex);
    bool IsValidTeam(const char *pTeamName);
    void MarkAchievement(IRecipientFilter &filter, char const *pchAchievementName);
    // CMultiplayRules
    void GetNextLevelName(char *szName, int bufsize);
    void ChangeLevel();
    void AdvanceMapCycle();
    // CTeamplayRules
    void SetStalemate(int iReason, bool bForceMapReset, bool bUnknown);
    void SetSwitchTeams(bool bScramble);
    void HandleSwitchTeams();
    void SetScrambleTeams(bool bScramble);

    // Hooks
    void OnChangeLevel();
    void OnGoToIntermission();
    void OnHandleSwitchTeams();
    void OnHandleScrambleTeams();
private:
    void SetupHooks();
    void RemoveHooks();

    void *pGameRules;
    int hookId1;
    int hookId2;
    int hookId3;
    int hookId4;
};

enum stalemate_reasons_t
{
	STALEMATE_JOIN_MID,
	STALEMATE_TIMER,
	STALEMATE_SERVER_TIMELIMIT,

	NUM_STALEMATE_REASONS,
};


#endif // SOPGAMERULESPROXY_H
