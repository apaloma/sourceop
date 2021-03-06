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

#ifndef CLIENTCOMMANDS_H
#define CLIENTCOMMANDS_H

typedef PLUGIN_RESULT (__cdecl* CliComFunc)( void );

typedef struct clicom_s
{
    char        cmd[80];
    CliComFunc  func;
} clicom_t;

class ClientCommands
{
public:
    void Init();
    void RemoveAll();
    clicom_t CreateCliCom(const char *pszCmd, CliComFunc func);
    PLUGIN_RESULT RunCmd(const CCommand &args, edict_t *pEntity, CBasePlayer *pPlayer, IPlayerInfo *playerinfo);
private:
    CUtlVector<clicom_t> cmdList;
};

#endif
