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

#ifndef CAOPEXPLODING_H
#define CAOPEXPLODING_H

#include "CAOPEntity.h"

#include "tier0/memdbgon.h"

class CAOPExploding : public CAOPEntity
{
public:
    DECLARE_CLASS(CAOPExploding, CAOPEntity);

    DECLARE_DATADESC();

    CAOPExploding();
    void Set(edict_t *pent);

    void SetDamage(float damage);
    void SetDamageRadius(float dmgradius);
    void SetOwner(CBaseEntity *pOwner);
    void Touch(CBaseEntity *pOther);
    void Think();
    virtual void UpdateOnRemove(void);
};

#endif
