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

#ifndef CAOPRADIO_H
#define CAOPRADIO_H

#include "CAOPExploding.h"
#include "isopgamesystem.h"

#include "tier0/memdbgon.h"

class CAOPRadio : public CAOPExploding, CAutoSOPGameSystem
{
public:
    DECLARE_CLASS(CAOPRadio, CAOPExploding);

    DECLARE_DATADESC();

    CAOPRadio();
    virtual ~CAOPRadio();

    virtual void Spawn(void);
    virtual void Precache(void);
    virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
    void Play( const char *pszFile );
    void SetRadioOwner( int index );
    virtual void ClientDisconnect(CBaseEntity *pEntity);
    virtual int ClientCommand(CBaseEntity *pEntity, const CCommand &args);
    void Think();
    void GetRangeSample();
    void StopRadio();
    virtual void UpdateOnRemove( void );

    bool m_bBeams;
    bool m_bRings;
    bool m_bFunnels;
private:
    void CreateVPhysics();
    void BeamEffect();
    void RingEffect();
    void UpdateRangeSample();
    void ShowRadioMenu(int playerID, const CCommand &args);

    Vector originOverride;
    int m_iOwner;
    int m_beamIndex;
    float m_flNextBeam;
    float m_flNextRing;
    float m_flNextFunnel;

    CUtlVector <int> inRange;
};

#endif
