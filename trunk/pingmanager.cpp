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

#include <stdio.h>

#include "pingmanager.h"

#include "tier0/memdbgon.h"

extern IVEngineServer *engine;

void DFPingManager::Clear()
{
    SetPlayerIndex(0);
    maxCount = 0;
    lastRecordTime = 0;
    pingData.Purge();
}

void DFPingManager::SetPlayerIndex(unsigned short playerIndex)
{
    player = playerIndex;
}

int DFPingManager::RecordPing()
{
    int ping, packetloss;

    lastRecordTime = engine->Time();
    if(!player || !maxCount) return -1;
    if(pingData.Count() >= maxCount) pingData.Remove(0);
    UTIL_GetPlayerConnectionInfo(player, ping, packetloss);
    pingData.AddToTail(ping);
    return ping;
}

int DFPingManager::Count()
{
    return pingData.Count();
}

int DFPingManager::GetAverage()
{
    int total = 0;
    float avg;
    if(!pingData.Count()) return 0;
    for(unsigned short i = 0; i < pingData.Count(); i++)
    {
        total += pingData.Element(i);
    }
    avg = ((float)total)/((float)pingData.Count());
    return (int)avg;
}
