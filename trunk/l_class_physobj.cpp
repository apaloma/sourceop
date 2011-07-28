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

#include "vfuncs.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "lunar.h"
#include "l_class_entity.h"
#include "l_class_vector.h"
#include "l_class_physobj.h"

extern const char *progname;
extern void l_message (const char *pname, const char *msg);

SOPPhysObj::SOPPhysObj(int index) {
    m_iIndex = index;
}
SOPPhysObj::~SOPPhysObj() {
}
void SOPPhysObj::ApplyForceCenter(SOPVector *forceVector) {
    IPhysicsObject *pPhys = GetPhysicsObject();
    if(pPhys)
    {
        pPhys->ApplyForceCenter(forceVector->ToVector());
    }
}
void SOPPhysObj::EnableCollisions(bool enable) {
    IPhysicsObject *pPhys = GetPhysicsObject();
    if(pPhys)
    {
        pPhys->EnableCollisions(enable);
    }
}
void SOPPhysObj::EnableDrag(bool enable) {
    IPhysicsObject *pPhys = GetPhysicsObject();
    if(pPhys)
    {
        pPhys->EnableDrag(enable);
    }
}
void SOPPhysObj::EnableGravity(bool enable) {
    IPhysicsObject *pPhys = GetPhysicsObject();
    if(pPhys)
    {
        pPhys->EnableGravity(enable);
    }
}
void SOPPhysObj::EnableMotion(bool enable) {
    IPhysicsObject *pPhys = GetPhysicsObject();
    if(pPhys)
    {
        pPhys->EnableMotion(enable);
    }
}
float SOPPhysObj::GetMass(void) {
    IPhysicsObject *pPhys = GetPhysicsObject();
    if(pPhys)
    {
        return pPhys->GetMass();
    }
    return 0;
}
bool SOPPhysObj::IsValid(void) {
    return GetPhysicsObject() != NULL;
}
void SOPPhysObj::SetMass(float mass) {
    IPhysicsObject *pPhys = GetPhysicsObject();
    if(pPhys)
    {
        pPhys->SetMass(mass);
    }
}
IPhysicsObject *SOPPhysObj::GetPhysicsObject(void) {
    CBaseEntity *pEnt = pAdminOP.GetEntity(EntIndex());
    if(pEnt)
    {
        return VFuncs::VPhysicsGetObject(pEnt);
    }
    return NULL;
}

// Lua interface
LUALIB_API int luaL_checkboolean (lua_State *L, int narg);
SOPPhysObj::SOPPhysObj(lua_State *L) {
    m_iIndex = luaL_checkinteger(L, 1);
}
int SOPPhysObj::ApplyForceCenter(lua_State *L) {
    SOPVector *forceVector = Lunar<SOPVector>::check(L, 1);
    ApplyForceCenter(forceVector);
    return 0;
}
int SOPPhysObj::EnableCollisions(lua_State *L) {
    bool enable = luaL_checkboolean(L, 1) != 0;
    EnableCollisions(enable);
    return 0;
}
int SOPPhysObj::EnableDrag(lua_State *L) {
    bool enable = luaL_checkboolean(L, 1) != 0;
    EnableDrag(enable);
    return 0;
}
int SOPPhysObj::EnableGravity(lua_State *L) {
    bool enable = luaL_checkboolean(L, 1) != 0;
    EnableGravity(enable);
    return 0;
}
int SOPPhysObj::EnableMotion(lua_State *L) {
    bool enable = luaL_checkboolean(L, 1) != 0;
    EnableMotion(enable);
    return 0;
}
int SOPPhysObj::GetMass(lua_State *L) {
    lua_pushnumber(L, GetMass());
    return 1;
}
int SOPPhysObj::IsValid(lua_State *L) {
    lua_pushboolean(L, IsValid());
    return 1;
}
int SOPPhysObj::SetMass(lua_State *L) {
    lua_Number mass = luaL_checknumber(L, 1);
    SetMass(mass);
    return 0;
}


const char SOPPhysObj::className[] = "PhysObj";
Lunar<SOPPhysObj>::DerivedType SOPPhysObj::derivedtypes[] = {
    {NULL}
};
Lunar<SOPPhysObj>::DynamicDerivedType SOPPhysObj::dynamicDerivedTypes;

#define method(class, name) {#name, &class::name}

Lunar<SOPPhysObj>::RegType SOPPhysObj::metas[] = {
    {0,0}
};

Lunar<SOPPhysObj>::RegType SOPPhysObj::methods[] = {
    method(SOPPhysObj, ApplyForceCenter),
    method(SOPPhysObj, EnableCollisions),
    method(SOPPhysObj, EnableDrag),
    method(SOPPhysObj, EnableGravity),
    method(SOPPhysObj, EnableMotion),
    method(SOPPhysObj, GetMass),
    method(SOPPhysObj, IsValid),
    method(SOPPhysObj, SetMass),
    {0,0}
};

void lua_SOPPhysObj_register(lua_State *L)
{
    Lunar<SOPPhysObj>::Register(L);
}
