
local aba_removestaleinterval = 60
local aba_history = {}

function aba_removestale()
  local staletime = convar.GetInteger("DF_autobalanceenforcer_time")
  local curtime = CurTime()
  for id, historyItem in pairs(aba_history) do
    local expiration = historyItem[1]
    if(expiration <= CurTime()) then
      ////Msg("Removing stale entry for " .. id .. "\n")
      aba_history[id] = nil
    end
  end
end

function aba_punishplayer(pPlayer, extra)
  local dealmethod = convar.GetInteger("DF_autobalanceenforcer_dealmethod")

  if(dealmethod == 1) then
    pPlayer:Kick()
  elseif(dealmethod == 2) then
    pPlayer:BanByID(convar.GetInteger("DF_autobalanceenforcer_bantime"), "Autobalance avoidance", extra)
  else
    pPlayer:BanByID(0, "Autobalance avoidance", extra)
  end
end

function aba_gameevent(eventName, eventKeyVals)
  ////Msg("Event " .. eventName .. "\n")
  if(convar.GetInteger("DF_autobalanceenforcer_enabled") == 0) then
    return
  end

  if(eventName == "teamplay_teambalanced_player") then
    local playerIndex = eventKeyVals["player"]
    local newTeam = eventKeyVals["team"]
    local pPlayer = player.GetByID(playerIndex)
    if(!pPlayer:IsBot() && newTeam != nil) then
      // if the player is not admin or immunity is disabled then log
      if(!pPlayer:IsAdmin() || convar.GetInteger("DF_autobalanceenforcer_adminsimmune") == 0) then
        local steamid = pPlayer:SteamID()
        local staletime = convar.GetInteger("DF_autobalanceenforcer_time")
        local curtime = CurTime()
        ////Msg(steamid .. " is being balanced to team " .. tostring(newTeam) .. "\n")
        if(ValidSteamID(steamid)) then
          aba_history[steamid] = {curtime + staletime, newTeam, curtime}
        end
      end
    end
  end
end

function aba_levelinit()
  // old timers by the same name are automatically removed
  timer.Create("aba_removestale", aba_removestaleinterval, 0, aba_removestale)
  aba_history = {}
end

function aba_playerchangedteam(pPlayer, newTeam)
  if(convar.GetInteger("DF_autobalanceenforcer_enabled") == 0) then
    return
  end

  // The teamplay_teambalanced_player event always comes after the team switch
  // event and we do not want a player that has been balanced just after being
  // balanced earlier to be punished.
  // Delay handling the team switch to avoid this.
  // Use named timer to remove old timers when, for example, a player is
  // balanced during a team switch.
  timer.Create("aba_delayteamswitch" .. pPlayer:SteamID(), 0.05, 1, (function() aba_playerchangedteam_delayed(pPlayer, newTeam) end))
end

function aba_playerchangedteam_delayed(pPlayer, newTeam)
  // it's possible the player has disconnected after receiving the original 
  // team switch event
  if(!pPlayer:IsPlaying()) then
    return
  end

  local steamid = pPlayer:SteamID()
  local historyCheck = aba_history[steamid]
  if(historyCheck != nil) then
    local expiration = historyCheck[1]
    local forcedTeam = historyCheck[2]
    local balanceTime = historyCheck[3]
    local forcedTeamName = gamerules.GetIndexedTeamName(forcedTeam) or tostring(forcedTeam)
    local newTeamName = gamerules.GetIndexedTeamName(newTeam) or tostring(newTeam)
    local allowTeamSwitch = convar.GetBool("DF_autobalanceenforcer_allowteamswitch")
    local curtime = CurTime()
    ////Msg("Player has history on " .. tostring(balanceTime) .. " was moved to team " .. forcedTeamName .. "\n")
    if(expiration > curtime && forcedTeam != newTeam && (!allowTeamSwitch || newTeamName == "Spectator")) then
      local extraInfo = "Player was balanced to team " .. forcedTeamName .. " but then switched to team " .. newTeamName .. " " .. string.format("%.2f", curtime - balanceTime) .. " seconds later."
      ////Msg("Player needs to be punished\n  " .. extraInfo .. "\n")
      aba_punishplayer(pPlayer, extraInfo)
    end
  end
end

function aba_playeractivate(pPlayer)
  local steamid = pPlayer:SteamID()
  local historyCheck = aba_history[steamid]
  if(historyCheck != nil) then
    local expiration = historyCheck[1]
    local forcedTeam = historyCheck[2]
    local balanceTime = historyCheck[3]
    local curtime = CurTime()
    ////Msg("Player has history on " .. tostring(balanceTime) .. " reconnected\n")
    if(expiration > curtime && forcedTeam != newTeam) then
      ////Msg("Player needs to be punished\n")
      timer.Simple(0.01, function()
        aba_punishplayer(pPlayer, "Player was balanced to team " .. tostring(forcedTeam) .. " but then reconnected " .. string.format("%.2f", curtime - balanceTime) .. " seconds later.")
      end)
    end
  end
end

function aba_preswitchteams()
  ////Msg("Teams are being switched.\n")
  // we know how to handle tf2 team switch
  if(game:IsTF2()) then
    for id, historyItem in pairs(aba_history) do
      if(historyItem[2] == 2) then
        ////Msg("Switching history item for " .. id .. " to team 3\n")
        historyItem[2] = 3
      elseif(historyItem[2] == 3) then
        ////Msg("Switching history item for " .. id .. " to team 2\n")
        historyItem[2] = 2
      else // wat
        aba_history[id] = nil
      end
    end
  else
    aba_history = {}
  end
end

function aba_prescrambleteams()
  ////Msg("Teams are being scrambled.\n")
  aba_history = {}
end

convar.Add("DF_autobalanceenforcer_enabled", "0", 0, "Enable automatic kicking or banning of players who circumvent the autobalancer.", true, 0, true, 1)
convar.Add("DF_autobalanceenforcer_dealmethod", "2", 0, "Punishment for offenders. 1: Kick, 2: Temporary ban, 3: Permanent ban", true, 1, true, 3)
convar.Add("DF_autobalanceenforcer_bantime", "5", 0, "Number of minutes to ban players if the temporary ban deal method is selected.")
convar.Add("DF_autobalanceenforcer_time", "60", 0, "The number of seconds a player must wait before switching teams after being balanced to avoid being punished.")
convar.Add("DF_autobalanceenforcer_adminsimmune", "1", 0, "Whether or not admins are immune to being punished for autobalance avoidance.", true, 0, true, 1)
convar.Add("DF_autobalanceenforcer_allowteamswitch", "1", 0, "Whether or not switching teams (except to spec) after being autobalanced is allowed.", true, 0, true, 1)
hook.Add("GameEvent", "aba_gameevent", aba_gameevent)
hook.Add("LevelInit", "aba_levelinit", aba_levelinit)
hook.Add("PlayerChangedTeam", "aba_playerchangedteam", aba_playerchangedteam)
hook.Add("PreSwitchTeams", "aba_preswitchteams", aba_preswitchteams)
hook.Add("PlayerActivate", "aba_playeractivate", aba_playeractivate)
hook.Add("PreScrambleTeams", "aba_prescrambleteams", aba_prescrambleteams)
sourceop.AddGameEventListener("teamplay_teambalanced_player")
aba_levelinit()