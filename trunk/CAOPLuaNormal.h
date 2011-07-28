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

#ifndef CAOPLUANORMAL_H
#define CAOPLUANORMAL_H

#include "CAOPNormal.h"

#include "tier0/memdbgon.h"

class SOPEntity;

class CAOPLuaNormal : public CAOPNormal
{
public:
    DECLARE_CLASS(CAOPLuaNormal, CAOPNormal);

    DECLARE_DATADESC();

    CAOPLuaNormal();
    virtual ~CAOPLuaNormal(void);

    virtual void PostConstructor( const char *szClassname );

    virtual void Spawn(void);
    virtual void Precache(void);

    void Think();
    void Touch(CBaseEntity *pOther);

    virtual bool AcceptInput( const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t Value, int outputID );
    virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

private:
    int m_iRef;
    int m_iEntRef;
    const char *m_szClassname;
    SOPEntity *m_LuaEnt;
};

#endif
