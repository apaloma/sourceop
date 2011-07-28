local scramblevote_times = 0

function scrambleteams()
  local allplayers = player.GetAll()
  local players = {}
  local players_teamred = {}
  local players_teamblue = {}
  local unbalancelimit = convar.GetInteger("mp_teams_unbalance_limit")
  convar.SetInteger("mp_teams_unbalance_limit", 0)

  // filter and reindex the players
  for _, v in pairs(allplayers) do
    if(v:GetTeam() >= 2 && v:GetTeam() <= 3) then
      table.insert(players,v)
    end
  end

  //Msg("total: " .. tostring(table.Count(players)) .. "\n")
  local teamplayersleft= math.floor(table.Count(players) / 2)
  while teamplayersleft > 0 do
    local high = table.Count(players)
    local choice = math.random(1, high)
    //Msg("math.random(1, " .. tostring(high) .. ") = " .. tostring(choice) .. "\n")
    local chosenplayer = players[choice]
    //Msg(" - " .. tostring(chosenplayer) .. "\n")
    table.remove(players, choice)
    table.insert(players_teamred, chosenplayer)
    //PrintTable(players)
    teamplayersleft = teamplayersleft - 1
  end

  //Msg("Red: " .. tostring(table.Count(players_teamred)) .. "\n")
  // place random players on red
  for _, v in pairs(players_teamred) do
    v:SetTeam(2)
  end

  //Msg("Blue: " .. tostring(table.Count(players_teamred)) .. "\n")
  // place all remaining players on blue
  for _, v in pairs(players) do
    v:SetTeam(3)
  end

  convar.SetInteger("mp_teams_unbalance_limit", unbalancelimit)

  // Restart the game and reset the timelimit appropriately
  local oldtimeleft = game.GetTimeRemaining()
  local timelimit = game.GetTimeLimit()
  convar.SetInteger("mp_restartgame", 1)
  timer.Simple(2, (function()
    Msg("Time check\n")
    // if time has been added to the map timer, set it back
    local newtimeleft = game.GetTimeRemaining()
    if(timelimit > 0 && newtimeleft > oldtimeleft) then
      Msg("newtimeleft: " .. tostring(newtimeleft) .. " oldtimeleft: " .. tostring(oldtimeleft) .. "\n")
      local newtimelimit = timelimit - math.floor((newtimeleft - oldtimeleft) / 60)

      // do not set timelimit below 1
      if(newtimelimit < 1) then
        newtimelimit = 1
      end

      Msg("mp_timelimit " .. tostring(newtimelimit) .. "\n")
      convar.SetInteger("mp_timelimit", newtimelimit)
    end
  end))
end

/*function scrambleteams()
  local oldtimeleft = game.GetTimeRemaining()
  local timelimit = game.GetTimeLimit()

  gamerules.SetScrambleTeams(true)
  //game.ConsoleCommand("mp_scrambleteams")
  //game.ServerExecute()
  convar.SetInteger("mp_restartgame", 1)

  timer.Simple(2, (function()
    Msg("Time check\n")
    // if time has been added to the map timer, set it back
    local newtimeleft = game.GetTimeRemaining()
    if(timelimit > 0 && newtimeleft > oldtimeleft) then
      Msg("newtimeleft: " .. tostring(newtimeleft) .. " oldtimeleft: " .. tostring(oldtimeleft) .. "\n")
      local newtimelimit = timelimit - math.floor((newtimeleft - oldtimeleft) / 60)

      // do not set timelimit below 1
      if(newtimelimit < 1) then
        newtimelimit = 1
      end

      Msg("mp_timelimit " .. tostring(newtimelimit) .. "\n")
      convar.SetInteger("mp_timelimit", newtimelimit)
    end
  end))
end*/

function scramblevoteover( vote )
  local yesvotes            = vote.picks[1]
  local total               = vote.picks[1] + vote.picks[2]
  local yespercentage       = (yesvotes / total) * 100
  local requiredpercentage  = convar.GetNumber("DF_scramblevote_percentage")
  local percentyes          = tostring(math.floor(yespercentage)) .. "%"

  sourceop.SayTextAll("The vote to scramble got " .. yesvotes .. "/" .. total .. " (" .. percentyes .. ") yes votes.\n")
  if(yespercentage >= requiredpercentage) then
    scrambleteams()
  end
end

function adminscramblevote( pPlayer, command, arguments )
  local limit = convar.GetInteger("DF_scramblevote_limit")
  if(limit > 0 && scramblevote_times < limit) then
    Vote("Scramble the teams?", {"Yes", "No"}, 30, "", scramblevoteover)
    scramblevote_times = scramblevote_times + 1
  else
    local msgplayer
    if(pPlayer) then
      msgplayer = function(txt) pPlayer:SayText(txt) end
    else
      msgplayer = Msg
    end

    msgplayer("No more scramble votes are allowed this map.\n")
  end
end

function adminscramble( pPlayer, command, arguments )
  scrambleteams()

  if(!pPlayer) then
    Msg("The teams have been scrambled.\n")
  end
  DelayedSayToAll("The teams have been scrambled.\n")
end

function scramble_levelinit()
  scramblevote_times = 0
end

if(sourceop.FeatureStatus(FEAT_ADMINCOMMANDS)) then
  admincommand.Add("scramble", 2048, 0, "", adminscramble)
  admincommand.Add("scramblevote", 2048, 0, "", adminscramblevote)
  convar.Add("DF_scramblevote_percentage", "51", 0, "The percentage of yes votes required to scramble the teams.", true, 0, true, 100)
  convar.Add("DF_scramblevote_limit", "1", 0, "The number of times per map a scramble vote is allowed. A value of 0 means no limit.", true, 0, false, 0)
  hook.Add("LevelInit", "scrambleinit", scramble_levelinit)
end
