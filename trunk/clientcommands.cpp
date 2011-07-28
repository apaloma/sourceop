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

#include "fixdebug.h"
#include "utlvector.h"
#include "recipientfilter.h"

// HACK HACK HACK : BASEENTITY ACCESS
#define GAME_DLL
#include "cbase.h"
#include "baseentity.h"
#include "player.h"
// HACK HACK HACK : BASEENTITY ACCESS

#include "vmatrix.h"
#include "util.h"
#include "vphysics_interface.h"
#include "vcollide_parse.h"

#include "AdminOP.h"
#include "vfuncs.h"

#include "clientcommands.h"

#include "tier0/memdbgon.h"

PLUGIN_RESULT CmdTestCommand()
{
    Msg("MSGCAT IS IN UR SERVER TXTIN UR CONSOLE!\n");
    return PLUGIN_STOP;
}

void ClientCommands::Init()
{
    cmdList.Purge();
    //cmdList.AddToTail(CreateCliCom("testmsgcmd", CmdTestCommand));
}

void ClientCommands::RemoveAll()
{
    cmdList.Purge();
}

clicom_t ClientCommands::CreateCliCom(const char *pszCmd, CliComFunc func)
{
    clicom_t newCmd;
    strncpy(newCmd.cmd, pszCmd, sizeof(newCmd.cmd));
    newCmd.cmd[sizeof(newCmd.cmd)-1] = '\0';
    newCmd.func = func;

    return newCmd;
}

PLUGIN_RESULT ClientCommands::RunCmd(const CCommand &args, edict_t *pEntity, CBasePlayer *pPlayer, IPlayerInfo *playerinfo)
{
    for(int i = 0; i < cmdList.Count(); i++)
    {
        clicom_t *cliCmd = cmdList.Base() + i; // fast access

        if(FStrEq(args[0], cliCmd->cmd))
        {
            return (cliCmd->func)();
        }
    }
    return PLUGIN_CONTINUE;
}
