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

#ifdef DFENTS

// HACK HACK HACK : BASEENTITY ACCESS
#define GAME_DLL
#include "cbase.h"
#include "baseentity.h"
// HACK HACK HACK : BASEENTITY ACCESS

#include "vmatrix.h"
#include "util.h"
#include "vphysics_interface.h"

#include <stdio.h>

#include "AdminOP.h"
#include "cvars.h"

void *DFBaseent::operator new( size_t stAllocateBlock )
{
    void *mem = ::operator new( stAllocateBlock );
    memset( mem, 0, stAllocateBlock );
    return mem;
}

/*void DFBaseent::operator delete(void* p)
{
    if(p == NULL) return;
    free(p);
}*/

/*edict_t * DFBaseent::Get( void ) 
{
    if (m_pent)
    {
        if (m_pent->serialnumber == m_serialnumber) 
            return m_pent; 
        else
            return NULL;
    }
    return NULL; 
};*/

edict_t * DFBaseent::Set( edict_t *pent ) 
{ 
    m_pent = pent;
    if (pent)
    {
        m_serialnumber = m_pent->m_NetworkSerialNumber;
        //EntInfo[ENTINDEX(m_pent)].pEnt = this;
    }
    return pent; 
};

#endif
