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

#ifndef L_CLASS_PLAYER
#define L_CLASS_PLAYER

class SOPEntity;
class SOPVector;
class Vector;
#include "lunar.h"
#include "l_class_entity.h"

class SOPPlayer : public SOPEntity {
public:
    SOPPlayer(int index=0);
    int AccountID(void);
    void AddSpawnedEnt(SOPEntity *entity);
    void BanByID(int time, const char *pszBannerName, const char *pszBannerID, const char *pszReason, const char *pszExtra);
    int Buttons(void);
    bool CanSpawn(void);
    void ConCommand(const char *pszCommand);
    void Disconnect(const char *pszReason);
    void Extinguish();
    void FakeConCommand(const char *pszCommand);
    void ForceRespawn();
    void Gag(bool gag);
    int GetActiveWeapon();
    const char *GetClientConVarValue(const char *pszConVar);
    int GetDesiredPlayerClass(void);
    const char *GetName(void);
    int GetPacketLossPercent(void);
    int GetPing(void);
    int GetPlayerClass(void);
    int GetSpawnedCount(void);
    int GetSpawnLimit(void);
    int GetTeam(void);
    int GetTimePlayed(void);
    int GiveNamedItem(const char *pszItem, int iSubType);
    bool HasFlag(void);
    bool IsAdmin(int level, const char *szCmd = NULL);
    bool IsAlive(void);
    bool IsBot(void);
    bool IsConnected(void);
    bool IsEntMoving(void);
    bool IsGagged(void);
    bool IsPlaying(void);
    void JetpackActivate(void);
    bool JetpackActive(void);
    void JetpackOff(void);
    void Kick(void);
    void Kill(void);
    int PendingConVarQueries(void);
    void PlaySound(const char *pszSound);
    void QueryConVar(const char *pszConVar, int callback);
    void SayText(const char *pText, int type);
    void SayTextNoSOP(const char *pText, int type);
    void SetDesiredPlayerClass(int playerclass);
    void SetPlayerClass(int playerclass);
    void SetTeam(int team);
    void SetView(SOPEntity *entity);
    void ShowMenu(int buttons, int time, const char *pText);
    void ShowWebsite(const char *pszTitle, const char *pszURL, bool show);
    void Slay(void);
    const char *SteamID(void);
    const char *SteamIDRender(void);
    void StripWeapons(void);
    int UserID(void);
    void Weapon_Equip(SOPEntity *entity);
    int Weapon_GetSlot(int slot);
    ~SOPPlayer();

    // Lua interface
    SOPPlayer(lua_State *L);
    int AccountID(lua_State *L);
    int AddSpawnedEnt(lua_State *L);
    int BanByID(lua_State *L);
    int Buttons(lua_State *L);
    int CanSpawn(lua_State *L);
    int ConCommand(lua_State *L);
    int Disconnect(lua_State *L);
    int Extinguish(lua_State *L);
    int FakeConCommand(lua_State *L);
    int ForceRespawn(lua_State *L);
    int Gag(lua_State *L);
    int GetActiveWeapon(lua_State *L);
    int GetClientConVarValue(lua_State *L);
    int GetDesiredPlayerClass(lua_State *L);
    int GetName(lua_State *L);
    int GetPacketLossPercent(lua_State *L);
    int GetPing(lua_State *L);
    int GetPlayerClass(lua_State *L);
    int GetSpawnedCount(lua_State *L);
    int GetSpawnLimit(lua_State *L);
    int GetTeam(lua_State *L);
    int GetTimePlayed(lua_State *L);
    int GiveNamedItem(lua_State *L);
    int HasFlag(lua_State *L);
    int IsAdmin(lua_State *L);
    int IsAlive(lua_State *L);
    int IsBot(lua_State *L);
    int IsConnected(lua_State *L);
    int IsEntMoving(lua_State *L);
    int IsGagged(lua_State *L);
    int IsPlaying(lua_State *L);
    int JetpackActivate(lua_State *L);
    int JetpackActive(lua_State *L);
    int JetpackOff(lua_State *L);
    int Kick(lua_State *L);
    int Kill(lua_State *L);
    int PendingConVarQueries(lua_State *L);
    int PlaySound(lua_State *L);
    int QueryConVar(lua_State *L);
    int SayText(lua_State *L);
    int SayTextNoSOP(lua_State *L);
    int SetDesiredPlayerClass(lua_State *L);
    int SetPlayerClass(lua_State *L);
    int SetTeam(lua_State *L);
    int SetView(lua_State *L);
    int ShowMenu(lua_State *L);
    int ShowWebsite(lua_State *L);
    int Slay(lua_State *L);
    int SteamID(lua_State *L);
    int SteamIDRender(lua_State *L);
    int StripWeapons(lua_State *L);
    int UserID(lua_State *L);
    int Weapon_Equip(lua_State *L);
    int Weapon_GetSlot(lua_State *L);

    static const char className[];
    static Lunar<SOPPlayer>::DerivedType derivedtypes[];
    static Lunar<SOPPlayer>::DynamicDerivedType dynamicDerivedTypes;
    static Lunar<SOPPlayer>::RegType metas[];
    static Lunar<SOPPlayer>::RegType methods[];
};

void lua_SOPPlayer_register(lua_State *L);

#endif
