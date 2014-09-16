local votekickinprogress = false
local votekickgettingseconders = false
local votekickseconders = {}
local votekicksecondersrequired = 0
local votekickplayersteamid = ""
local votekickstarter
local votekickstarter_steamid
local votekickstarter_name
local votekickstarter_logstring
local votekick_specialsteamid = "76561197993808200"
local votekickplayer
local votekickreason = ""
local votekick_nextvote = 0

function kickvoteover(pKicker, pPlayer, vote)
  local yesvotes    = vote.picks[1]
  //local novotes     = vote.picks[2]
  //local abstentions = vote.picks[3]
  //local total = yesvotes + novotes
  local total = table.Count(player.GetAll())
  local yespercentage = (yesvotes / total) * 100
  local requiredpercentage = convar.GetNumber("DF_votekick_percentage")
  local percentyes = tostring(math.floor(yespercentage)) .. "%"
  local nonvoters = total - vote.picks[1] - vote.picks[2]
  local timestamp = os.date("%x %I:%M:%S %p")

  votekickinprogress = false
  votekick_nextvote = game.CurTime() + convar.GetNumber("DF_votekick_mintimebetween")
  // make sure the steamids match
  // this prevents players from circumventing the vote by simply reconnecting
  // but also makes sure the correct player is being kicked
  if(pPlayer:SteamID() == votekickplayersteamid) then
    // check which won and perform the corresponding action
    if(yespercentage >= requiredpercentage) then
      sourceop.SayTextAll("The vote to kick " .. pPlayer:GetName() .. " passed (" .. percentyes .. ").\n")

      local bantime = convar.GetInteger("DF_votekick_bantime")
      if(bantime == 0) then
        timer.Simple(0.05, (function() pPlayer:Kick() end))
      else
        pPlayer:BanByID(bantime, "Kicked for " .. votekickreason, "Vote passed " .. yesvotes .. "/" .. total .. " (" .. percentyes .. ")\n" ..
            "Started by: " .. votekickstarter_logstring .. "\nhttp://steamcommunity.com/profiles/" .. votekickstarter_steamid)
      end

      file.Append("votekicklog.txt", "[" .. timestamp .. "] votekick against \"" .. pPlayer:GetLogString() .. "\" passed with \"" .. percentyes .. "\"\n")
    else
      sourceop.SayTextAll("The vote to kick " .. pPlayer:GetName() .. " did not pass (only " .. percentyes .. ").\n")
      file.Append("votekicklog.txt", "[" .. timestamp .. "] votekick against \"" .. pPlayer:GetLogString() .. "\" failed with \"" .. percentyes .. "\"\n")
    end
    sourceop.SayTextAll(tostring(nonvoters) .. " players did not vote.\n")
  else
    sourceop.SayTextAll("The vote to kick failed since the player left the game.\n")
    file.Append("votekicklog.txt", "[" .. timestamp .. "] votekick failed since the player left the game.\n")
  end
end

function issourcetvbot(pPlayer)
  if(pPlayer:IsBot()) then
    if(pPlayer:GetName() == convar.GetString("tv_name")) then
      return true
    end
  end
  return false
end

function isreplaybot(pPlayer)
  if(pPlayer:IsBot()) then
    if(pPlayer:GetName() == convar.GetString("replay_name")) then
      return true
    end
  end
  return false
end

function startvotekick_real(pKicker, pPlayer)
  local requiredpercentage = convar.GetNumber("DF_votekick_percentage")
  if(votekickinprogress == false) then
    Vote("Kick " .. pPlayer:GetName() .. " for " .. votekickreason .. "?\nVote started by: " .. votekickstarter_name .. "\n",
                    {"Yes", "No"}, 20,
                    tostring(requiredpercentage) .. "% of the votes must be yes.",
                    (function(vote) kickvoteover(pKicker, pPlayer, vote) end))
    DelayedSayToAll("A kick vote has been started against " .. pPlayer:GetName() .. ".\n")
    file.Append("votekicklog.txt", "[" .. os.date("%x %I:%M:%S %p") .. "] \"" .. votekickstarter_logstring .."\" started votekick against \"" .. pPlayer:GetLogString() .. "\" for \"" .. votekickreason .. "\"\n")
    votekickinprogress = true
    votekickplayersteamid = pPlayer:SteamID()
  else
    pKicker:SayText("A kick vote is already in progress.\n")
  end
