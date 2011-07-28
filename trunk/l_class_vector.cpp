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

#include "tier1/strtools.h"
#include "utlvector.h"
#include "mathlib/vmatrix.h"

#include "shareddefs.h"

#include "util.h"

//#include "AdminOP.h"

//#include "vfuncs.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "lunar.h"
#include "l_class_vector.h"
#include "l_class_angle.h"

SOPVector::SOPVector(lua_Number x, lua_Number y, lua_Number z) {
    this->x = x;
    this->y = y;
    this->z = z;
}
SOPVector::~SOPVector() {
}
SOPVector *SOPVector::Add(SOPVector *vec) {
    if(vec)
        return new SOPVector(x+vec->x, y+vec->y, z+vec->z);
    return NULL;
}
SOPVector *SOPVector::Div(lua_Number div) {
    return new SOPVector(x/div, y/div, z/div);
}
bool SOPVector::Eq(SOPVector *vec) {
    if(vec)
        return x == vec->x && y == vec->y && z == vec->z;
    return false;
}
SOPVector *SOPVector::Mul(lua_Number mult) {
    return new SOPVector(x*mult, y*mult, z*mult);
}
SOPVector *SOPVector::Sub(SOPVector *vec) {
    if(vec)
        return new SOPVector(x-vec->x, y-vec->y, z-vec->z);
    return NULL;
}
SOPAngle *SOPVector::Angle(void) {
    QAngle angles;

    VectorAngles(ToVector(), angles);
    return new SOPAngle(angles.x, angles.y, angles.z);
}
SOPVector *SOPVector::GetNormal(void) {
    Vector normal = ToVector();

    VectorNormalize(normal);
    return new SOPVector(normal.x, normal.y, normal.z);
}
lua_Number SOPVector::Length(void) {
    return ToVector().Length();
}
Vector SOPVector::ToVector(void) {
    return Vector(x, y, z);
}

// Lua interface
SOPVector::SOPVector(lua_State *L) {
    x = luaL_checknumber(L, 1);
    y = luaL_checknumber(L, 2);
    z = luaL_checknumber(L, 3);
}
int SOPVector::__add(lua_State *L) {
    SOPVector *vec = Add(Lunar<SOPVector>::check(L, 1));
    if(vec)
    {
        Lunar<SOPVector>::push(L, vec, true);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}
int SOPVector::__div(lua_State *L) {
    SOPVector *vec = Div(luaL_checknumber(L, 1));
    if(vec)
    {
        Lunar<SOPVector>::push(L, vec, true);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}
int SOPVector::__eq(lua_State *L) {
    lua_pushboolean(L, Eq(Lunar<SOPVector>::check(L, 1)));
    return 1;
}
int SOPVector::__mul(lua_State *L) {
    SOPVector *vec = Mul(luaL_checknumber(L, 1));
    if(vec)
    {
        Lunar<SOPVector>::push(L, vec, true);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}
int SOPVector::__sub(lua_State *L) {
    SOPVector *vec = Sub(Lunar<SOPVector>::check(L, 1));
    if(vec)
    {
        Lunar<SOPVector>::push(L, vec, true);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}
int SOPVector::GetX(lua_State *L) {
    lua_pushnumber(L, x);
    return 1;
}
int SOPVector::GetY(lua_State *L) {
    lua_pushnumber(L, y);
    return 1;
}
int SOPVector::GetZ(lua_State *L) {
    lua_pushnumber(L, z);
    return 1;
}
int SOPVector::Angle(lua_State *L) {
    SOPAngle *ang = Angle();
    if(ang)
    {
        Lunar<SOPAngle>::push(L, ang, true);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}
int SOPVector::GetNormal(lua_State *L) {
    SOPVector *normal = GetNormal();
    if(normal)
    {
        Lunar<SOPVector>::push(L, normal, true);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}
int SOPVector::Length(lua_State *L) {
    lua_pushnumber(L, Length());
    return 1;
}

const char SOPVector::className[] = "Vector";
Lunar<SOPVector>::DerivedType SOPVector::derivedtypes[] = {
    {NULL}
};
Lunar<SOPVector>::DynamicDerivedType SOPVector::dynamicDerivedTypes;

#define method(class, name) {#name, &class::name}

template <>
int Lunar<SOPVector>::tostring_T (lua_State *L) {
    userdataType *ud = static_cast<userdataType*>(lua_touserdata(L, 1));
    SOPVector *obj = ud->pT;
    lua_pushstring(L, UTIL_VarArgs("%f %f %f", obj->x, obj->y, obj->z));
    return 1; 
}

Lunar<SOPVector>::RegType SOPVector::metas[] = {
    method(SOPVector, __add),
    method(SOPVector, __div),
    method(SOPVector, __eq),
    method(SOPVector, __mul),
    method(SOPVector, __sub),
    {0,0}
};

Lunar<SOPVector>::RegType SOPVector::methods[] = {
    method(SOPVector, GetX),
    method(SOPVector, GetY),
    method(SOPVector, GetZ),
    method(SOPVector, Angle),
    method(SOPVector, GetNormal),
    method(SOPVector, Length),
    {0,0}
};

void lua_SOPVector_register(lua_State *L)
{
    Lunar<SOPVector>::Register(L);
}
