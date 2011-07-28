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

#include "AdminOP.h"

#define LUA_LIB

#include "lua.h"

#include "lauxlib.h"
#include "luasoplibs.h"

#include "lentitycache.h"

class CLuaCommand : public ICommandCallback
{
    char command[256];
    char description[256];
    lua_State *L;
    int m_func;
    ConCommand *concmd;

public:
    CLuaCommand(lua_State *state, const char *cmd, int func, const char *desc)
    {
        L = state;
        m_func = func;
        concmd = NULL;

        strncpy(command, cmd, sizeof(command));
        command[sizeof(command)-1] = '\0';
        if(desc)
        {
            strncpy(description, desc, sizeof(description));
            command[sizeof(description)-1] = '\0';
        }
        else
        {
            lua_Debug ar;

            // default description
            strcpy(description, "Lua command added by SourceOP.");

            // or make a better one if one is available
            if (lua_getstack(L, 1, &ar)) {  // check function at level
                lua_getinfo(L, "Sln", &ar);  // get info about it
                if (ar.currentline > 0) {   // is there info?
                    if (ar.name) {
                        sprintf(description, "Lua command added by SourceOP (%s:%d in function %s).", V_UnqualifiedFileName(ar.short_src), ar.currentline, ar.name);
                    } else {
                        sprintf(description, "Lua command added by SourceOP (%s:%d).", V_UnqualifiedFileName(ar.short_src), ar.currentline);
                    }
                }
            }
        }
        Init();
    }
    ConCommand *Init()
    {
        if(!concmd)
            concmd = new ConCommand(command, this, description, FCVAR_GAMEDLL);
        return concmd;
    }
    inline const char *GetName()
    {
        return command;
    }
    void Unregister()
    {
        if(concmd)
        {
            g_metamodOverrides->UnregisterConCommand(concmd);
            delete concmd;
            concmd = NULL;
            lua_unref(L, m_func);
        }
    }
    ~CLuaCommand()
    {
        Unregister();
    }
    void CommandCallback( const CCommand &command )
    {
        // don't even try this unless we're still using the same lua state
        if(pAdminOP.GetLuaState() != L) return;

        lua_getref(L, m_func);
        if(pAdminOP.m_iClientCommandIndex>=0)
            g_entityCache.PushPlayer(pAdminOP.m_iClientCommandIndex+1);
        else
            lua_pushnil(L);
        lua_pushstring(L, command[0]);
        lua_newtable(L);
        int idx = lua_gettop(L);
        // add full args
        lua_pushstring(L, "args");
        lua_pushstring(L, command.ArgS());
        lua_settable(L, idx);
        // add command
        lua_pushinteger(L, 0);
        lua_pushstring(L, command[0]);
        lua_settable(L, idx);
        // add all arguments
        for(int i = 1; i < command.ArgC(); i++)
        {
            lua_pushinteger(L, i);
            lua_pushstring(L, command[i]);
            lua_settable(L, idx);
        }
        if(lua_pcall(L, 3, 0, 0) != 0)
            CAdminOP::ColorMsg(CONCOLOR_LUA_ERR, "[SOURCEOP] Lua: error running concommand `%s':\n %s\n", command[0], lua_tostring(L, -1));
    }
};

CUtlVector <CLuaCommand *> newCommands; 

static int concommand_add(lua_State *L)
{
    const char *cmd = luaL_checkstring(L, 1);
    // this isn't good:
    //   lua_pop(L, 1);
    int func;
    const char *desc = luaL_optstring(L, 3, NULL);

    // don't get the description if one was passed. pop and get next variable
    if(desc) lua_pop(L, 1);
    func = lua_ref(L, true);
    //if(desc) lua_pop(L, -1);

    CLuaCommand *newCommand = new CLuaCommand(L, cmd, func, desc);
    //newCommand->Init();
    newCommands.AddToTail(newCommand);


    return 0;
}

static int concommand_remove(lua_State *L)
{
    const char *cmd = luaL_checkstring(L, 1);

    for(int i = 0; i < newCommands.Count(); i++)
    {
        CLuaCommand *command = newCommands[i];
        if(!strcmp(command->GetName(), cmd))
        {
            command->Unregister();
            //cvar->UnregisterConCommand(command);
            newCommands.Remove(i);
            delete command;
            return 0;
        }
    }
    lua_pushstring(L, "concommand.Remove did not find the specified command\n");
    lua_error(L);
    return 0;
}

static int concommand_removeall(lua_State *L)
{
    for(int i = 0; i < newCommands.Count(); i++)
    {
        CLuaCommand *command = newCommands[i];
        command->Unregister();
        //cvar->UnregisterConCommand(command);
        delete command;
    }
    newCommands.RemoveAll();
    return 0;
}

static const luaL_Reg concommandlib[] = {
    {"Add",         concommand_add},
    {"Remove",      concommand_remove},
    {"RemoveAll",   concommand_removeall},
    {NULL, NULL}
};


/*
** Open library
*/
LUALIB_API int luaopen_concommand (lua_State *L) {
  luaL_register(L, LUA_CONCOMMANDLIBNAME, concommandlib);
  return 1;
}