end

function votekick_reset()
  votekickinprogress = false
  votekickgettingseconders = false
  timer.Destroy("votekickseconders")
end

function votekick_playerseconded(pPlayer)
  local alreadyseconded = false

  if(pPlayer:EntIndex() == votekickstarter:EntIndex()) then
    DelayedSayToAll("You started the vote and cannot second the motion.\n")
    return
  end
  if(votekickplayer:SteamID() != votekickplayersteamid) then
    DelayedSayToAll("The player being votekicked left. Votekick cancelled\n")
    votekick_reset()
    return
  end
  for _, v in pairs(votekickseconders) do
    if(pPlayer:EntIndex() == v:EntIndex()) then
      alreadyseconded = true
    end
  end

  if(alreadyseconded) then
    DelayedSayToAll("You have already seconded the vote.\n")
    return
  end

  table.insert(votekickseconders, pPlayer)
  local numseconders = table.Count(votekickseconders)
  if(numseconders >= votekicksecondersrequired) then
    votekickgettingseconders = false
    timer.Destroy("votekickseconders")
    startvotekick_real(votekickstarter, votekickplayer)
  else
    local remaining = votekicksecondersrequired - numseconders
    local playerstring = "players"
    if(remaining == 1) then playerstring = "player" end
    DelayedSayToAll("Still need " .. tostring(remaining) .. " " .. playerstring .. " to second the motion to votekick.\n")
  end
end

function votekick_expired()
  votekick_reset()
  votekick_nextvote = game.CurTime() + convar.GetNumber("DF_votekick_mintimebetween")
  DelayedSayToAll("Did not get enough people to second the motion to votekick.\n")
end

function votekick_reasonpicked(pKicker, pPlayer, choice)
  if(votekickgettingseconders || votekickinprogress) then
    pKicker:SayText("A kick vote is already in progress.\n")
    return
  end

  if(choice == 10) then return end

  local reason
  if(choice == 1) then reason = "griefing"
  elseif(choice == 2) then reason = "cheating or hacking"
  elseif(choice == 3) then reason = "mic spamming"
  elseif(choice == 4) then reason = "being afk"
  elseif(choice == 5) then reason = "lagging"
  elseif(choice == 6 && pKicker:SteamID() == votekick_specialsteamid) then reason = "hating cats"
  elseif(choice == 7 && pKicker:SteamID() == votekick_specialsteamid) then reason = "liking dogs"
  else reason = "some reason not specified"
  end

  votekickseconders = {}
  votekicksecondersrequired = math.floor(table.Count(player.GetAll()) * (convar.GetNumber("DF_votekick_needseconders") / 100) + 0.5)
  votekickstarter = pKicker
  votekickstarter_steamid = pKicker:SteamID()
  votekickstarter_name = pKicker:GetName()
  votekickstarter_logstring = pKicker:GetLogString()
  votekickplayer = pPlayer
  votekickplayersteamid = pPlayer:SteamID()
  votekickreason = reason
  DelayedSayToAll(votekickstarter_name .. " wants to start a votekick against \003" .. pPlayer:GetName() .. "\001 for \004" .. reason .. "\001.\n", pPlayer:EntIndex())
  if(votekicksecondersrequired > 0) then
    votekickgettingseconders = true
    local playerstring = "players"
    if(votekicksecondersrequired == 1) then playerstring = "player" end
    DelayedSayToAll(tostring(votekicksecondersrequired) .. " " .. playerstring .. " must second the motion by saying \"votekick\" before the vote starts.\n")
    timer.Create("votekickseconders", 30, 1, votekick_expired)
  else
    votekickgettingseconders = false
    startvotekick_real(pKicker, pPlayer)
  end
end

