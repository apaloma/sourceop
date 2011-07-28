function adminreplicate( pPlayer, command, arguments )
  local msgplayer
  if(pPlayer) then
    msgplayer = function(txt) pPlayer:SayText(txt) end
  else
    msgplayer = Msg
  end

  game.ReplicateConVar(arguments[1], arguments[2])
  msgplayer(arguments[1] .. " = " .. arguments[2] .. " replicated to all players.\n")
end

if(sourceop.FeatureStatus(FEAT_ADMINCOMMANDS)) then
  admincommand.Add("replicate", 8192, 2, "<var> <value>", adminreplicate)
end
