/**
 * \file hook.lua
 * \brief A module that allows hooks to be installed for certain events.
 */


local pairs = pairs
local Error = Error
local tostring = tostring
local pcall = pcall
local unpack = unpack
local PrintTable = PrintTable
local concommand = concommand

module("hook")


// The table that stores all hooks by event name
local Hooks = {}


/**
 * Adds a hook for an event.
 *
 * @param event_name The name of the event to hook.
 * @param name The name of the specific hook. Can be anything. Used to avoid
               accidentally hooking something twice.
 * @param func The function to call when the event occurs.
 */
function Add(event_name, name, func)
  if(Hooks[event_name] == nil) then
    Hooks[event_name] = {}
  end

  Hooks[event_name][name] = func
end

/**
 * Called by SourceOP when an event that can be hooked occurs. Executes all
 * hooks for that event. Should not be called from Lua.
 *
 * @param name The event that occured.
 */
function Call(name, ...)
  local retval
  local EventHooks = Hooks[name]

  if (EventHooks != nil) then
    for k, v in pairs(EventHooks) do
      if (v == nil) then
        Error("Hook '" .. tostring(k) .. "' has nil handler\n")
        Error("Removing hook '" .. tostring(k) .. "'\n")
        EventHooks[k] = nil
        break;
      else
        local b, newret = pcall(v, unpack(arg))
        if(b && newret != nil) then
          retval = newret
        end

        // check for errors
        if (!b) then
          Error("Hook '" .. tostring(k) .. "' failed with error: " .. tostring(retval) .. "\n")
          Error("Removing hook '" .. tostring(k) .. "'\n")
          EventHooks[k] = nil
        end
      end
    end
  end

  return retval
end

/**
 * Gets the hook table.
 *
 * @return The hook table.
 */
function GetTable()
  return Hooks
end

/**
 * Removes a specific hook.
 *
 * @param event_name The event hooked.
 * @param name The name of the specific hook.
 */
function Remove(event_name, name)
  if(Hooks[event_name] != nil) then
    Hooks[event_name][name] = nil
  end
end


// A console command for coders to see all installed hooks.
concommand.Add("DF_dump_hooks", function() PrintTable(Hooks) end)
