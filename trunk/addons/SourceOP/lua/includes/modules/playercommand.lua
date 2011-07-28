/**
 * \file playercommand.lua
 * \brief A module to take care of the registration and calling of Lua player
 *        console commands.
 */

local string = string
local pcall = pcall
local tostring = tostring
local Error = Error
local hook = require "hook"

module("playercommand")

local CommandList   = {}

/**
 * Add a new player command.
 *
 * @param name The name of the command.
 * @param func The function to call when the command is executed.
 */
function Add( name, func )
  CommandList[string.lower(name)] = func
end

/**
 * Removes a player command.
 *
 * @param The name of the command.
 */
function Remove( name )
  CommandList[string.lower(name)] = nil
end


/**
 * Called by SourceOP when a player types a command that isn't registered as a
 * concommand.
 *
 * @param name The event that occured.
 */
function Run(pPlayer, command, arguments)
  local lcommand = string.lower(command)

  if (CommandList[lcommand] != nil) then
    local b, retval = pcall(CommandList[lcommand], pPlayer, command, arguments)
    if (!b) then
      Error("playercommand '" .. lcommand .. "' failed with error: " .. tostring(retval) .. "\n")
    end

    if(retval != nil) then
      return retval
    else
      return false
    end
  end

  return nil
end

hook.Add( "PlayerCommand", "playercommands", Run )
