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

#ifndef MAPCYCLETRACKER_H
#define MAPCYCLETRACKER_H

class CMapCycleTracker
{
public:
    CMapCycleTracker();
    void UpdateCycleIfNecessary();
    void AdvanceCycle(bool bForce);
    void GetNextLevelName(char *pszNextMap, int bufsize, int mapsAhead = 0);
    int GetCurrentCycleIndex() { return m_nMapCycleindex; }
    void GetCurrentLevelName(char *pszCurrentMap, int bufsize);
    int GetNumMapsInList() { return m_MapList.Count(); }
private:
    int m_nMapCycleTimeStamp;
    int m_nMapCycleindex;
    CUtlVector<char*> m_MapList;
};

extern bool DFIsMapValid( const char *pszMapName );

#endif // MAPCYCLETRACKER_H