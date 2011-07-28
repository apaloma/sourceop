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

// HACK HACK HACK : BASEENTITY ACCESS
#define GAME_DLL
#include "fixdebug.h"
#include "cbase.h"
#include "baseentity.h"
// HACK HACK HACK : BASEENTITY ACCESS

#include "AdminOP.h"
#include "particle_parse.h"
#include "CAOPEntity.h"

#include "vfuncs.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "lunar.h"
#include "l_class_entity.h"
#include "l_class_vector.h"
#include "l_class_angle.h"
#include "l_class_physobj.h"
#include "l_class_damageinfo.h"
#include "lentitycache.h"

extern const char *progname;
extern void l_message (const char *pname, const char *msg);

SOPEntity::SOPEntity(int index, const char *szClassname) {
    m_iIndex = index;
    m_ClassName = szClassname ? szClassname : className;
    int top = lua_gettop(pAdminOP.GetLuaState());
    lua_newtable(pAdminOP.GetLuaState());
    datatable = lua_ref(pAdminOP.GetLuaState(), true);
    // self.Entity = Entity
    lua_getref(pAdminOP.GetLuaState(), datatable);
    int idx = lua_gettop(pAdminOP.GetLuaState());
    lua_pushliteral(pAdminOP.GetLuaState(), "Entity");
    luaL_getmetatable(pAdminOP.GetLuaState(), m_ClassName);
    //lua_pushliteral(pAdminOP.GetLuaState(), "Entity");
    lua_settable(pAdminOP.GetLuaState(), idx);
    lua_settop(pAdminOP.GetLuaState(), top);
}
SOPEntity::~SOPEntity() {
    lua_unref(pAdminOP.GetLuaState(), datatable);
    //printf("deleted SOPEntity (%p)\n", this);
}
bool SOPEntity::Eq(SOPEntity *other)
{
    if(other)
    {
        return EntIndex() == other->EntIndex();
    }
    return false;
}
void SOPEntity::Activate()
{
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        VFuncs::Activate(pEnt);
    }
}
void SOPEntity::AddEffects(int nEffects)
{
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        VFuncs::AddEffects(pEnt, nEffects);
    }
}
void SOPEntity::AddFlag(int flags)
{
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        VFuncs::AddFlag(pEnt, flags);
    }
}
float SOPEntity::BoundingRadius()
{
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        CCollisionProperty *colprop = VFuncs::CollisionProp(pEnt);
        if(colprop)
        {
            return VFuncs::CCollProp_BoundingRadius(colprop);
        }
    }
    return 0;
}
void SOPEntity::DispatchTraceAttack(SOPDamageInfo *damage, SOPVector *startpos, SOPVector *endpos)
{
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        Vector startvec = startpos->ToVector();
        Vector endvec = endpos->ToVector();
        Vector direction = (endvec - startvec);
        VectorNormalize(direction);

        trace_t tr;
        CTraceFilterHitAll traceFilter;
        Ray_t ray;
        ray.Init( startvec, endvec );
        enginetrace->TraceRay( ray, MASK_NPCSOLID, &traceFilter, &tr );

        ClearMultiDamage();
        pEnt->DispatchTraceAttack(damage->ToCTakeDamageInfo(), direction, &tr);
        ApplyMultiDamage();
    }
}
float SOPEntity::DistanceTo(SOPVector *checkOrigin) {
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        Vector vecOrigin = VFuncs::GetAbsOrigin(pEnt);
        Vector vecCheck = checkOrigin->ToVector();
        return (vecCheck - vecOrigin).Length();
    }
    return 0.0f;
}
void SOPEntity::DrawShadow(bool shouldDraw)
{
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        if(shouldDraw)
        {
            VFuncs::AddEffects(pEnt, EF_NOSHADOW);
        }
        else
        {
            VFuncs::RemoveEffects(pEnt, EF_NOSHADOW);
        }
    }
}
void SOPEntity::EmitSound(const char *pszSound, lua_Number vol, lua_Number pitch) {
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        Vector origin = VFuncs::GetAbsOrigin(pEnt);
        CPASFilter filter( origin );
        enginesound->EmitSound((CRecipientFilter&)filter, VFuncs::entindex(pEnt), CHAN_AUTO, pszSound, vol/100, SNDLVL_NORM, 0, clamp(pitch,1,255), &origin);
    }
}
SOPVector *SOPEntity::EyePos(void) {
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        Vector origin = VFuncs::EyePosition(pEnt);
        return new SOPVector(origin.x, origin.y, origin.z);
    }
    return new SOPVector(0,0,0);
}
int SOPEntity::FindEntityForward(unsigned int mask, Vector *endpos) {
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        Vector forward;
        QAngle playerAngles = VFuncs::EyeAngles(pEnt);

        AngleVectors( playerAngles, &forward, NULL, NULL );

        float distance = 16384;
        Vector start = VFuncs::EyePosition(pEnt);
        Vector end = start + ( forward * distance );

        trace_t tr;
        CTraceFilterSimple traceFilter( pEnt, COLLISION_GROUP_NONE );
        Ray_t ray;
        ray.Init( start, end );
        enginetrace->TraceRay( ray, mask, &traceFilter, &tr );
        if(endpos) *endpos = tr.endpos;
        if ( tr.fraction != 1.0 && tr.DidHitNonWorldEntity() )
        {
            return VFuncs::entindex(tr.m_pEnt);
        }
        return -1;
    }
    return -1;
}
void SOPEntity::Fire(const char *pszInput, const char *pszParam)
{
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        variant_t value;
        value.SetString(MAKE_STRING(pszParam));
        VFuncs::AcceptInput(pEnt, pszInput, pEnt, pEnt, value, 0);
    }
}
int SOPEntity::Flags(void)
{
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        return VFuncs::GetFlags(pEnt);
    }
    return 0;
}
SOPVector *SOPEntity::GetAbsOrigin(void) {
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        Vector origin = VFuncs::GetAbsOrigin(pEnt);
        return new SOPVector(origin.x, origin.y, origin.z);
    }
    return new SOPVector(0,0,0);
}
SOPAngle *SOPEntity::GetAngles(void) {
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        QAngle angles = VFuncs::GetAbsAngles(pEnt);
        return new SOPAngle(angles.x, angles.y, angles.z);
    }
    return new SOPAngle(0,0,0);
}
const char *SOPEntity::GetClassname(void) {
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        return VFuncs::GetClassname(pEnt);
    }
    return "";
}
SOPVector *SOPEntity::GetForward(void) {
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        Vector forward;
        QAngle playerAngles = VFuncs::EyeAngles(pEnt);
        AngleVectors( playerAngles, &forward, NULL, NULL );

        return new SOPVector(forward.x, forward.y, forward.z);
    }
    return new SOPVector(0,0,0);
}
int SOPEntity::GetMaxHealth(void)
{
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        return VFuncs::GetMaxHealth(pEnt);
    }
    return 0;
}
SOPVector *SOPEntity::GetMaxs(void) {
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        CCollisionProperty *pColProp = VFuncs::CollisionProp(pEnt);
        if(pColProp)
        {
            Vector ret = VFuncs::CCollProp_GetMaxs(pColProp);
            return new SOPVector(ret.x, ret.y, ret.z);
        }
    }
    return new SOPVector(0,0,0);
}
SOPVector *SOPEntity::GetMins(void) {
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        CCollisionProperty *pColProp = VFuncs::CollisionProp(pEnt);
        if(pColProp)
        {
            Vector ret = VFuncs::CCollProp_GetMins(pColProp);
            return new SOPVector(ret.x, ret.y, ret.z);
        }
    }
    return new SOPVector(0,0,0);
}
const char *SOPEntity::GetModel(void) {
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        return STRING(VFuncs::GetModelName(pEnt));
    }
    return "";
}
const char *SOPEntity::GetName(void) {
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        return STRING(VFuncs::GetEntityName(pEnt));
    }
    return "";
}
unsigned char SOPEntity::GetOffsetByte(int offset) {
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        DECLARE_VAR_RVAL(unsigned char, var, pEnt, offset, 0);
        return *var;
    }
    return 0;
}
int SOPEntity::GetOffsetEntity(int offset) {
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        DECLARE_VAR_RVAL(CHandle<CBaseEntity>, var, pEnt, offset, 0);
        CBaseEntity *pEnt = *var;
        if(pEnt)
        {
            return VFuncs::entindex(pEnt);
        }
    }
    return -1;
}
float SOPEntity::GetOffsetFloat(int offset) {
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        DECLARE_VAR_RVAL(float, var, pEnt, offset, 0.0);
        return *var;
    }
    return 0.0; // 0.o   o.0   ^_^
}
int SOPEntity::GetOffsetInt(int offset) {
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        DECLARE_VAR_RVAL(int, var, pEnt, offset, 0);
        return *var;
    }
    return 0;
}
int SOPEntity::GetOwner(void) {
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        CBaseEntity *pOwner = VFuncs::GetOwnerEntity(pEnt);
        if(pOwner)
        {
            return VFuncs::entindex(pOwner);
        }
    }
    return -1;
}
int SOPEntity::GetSpeed(void) {
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        Vector velocity = VFuncs::GetAbsVelocity(pEnt);
        return velocity.Length();
    }
    return 0;
}
SOPVector *SOPEntity::GetVelocity(void) {
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        Vector velocity = VFuncs::GetAbsVelocity(pEnt);
        return new SOPVector(velocity.x, velocity.y, velocity.z);
    }
    return new SOPVector(0,0,0);
}
int SOPEntity::Health(void)
{
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        return VFuncs::GetHealth(pEnt);
    }
    return 0;
}
bool SOPEntity::IsNPC(void)
{
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        return VFuncs::IsNPC(pEnt);
    }
    return false;
}
bool SOPEntity::IsPlayer(void)
{
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        return VFuncs::IsPlayer(pEnt);
    }
    return false;
}
bool SOPEntity::IsValid(void)
{
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        return true;
    }
    return false;
}
bool SOPEntity::KeyValue(const char *szKeyName, const char *szValue)
{
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        return VFuncs::KeyValue(pEnt, szKeyName, szValue);
    }
    return false;
}
void SOPEntity::Kill(void) {
    if(EntIndex() <= pAdminOP.GetMaxClients())
    {
        l_message(progname, UTIL_VarArgs("Deletion of entity %i not allowed.", m_iIndex));
        return;
    }

    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        UTIL_Remove(pEnt);
        return;
    }
}
SOPVector *SOPEntity::LocalToWorld(SOPVector *localPosition) {
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt && localPosition)
    {
        IPhysicsObject *pObj = VFuncs::VPhysicsGetObject(pEnt);
        if(pObj)
        {
            Vector world;
            Vector local = localPosition->ToVector();
            pObj->LocalToWorld(&world, local);

            return new SOPVector(world.x, world.y, world.z);
        }
    }
    return new SOPVector(0,0,0);
}
void SOPEntity::NextThink(float thinktime) {
    for(int i = 0; i < pAdminOP.myThinkEnts.Count(); i++)
    {
        CAOPEntity *pAOPEntity = pAdminOP.myThinkEnts[i];
        CBaseEntity *pBase = pAOPEntity->GetBase();
        if(pBase && pAOPEntity->GetIndex() == EntIndex())
        {
            pAOPEntity->SetNextThink(thinktime);
            return;
        }
    }

    //TODO: NextThink for non-sop entities
    CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] NextThink is not implemented for non-SourceOP entities.\n");
}
SOPVector *SOPEntity::OBBCenter(void) {
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        CCollisionProperty *colprop = VFuncs::CollisionProp(pEnt);
        if(colprop)
        {
            Vector center = VFuncs::CCollProp_OBBCenter(colprop);
    	    return new SOPVector(center.x, center.y, center.z);
        }
    }
    return new SOPVector(0,0,0);
}
void SOPEntity::PhysicsInit(int solidType)
{
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        if(solidType >= 0 && solidType < SOLID_LAST)
        {
            pEnt->VPhysicsInitNormal((SolidType_t)solidType, 0, false);
        }
    }
}
void SOPEntity::SetAbsOrigin(SOPVector *absOrigin)
{
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        Vector origin = absOrigin->ToVector();
        VFuncs::SetAbsOrigin(pEnt, origin);
    }
}
void SOPEntity::SetAngles(SOPAngle *absAngles)
{
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        QAngle angles = absAngles->ToQAngle();
        VFuncs::SetAbsAngles(pEnt, angles);
    }
}
void SOPEntity::SetCollisionGroup(int collisionGroup)
{
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        VFuncs::SetCollisionGroup(pEnt, collisionGroup);
    }
}
void SOPEntity::SetGravity(double gravity)
{
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        VFuncs::SetGravity(pEnt, gravity);
    }
}
void SOPEntity::SetHealth(int health)
{
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        VFuncs::SetHealth(pEnt, health);
    }
}
void SOPEntity::SetLocalAngles(SOPAngle *locAngles)
{
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        QAngle angles = locAngles->ToQAngle();
        VFuncs::SetLocalAngles(pEnt, angles);
    }
}
void SOPEntity::SetLocalOrigin(SOPVector *locOrigin)
{
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        Vector origin = locOrigin->ToVector();
        VFuncs::SetLocalOrigin(pEnt, origin);
    }
}
void SOPEntity::SetModel(const char *ModelName)
{
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        VFuncs::SetModel(pEnt, ModelName);
    }
}
void SOPEntity::SetMoveType(int movetype)
{
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        VFuncs::SetMoveType(pEnt, (MoveType_t)movetype);
    }
}
void SOPEntity::SetOffsetByte(int offset, int val) {
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        DECLARE_VAR(unsigned char, playerVar, pEnt, offset);
        *playerVar = val;
    }
}
void SOPEntity::SetOffsetEnt(int offset, SOPEntity *val) {
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        CBaseEntity *pVal = pAdminOP.GetEntity(val->EntIndex());
        DECLARE_VAR(EHANDLE, playerVar, pEnt, offset);
        playerVar->Set(pVal);
    }
}
void SOPEntity::SetOffsetFloat(int offset, float val) {
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        DECLARE_VAR(float, playerVar, pEnt, offset);
        *playerVar = val;
    }
}
void SOPEntity::SetOffsetInt(int offset, int val) {
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        DECLARE_VAR(int, playerVar, pEnt, offset);
        *playerVar = val;
    }
}
void SOPEntity::SetOwner(SOPEntity *pOwner) {
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        CBaseEntity *pEntOwner = pAdminOP.GetEntity(pOwner->EntIndex());
        if(pEntOwner)
        {
            VFuncs::SetOwnerEntity(pEnt, pEntOwner);
        }
    }
}
void SOPEntity::SetParent(SOPEntity *pParent, int attachment) {
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        if(pParent)
        {
            CBaseEntity *pEntParent = pAdminOP.GetEntity(pParent->EntIndex());
            if(pEntParent)
            {
                VFuncs::SetParent(pEnt, pEntParent, attachment);
            }
        }
        else
        {
            VFuncs::SetParent(pEnt, NULL);
        }
    }
}
void SOPEntity::SetRenderAmt(int val)
{
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        VFuncs::SetRenderColorA(pEnt, val);
    }
}
void SOPEntity::SetRenderMode(int val)
{
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        VFuncs::SetRenderMode(pEnt, val);
    }
}
void SOPEntity::SetSolid(int val)
{
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        VFuncs::SetSolid(pEnt, (SolidType_t)val);
    }
}
void SOPEntity::SetTrigger(bool enabled)
{
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        if(enabled)
        {
            VFuncs::AddSolidFlags(pEnt, FSOLID_TRIGGER);
            engine->TriggerMoved(VFuncs::GetEdict(VFuncs::GetNetworkable(pEnt)), false);
        }
        else
        {
            VFuncs::RemoveSolidFlags(pEnt, FSOLID_TRIGGER);
        }
    }
}
void SOPEntity::SetVelocity(SOPVector *absVelocity)
{
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        Vector velocity = absVelocity->ToVector();
        VFuncs::SetAbsVelocity(pEnt, velocity);
    }
}
void SOPEntity::Spawn(void)
{
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        VFuncs::Spawn(pEnt);
    }
}
void SOPEntity::StateChanged(unsigned short offset)
{
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        VFuncs::SetEdictStateChanged(servergameents->BaseEntityToEdict(pEnt), offset);
    }
}
void SOPEntity::StopParticleEffects(void)
{
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        ::StopParticleEffects(pEnt);
    }
}
bool SOPEntity::Visible(SOPEntity *pOther)
{
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        CBaseEntity *pEntOther = pAdminOP.GetEntity(pOther->EntIndex());
        if(pEntOther)
        {
            return VFuncs::FVisible(pEnt, pEntOther);
        }
    }
    return false;
}

