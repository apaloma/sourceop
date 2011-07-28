function adminteam( pPlayer, command, arguments )
  local msgplayer
  if(pPlayer) then
    msgplayer = function(txt) pPlayer:SayText(txt) end
  else
    msgplayer = Msg
  end

  local playerlist = player.Find(arguments[1])
  // make sure there is exactly one player match
  if(#playerlist == 1) then
    local pChangePlayer = playerlist[1]
    local teamnumber = string.lower(arguments[2])
    // if this is a number, use that number
    if(tonumber(teamnumber) != nil) then
      teamnumber = tonumber(teamnumber)
    // allow these names for "Spectator"
    elseif(teamnumber == "spec" || teamnumber == "spectate" || teamnumber == "spectators") then
      teamnumber = gamerules.GetTeamIndex("Spectator")
    // otherwise, try and find the team number
    else
      teamnumber = gamerules.GetTeamIndex(teamnumber)
    end
    // make sure the number used is valid
    if(gamerules.IsValidTeamNumber(teamnumber)) then
      pChangePlayer:SetTeam(teamnumber)
    else
      msgplayer("Invalid team entered. Try a number 0-5 or a team name.\n")
    end
  // if no players found, print the not found message
  elseif(#playerlist < 1) then
    msgplayer("Unknown player '" .. arguments[1] .. "'.\n")
  // otherwise, more than one player was found
  else
    msgplayer("The name '" .. arguments[1] .. "' is ambiguous.\n")
  end
end

if(sourceop.FeatureStatus(FEAT_ADMINCOMMANDS)) then
  admincommand.Add("team", 8192, 2, "<player> <team>", adminteam)
end