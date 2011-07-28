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

#ifndef L_CLASS_PHYSOBJ
#define L_CLASS_PHYSOBJ

class SOPVector;
class SOPEntity;
class IPhysicsObject;
#include "lunar.h"

class SOPPhysObj {
protected:
    lua_Integer m_iIndex;
public:
    SOPPhysObj(int index=0);
    ~SOPPhysObj();
    void ApplyForceCenter(SOPVector *forceVector);
    void EnableCollisions(bool enable);
    void EnableDrag(bool enable);
    void EnableGravity(bool enable);
    void EnableMotion(bool enable);
    float GetMass(void);
    bool IsValid(void);
    void SetMass(float mass);

    // Lua interface
    SOPPhysObj(lua_State *L);
    int ApplyForceCenter(lua_State *L);
    int EnableCollisions(lua_State *L);
    int EnableDrag(lua_State *L);
    int EnableGravity(lua_State *L);
    int EnableMotion(lua_State *L);
    int GetMass(lua_State *L);
    int IsValid(lua_State *L);
    int SetMass(lua_State *L);

    static const char className[];
    static Lunar<SOPPhysObj>::DerivedType derivedtypes[];
    static Lunar<SOPPhysObj>::DynamicDerivedType dynamicDerivedTypes;
    static Lunar<SOPPhysObj>::RegType metas[];
    static Lunar<SOPPhysObj>::RegType methods[];
private:
    IPhysicsObject *GetPhysicsObject(void);
    int EntIndex(void) { return m_iIndex; }
};

void lua_SOPPhysObj_register(lua_State *L);

#endif
