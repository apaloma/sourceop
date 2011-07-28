function adminforceclass( pPlayer, command, arguments )
  local msgplayer
  if(pPlayer) then
    msgplayer = function(txt) pPlayer:SayText(txt) end
  else
    msgplayer = Msg
  end

  local playerlist = player.Find(arguments[1])
  // make sure there is exactly one player match
  if(#playerlist == 1) then
    local pChangeClassPlayer = playerlist[1]
    local playerclass = tonumber(arguments[2])
    if(playerclass != nil && playerclass >= 0 && playerclass <= 10) then
      pChangeClassPlayer:SetPlayerClass(playerclass)
      pChangeClassPlayer:SetDesiredPlayerClass(playerclass)
      pChangeClassPlayer:ForceRespawn()
    else
      msgplayer("Invalid class number (" .. arguments[2] .. ").\n")
    end
  // if no players found, print the not found message
  elseif(#playerlist < 1) then
    msgplayer("Unknown player '" .. arguments[1] .. "'.\n")
  // otherwise, more than one player was found
  else
    msgplayer("The name '" .. arguments[1] .. "' is ambiguous.\n")
  end
end

if(sourceop.FeatureStatus(FEAT_ADMINCOMMANDS) && game:IsTF2()) then
  admincommand.Add("forceclass", 8192, 2, "<player> <class number>", adminforceclass)
end
