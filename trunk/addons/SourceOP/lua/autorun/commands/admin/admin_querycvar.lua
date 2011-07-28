function queryfinished( msgplayer, estatus, name, value )
  if(estatus == 0) then
    msgplayer(name .. " has a value of \"" .. value .. "\".\n")
  elseif(estatus == 1) then
    msgplayer(name .. " was not found.\n")
  elseif(estatus == 2) then
    msgplayer(name .. " is not a ConVar.\n")
  else
    msgplayer(name .. " is a protected ConVar.\n")
  end
end

function adminquerycvar( pPlayer, command, arguments )
  local msgplayer
  if(pPlayer) then
    msgplayer = function(txt) pPlayer:SayText(txt) end
  else
    msgplayer = Msg
  end

  local playerlist = player.Find(arguments[1])
  // make sure there is exactly one player match
  if(#playerlist == 1) then
  // if no players found, print the not found message
    msgplayer("Querying cvar " .. arguments[2] .. " on " .. playerlist[1]:GetName() .. ".\n")
    playerlist[1]:QueryConVar(arguments[2], function(estatus, name, value) queryfinished(msgplayer, estatus, name, value) end)
  elseif(#playerlist < 1) then
    msgplayer("Unknown player '" .. arguments[1] .. "'.\n")
  // otherwise, more than one player was found
  else
    msgplayer("The name '" .. arguments[1] .. "' is ambiguous.\n")
  end
end

if(sourceop.FeatureStatus(FEAT_ADMINCOMMANDS)) then
  admincommand.Add("querycvar", 8192, 2, "<player> <cvar>", adminquerycvar)
end
