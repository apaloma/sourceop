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

#ifndef L_CLASS_DAMAGEINFO
#define L_CLASS_DAMAGEINFO

#include "lunar.h"

class SOPEntity;
class CTakeDamageInfo;

class SOPDamageInfo {
protected:
    CTakeDamageInfo damageInfo;
public:
    SOPDamageInfo(SOPEntity *inflictor, SOPEntity *attacker, lua_Number damage, lua_Integer damageType);
    void Set(SOPEntity *inflictor, SOPEntity *attacker, lua_Number damage, lua_Integer damageType);

    void AddDamage(int damage);
    void SetDamageCustom(int iDamageCustom);
    ~SOPDamageInfo();

    CTakeDamageInfo ToCTakeDamageInfo();

    // Lua interface
    SOPDamageInfo(lua_State *L);
    int AddDamage(lua_State *L);
    int SetDamageCustom(lua_State *L);

    static const char className[];
    static Lunar<SOPDamageInfo>::DerivedType derivedtypes[];
    static Lunar<SOPDamageInfo>::DynamicDerivedType dynamicDerivedTypes;
    static Lunar<SOPDamageInfo>::RegType metas[];
    static Lunar<SOPDamageInfo>::RegType methods[];
};

void lua_SOPDamageInfo_register(lua_State *L);

#endif