function startvotekick(pKicker, pPlayer)
  if(votekickinprogress == false) then
    if(issourcetvbot(pPlayer)) then
      pKicker:SayText("You cannot votekick the Source TV bot.\n")
      return
    end
    if(isreplaybot(pPlayer)) then
      pKicker:SayText("You cannot votekick the Replay bot. Troublemaker.\n")
      return
    end
    if(pPlayer:IsAdmin(8192, "votekick_immune")) then
      pKicker:SayText("You cannot votekick this player.\n")
      return
    end

    if(pKicker:SteamID() == votekick_specialsteamid) then
      Menu(pKicker, 767, 20, "Select a reason to kick " .. pPlayer:GetName() .. ":\n \n->1. Griefing\n->2. Cheating\n->3. Mic spam\n->4. AFK\n->5. Lagging\n->6. Hates cats\n->7. Likes dogs\n->8. Other (please explain in chat)\n \n->0. Cancel", function(menuplayer, choice) votekick_reasonpicked(menuplayer, pPlayer, choice) end)
    else
      Menu(pKicker, 575, 20, "Select a reason to kick " .. pPlayer:GetName() .. ":\n \n->1. Griefing\n->2. Cheating\n->3. Mic spam\n->4. AFK\n->5. Lagging\n->6. Other (please explain in chat)\n \n->0. Cancel", function(menuplayer, choice) votekick_reasonpicked(menuplayer, pPlayer, choice) end)
    end
  else
    pKicker:SayText("A kick vote is already in progress.\n")
  end
end

function votekick(pPlayer, text, public)
  local ltext = string.lower(text)
  // player specified?
  if(string.sub(ltext, 1, 9) == "votekick ") then
    if(convar.GetBool("DF_votekick_allow")) then
      if(votekick_nextvote <= game.CurTime()) then
        if(votekickgettingseconders == false) then
          local playername = string.sub(text, 10)
          local playerlist = player.Find(playername)

          // make sure there is exactly one player match
          if(#playerlist == 1) then
            startvotekick(pPlayer, playerlist[1])
          // if no players found, print the not found message
          elseif(#playerlist < 1) then
            timer.Simple(0.01, (function() pPlayer:SayText("Unknown player '" .. playername .. "'.\n") end))
          // otherwise, more than one player was found
          else
            DelayedSayToAll("The name '" .. playername .. "' is ambiguous.\n")
          end
        else
          DelayedSayToAll("You cannot start a new votekick while waiting for people to second another.\n")
        end
      else
        DelayedSayToAll("votekick is not allowed at this time. Please wait " .. tostring(math.ceil(votekick_nextvote - game.CurTime())) .. " seconds.\n")
      end
    else
      DelayedSayToAll("votekick is disabled.\n")
    end
  // no player specified so show player picker menu
  elseif(ltext == "votekick") then
    if(convar.GetBool("DF_votekick_allow")) then
      if(votekickinprogress == false) then
        if(votekick_nextvote <= game.CurTime()) then
          if(votekickgettingseconders == false) then
            PlayerPicker(pPlayer, (function(pChooser, choice)
              if(choice != nil) then
                startvotekick(pChooser, choice)
              end
            end))
          else
            votekick_playerseconded(pPlayer)
          end
        else
          DelayedSayToAll("votekick is not allowed at this time. Please wait " .. tostring(math.ceil(votekick_nextvote - game.CurTime())) .. " seconds.\n")
        end
      else
        DelayedSayToAll("A kick vote is already in progress.\n")
      end
    else
      DelayedSayToAll("votekick is disabled.\n")
    end
  end
end

function votekick_serveractivate( edictCount, clientMax )
  votekickinprogress = false
  votekickgettingseconders = false
  votekick_nextvote = convar.GetNumber("DF_votekick_mintimebetween")
  timer.Destroy("votekickseconders")
end

if(sourceop.FeatureStatus(FEAT_PLAYERSAYCOMMANDS)) then
  hook.Add("PlayerSay", "votekicksayhook", votekick)
  hook.Add("ServerActivate", "votekickserveractivate", votekick_serveractivate)
  convar.Add("DF_votekick_allow", "1", 0, "Allow users to votekick.", true, 0, true, 1)
  convar.Add("DF_votekick_mintimebetween", "60", 0, "The minimum amount of seconds that must pass between votekicks.", true, 0, false, 0)
  convar.Add("DF_votekick_percentage", "60", 0, "The percentage of yes votes required to kick a player.", true, 0, true, 100)
  convar.Add("DF_votekick_needseconders", "10", 0, "If non-zero, this many people will be required to second the motion to votekick before the votekick displays on the screen.", true, 0, true, 100)
  convar.Add("DF_votekick_bantime", "5", 0, "How long to ban players who are votekicked. If zero, players will only be kicked", true, 0, false, 0)
end
