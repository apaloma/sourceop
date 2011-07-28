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

#include "AdminOP.h"
#include "cvars.h"

#include "vfuncs.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "lunar.h"
#include "l_class_entity.h"
#include "l_class_player.h"
#include "l_class_vector.h"
#include "lentitycache.h"

LUALIB_API int luaL_checkboolean (lua_State *L, int narg);
LUALIB_API int luaL_optbool (lua_State *L, int narg, int def);

SOPPlayer::SOPPlayer(int index) {
    m_iIndex = index;
}
SOPPlayer::~SOPPlayer() {
}
int SOPPlayer::AccountID() {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        if(pAdminOP.pAOPPlayers[EntIndex()-1].GetPlayerState() > 0)
        {
            return pAdminOP.pAOPPlayers[EntIndex()-1].GetSteamID().GetAccountID();
        }
    }

    return 0;
}
void SOPPlayer::AddSpawnedEnt(SOPEntity *entity) {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        pAdminOP.pAOPPlayers[EntIndex()-1].spawnedEnts.AddToTail(entity->EntIndex());
    }
}
void SOPPlayer::BanByID(int time, const char *pszBannerName, const char *pszBannerID, const char *pszReason, const char *pszExtra)
{
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+EntIndex());

        if(info && info->IsConnected())
        {
            pAdminOP.BanPlayer(EntIndex(), info, pAdminOP.pAOPPlayers[EntIndex()-1].GetSteamID(), time, pszBannerName, pszBannerID, pszReason, false, pszExtra);
        }
    }
}
int SOPPlayer::Buttons()
{
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        CBasePlayer *pPlayer = (CBasePlayer *)pAdminOP.GetEntity(EntIndex());
        if(pPlayer)
        {
            return VFuncs::GetButtons(pPlayer);
        }
    }
    return 0;
}
bool SOPPlayer::CanSpawn() {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        int playerSpawnLimit = pAdminOP.pAOPPlayers[EntIndex()-1].GetSpawnLimit();
        return (playerSpawnLimit  == -1          || pAdminOP.pAOPPlayers[EntIndex()-1].spawnedEnts.Count() < playerSpawnLimit) &&
               (spawnlimit_server.GetInt() == -1 || pAdminOP.spawnedServerEnts.Count()                     < spawnlimit_server.GetInt());
    }
    return false;
}
void SOPPlayer::ConCommand(const char *pszCommand) {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        edict_t *pEnt = pAdminOP.GetEntityList()+EntIndex();
        if(pEnt)
        {
            engine->ClientCommand(pEnt, pszCommand);
        }
    }
}
void SOPPlayer::Disconnect(const char *pszReason) {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        void *pBaseClient = pAdminOP.pAOPPlayers[EntIndex()-1].baseclient;
        if(pBaseClient)
        {
            VFuncs::Disconnect(pBaseClient, "%s", pszReason);
        }
    }
}
void SOPPlayer::Extinguish() {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        CBasePlayer *pPlayer = (CBasePlayer *)pAdminOP.GetEntity(EntIndex());
        if(pPlayer)
        {
            VFuncs::Extinguish(pPlayer);
        }
    }
}
void SOPPlayer::FakeConCommand(const char *pszCommand) {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        edict_t *pEnt = pAdminOP.GetEntityList()+EntIndex();
        if(pEnt)
        {
            CCommand fake;
            fake.Tokenize(pszCommand);
            pAdminOP.FakeClientCommand(pEnt, false, false, fake);
        }
    }
}
void SOPPlayer::ForceRespawn() {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        CBasePlayer *pPlayer = (CBasePlayer *)pAdminOP.GetEntity(EntIndex());
        if(pPlayer)
        {
            VFuncs::ForceRespawn(pPlayer);
        }
    }
}
void SOPPlayer::Gag(bool gag) {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        pAdminOP.pAOPPlayers[EntIndex()-1].GagPlayer(gag);
    }
}
int SOPPlayer::GetActiveWeapon()
{
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        //CBaseEntity *pWeapon = pAdminOP.pAOPPlayers[EntIndex()-1].pActiveWeapon;
        CBasePlayer *pPlayer = (CBasePlayer *)pAdminOP.GetEntity(EntIndex());
        if(pPlayer)
        {
            CBaseEntity *pWeapon = VFuncs::GetActiveWeapon(pPlayer);
            if(pWeapon)
                return VFuncs::entindex(pWeapon);
            else
                return -1;
        }
    }

    return -1;
}
const char *SOPPlayer::GetClientConVarValue(const char *pszConVar) {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients() && pszConVar)
    {
        return engine->GetClientConVarValue(EntIndex(), pszConVar);
    }
    return "";
}
int SOPPlayer::GetDesiredPlayerClass() {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        CBasePlayer *pPlayer = (CBasePlayer *)pAdminOP.GetEntity(EntIndex());
        if(pPlayer)
        {
            return VFuncs::GetDesiredPlayerClass(pPlayer);
        }
    }
    return 0;
}
const char *SOPPlayer::GetName() {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        return pAdminOP.pAOPPlayers[EntIndex()-1].GetCurrentName();
    }
    return "";
}
int SOPPlayer::GetPacketLossPercent() {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        int ping, packetloss;
        UTIL_GetPlayerConnectionInfo(EntIndex(), ping, packetloss);
        return packetloss;
    }
    return 0;
}
int SOPPlayer::GetPing() {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        int ping, packetloss;
        UTIL_GetPlayerConnectionInfo(EntIndex(), ping, packetloss);
        return ping;
    }
    return 0;
}
int SOPPlayer::GetPlayerClass() {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        CBasePlayer *pPlayer = (CBasePlayer *)pAdminOP.GetEntity(EntIndex());
        if(pPlayer)
        {
            return VFuncs::GetPlayerClass(pPlayer);
        }
    }
    return 0;
}
int SOPPlayer::GetSpawnedCount() {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        return pAdminOP.pAOPPlayers[EntIndex()-1].spawnedEnts.Count();
    }
    return 0;
}
int SOPPlayer::GetSpawnLimit() {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        return pAdminOP.pAOPPlayers[EntIndex()-1].GetSpawnLimit();
    }
    return 0;
}
int SOPPlayer::GetTeam() {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        return pAdminOP.pAOPPlayers[EntIndex()-1].GetTeam();
    }
    return 0;
}
int SOPPlayer::GetTimePlayed() {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        return pAdminOP.pAOPPlayers[EntIndex()-1].GetTimePlayed();
    }
    return 0;
}
int SOPPlayer::GiveNamedItem(const char *pszItem, int iSubType) {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        CBaseEntity *pEnt = pAdminOP.pAOPPlayers[EntIndex()-1].GiveNamedItem(pszItem, iSubType);
        if(pEnt)
            return VFuncs::entindex(pEnt);
        else
            return -1;
    }
    return -1;
}
bool SOPPlayer::HasFlag() {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        return pAdminOP.pAOPPlayers[EntIndex()-1].HasFlag();
    }
    return 0;
}
bool SOPPlayer::IsAdmin(int level, const char *szCmd) {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        if(szCmd)
            return pAdminOP.pAOPPlayers[EntIndex()-1].IsAdmin(level, szCmd) != 0;
        else
            return pAdminOP.pAOPPlayers[EntIndex()-1].IsAdmin(level) != 0;
    }
    return 0;
}
bool SOPPlayer::IsAlive() {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        return pAdminOP.pAOPPlayers[EntIndex()-1].IsAlive();
    }
    return 0;
}
bool SOPPlayer::IsBot() {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        return !pAdminOP.pAOPPlayers[EntIndex()-1].NotBot();
    }
    return 0;
}
bool SOPPlayer::IsConnected() {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        return pAdminOP.pAOPPlayers[EntIndex()-1].GetPlayerState() > 0;
    }
    return 0;
}
bool SOPPlayer::IsEntMoving() {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        return pAdminOP.pAOPPlayers[EntIndex()-1].entMoving;
    }
    return 0;
}
bool SOPPlayer::IsGagged() {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        return pAdminOP.pAOPPlayers[EntIndex()-1].IsGagged();
    }
    return 0;
}
bool SOPPlayer::IsPlaying() {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        return pAdminOP.pAOPPlayers[EntIndex()-1].GetPlayerState() > 1;
    }
    return 0;
}
void SOPPlayer::JetpackActivate(void) {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        pAdminOP.pAOPPlayers[EntIndex()-1].JetpackActivate();
    }
}
bool SOPPlayer::JetpackActive(void) {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        return pAdminOP.pAOPPlayers[EntIndex()-1].JetpackActive();
    }
    return false;
}
void SOPPlayer::JetpackOff(void) {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        pAdminOP.pAOPPlayers[EntIndex()-1].JetpackOff();
    }
}
void SOPPlayer::Kick(void) {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+EntIndex());

        if(info)
        {
            pAdminOP.KickPlayer(info);
        }
    }
}
void SOPPlayer::Kill(void) {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        pAdminOP.pAOPPlayers[EntIndex()-1].Kill();
    }
}
int SOPPlayer::PendingConVarQueries(void) {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        return pAdminOP.pAOPPlayers[EntIndex()-1].m_pendingCvarQueries.Count();
    }
    return 0;
}
void SOPPlayer::PlaySound(const char *pszSound) {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        pAdminOP.pAOPPlayers[EntIndex()-1].PlaySoundLocal(pszSound);
    }
}
void SOPPlayer::QueryConVar(const char *pszConVar, int callback) {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        cvarquery_t newQuery;
        newQuery.luacallback = callback;
        newQuery.cookie = helpers->StartQueryCvarValue(pAdminOP.GetEntityList()+EntIndex(), pszConVar);

        if(newQuery.cookie != InvalidQueryCvarCookie)
        {
            pAdminOP.pAOPPlayers[EntIndex()-1].m_pendingCvarQueries.AddToTail(newQuery);
        }
        else
        {
            Msg("[SOURCEOP] StartQueryCvarValue returned InvalidQueryCvarCookie for ent %i.\n", EntIndex());
        }
    }
}
void SOPPlayer::SayText(const char *pText, int type) {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        pAdminOP.pAOPPlayers[EntIndex()-1].SayText(UTIL_VarArgs("[%s] %s", pAdminOP.adminname, pText), type);
    }
}
void SOPPlayer::SayTextNoSOP(const char *pText, int type) {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        pAdminOP.pAOPPlayers[EntIndex()-1].SayText(pText, type);
    }
}
void SOPPlayer::SetDesiredPlayerClass(int playerclass)
{
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        CBasePlayer *pPlayer = (CBasePlayer *)pAdminOP.GetEntity(EntIndex());
        if(pPlayer)
        {
            VFuncs::SetDesiredPlayerClass(pPlayer, playerclass);
        }
    }
}
void SOPPlayer::SetPlayerClass(int playerclass)
{
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        pAdminOP.pAOPPlayers[EntIndex()-1].SetPlayerClass(playerclass);
    }
}
void SOPPlayer::SetTeam(int team)
{
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        pAdminOP.pAOPPlayers[EntIndex()-1].SetTeam(team);
    }
}
void SOPPlayer::SetView(SOPEntity *entity)
{
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        edict_t *pEdPlayer = pAdminOP.GetEntityList()+EntIndex();
        edict_t *pEdEntity = pAdminOP.GetEntityList()+entity->EntIndex();
        engine->SetView(pEdPlayer, pEdEntity);
    }
}
void SOPPlayer::ShowMenu(int buttons, int time, const char *pText)
{
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        CRecipientFilter filter;
        filter.AddRecipient(EntIndex());

        int umsg = pAdminOP.GetMessageByName("showmenu");
        if(umsg != -1)
        {
            char text[2048];
            char buf[251];
            char *p = text;
            int limit = strlen(pText);

            strncpy(text, pText, sizeof(text));
            text[sizeof(text)-1] = '\0';

            // write messages with more option enabled while there is enough data
            while(strlen(p) > sizeof(buf)-1)
            {
                strncpy(buf, p, sizeof(buf));
                buf[sizeof(buf)-1] = '\0';

                bf_write *pBuffer = engine->UserMessageBegin(&filter, umsg);
                    pBuffer->WriteShort(buttons);       // Sets how many options the menu has
                    pBuffer->WriteChar(time);           // Sets how long the menu stays open -1 for stay until option selected
                    pBuffer->WriteByte(true);           // more?
                    pBuffer->WriteString(buf);          // The text shown on the menu
                engine->MessageEnd();

                p += sizeof(buf) - 1;
            }
            // then send last bit
            bf_write *pBuffer = engine->UserMessageBegin(&filter, umsg);
                pBuffer->WriteShort(buttons);       // Sets how many options the menu has
                pBuffer->WriteChar(time);           // Sets how long the menu stays open -1 for stay until option selected
                pBuffer->WriteByte(false);          // more?
                pBuffer->WriteString(p);            // The text shown on the menu
            engine->MessageEnd();
        }
    }
}
void SOPPlayer::ShowWebsite(const char *pszTitle, const char *pszURL, bool show) {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        KeyValues *data = new KeyValues("data");
        data->SetString( "title", pszTitle );
        data->SetString( "type", "2" );
        data->SetString( "msg", pszURL );
        if(pAdminOP.isTF2)
        {
            data->SetString( "msg_fallback", "motd_text" );
            data->SetString( "customsvr", "1" );
        }

        data->SetInt( "cmd", INFO_PANEL_COMMAND_CLOSED_HTMLPAGE );

        pAdminOP.pAOPPlayers[EntIndex()-1].ShowViewPortPanel( "info", show, data );
        data->deleteThis();
    }
}
void SOPPlayer::Slay(void) {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        pAdminOP.SlayPlayer(EntIndex());
    }
}
const char *SOPPlayer::SteamID() {
    static char steamid[48];
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        if(pAdminOP.pAOPPlayers[EntIndex()-1].GetPlayerState() > 0)
        {
            V_snprintf(steamid, sizeof(steamid), "%llu", pAdminOP.pAOPPlayers[EntIndex()-1].GetSteamID().ConvertToUint64());
            return steamid;
        }
    }
    return "";
}
const char *SOPPlayer::SteamIDRender() {
    static char steamid[48];
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        if(pAdminOP.pAOPPlayers[EntIndex()-1].GetPlayerState() > 0)
        {
            V_snprintf(steamid, sizeof(steamid), "%s", pAdminOP.pAOPPlayers[EntIndex()-1].GetSteamID().Render());
            return steamid;
        }
    }
    return "";
}
void SOPPlayer::StripWeapons()
{
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        CBasePlayer *pPlayer = (CBasePlayer *)pAdminOP.GetEntity(EntIndex());
        if(pPlayer)
        {
            int iterations = 0;
            bool bRemovedSomething = false;
            CBaseCombatWeapon *pWeapon = NULL;

            do
            {
                int slotRemoved = -1;
                bRemovedSomething = false;
                for(int i = 0; i < 10; i++)
                {
                    pWeapon = VFuncs::Weapon_GetSlot(pPlayer, i);
                    if(pWeapon)
                    {
                        VFuncs::RemovePlayerItem(pPlayer, pWeapon);
                        UTIL_Remove(pWeapon);
                        bRemovedSomething = true;
                        slotRemoved = i;
                    }
                }

                iterations++;

                // let's break out of the loop if we keep coming up with weapons to remove
                // this could happen if RemovePlayerItem is failing or doesn't exist
                if(iterations > 10 && bRemovedSomething)
                {
                    pAdminOP.TimeLog("SourceOPErrors.log", "Breaking out of StripWeapons early. Slot %i still has weapons apparently.\n", slotRemoved);
                    break;
                }
            }
            while(bRemovedSomething);

            // when a player has checked the "remember last weapon between deaths"
            // option, the player may have an active weapon still at this point
            pWeapon = VFuncs::GetActiveWeapon(pPlayer);
            if(pWeapon != NULL)
            {
                VFuncs::RemovePlayerItem(pPlayer, pWeapon);
                UTIL_Remove(pWeapon);
            }
        }
    }
}
int SOPPlayer::UserID() {
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pAdminOP.GetEntityList()+EntIndex());
        if(info && info->IsConnected())
        {
            return info->GetUserID();
        }
    }
    return 0;
}
void SOPPlayer::Weapon_Equip(SOPEntity *entity)
{
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        CBasePlayer *pPlayer = (CBasePlayer *)pAdminOP.GetEntity(EntIndex());
        if(pPlayer)
        {
            CBaseEntity *pWeapon = pAdminOP.GetEntity(entity->EntIndex());
            if(VFuncs::IsBaseCombatWeapon(pWeapon))
            {
                VFuncs::Weapon_Equip(pPlayer, (CBaseCombatWeapon *)pWeapon);
            }
        }
    }
}
int SOPPlayer::Weapon_GetSlot(int slot)
{
    if(EntIndex() > 0 && EntIndex() <= pAdminOP.GetMaxClients())
    {
        CBasePlayer *pPlayer = (CBasePlayer *)pAdminOP.GetEntity(EntIndex());
        if(pPlayer)
        {
            CBaseEntity *pWeapon = VFuncs::Weapon_GetSlot(pPlayer, slot);
            if(pWeapon)
                return VFuncs::entindex(pWeapon);
            else
                return -1;
        }
    }

    return -1;
}

