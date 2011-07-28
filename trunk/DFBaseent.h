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

#ifndef DFBASEENT_H
#define DFBASEENT_H

class DFBaseent;
template <class T> T * ECreateClassPtr( T *a, edict_t *pOwner);
template <class T> T * EGetClassPtr( T *a );

#define MakeThink( a ) m_pfnThink = static_cast <void (DFBaseent::*)(void)> (a)
#define MakeTouch( a ) m_pfnTouch = static_cast <void (DFBaseent::*)(CBaseEntity *)> (a)
#define MakeMoveDone( a ) m_pfnCallWhenMoveDone = static_cast <void (DFBaseent::*)(void)> (a)
#define MakeCollisionBox( a ) m_pfnObjectCollisionBox = static_cast <void (DFBaseent::*)(void)> (a)

class DFBaseent
{
public:
    //entvars_t *pev;
    void *operator new( size_t stAllocateBlock );
    //void operator delete(void* p);

    //edict_t *Get( void );
    edict_t *Get() { return m_pent; };
    edict_t *Set( edict_t *pent );

    //operator int ();

    //operator CBaseEntity *();

    //CBaseEntity * operator = (CBaseEntity *pEntity);
    //CBaseEntity * operator ->();

    float nextthink;

    void (DFBaseent ::*m_pfnThink)(void);
    bool Think( void ){ if (m_pfnThink) { (this->*m_pfnThink)(); return 1; } else {return 0;} };
    void (DFBaseent ::*m_pfnTouch)( CBaseEntity *pOther );
    void Touch( CBaseEntity *pOther ){  if (m_pfnTouch) (this->*m_pfnTouch)(pOther); };

    void DFLinearMove( Vector vecDest, float flSpeed );
    void DFLinearMoveDone( void );
    void (DFBaseent ::*m_pfnCallWhenMoveDone)(void);

    void (DFBaseent ::*m_pfnObjectCollisionBox)(void);
    void SetObjectCollisionBox( void ){ if (m_pfnObjectCollisionBox) (this->*m_pfnObjectCollisionBox)(); };

    edict_t *edict() { return m_pent; };

private:
    int     m_serialnumber;
    edict_t *m_pent;

    Vector m_vecFinalDest;
    Vector vecDest;
    Vector vecDestDelta;

    /*char szSpam0[2048][2048]; // Memory problems?
    char szSpam1[2048][2048];
    char szSpam2[2048][2048];
    char szSpam3[2048][2048];
    char szSpam4[2048][2048];
    char szSpam5[2048][2048];
    char szSpam6[2048][2048];
    char szSpam7[2048][2048];
    char szSpam8[2048][2048];
    char szSpam9[2048][2048];
    char szSpam10[2048][2048];
    char szSpam11[2048][2048];
    char szSpam12[2048][2048];
    char szSpam13[2048][2048];
    char szSpam14[2048][2048];
    char szSpam15[2048][2048];
    char szSpam16[2048][2048];
    char szSpam17[2048][2048];
    char szSpam18[2048][2048];
    char szSpam19[2048][2048];
    char szSpam20[2048][2048];
    char szSpam21[2048][2048];
    char szSpam22[2048][2048];
    char szSpam23[2048][2048];
    char szSpam24[2048][2048];
    char szSpam25[2048][2048];
    char szSpam26[2048][2048];
    char szSpam27[2048][2048];
    char szSpam28[2048][2048];
    char szSpam29[2048][2048];
    char szSpam30[2048][2048];
    char szSpam31[2048][2048];
    char szSpam32[2048][2048];
    char szSpam33[2048][2048];
    char szSpam34[2048][2048];
    char szSpam35[2048][2048];
    char szSpam36[2048][2048];
    char szSpam37[2048][2048];
    char szSpam38[2048][2048];
    char szSpam39[2048][2048];
    char szSpam40[2048][2048];
    char szSpam41[2048][2048];
    char szSpam42[2048][2048];
    char szSpam43[2048][2048];
    char szSpam44[2048][2048];
    char szSpam45[2048][2048];
    char szSpam46[2048][2048];
    char szSpam47[2048][2048];
    char szSpam48[2048][2048];
    char szSpam49[2048][2048];*/
};

#endif /*DFBASEENT_H*/

#endif
