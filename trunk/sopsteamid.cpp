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

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "strtools.h"
#include "tier0/dbg.h"

#include "sopsteamid.h"

#include "tier0/memdbgon.h"

extern int g_steamIDPrefix;

bool IsValidSteamID(const char *pchSteamID, char *pszErr, int maxerr)
{
    if(!strcmp(pchSteamID, "STEAM_ID_PENDING"))
    {
        return true;
    }
    if(!strcmp(pchSteamID, "BOT"))
    {
        return true;
    }
    if(strncmp(pchSteamID, "STEAM_", 6))
    {
        if(pszErr)
            V_snprintf(pszErr, maxerr, "[SOURCEOP] Unexpected SteamID format. Expected STEAM_.");
        return false;
    }
    if(!isdigit(pchSteamID[6]))
    {
        if(pszErr)
            V_snprintf(pszErr, maxerr, "[SOURCEOP] Expected digit after STEAM_");
        return false;
    }
    if(pchSteamID[7] != ':')
    {
        if(pszErr)
            V_snprintf(pszErr, maxerr, "[SOURCEOP] Expected colon after STEAM_#");
        return false;
    }
    if(!isdigit(pchSteamID[8]))
    {
        if(pszErr)
            V_snprintf(pszErr, maxerr, "[SOURCEOP] Expected digit after STEAM_#:");
        return false;
    }
    unsigned char middledigit = (pchSteamID[8] - '0');
    if(middledigit != 0 && middledigit != 1)
    {
        if(pszErr)
            V_snprintf(pszErr, maxerr, "[SOURCEOP] Expected only 0 or 1 after STEAM_#:");
        return false;
    }
    if(pchSteamID[9] != ':')
    {
        if(pszErr)
            V_snprintf(pszErr, maxerr, "[SOURCEOP] Expected colon after STEAM_#:#");
        return false;
    }
    int steamidlen = strlen(pchSteamID);
    if(steamidlen <= 10)
    {
        if(pszErr)
            V_snprintf(pszErr, maxerr, "[SOURCEOP] Expected at least one digit after STEAM_#:#:");
        return false;
    }
    for(int i = 10; i < steamidlen; i++)
    {
        if(!isdigit(pchSteamID[i]))
        {
            if(pszErr)
                V_snprintf(pszErr, maxerr, "[SOURCEOP] Expected only digits after STEAM_#:#:");
            return false;
        }
    }

    return true;
}

CSteamID::CSteamID(const char *pchSteamID, EUniverse eDefaultUniverse)
{
    if(!strcmp(pchSteamID, "STEAM_ID_PENDING"))
    {
        Msg("[SOURCEOP] Warning, assigning invalid CSteamID for %s.\n", pchSteamID);
        m_unAccountID = 0;
        m_EAccountType = k_EAccountTypeInvalid;
        m_EUniverse = k_EUniverseInvalid;
        m_unAccountInstance = 0;
        return;
    }
    if(!strcmp(pchSteamID, "BOT"))
    {
        Msg("[SOURCEOP] Warning, assigning zero'd CSteamID for %s.\n", pchSteamID);
        Set(0, eDefaultUniverse, k_EAccountTypeIndividual);
        return;
    }
    char errmsg[128];
    bool valid = IsValidSteamID(pchSteamID, errmsg, sizeof(errmsg));
    if(!valid)
    {
        Error("%s\n", errmsg);
        return;
    }

    // validation complete
    int accountid = (atoi(&pchSteamID[10]) << 1) + (pchSteamID[8] - '0');
    Set(accountid, eDefaultUniverse, k_EAccountTypeIndividual);
}

char *CSteamID::Render() const
{
    static char pszSteamID[64];

    sprintf(pszSteamID, "STEAM_%u:%u:%u", g_steamIDPrefix, m_unAccountID & 0x01, m_unAccountID >> 1);

    return pszSteamID;
}

char *CSteamID::Render(uint64 ulSteamID)
{
    static char pszSteamID[64];
    uint32 accountID = (uint32)ulSteamID & k_unSteamAccountIDMask;

    sprintf(pszSteamID, "STEAM_%u:%u:%u", g_steamIDPrefix, accountID & 0x01, accountID >> 1);

    return pszSteamID;
}
