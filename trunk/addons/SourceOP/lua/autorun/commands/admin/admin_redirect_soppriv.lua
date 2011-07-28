function redirectplayer( pPlayer, delayTime, server )
  //pPlayer:ShowWebsite("redirect", "about:<meta http-equiv=\"REFRESH\" content=\"" .. tostring(delayTime) .. ";url=steam://connect/" .. server .. "\">", false)
  pPlayer:ShowWebsite("redirect", "http://www.sourceop.com/redir.php?d=" .. tostring(delayTime) .. "&s=" .. server, false)
end

function adminredirect( pPlayer, command, arguments )
  local msgplayer
  if(pPlayer) then
    msgplayer = function(txt) pPlayer:SayText(txt) end
  else
    msgplayer = Msg
  end

  local players = player.GetAll()
  local playersToRedirect = tonumber(arguments[1])
  local delayTime = tonumber(arguments[3]) or 0
  local playersRedirected = 0

  if(playersToRedirect <= 0 || playersToRedirect > table.Count(players)) then
    msgplayer("Invalid number of players to redirect.\n")
    return
  end

  if(delayTime < 0) then
    msgplayer("Invalid delay time.\n")
    return
  end

  // redirect players with least time played
  for i=1,playersToRedirect do
    local smallestTime = -1
    local smallestPlayer = nil
    local smallestPlayerIndex
    for k, v in pairs(players) do
      local currentTimePlayed = v:GetTimePlayed()
      if((smallestTime == -1 || currentTimePlayed < smallestTime) && v:IsPlaying()) then
        smallestTime = currentTimePlayed
        smallestPlayer = v
        smallestPlayerIndex = k
      end
    end

    if(smallestPlayer != nil) then
      table.remove(players, smallestPlayerIndex)
      playersRedirected = playersRedirected + 1
      redirectplayer(smallestPlayer, delayTime, arguments[2])
    else
      break
    end
  end

  local playerString = (playersRedirected == 1) and "player" or "players"
  local secondsString = (delayTime == 1) and "second" or "seconds"
  if(delayTime > 0) then
    msgplayer(tostring(playersRedirected) .. " " .. playerString .. " will be redirected in " .. tostring(delayTime) .. " " .. secondsString .. ".\n")
  else
    msgplayer(tostring(playersRedirected) .. " " .. playerString .. " redirected.\n")
  end
end

function adminredirectall( pPlayer, command, arguments )
  local msgplayer
  if(pPlayer) then
    msgplayer = function(txt) pPlayer:SayText(txt) end
  else
    msgplayer = Msg
  end

  local players = player.GetAll()
  local delayTime = tonumber(arguments[2]) or 0
  local playersRedirected = 0

  if(delayTime < 0) then
    msgplayer("Invalid delay time.\n")
    return
  end

  for _, v in pairs(players) do
    redirectplayer(v, delayTime, arguments[1])
    playersRedirected = playersRedirected + 1
  end

  local playerString = (playersRedirected == 1) and "player" or "players"
  local secondsString = (delayTime == 1) and "second" or "seconds"
  if(delayTime > 0) then
    msgplayer(tostring(playersRedirected) .. " " .. playerString .. " will be redirected in " .. tostring(delayTime) .. " " .. secondsString .. ".\n")
  else
    msgplayer(tostring(playersRedirected) .. " " .. playerString .. " redirected.\n")
  end
end

if(sourceop.FeatureStatus(FEAT_ADMINCOMMANDS)) then
  admincommand.Add("redirect", 8192, 2, "<number of players> <server ip> [delay]", adminredirect)
  admincommand.Add("redirect_all", 8192, 1, "<server ip>", adminredirectall)
end
