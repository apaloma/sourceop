
function nosetupkills_RemoveSetupHook()
  hook.Remove("PlayerDied", "nosetupkills_PlayerDied")
end

function nosetupkills_LevelInit(mapName)
  nosetupkills_RemoveSetupHook()
end

function nosetupkills_LevelShutdown(mapName)
  nosetupkills_RemoveSetupHook()
end

function nosetupkills_SetupStart()
  if(convar.GetBool("DF_setupround_punishkills")) then
    hook.Add("PlayerDied", "nosetupkills_PlayerDied", nosetupkills_PlayerDied)
    if(convar.GetBool("DF_setupround_punishkills_notice")) then
      timer.Simple(1.5,
        (function()
          sourceop.SayTextAll("------------\n")
          sourceop.SayTextAll("Warning: Do not attack players during setup round.\n")
          sourceop.SayTextAll("------------\n")
        end))
     end
  end
end

function nosetupkills_SetupEnd()
  nosetupkills_RemoveSetupHook()
end

function nosetupkills_PlayerDied(pPlayer, pAttacker, weapon)
  if(pPlayer == nil || pAttacker == nil) then
    return
  end

  if(pPlayer:EntIndex() != pAttacker:EntIndex()) then
    local timestamp = os.date("%x %I:%M:%S %p")
    file.Append("setuproundkills.txt",
      "[" .. timestamp .. "] \"" ..
      pAttacker:Name() .. "\" " .. pAttacker:SteamIDRender() .. " (" .. pAttacker:SteamID() .. ") killed " ..
      pAttacker:Name() .. "\" " .. pAttacker:SteamIDRender() .. " (" .. pAttacker:SteamID() .. ") with " ..
      weapon .. "\n")
    timer.Simple(0.01, function() pAttacker:Disconnect("Do not kill players during setup round") end)
  end
end

convar.Add("DF_setupround_punishkills", "1", 0, "Whether or not to punish players who kill during setup round.", true, 0, true, 1)
convar.Add("DF_setupround_punishkills_notice", "1", 0, "Whether or not to print a warning notice when setup round.", true, 0, true, 1)
hook.Add("LevelInit", "nosetupkills_LevelInit", nosetupkills_LevelInit)
hook.Add("LevelShutdown", "nosetupkills_LevelShutdown", nosetupkills_LevelShutdown)
hook.Add("SetupStart", "nosetupkills_SetupStart", nosetupkills_SetupStart)
hook.Add("SetupEnd", "nosetupkills_SetupEnd", nosetupkills_SetupEnd)