// Lua interface
LUALIB_API int luaL_checkboolean (lua_State *L, int narg);
SOPEntity::SOPEntity(lua_State *L) {
    m_iIndex = luaL_checkinteger(L, 1);
}
int SOPEntity::__eq(lua_State *L) {
    lua_pushboolean(L, Eq(Lunar<SOPEntity>::check(L, 1)));
    return 1;
}
int SOPEntity::__index(lua_State *L) {
    int top = lua_gettop(L);
    
    lua_getref(pAdminOP.GetLuaState(), datatable);
    lua_pushvalue(L, 1);
    lua_rawget(pAdminOP.GetLuaState(), -2);
    if(!lua_isnil(L, -1))
    {
        return 1;
    }
    lua_settop(L, top);

    luaL_getmetatable(L, m_ClassName);  // lookup metatable in Lua registry
    if (lua_isnil(L, -1)) luaL_error(L, "%s missing metatable", m_ClassName);
    lua_pushliteral(L, "__metatable");
    lua_rawget(pAdminOP.GetLuaState(), -2);
    if(lua_isnil(L, -1)) luaL_error(L, "%s's metatable has no __metatable", m_ClassName);
    lua_pushvalue(L, 1);
    lua_rawget(pAdminOP.GetLuaState(), -2);
    if(!lua_isnil(L, -1))
    {
        //Msg("    Found in __metatable.\n");
        return 1;
    }
    lua_settop(L, top);

    lua_pushnil(L);
    return 1;
}
int SOPEntity::__newindex(lua_State *L) {
    lua_getref(pAdminOP.GetLuaState(), datatable);
    int table = lua_gettop(pAdminOP.GetLuaState());
    lua_pushvalue(pAdminOP.GetLuaState(), 1);
    lua_pushvalue(pAdminOP.GetLuaState(), 2);
    lua_settable(pAdminOP.GetLuaState(), table);
    return 0;
}
int SOPEntity::Activate(lua_State *L) {
    Activate();
    return 0;
}
int SOPEntity::AddEffects(lua_State *L) {
    AddEffects(luaL_checkinteger(L, 1));
    return 0;
}
int SOPEntity::AddFlag(lua_State *L) {
    AddFlag(luaL_checkinteger(L, 1));
    return 0;
}
int SOPEntity::BoundingRadius(lua_State *L) {
    lua_pushnumber(L, BoundingRadius());
    return 1;
}
int SOPEntity::DispatchTraceAttack(lua_State *L) {
    SOPDamageInfo *damage = Lunar<SOPDamageInfo>::check(L, 1);
    SOPVector *startpos = Lunar<SOPVector>::check(L, 2);
    SOPVector *endpos = Lunar<SOPVector>::check(L, 3);

    DispatchTraceAttack(damage, startpos, endpos);
    return 0;
}
int SOPEntity::DistanceTo(lua_State *L) {
    SOPVector *origin = Lunar<SOPVector>::check(L, 1);
    lua_pushnumber(L, DistanceTo(origin));
    return 1;

}
int SOPEntity::DrawShadow(lua_State *L) {
    DrawShadow(luaL_checkboolean(L, 1) != 0);
    return 0;
}
int SOPEntity::EmitSound(lua_State *L) {
    size_t l;
    const char *s = luaL_checklstring(L, 1, &l);
    lua_Number vol = luaL_checknumber(L, 2);
    lua_Number pitch = luaL_checknumber(L, 3);

    EmitSound(s, vol, pitch);
    return 0;
}
int SOPEntity::EntIndex(lua_State *L) {
    lua_pushinteger(L, EntIndex());
    return 1;
}
int SOPEntity::EyePos(lua_State *L) {
    SOPVector *vec = EyePos();
    if(vec)
    {
        Lunar<SOPVector>::push(L, vec, true);
        return 1;
    }
    lua_pushnil(L);
    return 1;

}
int SOPEntity::FindEntityForward(lua_State *L) {
    unsigned int mask = luaL_optinteger(L, 1, MASK_PLAYERSOLID);
    Vector endpos;

    int ent = FindEntityForward(mask, &endpos);
    if(ent > -1)
    {
        g_entityCache.PushEntity(ent);
        Lunar<SOPVector>::push(L, new SOPVector(endpos.x, endpos.y, endpos.z), true); // true to allow garbage collection
        return 2;
    }
    lua_pushnil(L);
    lua_pushnil(L);
    return 2;
}
int SOPEntity::Fire(lua_State *L) {
    Fire(luaL_checkstring(L, 1), luaL_checkstring(L, 2));
    return 1;
}
int SOPEntity::Flags(lua_State *L) {
    lua_pushinteger(L, Flags());
    return 1;
}
int SOPEntity::GetAbsOrigin(lua_State *L) {
    SOPVector *vec = GetAbsOrigin();
    if(vec)
    {
        Lunar<SOPVector>::push(L, vec, true);
        return 1;
    }
    lua_pushnil(L);
    return 1;

}
int SOPEntity::GetAngles(lua_State *L) {
    SOPAngle *ang = GetAngles();
    if(ang)
    {
        Lunar<SOPAngle>::push(L, ang, true);
        return 1;
    }
    lua_pushnil(L);
    return 1;

}
int SOPEntity::GetClassname(lua_State *L) {
    lua_pushstring(L, GetClassname());
    return 1;
}
int SOPEntity::GetForward(lua_State *L) {
    SOPVector *vec = GetForward();
    if(vec)
    {
        Lunar<SOPVector>::push(L, vec, true);
        return 1;
    }
    lua_pushnil(L);
    return 1;

}
int SOPEntity::GetMaxHealth(lua_State *L) {
    lua_pushinteger(L, GetMaxHealth());
    return 1;
}
int SOPEntity::GetMaxs(lua_State *L) {
    SOPVector *vec = GetMaxs();
    if(vec)
    {
        Lunar<SOPVector>::push(L, vec, true);
        return 1;
    }
    lua_pushnil(L);
    return 1;

}
int SOPEntity::GetMins(lua_State *L) {
    SOPVector *vec = GetMins();
    if(vec)
    {
        Lunar<SOPVector>::push(L, vec, true);
        return 1;
    }
    lua_pushnil(L);
    return 1;

}
int SOPEntity::GetModel(lua_State *L) {
    lua_pushstring(L, GetModel());
    return 1;
}
int SOPEntity::GetName(lua_State *L) {
    lua_pushstring(L, GetName());
    return 1;
}
int SOPEntity::GetOffsetByte(lua_State *L) {
    lua_pushinteger(L, GetOffsetByte(luaL_checkinteger(L, 1)));
    return 1;
}
int SOPEntity::GetOffsetEntity(lua_State *L) {
    int entity = GetOffsetEntity(luaL_checkinteger(L, 1));
    if(entity > -1)
    {
        g_entityCache.PushEntity(entity);
    }
    else
    {
        lua_pushnil(L);
    }

    return 1;
}
int SOPEntity::GetOffsetFloat(lua_State *L) {
    lua_pushnumber(L, GetOffsetFloat(luaL_checkinteger(L, 1)));
    return 1;
}
int SOPEntity::GetOffsetInt(lua_State *L) {
    lua_pushinteger(L, GetOffsetInt(luaL_checkinteger(L, 1)));
    return 1;
}
int SOPEntity::GetOwner(lua_State *L) {
    int owner = GetOwner();
    if(owner > -1)
    {
        g_entityCache.PushEntity(owner);
    }
    else
    {
        lua_pushnil(L);
    }

    return 1;
}
int SOPEntity::GetSpeed(lua_State *L) {
    lua_pushinteger(L, GetSpeed());
    return 1;
}
int SOPEntity::GetVelocity(lua_State *L) {
    SOPVector *vec = GetVelocity();
    if(vec)
    {
        Lunar<SOPVector>::push(L, vec, true);
        return 1;
    }
    lua_pushnil(L);
    return 1;

}
int SOPEntity::GetPhysicsObject(lua_State *L) {
    g_entityCache.PushPhysObj(EntIndex());
    return 1;
}
int SOPEntity::Health(lua_State *L) {
    lua_pushinteger(L, Health());
    return 1;
}
int SOPEntity::IsNPC(lua_State *L) {
    lua_pushboolean(L, IsNPC());
    return 1;
}
int SOPEntity::IsPlayer(lua_State *L) {
    lua_pushboolean(L, IsPlayer());
    return 1;
}
int SOPEntity::IsValid(lua_State *L) {
    lua_pushboolean(L, IsValid());
    return 1;
}
int SOPEntity::KeyValue(lua_State *L) {
    lua_pushboolean(L, KeyValue(luaL_checkstring(L, 1), luaL_checkstring(L, 2)));
    return 1;
}
int SOPEntity::Kill(lua_State *L) {
    Kill();
    return 0;
}
int SOPEntity::LocalToWorld(lua_State *L) {
    SOPVector *localPosition = Lunar<SOPVector>::check(L, 1);
    SOPVector *vec = LocalToWorld(localPosition);
    if(vec)
    {
        Lunar<SOPVector>::push(L, vec, true);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}
int SOPEntity::NextThink(lua_State *L) {
    lua_Number thinktime = luaL_checknumber(L, 1);
    NextThink(thinktime);
    return 0;
}
int SOPEntity::OBBCenter(lua_State *L) {
    SOPVector *vec = OBBCenter();
    if(vec)
    {
        Lunar<SOPVector>::push(L, vec, true);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}
int SOPEntity::PhysicsInit(lua_State *L) {
    PhysicsInit(luaL_checkinteger(L, 1));
    return 0;
}
int SOPEntity::SetAbsOrigin(lua_State *L) {
    SOPVector *absOrigin = Lunar<SOPVector>::check(L, 1);
    SetAbsOrigin(absOrigin);
    return 0;
}
int SOPEntity::SetAngles(lua_State *L) {
    SOPAngle *absAngles = Lunar<SOPAngle>::check(L, 1);
    SetAngles(absAngles);
    return 0;
}
int SOPEntity::SetCollisionGroup(lua_State *L) {
    SetCollisionGroup(luaL_checkinteger(L, 1));
    return 0;
}
int SOPEntity::SetGravity(lua_State *L) {
    SetGravity(luaL_checknumber(L, 1));
    return 0;
}
int SOPEntity::SetHealth(lua_State *L) {
    SetHealth(luaL_checkinteger(L, 1));
    return 0;
}
int SOPEntity::SetLocalAngles(lua_State *L) {
    SOPAngle *locAngles = Lunar<SOPAngle>::check(L, 1);
    SetLocalAngles(locAngles);
    return 0;
}
int SOPEntity::SetLocalOrigin(lua_State *L) {
    SOPVector *locOrigin = Lunar<SOPVector>::check(L, 1);
    SetLocalOrigin(locOrigin);
    return 0;
}
int SOPEntity::SetModel(lua_State *L) {
    SetModel(luaL_checkstring(L, 1));
    return 0;
}
int SOPEntity::SetMoveType(lua_State *L) {
    SetMoveType(luaL_checkinteger(L, 1));
    return 0;
}
int SOPEntity::SetOffsetByte(lua_State *L) {
    int offset = luaL_checkinteger(L, 1);
    int val = luaL_checkinteger(L, 2);
    SetOffsetByte(offset, val);
    return 0;
}
int SOPEntity::SetOffsetEnt(lua_State *L) {
    int offset = luaL_checkinteger(L, 1);
    SOPEntity *val = Lunar<SOPEntity>::check(L, 2);
    SetOffsetEnt(offset, val);
    return 0;
}
int SOPEntity::SetOffsetFloat(lua_State *L) {
    int offset = luaL_checkinteger(L, 1);
    float val = luaL_checknumber(L, 2);
    SetOffsetFloat(offset, val);
    return 0;
}
int SOPEntity::SetOffsetInt(lua_State *L) {
    int offset = luaL_checkinteger(L, 1);
    int val = luaL_checkinteger(L, 2);
    SetOffsetInt(offset, val);
    return 0;
}
int SOPEntity::SetOwner(lua_State *L) {
    SOPEntity *owner = Lunar<SOPEntity>::check(L, 1);
    SetOwner(owner);
    return 0;
}
int SOPEntity::SetParent(lua_State *L) {
    SOPEntity *parent = NULL;
    if(!lua_isnil(L, 1))
    {
        parent = Lunar<SOPEntity>::check(L, 1);
    }

    int attachment = luaL_optinteger(L, 2, -1);

    SetParent(parent, attachment);
    return 0;
}
int SOPEntity::SetRenderAmt(lua_State *L) {
    SetRenderAmt(luaL_checkinteger(L, 1));
    return 0;
}
int SOPEntity::SetRenderMode(lua_State *L) {
    SetRenderMode(luaL_checkinteger(L, 1));
    return 0;
}
int SOPEntity::SetSolid(lua_State *L) {
    SetSolid(luaL_checkinteger(L, 1));
    return 0;
}
int SOPEntity::SetTrigger(lua_State *L) {
    SetTrigger(luaL_checkboolean(L, 1) != 0);
    return 0;
}
int SOPEntity::SetVelocity(lua_State *L) {
    SOPVector *absVelocity = Lunar<SOPVector>::check(L, 1);
    SetVelocity(absVelocity);
    return 0;
}
int SOPEntity::Spawn(lua_State *L) {
    Spawn();
    return 0;
}
int SOPEntity::StateChanged(lua_State *L) {
    unsigned short offset = luaL_optinteger(L, 1, 0);
    StateChanged(offset);
    return 0;
}
int SOPEntity::StopParticleEffects(lua_State *L) {
    StopParticleEffects();
    return 0;
}
int SOPEntity::Visible(lua_State *L) {
    SOPEntity *other = Lunar<SOPEntity>::check(L, 1);
    lua_pushboolean(L, Visible(other));
    return 1;
}

const char SOPEntity::className[] = "Entity";
Lunar<SOPEntity>::DerivedType SOPEntity::derivedtypes[] = {
    {"Player"},
    {NULL}
};
Lunar<SOPEntity>::DynamicDerivedType SOPEntity::dynamicDerivedTypes;

#define method(class, name) {#name, &class::name}

Lunar<SOPEntity>::RegType SOPEntity::metas[] = {
    method(SOPEntity, __eq),
    method(SOPEntity, __index),
    method(SOPEntity, __newindex),
    {0,0}
};

Lunar<SOPEntity>::RegType SOPEntity::methods[] = {
    method(SOPEntity, Activate),
    method(SOPEntity, AddEffects),
    method(SOPEntity, AddFlag),
    method(SOPEntity, BoundingRadius),
    method(SOPEntity, DispatchTraceAttack),
    method(SOPEntity, DistanceTo),
    method(SOPEntity, DrawShadow),
    method(SOPEntity, EmitSound),
    method(SOPEntity, EntIndex),
    method(SOPEntity, EyePos),
    method(SOPEntity, FindEntityForward),
    method(SOPEntity, Fire),
    method(SOPEntity, Flags),
    method(SOPEntity, GetAbsOrigin),
    method(SOPEntity, GetAngles),
    {"GetClass",    &SOPEntity::GetClassname},
    method(SOPEntity, GetClassname),
    {"GetFlags",    &SOPEntity::Flags},
    method(SOPEntity, GetForward),
    method(SOPEntity, GetMaxHealth),
    method(SOPEntity, GetMaxs),
    method(SOPEntity, GetMins),
    method(SOPEntity, GetModel),
    method(SOPEntity, GetName),
    method(SOPEntity, GetOffsetByte),
    method(SOPEntity, GetOffsetEntity),
    method(SOPEntity, GetOffsetFloat),
    method(SOPEntity, GetOffsetInt),
    method(SOPEntity, GetOwner),
    {"GetPos",      &SOPEntity::GetAbsOrigin},
    method(SOPEntity, GetPhysicsObject),
    method(SOPEntity, GetSpeed),
    method(SOPEntity, GetVelocity),
    method(SOPEntity, Health),
    method(SOPEntity, IsNPC),
    method(SOPEntity, IsPlayer),
    method(SOPEntity, IsValid),
    method(SOPEntity, KeyValue),
    method(SOPEntity, Kill),
    method(SOPEntity, LocalToWorld),
    method(SOPEntity, NextThink),
    method(SOPEntity, OBBCenter),
    method(SOPEntity, PhysicsInit),
    {"Remove",      &SOPEntity::Kill},
    method(SOPEntity, SetAbsOrigin),
    method(SOPEntity, SetAngles),
    method(SOPEntity, SetCollisionGroup),
    method(SOPEntity, SetGravity),
    method(SOPEntity, SetHealth),
    method(SOPEntity, SetLocalAngles),
    method(SOPEntity, SetLocalOrigin),
    method(SOPEntity, SetModel),
    method(SOPEntity, SetMoveType),
    method(SOPEntity, SetOffsetByte),
    method(SOPEntity, SetOffsetEnt),
    method(SOPEntity, SetOffsetFloat),
    method(SOPEntity, SetOffsetInt),
    method(SOPEntity, SetOwner),
    method(SOPEntity, SetParent),
    {"SetPos",      &SOPEntity::SetAbsOrigin},
    method(SOPEntity, SetRenderAmt),
    method(SOPEntity, SetRenderMode),
    method(SOPEntity, SetSolid),
    method(SOPEntity, SetTrigger),
    method(SOPEntity, SetVelocity),
    method(SOPEntity, Spawn),
    method(SOPEntity, StateChanged),
    method(SOPEntity, StopParticleEffects),
    method(SOPEntity, Visible),
    {0,0}
};

void lua_SOPEntity_register(lua_State *L)
{
    Lunar<SOPEntity>::Register(L);
}