// Lua interface
SOPPlayer::SOPPlayer(lua_State *L) {
    m_iIndex = luaL_checkinteger(L, 1);
}
int SOPPlayer::AccountID(lua_State *L) {
    lua_pushinteger(L, AccountID());
    return 1;
}
int SOPPlayer::AddSpawnedEnt(lua_State *L) {
    SOPEntity *entity = Lunar<SOPEntity>::check(L, 1);

    AddSpawnedEnt(entity);
    return 0;
}
int SOPPlayer::BanByID(lua_State *L) {
    size_t l;
    int time = luaL_checkinteger(L, 1);
    const char *pszReason = luaL_optlstring(L, 2, NULL, &l);
    const char *pszExtra = luaL_optlstring(L, 3, NULL, &l);

    BanByID(time, "SOURCEOP", "SOURCEOP", pszReason, pszExtra);
    return 0;
}
int SOPPlayer::Buttons(lua_State *L) {
    lua_pushinteger(L, Buttons());
    return 1;
}
int SOPPlayer::CanSpawn(lua_State *L) {
    lua_pushboolean(L, CanSpawn());
    return 1;
}
int SOPPlayer::ConCommand(lua_State *L) {
    const char *pCommand = luaL_checkstring(L, 1);

    ConCommand(pCommand);
    return 0;
}
int SOPPlayer::Disconnect(lua_State *L) {
    const char *pszCommand = luaL_checkstring(L, 1);

    Disconnect(pszCommand);
    return 0;
}
int SOPPlayer::Extinguish(lua_State *L) {
    Extinguish();
    return 0;
}
int SOPPlayer::FakeConCommand(lua_State *L) {
    const char *pCommand = luaL_checkstring(L, 1);

    FakeConCommand(pCommand);
    return 0;
}
int SOPPlayer::ForceRespawn(lua_State *L) {
    ForceRespawn();
    return 0;
}
int SOPPlayer::Gag(lua_State *L) {
    Gag(luaL_checkboolean(L, 1) != 0);
    return 0;
}
int SOPPlayer::GetActiveWeapon(lua_State *L) {
    int ent = GetActiveWeapon();

    if(ent > -1)
    {
        g_entityCache.PushEntity(ent);
    }
    else
    {
        lua_pushnil(L);
    }

    return 1;
}
int SOPPlayer::GetClientConVarValue(lua_State *L) {
    lua_pushstring(L, GetClientConVarValue(luaL_checkstring(L, 1)));
    return 1;
}
int SOPPlayer::GetDesiredPlayerClass(lua_State *L) {
    lua_pushinteger(L, GetDesiredPlayerClass());
    return 1;
}
int SOPPlayer::GetName(lua_State *L) {
    lua_pushstring(L, GetName());
    return 1;
}
int SOPPlayer::GetPacketLossPercent(lua_State *L) {
    lua_pushinteger(L, GetPacketLossPercent());
    return 1;
}
int SOPPlayer::GetPing(lua_State *L) {
    lua_pushinteger(L, GetPing());
    return 1;
}
int SOPPlayer::GetPlayerClass(lua_State *L) {
    lua_pushinteger(L, GetPlayerClass());
    return 1;
}
int SOPPlayer::GetSpawnedCount(lua_State *L) {
    lua_pushinteger(L, GetSpawnedCount());
    return 1;
}
int SOPPlayer::GetSpawnLimit(lua_State *L) {
    lua_pushinteger(L, GetSpawnLimit());
    return 1;
}
int SOPPlayer::GetTeam(lua_State *L) {
    lua_pushinteger(L, GetTeam());
    return 1;
}
int SOPPlayer::GetTimePlayed(lua_State *L) {
    lua_pushinteger(L, GetTimePlayed());
    return 1;
}
int SOPPlayer::GiveNamedItem(lua_State *L) {
    const char *pszItem = luaL_checkstring(L, 1);
    int subType = luaL_optinteger(L, 2, 0);
    int ent = GiveNamedItem(pszItem, subType);
    if(ent > -1)
    {
        g_entityCache.PushEntity(ent);
    }
    else
    {
        lua_pushnil(L);
    }
    return 1;
}
int SOPPlayer::HasFlag(lua_State *L) {
    lua_pushboolean(L, HasFlag());
    return 1;
}
int SOPPlayer::IsAdmin(lua_State *L) {
    size_t l;
    int level = luaL_optinteger(L, 1, 0);
    const char *szCmd = luaL_optlstring(L, 2, NULL, &l);
    lua_pushboolean(L, IsAdmin(level, szCmd));
    return 1;
}
int SOPPlayer::IsAlive(lua_State *L) {
    lua_pushboolean(L, IsAlive());
    return 1;
}
int SOPPlayer::IsBot(lua_State *L) {
    lua_pushboolean(L, IsBot());
    return 1;
}
int SOPPlayer::IsConnected(lua_State *L) {
    lua_pushboolean(L, IsConnected());
    return 1;
}
int SOPPlayer::IsEntMoving(lua_State *L) {
    lua_pushboolean(L, IsEntMoving());
    return 1;
}
int SOPPlayer::IsGagged(lua_State *L) {
    lua_pushboolean(L, IsGagged());
    return 1;
}
int SOPPlayer::IsPlaying(lua_State *L) {
    lua_pushboolean(L, IsPlaying());
    return 1;
}
int SOPPlayer::JetpackActivate(lua_State *L) {
    JetpackActivate();
    return 0;
}
int SOPPlayer::JetpackActive(lua_State *L) {
    lua_pushboolean(L, JetpackActive());
    return 1;
}
int SOPPlayer::JetpackOff(lua_State *L) {
    JetpackOff();
    return 0;
}
int SOPPlayer::Kick(lua_State *L) {
    Kick();
    return 0;
}
int SOPPlayer::Kill(lua_State *L) {
    Kill();
    return 0;
}
int SOPPlayer::PendingConVarQueries(lua_State *L) {
    lua_pushinteger(L, PendingConVarQueries());
    return 1;
}
int SOPPlayer::PlaySound(lua_State *L) {
    const char *pText = luaL_checkstring(L, 1);
    PlaySound(pText);
    return 0;
}
int SOPPlayer::QueryConVar(lua_State *L) {
    QueryConVar(luaL_checkstring(L, 1), lua_ref(L, true));
    return 0;
}
int SOPPlayer::SayText(lua_State *L) {
    size_t l;
    const char *pText = luaL_checklstring(L, 1, &l);
    int type = luaL_optinteger(L, 2, HUD_PRINTTALK);
    SayText(pText, type);
    return 0;
}
int SOPPlayer::SayTextNoSOP(lua_State *L) {
    size_t l;
    const char *pText = luaL_checklstring(L, 1, &l);
    int type = luaL_optinteger(L, 2, HUD_PRINTTALK);
    SayTextNoSOP(pText, type);
    return 0;
}
int SOPPlayer::SetDesiredPlayerClass(lua_State *L) {
    int team = luaL_checkinteger(L, 1);
    SetDesiredPlayerClass(team);
    return 0;
}
int SOPPlayer::SetPlayerClass(lua_State *L) {
    int team = luaL_checkinteger(L, 1);
    SetPlayerClass(team);
    return 0;
}
int SOPPlayer::SetTeam(lua_State *L) {
    int team = luaL_checkinteger(L, 1);
    SetTeam(team);
    return 0;
}
int SOPPlayer::SetView(lua_State *L) {
    SOPEntity *view = Lunar<SOPEntity>::check(L, 1);
    SetView(view);
    return 0;
}
int SOPPlayer::ShowMenu(lua_State *L) {
    int buttons = luaL_checkinteger(L, 1);
    int time = luaL_checkinteger(L, 2);
    const char *pText = luaL_checkstring(L, 3);

    ShowMenu(buttons, time, pText);
    return 0;
}
int SOPPlayer::ShowWebsite(lua_State *L) {
    const char *pszTitle = luaL_checkstring(L, 1);
    const char *pszURL = luaL_checkstring(L, 2);
    bool show = luaL_optbool(L, 3, 1) != 0;

    ShowWebsite(pszTitle, pszURL, show);
    return 0;
}
int SOPPlayer::Slay(lua_State *L) {
    Slay();
    return 0;
}
int SOPPlayer::SteamID(lua_State *L) {
    lua_pushstring(L, SteamID());
    return 1;
}
int SOPPlayer::SteamIDRender(lua_State *L) {
    lua_pushstring(L, SteamIDRender());
    return 1;
}
int SOPPlayer::StripWeapons(lua_State *L) {
    StripWeapons();
    return 0;
}
int SOPPlayer::UserID(lua_State *L) {
    lua_pushinteger(L, UserID());
    return 1;
}
int SOPPlayer::Weapon_Equip(lua_State *L) {
    SOPEntity *weapon = Lunar<SOPEntity>::check(L, 1);
    Weapon_Equip(weapon);
    return 0;
}
int SOPPlayer::Weapon_GetSlot(lua_State *L) {
    int slot = luaL_checkinteger(L, 1);

    int ent = Weapon_GetSlot(slot);

    if(ent > -1)
    {
        g_entityCache.PushEntity(ent);
    }
    else
    {
        lua_pushnil(L);
    }

    return 1;
}

