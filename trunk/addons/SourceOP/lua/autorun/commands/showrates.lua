function showrates( pPlayer, command, arguments )
  local msgplayer
  if(pPlayer) then
    msgplayer = function(txt) pPlayer:SayTextNoSOP(txt, HUD_PRINTCONSOLE) end
  else
    msgplayer = Msg
  end

  local players = player.GetAll()
  msgplayer(string.format("%-32s %-20s %-4s %-4s %-6s %-3s %-3s", "Player name", "SteamID", "ping", "loss", "rate", "upd", "cmd") .. "\n")
  for _, v in pairs(players) do
    msgplayer(string.format("%-32s %-20s %-4i %-4i %-6s %-3s %-3s", v:GetName(), v:SteamIDRender(), v:GetPing(), v:GetPacketLossPercent(), v:GetClientConVarValue("rate"), v:GetClientConVarValue("cl_updaterate"), v:GetClientConVarValue("cl_cmdrate")) .. "\n")
  end
end

concommand.Add("showrates", showrates, "Shows player rates.")
