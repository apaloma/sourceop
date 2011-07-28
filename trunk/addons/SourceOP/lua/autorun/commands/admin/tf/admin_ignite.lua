function adminignite( pPlayer, command, arguments )
  local msgplayer
  if(pPlayer) then
    msgplayer = function(txt) pPlayer:SayText(txt) end
  else
    msgplayer = Msg
  end

  local playerlist = player.Find(arguments[1])
  // make sure there is exactly one player match
  if(#playerlist == 1) then
    local pIgnitePlayer = playerlist[1]
    local damage = DamageInfo(pIgnitePlayer, pIgnitePlayer, 1, DMG_CUSTOM_TF2_IGNITE)
    pIgnitePlayer:DispatchTraceAttack(damage, pIgnitePlayer:GetAbsOrigin(), pIgnitePlayer:GetAbsOrigin())
  // if no players found, print the not found message
  elseif(#playerlist < 1) then
    msgplayer("Unknown player '" .. arguments[1] .. "'.\n")
  // otherwise, more than one player was found
  else
    msgplayer("The name '" .. arguments[1] .. "' is ambiguous.\n")
  end
end

if(sourceop.FeatureStatus(FEAT_ADMINCOMMANDS) && game:IsTF2()) then
  admincommand.Add("ignite", 8192, 1, "<player>", adminignite)
end
