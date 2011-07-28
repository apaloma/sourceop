
function hlxmsay( pPlayer, command, arguments )
  if(!pPlayer) then
    if(#arguments == 3) then
      local showtime = tonumber(arguments[1])
      local playerid = tonumber(arguments[2])
      local menutext = string.gsub(arguments[3], "\\n", "\n")
      local pMenuPlayer = player.GetByUserID(playerid)
      if(pMenuPlayer) then
        local menu = Menu(pMenuPlayer, 1023, showtime, menutext)
      end
    end
  end
end

concommand.Add("hlx_msay", hlxmsay, "Shows a menu for a player given a length and UserID.")
