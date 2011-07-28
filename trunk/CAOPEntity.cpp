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
#include "recipientfilter.h"

// HACK HACK HACK : BASEENTITY ACCESS
#define GAME_DLL
#ifdef _L4D_PLUGIN
#include "convar_l4d.h"
#include "datamap_l4d.h"
#include "dt_send_l4d.h"
#endif
#include "cbase.h"
#include "baseentity.h"
#include "player.h"
// HACK HACK HACK : BASEENTITY ACCESS

#include "vmatrix.h"
#include "util.h"
#include "vphysics_interface.h"
#include "vcollide_parse.h"

#include <stdio.h>
#include <time.h>

#include "AdminOP.h"
#include "sourcehooks.h"
#include "cvars.h"
#include "bitbuf.h"
#include "CAOPEntity.h"
#include "vfuncs.h"

#include "tier0/memdbgon.h"

extern bool ParseKeyvalue( void *pObject, typedescription_t *pFields, int iNumFields, const char *szKeyName, const char *szValue );

bool g_hookTest = 0;

BEGIN_DATADESC_NO_BASE( CAOPEntity )
    DEFINE_INPUTFUNC( FIELD_INPUT, "SOPCopyDatadesc", InputCopyDatadesc ),
END_DATADESC()

CAOPEntity::~CAOPEntity()
{
}

void CAOPEntity::PostConstructor( const char *szClassname )
{
    CBaseEntity *pEnt = GetBase();
    if(pEnt)
    {
        VFuncs::SetClassname(pEnt, szClassname);
    }
}

void CAOPEntity::Set(edict_t *pent) 
{
    MakeStartTouch(&CAOPEntity::DefaultStartTouch);
    MakeTouch(&CAOPEntity::DefaultTouch);
    MakeThink(&CAOPEntity::DefaultThink);
    m_pent = pent;
    m_base = CBaseEntity::Instance(pent);
    m_entindex = VFuncs::entindex(m_base);
    keyValuePairs = new CUtlLinkedList <keyvalue_t, unsigned short>;
}

CServerNetworkProperty *CAOPEntity::NetworkProp()
{
    CBaseEntity *pEnt = GetBase();
    return VFuncs::NetworkProp(pEnt);
}

void CAOPEntity::Spawn()
{
}

void CAOPEntity::DefaultStartTouch(CBaseEntity *pOther)
{
#ifdef OFFICIALSERV_ONLY
    if(disabletouch.GetBool())
    {
        RETURN_META(MRES_SUPERCEDE);
    }
    else
    {
#endif
        //update touch trace
        trace_t tr;
        UTIL_ClearTrace( tr );
        tr.endpos = (VFuncs::GetAbsOrigin(GetBase()) + VFuncs::GetAbsOrigin(pOther)) * 0.5;
        g_TouchTrace = tr;
        RETURN_META(MRES_IGNORED);
#ifdef OFFICIALSERV_ONLY
    }
#endif
}

void CAOPEntity::DefaultTouch(CBaseEntity *pOther)
{
#ifdef OFFICIALSERV_ONLY
    if(disabletouch.GetBool())
        RETURN_META(MRES_SUPERCEDE);
    else
#endif
        RETURN_META(MRES_IGNORED);
}

void CAOPEntity::DefaultThink(void)
{
/*#ifdef OFFICIALSERV_ONLY
    if(disablethink.GetBool())
        RETURN_META(MRES_SUPERCEDE);
    else
#endif
        RETURN_META(MRES_IGNORED);*/
}

