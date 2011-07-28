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

#ifndef L_CLASS_ENTITY
#define L_CLASS_ENTITY

class SOPDamageInfo;
class SOPVector;
class SOPAngle;
class SOPPhysObj;
#include "lunar.h"

class SOPEntity {
protected:
    lua_Integer m_iIndex;
    int datatable;
    const char *m_ClassName;
public:
    SOPEntity(int index=0, const char *szClassname=NULL);
    bool Eq(SOPEntity *other);

    void Activate(void);
    void AddEffects(int nEffects);
    void AddFlag(int flags);
    float BoundingRadius(void);
    void DispatchTraceAttack(SOPDamageInfo *damage, SOPVector *startpos, SOPVector *endpos);
    float DistanceTo(SOPVector *checkOrigin);
    void DrawShadow(bool shouldDraw);
    void EmitSound(const char *pszSound, lua_Number vol, lua_Number pitch);
    int EntIndex(void) { return m_iIndex; }
    SOPVector *EyePos(void);
    int FindEntityForward(unsigned int mask, Vector *endpos = NULL);
    void Fire(const char *pszInput, const char *pszParam);
    int Flags(void);
    SOPVector *GetAbsOrigin(void);
    SOPAngle *GetAngles(void);
    const char *GetClassname(void);
    SOPVector *GetForward(void);
    int GetMaxHealth(void);
    SOPVector *GetMaxs(void);
    SOPVector *GetMins(void);
    const char *GetModel(void);
    const char *GetName(void);
    unsigned char GetOffsetByte(int offset);
    int GetOffsetEntity(int offset);
    float GetOffsetFloat(int offset);
    int GetOffsetInt(int offset);
    int GetOwner(void);
    int GetSpeed(void);
    SOPVector *GetVelocity(void);
    int Health(void);
    bool IsNPC(void);
    bool IsPlayer(void);
    bool IsValid(void);
    bool KeyValue(const char *szKeyName, const char *szValue);
    void Kill(void);
    SOPVector *LocalToWorld(SOPVector *localPosition);
    void NextThink(float thinktime);
    SOPVector *OBBCenter(void);
    void PhysicsInit(int solidType);
    void SetAbsOrigin(SOPVector *absOrigin);
    void SetAngles(SOPAngle *absAngles);
    void SetCollisionGroup(int collisionGroup);
    void SetGravity(double gravity);
    void SetHealth(int health);
    void SetLocalAngles(SOPAngle *locAngles);
    void SetLocalOrigin(SOPVector *locOrigin);
    void SetModel(const char *ModelName);
    void SetMoveType(int movetype);
    void SetOffsetByte(int offset, int val);
    void SetOffsetEnt(int offset, SOPEntity *val);
    void SetOffsetFloat(int offset, float val);
    void SetOffsetInt(int offset, int val);
    void SetOwner(SOPEntity *pOwner);
    void SetParent(SOPEntity *pParent, int attachment);
    void SetRenderAmt(int val);
    void SetRenderMode(int val);
    void SetSolid(int val);
    void SetTrigger(bool enabled);
    void SetVelocity(SOPVector *absVelocity);
    void Spawn(void);
    void StateChanged(unsigned short offset);
    void StopParticleEffects(void);
    bool Visible(SOPEntity *pOther);
    ~SOPEntity();

    // Lua interface
    SOPEntity(lua_State *L);
    int __eq(lua_State *L);
    int __index(lua_State *L);
    int __newindex(lua_State *L);
    int Activate(lua_State *L);
    int AddEffects(lua_State *L);
    int AddFlag(lua_State *L);
    int BoundingRadius(lua_State *L);
    int DispatchTraceAttack(lua_State *L);
    int DistanceTo(lua_State *L);
    int DrawShadow(lua_State *L);
    int EmitSound(lua_State *L);
    int EntIndex (lua_State *L);
    int EyePos(lua_State *L);
    int FindEntityForward(lua_State *L);
    int Fire(lua_State *L);
    int Flags(lua_State *L);
    int GetAbsOrigin(lua_State *L);
    int GetAngles(lua_State *L);
    int GetClassname(lua_State *L);
    int GetForward(lua_State *L);
    int GetMaxHealth(lua_State *L);
    int GetMaxs(lua_State *L);
    int GetMins(lua_State *L);
    int GetModel(lua_State *L);
    int GetName(lua_State *L);
    int GetOffsetByte(lua_State *L);
    int GetOffsetEntity(lua_State *L);
    int GetOffsetFloat(lua_State *L);
    int GetOffsetInt(lua_State *L);
    int GetOwner(lua_State *L);
    int GetSpeed(lua_State *L);
    int GetPhysicsObject(lua_State *L);
    int GetVelocity(lua_State *L);
    int Health(lua_State *L);
    int IsNPC(lua_State *L);
    int IsPlayer(lua_State *L);
    int IsValid(lua_State *L);
    int KeyValue(lua_State *L);
    int Kill(lua_State *L);
    int LocalToWorld(lua_State *L);
    int NextThink(lua_State *L);
    int OBBCenter(lua_State *L);
    int PhysicsInit(lua_State *L);
    int SetAbsOrigin(lua_State *L);
    int SetAngles(lua_State *L);
    int SetCollisionGroup(lua_State *L);
    int SetGravity(lua_State *L);
    int SetHealth(lua_State *L);
    int SetLocalAngles(lua_State *L);
    int SetLocalOrigin(lua_State *L);
    int SetModel(lua_State *L);
    int SetMoveType(lua_State *L);
    int SetOffsetByte(lua_State *L);
    int SetOffsetEnt(lua_State *L);
    int SetOffsetFloat(lua_State *L);
    int SetOffsetInt(lua_State *L);
    int SetOwner(lua_State *L);
    int SetParent(lua_State *L);
    int SetRenderAmt(lua_State *L);
    int SetRenderMode(lua_State *L);
    int SetSolid(lua_State *L);
    int SetTrigger(lua_State *L);
    int SetVelocity(lua_State *L);
    int Spawn(lua_State *L);
    int StateChanged(lua_State *L);
    int StopParticleEffects(lua_State *L);
    int Visible(lua_State *L);

    static const char className[];
    static Lunar<SOPEntity>::DerivedType derivedtypes[];
    static Lunar<SOPEntity>::DynamicDerivedType dynamicDerivedTypes;
    static Lunar<SOPEntity>::RegType metas[];
    static Lunar<SOPEntity>::RegType methods[];
};

void lua_SOPEntity_register(lua_State *L);

#endif
