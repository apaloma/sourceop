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

#ifdef DFENTS

#ifndef DFRADIO_H
#define DFRADIO_H

class DFRadio : public DFBaseent
{
public:

    void RadioThink(void);
    void KillOff(void);
    void Spawn(edict_t *pEntity, Vector origin);
};

#endif /*DFRADIO_H*/

#endif