bool CAOPEntity::AcceptInput( const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t Value, int outputID )
{
    CBaseEntity *pEnt = GetBase();
	// loop through the data description list, restoring each data desc block
	for ( datamap_t *dmap = GetDataDescMap(); dmap != NULL; dmap = dmap->baseMap )
	{
		// search through all the actions in the data description, looking for a match
		for ( int i = 0; i < dmap->dataNumFields; i++ )
		{
			if ( dmap->dataDesc[i].flags & FTYPEDESC_INPUT )
			{
				if ( !Q_stricmp(dmap->dataDesc[i].externalName, szInputName) )
				{
					// found a match

					/*char szBuffer[256];
					// mapper debug message
					if (pCaller != NULL)
					{
						Q_snprintf( szBuffer, sizeof(szBuffer), "(%0.2f) input %s: %s.%s(%s)\n", gpGlobals->curtime, STRING(pCaller->m_iName), GetDebugName(), szInputName, Value.String() );
					}
					else
					{
						Q_snprintf( szBuffer, sizeof(szBuffer), "(%0.2f) input <NULL>: %s.%s(%s)\n", gpGlobals->curtime, GetDebugName(), szInputName, Value.String() );
					}
					DevMsg( 2, szBuffer );
					ADD_DEBUG_HISTORY( HISTORY_ENTITY_IO, szBuffer );

					if (m_debugOverlays & OVERLAY_MESSAGE_BIT)
					{
						DrawInputOverlay(szInputName,pCaller,Value);
					}*/

					// convert the value if necessary
					if ( Value.FieldType() != dmap->dataDesc[i].fieldType )
					{
						if ( !(Value.FieldType() == FIELD_VOID && dmap->dataDesc[i].fieldType == FIELD_STRING) ) // allow empty strings
						{
							if ( !Value.Convert( (fieldtype_t)dmap->dataDesc[i].fieldType ) )
							{
								// bad conversion
								Warning( "!! ERROR: bad input/output link:\n!! %s(%s,%s) doesn't match type from %s(%s)\n", 
									VFuncs::GetClassname(pEnt), VFuncs::GetClassname(pEnt), szInputName, 
									( pCaller != NULL ) ? VFuncs::GetClassname(pCaller) : "<null>",
									( pCaller != NULL ) ? /*STRING(pCaller->m_iName)*/"targetname" : "<null>" );
								//return false;
                                RETURN_META_VALUE(MRES_SUPERCEDE, false);
							}
						}
					}

					// call the input handler, or if there is none just set the value
					sopinputfunc_t pfnInput = (sopinputfunc_t)dmap->dataDesc[i].inputFunc;

					if ( pfnInput )
					{ 
						// Package the data into a struct for passing to the input handler.
						inputdata_t data;
						data.pActivator = pActivator;
						data.pCaller = pCaller;
						data.value = Value;
						data.nOutputID = outputID;

						(this->*pfnInput)( data );
					}
					else if ( dmap->dataDesc[i].flags & FTYPEDESC_KEY )
					{
						// set the value directly
						Value.SetOther( ((char*)this) + dmap->dataDesc[i].fieldOffset[ TD_OFFSET_NORMAL ]);
					
						// TODO: if this becomes evil and causes too many full entity updates, then we should make
						// a macro like this:
						//
						// define MAKE_INPUTVAR(x) void Note##x##Modified() { x.GetForModify(); }
						//
						// Then the datadesc points at that function and we call it here. The only pain is to add
						// that function for all the DEFINE_INPUT calls.
						//NetworkStateChanged();
					}

					//return true;
                    RETURN_META_VALUE(MRES_SUPERCEDE, true);
				}
			}
		}
	}

	//DevMsg( 2, "unhandled input: (%s) -> (%s,%s)\n", szInputName, STRING(m_iClassname), GetDebugName()/*,", from (%s,%s)" STRING(pCaller->m_iClassname), STRING(pCaller->m_iName)*/ );
    RETURN_META_VALUE(MRES_HANDLED, 0);
}

void CAOPEntity::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
    RETURN_META(MRES_IGNORED);
}

bool CAOPEntity::KeyValue( const char *szKeyName, const char *szValue )
{
	// loop through the data description, and try and place the keys in
	for ( datamap_t *dmap = GetDataDescMap(); dmap != NULL; dmap = dmap->baseMap )
	{
		if ( ::ParseKeyvalue(this, dmap->dataDesc, dmap->dataNumFields, szKeyName, szValue) )
			return true;
	}

    keyvalue_t keyval;
    strncpy(keyval.key, szKeyName, sizeof(keyval.key));
    keyval.key[sizeof(keyval.key)-1] = '\0';
    strncpy(keyval.val, szValue, sizeof(keyval.key));
    keyval.val[sizeof(keyval.val)-1] = '\0';
    // don't store the same key twice
    for(unsigned short i = keyValuePairs->Head(); i != keyValuePairs->InvalidIndex(); i = keyValuePairs->Next(i))
    {
        keyvalue_t *storedkv = &keyValuePairs->Element(i);
        if(!strcmp(storedkv->key, keyval.key))
        {
            keyValuePairs->Remove(i);
            break;
        }
    }
    keyValuePairs->AddToTail(keyval);

    RETURN_META_VALUE(MRES_HANDLED, 0);
}

void CAOPEntity::InputCopyDatadesc( inputdata_t &inputdata )
{
    int srcent = inputdata.value.Int();
    if(pAdminOP.myEntList.IsValidIndex(srcent) && pAdminOP.myEntList[srcent] != NULL)
    {
        CAOPEntity *src = pAdminOP.myEntList[srcent];
        datamap_t *datamapdest = GetDataDescMap();
        datamap_t *datamapsrc = src->GetDataDescMap();

        if(datamapdest && datamapsrc)
        {
            pAdminOP.CopyDataMap(this, src, datamapdest, datamapsrc);
        }
    }
}

void CAOPEntity::Teleport(Vector *origin, QAngle *angles)
{
    VFuncs::Teleport(GetBase(), origin, angles, NULL);
}

Vector CAOPEntity::GetAbsOrigin( void )
{
    return VFuncs::GetAbsOrigin(GetBase());
}

Vector CAOPEntity::GetAbsVelocity( void )
{
    return VFuncs::GetAbsVelocity(GetBase());
}

void CAOPEntity::UpdateOnRemove( void )
{
    if(keyValuePairs)
    {
        keyValuePairs->Purge();
        delete keyValuePairs;
        keyValuePairs = NULL;
    }
}

bool TestKeyValue( const char *szKeyName, const char *szValue )
{
    if(!strcmp(szKeyName, "sophooktest"))
    {
        //Msg("You've been hooked already!\n");
        g_hookTest = 1;
        RETURN_META_VALUE(MRES_SUPERCEDE, 0);
    }
    RETURN_META_VALUE(MRES_HANDLED, 0);
}