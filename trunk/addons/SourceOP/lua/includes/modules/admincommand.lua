/**
 * \file admincommand.lua
 * \brief A module to take care of the registration and calling of Lua admin
 *        console commands.
 */

local string = string
local pcall = pcall
local tostring = tostring
local sourceop = sourceop
local Error = Error
local Msg = Msg
local concommand = concommand
local HUD_PRINTCONSOLE = HUD_PRINTCONSOLE

module("admincommand")

local CommandList = {}

/**
 * Adds a new admin command.
 *
 * @param name The name of the admin command.
 * @param access The required access level.
 * @param minargs The minimum number of arguments.
 * @param usage A string to help users with usage.
 * @param func The function to call when the command is executed.
 */
function Add( name, access, minargs, usage, func )
  local LowerPrefix = string.lower(sourceop.GetCommandPrefix())
  local LowerName = string.lower(name)
  CommandList[LowerName] = {Access=access, Args=minargs, Usage=usage, Func=func, Prefix=LowerPrefix}

  local dispatch =
  function(pPlayer, command, arguments)
    // set the correct messaging function
    if(pPlayer) then
      msgplayer = function(txt) pPlayer:SayText(txt, HUD_PRINTCONSOLE) end
    else
      msgplayer = Msg
    end

    // check for access
    if(!pPlayer || pPlayer:IsAdmin(access, name)) then
      // run command if correct number of arguments and the arguments are not /?
      if(#arguments >= minargs && arguments["args"] != "/?") then
        local b, retval = pcall(func, pPlayer, command, arguments)
        if (!b) then
          Error("admincommand '" .. name .. "' failed with error:\n " .. tostring(retval) .."\n")
        end
      else
        msgplayer("Usage:\n")
        msgplayer(command .. " " .. usage .. "\n")
      end
    else
      msgplayer("You do not have access to the command " .. command .. ".\n")
    end
  end
  concommand.Add(LowerPrefix .. LowerName, dispatch, "SourceOP admin command. Usage: " .. LowerPrefix .. LowerName .. " " .. usage)
end

/**
 * Removes an admin command.
 *
 * @param name The name of the admin command.
 */
function Remove( name )
  local LowerName = string.lower(name)
  if(CommandList[LowerName] != nil) then
    // remove the actual concommand
    concommand.Remove(CommandList[LowerName]["Prefix"] .. LowerName)
    CommandList[LowerName] = nil
  end
end