const char SOPPlayer::className[] = "Player";
Lunar<SOPPlayer>::DerivedType SOPPlayer::derivedtypes[] = {
    {NULL}
};
Lunar<SOPPlayer>::DynamicDerivedType SOPPlayer::dynamicDerivedTypes;

#define method(class, name) {#name, &class::name}

template <>
int Lunar<SOPPlayer>::tostring_T (lua_State *L) {
    userdataType *ud = static_cast<userdataType*>(lua_touserdata(L, 1));
    SOPPlayer *obj = ud->pT;
    lua_pushstring(L, UTIL_VarArgs("%i", obj->EntIndex()));
    return 1; 
}

Lunar<SOPPlayer>::RegType SOPPlayer::metas[] = {
    {0,0}
};

Lunar<SOPPlayer>::RegType SOPPlayer::methods[] = {
    method(SOPPlayer, AccountID),
    method(SOPPlayer, AddSpawnedEnt),
    method(SOPPlayer, BanByID),
    method(SOPPlayer, Buttons),
    method(SOPPlayer, CanSpawn),
    method(SOPPlayer, ConCommand),
    method(SOPPlayer, Disconnect),
    method(SOPPlayer, Extinguish),
    method(SOPPlayer, FakeConCommand),
    method(SOPPlayer, ForceRespawn),
    method(SOPPlayer, Gag),
    method(SOPPlayer, GetActiveWeapon),
    method(SOPPlayer, GetClientConVarValue),
    method(SOPPlayer, GetDesiredPlayerClass),
    method(SOPPlayer, GetName),
    method(SOPPlayer, GetPacketLossPercent),
    method(SOPPlayer, GetPing),
    method(SOPPlayer, GetPlayerClass),
    method(SOPPlayer, GetSpawnedCount),
    method(SOPPlayer, GetSpawnLimit),
    method(SOPPlayer, GetTeam),
    method(SOPPlayer, GetTimePlayed),
    method(SOPPlayer, GiveNamedItem),
    method(SOPPlayer, HasFlag),
    method(SOPPlayer, IsAdmin),
    method(SOPPlayer, IsAlive),
    method(SOPPlayer, IsBot),
    method(SOPPlayer, IsConnected),
    method(SOPPlayer, IsEntMoving),
    method(SOPPlayer, IsGagged),
    method(SOPPlayer, IsPlaying),
    method(SOPPlayer, JetpackActivate),
    method(SOPPlayer, JetpackActive),
    method(SOPPlayer, JetpackOff),
    method(SOPPlayer, Kick),
    method(SOPPlayer, Kill),
    {"Name",        &SOPPlayer::GetName},
    method(SOPPlayer, PendingConVarQueries),
    method(SOPPlayer, PlaySound),
    method(SOPPlayer, QueryConVar),
    method(SOPPlayer, SayText),
    method(SOPPlayer, SayTextNoSOP),
    method(SOPPlayer, SetDesiredPlayerClass),
    method(SOPPlayer, SetPlayerClass),
    method(SOPPlayer, SetTeam),
    method(SOPPlayer, SetView),
    method(SOPPlayer, ShowMenu),
    method(SOPPlayer, ShowWebsite),
    method(SOPPlayer, Slay),
    method(SOPPlayer, SteamID),
    method(SOPPlayer, SteamIDRender),
    method(SOPPlayer, StripWeapons),
    {"Team",        &SOPPlayer::GetTeam},
    method(SOPPlayer, UserID),
    method(SOPPlayer, Weapon_Equip),
    method(SOPPlayer, Weapon_GetSlot),
    {0,0}
};

void lua_SOPPlayer_register(lua_State *L)
{
    Lunar<SOPPlayer>::Register(L);

    luaL_loadstring(L, "setmetatable( Player, { __index = Entity } )");
    lua_call(L, 0, 0);
}
