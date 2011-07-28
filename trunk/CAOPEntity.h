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

#ifndef CAOPENTITY_H
#define CAOPENTITY_H

#define BIG_HEALTH 100000

class CBaseEntity;
class CServerNetworkProperty;
class CAOPEntity;

#include "entityoutput.h"
#include "datamap.h"

#include "tier0/memdbgon.h"

typedef void (CAOPEntity::*sopinputfunc_t)(inputdata_t &data);
#undef DEFINE_INPUTFUNC
#ifdef _L4D_PLUGIN
#define DEFINE_INPUTFUNC( fieldtype, inputname, inputfunc ) { fieldtype, #inputfunc, { NULL }, 1, FTYPEDESC_INPUT, inputname, NULL, (inputfunc_t)static_cast <sopinputfunc_t> (&classNameTypedef::inputfunc) }
#else
#define DEFINE_INPUTFUNC( fieldtype, inputname, inputfunc ) { fieldtype, #inputfunc, { NULL, NULL }, 1, FTYPEDESC_INPUT, inputname, NULL, (inputfunc_t)static_cast <sopinputfunc_t> (&classNameTypedef::inputfunc) }
#endif

extern trace_t g_TouchTrace;

extern bool g_hookTest;

#define MakeThink( a ) m_pfnThink = static_cast <void (CAOPEntity::*)(void)> (a)
#define MakeMyThink( a ) m_pfnMyThink = static_cast <void (CAOPEntity::*)(void)> (a)
#define MakeTouch( a ) m_pfnTouch = static_cast <void (CAOPEntity::*)(CBaseEntity *)> (a)
#define MakeTouchPost( a ) m_pfnTouchPost = static_cast <void (CAOPEntity::*)(CBaseEntity *)> (a)
#define MakeStartTouch( a ) m_pfnStartTouch = static_cast <void (CAOPEntity::*)(CBaseEntity *)> (a)

#define ADD_TO_VP_ENTLIST(e, x) \
    do \
    { \
        int entindex = VFuncs::entindex(e); \
        if(pAdminOP.myEntList.Count() < entindex + 1) \
        { \
            for(int i = entindex+1 - pAdminOP.myEntList.Count(); i > 0; i--) \
            { \
                pAdminOP.myEntList.AddToTail(NULL); \
            } \
        } \
        pAdminOP.myEntList[entindex] = (x); \
    } \
    while(0)

typedef struct keyvalue_s {
    char    key[256];
    char    val[256];
} keyvalue_t;

enum
{
    AOPENTCLASS_BASE=0,
    AOPENTCLASS_FRAG
};

class CSOPOutputEvent : public CBaseEntityOutput
{
public:
    CSOPOutputEvent();
	// void Firing, no parameter
	void FireOutput( CBaseEntity *pActivator, CBaseEntity *pCaller, float fDelay = 0 );
};

void AddEntityToInstallList( IEntityFactory *pFactory, const char *pClassName, int feature );

template <class T>
class CSOPEntityFactory : public IEntityFactory
{
public:
	CSOPEntityFactory( const char *pClassName, int feature )
	{
        AddEntityToInstallList(this, pClassName, feature);
	}

	IServerNetworkable *Create( const char *pClassName )
	{
		T* pEnt = _CreateEntityTemplate((T*)NULL, pClassName);
		return pEnt->NetworkProp();
	}

	void Destroy( IServerNetworkable *pNetworkable )
	{
		if ( pNetworkable )
		{
			pNetworkable->Release();
		}
	}

	virtual size_t GetEntitySize()
	{
		return sizeof(T);
	}
};

#define SOP_LINK_ENTITY_TO_CLASS(mapClassName,DLLClassName) \
	static CSOPEntityFactory<DLLClassName> mapClassName( #mapClassName, -1 );
#define SOP_LINK_ENTITY_TO_CLASS_FEAT(mapClassName,DLLClassName,feature) \
	static CSOPEntityFactory<DLLClassName> mapClassName( #mapClassName, feature );

class CAOPEntity
{
public:
    DECLARE_CLASS_NOBASE(CAOPEntity);

    DECLARE_DATADESC();

    virtual ~CAOPEntity();

    virtual void PostConstructor( const char *szClassname );

    void Set(edict_t *pent);

    inline edict_t *Get() { return m_pent; }
    inline CBaseEntity *GetBase() { return m_base; }
    CServerNetworkProperty *NetworkProp();
    inline int GetIndex() { return m_entindex; };

    virtual void Spawn(void);
    virtual void Precache(void) {};

    void (CAOPEntity::*m_pfnThink)(void);
    void Think( void ) { if (m_pfnThink) (this->*m_pfnThink)();};
    void DefaultThink(void);
    void (CAOPEntity::*m_pfnMyThink)(void);
    void MyThink( void ) { if (m_flNextThink != -1 && m_pfnMyThink && m_flNextThink <= gpGlobals->curtime) (this->*m_pfnMyThink)();};
    void SetNextThink( float flNextThink ) { m_flNextThink = flNextThink; }

    void (CAOPEntity ::*m_pfnTouch)(CBaseEntity *pOther);
    void Touch(CBaseEntity *pOther){ if (m_pfnTouch) g_TouchTrace.m_pEnt = GetBase(); (this->*m_pfnTouch)(pOther); };
    void (CAOPEntity ::*m_pfnTouchPost)(CBaseEntity *pOther);
    void TouchPost(CBaseEntity *pOther){ if (m_pfnTouchPost) (this->*m_pfnTouchPost)(pOther); };
    void (CAOPEntity ::*m_pfnStartTouch)(CBaseEntity *pOther);
    void StartTouch(CBaseEntity *pOther){ if (m_pfnStartTouch) (this->*m_pfnStartTouch)(pOther); };
    void DefaultStartTouch(CBaseEntity *pOther);
    void DefaultTouch(CBaseEntity *pOther);

    virtual bool AcceptInput( const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t Value, int outputID );
    virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

    virtual bool KeyValue( const char *szKeyName, const char *szValue );
    CUtlLinkedList <keyvalue_t, unsigned short> *keyValuePairs;  // this must be a pointer so that delete[] on CAOPEntity works correctly

    void InputCopyDatadesc( inputdata_t &inputdata );

    void Teleport(Vector *origin, QAngle *angles);
    Vector GetAbsOrigin();
    Vector GetAbsVelocity( void );

    virtual void UpdateOnRemove( void );

    int hookID_spawn;
    int hookID_precache;
    int hookID_acceptinput;
    int hookID_touch;
    int hookID_touchPost;
    int hookID_startTouch;
    int hookID_think;
    int hookID_keyvalue;
    int hookID_keyValue;
    int hookID_updateOnRemove;
    int hookID_use;

protected:
    float m_flNextThink;
private:
    edict_t *m_pent;
    CBaseEntity *m_base;
    int m_entindex;
};

extern bool TestKeyValue( const char *szKeyName, const char *szValue );

#endif
