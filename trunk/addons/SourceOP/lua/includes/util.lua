/**
 * \file util.lua
 * \brief Utility functions included by init.
 *
 * @author Tony Paloma
 * @version 6/5/2010
*/

/**
 * Formats and prints a table to the server console.
 *
 * @param t The table to print.
*/
function PrintTable(t, indent, skip)
  skip = skip or {}
  indent = indent or 0

  for k, v in pairs(t) do
    Msg(string.rep("\t", indent))
    if(type(v) == "table" && not skip[v]) then
      skip[v] = true
      Msg(tostring (k) .. ":" .. "\n");
      PrintTable(v, indent + 2, skip)
    else
      Msg(tostring(k) .. "\t=\t")
      Msg(tostring(v) .. "\n")
    end
  end
end

/**
 * Prints a string with no limit to length. This is done by breaking up the
 * string and printing the segments.
 *
 * @param str The string to print.
*/
function MsgNoLimit(str)
  local pos = 1
  local len = string.len(str)

  while pos < len do
    Msg(string.sub(str, pos, pos+512))
    pos = pos + 512
  end
end

/**
 * Checks to see if an entity is valid.
 *
 * @param ent The entity to check.
 * @return True if the entity is valid.
 */
function ValidEntity(ent)
  if(ent == nil) then
    return false
  else
    return ent:IsValid()
  end
end

/**
 * Checks to see if a SteamID is valid.
 *
 * @param steamid SteamID in 64bit integer form (7656...)
 * @return True if the SteamID is valid.
 */
function ValidSteamID(steamid)
  return (steamid != nil && steamid != "" && steamid != "0")
end

/**
 * Delays saying the chat so that functions that print chat message in the
 * PlayerSay hook do not say stuff before the player actually speaks.
 *
 * @param str The chat to say.
*/
function DelayedSayToAll(str, player)
  timer.Simple(0.01, (function() sourceop.SayTextAll(str, player) end))
end
