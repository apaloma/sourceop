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
#include "mathlib/vmatrix.h"
#include "shareddefs.h"
#include "util.h"
#include "filesystem.h"

#include "interfaces.h"
#include "cvars.h"
#include "mapcycletracker.h"

#include "tier0/memdbgon.h"

CMapCycleTracker::CMapCycleTracker()
{
    m_nMapCycleTimeStamp = 0;
    m_nMapCycleindex = 0;
}

void StripChar(char *szBuffer, const char cWhiteSpace )
{
    while ( char *pSpace = strchr( szBuffer, cWhiteSpace ) )
    {
        char *pNextChar = pSpace + sizeof(char);
        V_strcpy( pSpace, pNextChar );
    }
}

void CMapCycleTracker::UpdateCycleIfNecessary()
{
    const char *mapcfile = mapcyclefile->GetString();
    Assert( mapcfile != NULL );

    // Check the time of the mapcycle file and re-populate the list of level names if the file has been modified
    const int nMapCycleTimeStamp = filesystem->GetPathTime( mapcfile, "GAME" );

    if ( 0 == nMapCycleTimeStamp )
    {
        // Map cycle file does not exist, make a list containing only the current map
        char *szCurrentMapName = new char[32];
        Q_strncpy( szCurrentMapName, STRING(gpGlobals->mapname), 32 );
        m_MapList.AddToTail( szCurrentMapName );
    }
    else
    {
        // If map cycle file has changed or this is the first time through ...
        if ( m_nMapCycleTimeStamp != nMapCycleTimeStamp )
        {
            // Reset map index and map cycle timestamp
            m_nMapCycleTimeStamp = nMapCycleTimeStamp;
            m_nMapCycleindex = 0;

            // Clear out existing map list. Not using Purge() because I don't think that it will do a 'delete []'
            for ( int i = 0; i < m_MapList.Count(); i++ )
            {
                delete [] m_MapList[i];
            }

            m_MapList.RemoveAll();

            // Repopulate map list from mapcycle file
            int nFileLength;
            char *aFileList = (char*)UTIL_LoadFileForMe( mapcfile, &nFileLength );
            if ( aFileList && nFileLength )
            {
                V_SplitString( aFileList, "\n", m_MapList );

                for ( int i = 0; i < m_MapList.Count(); i++ )
                {
                    // Strip out the spaces in the name
                    StripChar( m_MapList[i] , '\r');
                    StripChar( m_MapList[i] , ' ');

                    if ( !engine->IsMapValid( m_MapList[i] ) )
                    {
                        // If the engine doesn't consider it a valid map remove it from the lists
                        char szWarningMessage[MAX_PATH];
                        V_snprintf( szWarningMessage, MAX_PATH, "Invalid map '%s' included in map cycle file. Ignored.\n", m_MapList[i] );
                        Warning( szWarningMessage );

                        delete [] m_MapList[i];
                        m_MapList.Remove( i );
                        --i;
                    }
                }

                UTIL_FreeFile( (byte *)aFileList );
            }
        }
    }

    // If somehow we have no maps in the list then add the current one
    if ( 0 == m_MapList.Count() )
    {
        char *szDefaultMapName = new char[32];
        Q_strncpy( szDefaultMapName, STRING(gpGlobals->mapname), 32 );
        m_MapList.AddToTail( szDefaultMapName );
    }
}

void CMapCycleTracker::AdvanceCycle(bool bForce)
{
    if ( !bForce && nextlevel && nextlevel->GetString() && *nextlevel->GetString() && engine->IsMapValid( nextlevel->GetString() ) )
    {
        // map cycle will advance to nextlevel but the "nextmap" will remain the same
    }
    else
    {
        UpdateCycleIfNecessary();
        // Reset index if we've passed the end of the map list
        if ( ++m_nMapCycleindex >= m_MapList.Count() )
        {
            m_nMapCycleindex = 0;
        }
    }
}

void CMapCycleTracker::GetNextLevelName(char *pszNextMap, int bufsize, int mapsAhead)
{
    UpdateCycleIfNecessary();

    int query = m_nMapCycleindex + mapsAhead;
    query = query % m_MapList.Count();

    V_strncpy(pszNextMap, m_MapList[query], bufsize);
}

void CMapCycleTracker::GetCurrentLevelName(char *pszCurrentMap, int bufsize)
{
    UpdateCycleIfNecessary();

    int query = m_nMapCycleindex - 1;
    if(query < 0)
        query = m_MapList.Count() - 1;

    V_strncpy(pszCurrentMap, m_MapList[query], bufsize);
}
