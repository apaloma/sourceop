function cblock_executestringcommand(pPlayer, command)
  // Kick players who run commands before becoming in game.
  // Allow some commands.
  // In theory, menuclosed should be the only one to ever happen but I added a
  // couple more that occur real close to player entering game just in case.
  if(!pPlayer:IsPlaying() &&
      command != "menuclosed" &&
      command != "VModEnable" &&
      command != "vban") then

    // don't kick for some commands, but still block
    if(command != "spec_next" &&
        command != "spec_prev" &&
        command != "taunt" &&
        command != "voicemenu") then
      local timestamp = os.date("%x %I:%M:%S %p")

      file.Append("commandblock.txt", "[" .. timestamp .. "] \"" .. pPlayer:Name() .. "\" " .. pPlayer:SteamIDRender() .. " (" .. pPlayer:SteamID() .. "): " .. command .. "\n")
      pPlayer:Disconnect("Cannot run commands before entering game")
    end
    return true
  end
  return false
end

hook.Add("ExecuteStringCommand", "commandblockeschook", cblock_executestringcommand)
