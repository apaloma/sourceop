function adminluacount( pPlayer, command, arguments )
  local msgplayer
  if(pPlayer) then
    msgplayer = function(txt) pPlayer:SayText(txt) end
  else
    msgplayer = Msg
  end

  msgplayer("Lua is using " .. collectgarbage("count") .. "KB\n")
end

if(sourceop.FeatureStatus(FEAT_ADMINCOMMANDS)) then
  admincommand.Add("luacount", 65536, 0, "", adminluacount)
end
