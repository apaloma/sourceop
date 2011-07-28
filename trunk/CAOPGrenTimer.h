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

#ifndef CAOPGRENTIMER_H
#define CAOPGRENTIMER_H

#include "CAOPNormal.h"

#include "tier0/memdbgon.h"

class CAOPGrenTimer : public CAOPNormal
{
public:
    DECLARE_CLASS(CAOPGrenTimer, CAOPNormal);

    DECLARE_DATADESC();

    CAOPGrenTimer();
    virtual void Spawn(void);
    virtual void Precache(void);

    void BeepThink();
    void FinalBeepThink();
private:
    int m_iOwner;
    float m_flInterval;
    float m_flLife;
    float m_flFinishTime;
};

#endif
