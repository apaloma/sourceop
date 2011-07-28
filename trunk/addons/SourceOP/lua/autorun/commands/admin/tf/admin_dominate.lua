local dominatedoffset = sourceop.GetPropOffset("DT_TFPlayer", "m_Shared") + sourceop.GetPropOffset("DT_TFPlayerSharedLocal", "m_bPlayerDominated")
local dominatingmeoffset = sourceop.GetPropOffset("DT_TFPlayer", "m_Shared") + sourceop.GetPropOffset("DT_TFPlayerSharedLocal", "m_bPlayerDominatingMe")

function dominateplayer(pPlayer, pDominated)
  // check that the player is not already dominated
  if(pPlayer:GetOffsetByte(dominatedoffset + pDominated:EntIndex()) == 0) then
    local event = {userid=pDominated:UserID(),
        attacker=pPlayer:UserID(),
        weapon="domination",
        weaponid=0,
        damagebits=0,
        customkill=0,
        assister=-1,
        dominated=1,
        assister_dominated=0,
        revenge=0,
        assister_revenge=0,
        weapon_logclassname="domination",
        first_blood=0,
        stun_flags=0,
        priority=7}
    pPlayer:SetOffsetByte(dominatedoffset + pDominated:EntIndex(), 1)
    pDominated:SetOffsetByte(dominatingmeoffset + pPlayer:EntIndex(), 1)
    game.FireEvent("player_death", event)
  end
end

function admindominate(pPlayer, command, arguments)
  if(pPlayer) then
    local playerlist = player.Find(arguments[1])
    // make sure there is exactly one player match
    if(#playerlist == 1) then
      dominateplayer(pPlayer, playerlist[1])
      pPlayer:SayText("Dominating " .. playerlist[1]:GetName() .. ".\n")
    // if no players found, print the not found message
    elseif(#playerlist < 1) then
      pPlayer:SayText("Unknown player '" .. arguments[1] .. "'.\n")
    // otherwise, more than one player was found
    else
      pPlayer:SayText("The name '" .. arguments[1] .. "' is ambiguous.\n")
    end
  end
end

function admindominateteam(pPlayer, command, arguments)
  if(pPlayer) then
    local teamnumber = string.lower(arguments[1])

    // if this is a number, use that number
    if(tonumber(teamnumber) != nil) then
      teamnumber = tonumber(teamnumber)
    // otherwise, try and find the team number
    else
      teamnumber = gamerules.GetTeamIndex(teamnumber)
    end

    // make sure the number used is valid
    if(gamerules.IsValidTeamNumber(teamnumber)) then
      local players = player.GetAll()
      for _, v in pairs(players) do
        if(v:GetTeam() == teamnumber) then
          dominateplayer(pPlayer, v)
        end
      end
    else
      pPlayer:SayText("Invalid team entered. Try a number 0-5 or a team name.\n")
    end
  end
end

function adminsetdomination(pPlayer, command, arguments)
  local msgplayer
  if(pPlayer) then
    msgplayer = function(txt) pPlayer:SayText(txt) end
  else
    msgplayer = Msg
  end

  local pl1 = player.Find(arguments[1])
  local pl2 = player.Find(arguments[2])

  if(#pl1 < 1) then
    msgplayer("Unknown player '" .. arguments[1] .. "'.\n")
    return
  end
  if(#pl2 < 1) then
    msgplayer("Unknown player '" .. arguments[2] .. "'.\n")
    return
  end
  if(#pl1 > 1) then
    msgplayer("The name '" .. arguments[1] .. "' is ambiguous.\n")
    return
  end
  if(#pl2 > 1) then
    msgplayer("The name '" .. arguments[2] .. "' is ambiguous.\n")
    return
  end

  dominateplayer(pl1[1], pl2[1])
  msgplayer(pl1[1]:GetName() .. " is dominating " .. pl2[1]:GetName() .. ".\n")
end

if(sourceop.FeatureStatus(FEAT_ADMINCOMMANDS) && game:IsTF2()) then
  if(dominatedoffset > 400 && dominatingmeoffset > 400 && dominatedoffset != dominatingmeoffset) then
    admincommand.Add("dominate", 8192, 1, "<player>", admindominate)
    admincommand.Add("dominate_team", 8192, 1, "<team>", admindominateteam)
    admincommand.Add("setdomination", 8192, 2, "<player> <dominated player>", adminsetdomination)
  else
    // probably bad offsets
    Msg("Invalid offsets for admin_dominate.\n")
  end
end
