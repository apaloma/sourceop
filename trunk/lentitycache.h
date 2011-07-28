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

#ifndef LENTITYCACHE_H
#define LENTITYCACHE_H

class SOPEntity;

class CEntityCache
{
public:
    CEntityCache();
    void PushEntity(int index);
    void PushPhysObj(int index);
    void PushPlayer(int index);
    void EntityRemoved(int index);
    void PlayerDisconnected(int index);
    void InitCache();
    void ClearCache();

private:
    CUtlMap<int, int> cacheMap;
    CUtlMap<int, int> physCacheMap;
    CUtlMap<int, int> playerCacheMap;
};

extern CEntityCache g_entityCache;

#endif // LENTITYCACHE_H