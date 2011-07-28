
function hlxpsay( pPlayer, command, arguments )
  if(!pPlayer) then
    if(#arguments == 2) then
      local playerid = tonumber(arguments[1])
      local saytext = arguments[2]
      local pSayPlayer = player.GetByUserID(playerid)
      if(pSayPlayer) then
        pSayPlayer:SayTextNoSOP(saytext .. "\n")
      end
    end
  end
end

concommand.Add("hlx_psay", hlxpsay, "Private message to player given a UserID.")
