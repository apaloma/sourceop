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

//#include "vfuncs.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "lunar.h"
#include "l_class_damageinfo.h"
#include "l_class_entity.h"

SOPDamageInfo::SOPDamageInfo(SOPEntity *inflictor, SOPEntity *attacker, lua_Number damage, lua_Integer damageType) {
    Set(inflictor, attacker, damage, damageType);
}
SOPDamageInfo::~SOPDamageInfo() {
}
void SOPDamageInfo::Set(SOPEntity *inflictor, SOPEntity *attacker, lua_Number damage, lua_Integer damageType){
    CBaseEntity *pInflictor = pAdminOP.GetEntity(inflictor->EntIndex());
    CBaseEntity *pAttacker = pAdminOP.GetEntity(attacker->EntIndex());
    this->damageInfo = CTakeDamageInfo(pInflictor, pAttacker, damage, damageType);
}
void SOPDamageInfo::AddDamage(int damage) {
    this->damageInfo.AddDamage(damage);
}
void SOPDamageInfo::SetDamageCustom(int iDamageCustom) {
    this->damageInfo.SetDamageCustom(iDamageCustom);
}

CTakeDamageInfo SOPDamageInfo::ToCTakeDamageInfo() {
    return this->damageInfo;
}

// Lua interface
SOPDamageInfo::SOPDamageInfo(lua_State *L) {
    SOPEntity *inflictor = Lunar<SOPEntity>::check(L, 1);
    SOPEntity *attacker = Lunar<SOPEntity>::check(L, 2);
    lua_Number damage = luaL_checknumber(L, 3);
    lua_Integer damageType = luaL_checkinteger(L, 4);

    Set(inflictor, attacker, damage, damageType);
}
int SOPDamageInfo::AddDamage(lua_State *L) {
    int damage = luaL_checkinteger(L, 1);
    AddDamage(damage);
    return 0;
}
int SOPDamageInfo::SetDamageCustom(lua_State *L) {
    int iDamageCustom = luaL_checkinteger(L, 1);
    SetDamageCustom(iDamageCustom);
    return 0;
}


const char SOPDamageInfo::className[] = "DamageInfo";
Lunar<SOPDamageInfo>::DerivedType SOPDamageInfo::derivedtypes[] = {
    {NULL}
};
Lunar<SOPDamageInfo>::DynamicDerivedType SOPDamageInfo::dynamicDerivedTypes;

#define method(class, name) {#name, &class::name}

template <>
int Lunar<SOPDamageInfo>::tostring_T (lua_State *L) {
    userdataType *ud = static_cast<userdataType*>(lua_touserdata(L, 1));
    SOPDamageInfo *obj = ud->pT;
    //lua_pushstring(L, UTIL_VarArgs("%f %f %f", obj->x, obj->y, obj->z));
    lua_pushstring(L, "DamageInfo");
    return 1; 
}

Lunar<SOPDamageInfo>::RegType SOPDamageInfo::metas[] = {
    {0,0}
};

Lunar<SOPDamageInfo>::RegType SOPDamageInfo::methods[] = {
    method(SOPDamageInfo, AddDamage),
    method(SOPDamageInfo, SetDamageCustom),
    {0,0}
};

void lua_SOPDamageInfo_register(lua_State *L)
{
    Lunar<SOPDamageInfo>::Register(L);
}
