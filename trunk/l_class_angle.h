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

#ifndef L_CLASS_ANGLE
#define L_CLASS_ANGLE

class QAngle;
class SOPVector;

#include "lunar.h"

class SOPAngle {
public:
    lua_Number x;
    lua_Number y;
    lua_Number z;
public:
    SOPAngle(lua_Number x, lua_Number y, lua_Number z);
    SOPAngle *Add(SOPAngle *ang);
    SOPAngle *Div(lua_Number div);
    bool Eq(SOPAngle *ang);
    SOPAngle *Mul(lua_Number mult);
    SOPAngle *Sub(SOPAngle *ang);
    SOPVector *Forward(void);
    SOPVector *Right(void);
    SOPAngle *RotateAroundAxis(SOPVector *vec, lua_Number degrees);
    SOPVector *Up(void);
    ~SOPAngle();

    QAngle ToQAngle(void);

    // Lua interface
    SOPAngle(lua_State *L);
    int __add(lua_State *L);
    int __div(lua_State *L);
    int __eq(lua_State *L);
    int __mul(lua_State *L);
    int __sub(lua_State *L);
    int Forward(lua_State *L);
    int Right(lua_State *L);
    int RotateAroundAxis(lua_State *L);
    int Up(lua_State *L);
    int GetX(lua_State *L);
    int GetY(lua_State *L);
    int GetZ(lua_State *L);

    static const char className[];
    static Lunar<SOPAngle>::DerivedType derivedtypes[];
    static Lunar<SOPAngle>::DynamicDerivedType dynamicDerivedTypes;
    static Lunar<SOPAngle>::RegType metas[];
    static Lunar<SOPAngle>::RegType methods[];
};

inline QAngle SOPAngle::ToQAngle(void) {
    return QAngle(x, y, z);
}

void lua_SOPAngle_register(lua_State *L);

#endif
