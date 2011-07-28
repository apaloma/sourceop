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

#ifndef CAOPFRAGTEST_H
#define CAOPFRAGTEST_H

#include "CAOPExploding.h"

#include "tier0/memdbgon.h"

class CAOPFragTest : public CAOPExploding
{
public:
    DECLARE_CLASS(CAOPFragTest, CAOPExploding);

    DECLARE_DATADESC();

    CAOPFragTest();
    virtual void Spawn(void);
    virtual void Precache(void);

    void CreateTrail();
    void ExplodeThink();

    /*char szSpam0[2048][2048]; // Memory problems?
    char szSpam1[2048][2048];
    char szSpam2[2048][2048];
    char szSpam3[2048][2048];
    char szSpam4[2048][2048];
    char szSpam5[2048][2048];
    char szSpam6[2048][2048];
    char szSpam7[2048][2048];
    char szSpam8[2048][2048];
    char szSpam9[2048][2048];
    char szSpam10[2048][2048];
    char szSpam11[2048][2048];
    char szSpam12[2048][2048];
    char szSpam13[2048][2048];
    char szSpam14[2048][2048];
    char szSpam15[2048][2048];
    char szSpam16[2048][2048];
    char szSpam17[2048][2048];
    char szSpam18[2048][2048];
    char szSpam19[2048][2048];
    char szSpam20[2048][2048];
    char szSpam21[2048][2048];
    char szSpam22[2048][2048];
    char szSpam23[2048][2048];
    char szSpam24[2048][2048];
    char szSpam25[2048][2048];
    char szSpam26[2048][2048];
    char szSpam27[2048][2048];
    char szSpam28[2048][2048];
    char szSpam29[2048][2048];
    char szSpam30[2048][2048];
    char szSpam31[2048][2048];
    char szSpam32[2048][2048];
    char szSpam33[2048][2048];
    char szSpam34[2048][2048];
    char szSpam35[2048][2048];
    char szSpam36[2048][2048];
    char szSpam37[2048][2048];
    char szSpam38[2048][2048];
    char szSpam39[2048][2048];
    char szSpam40[2048][2048];
    char szSpam41[2048][2048];
    char szSpam42[2048][2048];
    char szSpam43[2048][2048];
    char szSpam44[2048][2048];
    char szSpam45[2048][2048];
    char szSpam46[2048][2048];
    char szSpam47[2048][2048];
    char szSpam48[2048][2048];
    char szSpam49[2048][2048];*/
private:
    int m_iOwner;
    float m_flLife;
};

#endif
