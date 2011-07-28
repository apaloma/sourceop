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

#ifndef CAOPSNARK_H
#define CAOPSNARK_H

#include "CAOPNormal.h"

#include "tier0/memdbgon.h"

#define SNARKDEPLOYSND  "squeek/sqk_deploy1.wav"
#define SNARKHUNTSND1   "squeek/sqk_hunt1.wav"
#define SNARKHUNTSND2   "squeek/sqk_hunt2.wav"
#define SNARKHUNTSND3   "squeek/sqk_hunt3.wav"
#define SNARKBLASTSND   "squeek/sqk_blast1.wav"
#define SNARKDIESND     "squeek/sqk_die1.wav"

class CAOPSnark : public CAOPNormal
{
public:
    DECLARE_CLASS(CAOPSnark, CAOPNormal);

    DECLARE_DATADESC();

    CAOPSnark();
    virtual void Spawn(void);
    virtual void Precache(void);
    void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
    virtual bool KeyValue( const char *szKeyName, const char *szValue );

    void SetSnarkOwner( int index );
    void InputExplode( inputdata_t &inputdata );
    bool IsOwnerStillValid();
    void HuntThink();
    void SuperBounceTouch( CBaseEntity *pOther );
    void SuperBounceTouchPost( CBaseEntity *pOther );
    void Event_Killed( const CTakeDamageInfo &inputInfo );
    virtual void UpdateOnRemove( void );

	//---------------------------------
	//	Outputs
	//---------------------------------
	CSOPOutputEvent		m_SnarkBite;

private:
    void ResetAngles();

    CBaseEntity *GetSnarkEnemy();
    void FindSnarkEnemy();

    float   m_flDie;
    Vector  m_vecTarget;
    float   m_flNextHunt;
    float   m_flNextHit;
    Vector  m_posPrev;
    float   m_flNextBounceSoundTime;
    CBaseEntity *m_hOwner;

    float   m_flNextAttack;
    int     m_iGround;
    bool    m_bOnGround;
    float   m_flBounce;
    
    CBaseEntity *m_hKiller;
    CBaseEntity *m_hEnemy;
    float m_flDamage;
    int m_iTeam;

    void CreateVPhysics();
    int m_iOwner;
};

#endif
