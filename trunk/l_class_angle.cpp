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
#include "l_class_angle.h"
#include "l_class_vector.h"

SOPAngle::SOPAngle(lua_Number x, lua_Number y, lua_Number z) {
    this->x = x;
    this->y = y;
    this->z = z;
}
SOPAngle::~SOPAngle() {
}
SOPAngle *SOPAngle::Add(SOPAngle *ang) {
    if(ang)
        return new SOPAngle(x+ang->x, y+ang->y, z+ang->z);
    return NULL;
}
SOPAngle *SOPAngle::Div(lua_Number div) {
    return new SOPAngle(x/div, y/div, z/div);
}
bool SOPAngle::Eq(SOPAngle *ang) {
    if(ang)
        return x == ang->x && y == ang->y && z == ang->z;
    return false;
}
SOPAngle *SOPAngle::Mul(lua_Number mult) {
    return new SOPAngle(x*mult, y*mult, z*mult);
}
SOPAngle *SOPAngle::Sub(SOPAngle *ang) {
    if(ang)
        return new SOPAngle(x-ang->x, y-ang->y, z-ang->z);
    return NULL;
}
SOPVector *SOPAngle::Forward(void) {
    Vector forward;
    AngleVectors( ToQAngle(), &forward, NULL, NULL );
    return new SOPVector(forward.x, forward.y, forward.z);
}
SOPVector *SOPAngle::Right(void) {
    Vector right;
    AngleVectors( ToQAngle(), NULL, &right, NULL );
    return new SOPVector(right.x, right.y, right.z);
}
SOPAngle *SOPAngle::RotateAroundAxis(SOPVector *vec, lua_Number degrees) {
    matrix3x4_t     m_rgflCoordinateFrame;
    Vector          rotationAxisLs;
    Quaternion      q;
    matrix3x4_t     xform;
    matrix3x4_t     localToWorldMatrix;
    Vector          axisvector = vec->ToVector();
    QAngle          rotatedAngles;

    QAngle          angOurAngs = ToQAngle();
    AngleMatrix( angOurAngs, m_rgflCoordinateFrame );
    VectorIRotate( axisvector, m_rgflCoordinateFrame, rotationAxisLs );
    AxisAngleQuaternion( rotationAxisLs, degrees, q );
    QuaternionMatrix( q, vec3_origin, xform );
    ConcatTransforms( m_rgflCoordinateFrame, xform, localToWorldMatrix );

    MatrixAngles( localToWorldMatrix, rotatedAngles );
    return new SOPAngle(rotatedAngles.x, rotatedAngles.y, rotatedAngles.z);
}
SOPVector *SOPAngle::Up(void) {
    Vector up;
    AngleVectors( ToQAngle(), NULL, NULL, &up );
    return new SOPVector(up.x, up.y, up.z);
}

// Lua interface
SOPAngle::SOPAngle(lua_State *L) {
    x = luaL_checknumber(L, 1);
    y = luaL_checknumber(L, 2);
    z = luaL_checknumber(L, 3);
}
int SOPAngle::__add(lua_State *L) {
    SOPAngle *ang = Add(Lunar<SOPAngle>::check(L, 1));
    if(ang)
    {
        Lunar<SOPAngle>::push(L, ang, true);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}
int SOPAngle::__div(lua_State *L) {
    SOPAngle *ang = Div(luaL_checknumber(L, 1));
    if(ang)
    {
        Lunar<SOPAngle>::push(L, ang, true);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}
int SOPAngle::__eq(lua_State *L) {
    lua_pushboolean(L, Eq(Lunar<SOPAngle>::check(L, 1)));
    return 1;
}
int SOPAngle::__mul(lua_State *L) {
    SOPAngle *ang = Mul(luaL_checknumber(L, 1));
    if(ang)
    {
        Lunar<SOPAngle>::push(L, ang, true);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}
int SOPAngle::__sub(lua_State *L) {
    SOPAngle *ang = Sub(Lunar<SOPAngle>::check(L, 1));
    if(ang)
    {
        Lunar<SOPAngle>::push(L, ang, true);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}
int SOPAngle::GetX(lua_State *L) {
    lua_pushnumber(L, x);
    return 1;
}
int SOPAngle::GetY(lua_State *L) {
    lua_pushnumber(L, y);
    return 1;
}
int SOPAngle::GetZ(lua_State *L) {
    lua_pushnumber(L, z);
    return 1;
}
int SOPAngle::Forward(lua_State *L) {
    SOPVector *vec = Forward();
    if(vec)
    {
        Lunar<SOPVector>::push(L, vec, true);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}
int SOPAngle::Right(lua_State *L) {
    SOPVector *vec = Right();
    if(vec)
    {
        Lunar<SOPVector>::push(L, vec, true);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}
int SOPAngle::RotateAroundAxis(lua_State *L) {
    SOPAngle *ang = RotateAroundAxis(Lunar<SOPVector>::check(L, 1), luaL_checknumber(L, 2));
    if(ang)
    {
        Lunar<SOPAngle>::push(L, ang, true);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}
int SOPAngle::Up(lua_State *L) {
    SOPVector *vec = Up();
    if(vec)
    {
        Lunar<SOPVector>::push(L, vec, true);
        return 1;
    }
    lua_pushnil(L);
    return 1;
}


const char SOPAngle::className[] = "Angle";
Lunar<SOPAngle>::DerivedType SOPAngle::derivedtypes[] = {
    {NULL}
};
Lunar<SOPAngle>::DynamicDerivedType SOPAngle::dynamicDerivedTypes;

#define method(class, name) {#name, &class::name}

template <>
int Lunar<SOPAngle>::tostring_T (lua_State *L) {
    userdataType *ud = static_cast<userdataType*>(lua_touserdata(L, 1));
    SOPAngle *obj = ud->pT;
    lua_pushstring(L, UTIL_VarArgs("%f %f %f", obj->x, obj->y, obj->z));
    return 1; 
}

Lunar<SOPAngle>::RegType SOPAngle::metas[] = {
    method(SOPAngle, __add),
    method(SOPAngle, __div),
    method(SOPAngle, __eq),
    method(SOPAngle, __mul),
    method(SOPAngle, __sub),
    {0,0}
};

Lunar<SOPAngle>::RegType SOPAngle::methods[] = {
    method(SOPAngle, GetX),
    method(SOPAngle, GetY),
    method(SOPAngle, GetZ),
    method(SOPAngle, Forward),
    method(SOPAngle, Right),
    method(SOPAngle, RotateAroundAxis),
    method(SOPAngle, Up),
    {0,0}
};

void lua_SOPAngle_register(lua_State *L)
{
    Lunar<SOPAngle>::Register(L);
}
