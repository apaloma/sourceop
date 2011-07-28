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

#ifndef L_CLASS_VECTOR
#define L_CLASS_VECTOR

class Vector;
class SOPAngle;

#include "lunar.h"

class SOPVector {
public:
    lua_Number x;
    lua_Number y;
    lua_Number z;
public:
    SOPVector(lua_Number x, lua_Number y, lua_Number z);
    SOPVector *Add(SOPVector *vec);
    SOPVector *Div(lua_Number div);
    bool Eq(SOPVector *vec);
    SOPVector *Mul(lua_Number mult);
    SOPVector *Sub(SOPVector *vec);
    SOPAngle *Angle(void);
    SOPVector *GetNormal(void);
    lua_Number Length(void);
    ~SOPVector();

    Vector ToVector(void);

    // Lua interface
    SOPVector(lua_State *L);
    int __add(lua_State *L);
    int __div(lua_State *L);
    int __eq(lua_State *L);
    int __mul(lua_State *L);
    int __sub(lua_State *L);
    int Angle (lua_State *L);
    int GetNormal (lua_State *L);
    int Length (lua_State *L);
    int GetX(lua_State *L);
    int GetY(lua_State *L);
    int GetZ(lua_State *L);

    static const char className[];
    static Lunar<SOPVector>::DerivedType derivedtypes[];
    static Lunar<SOPVector>::DynamicDerivedType dynamicDerivedTypes;
    static Lunar<SOPVector>::RegType metas[];
    static Lunar<SOPVector>::RegType methods[];
};

void lua_SOPVector_register(lua_State *L);

#endif
