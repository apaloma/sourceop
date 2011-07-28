// make offset relative to a nearby variable
local voicespamoffset = sourceop.GetPropOffset("DT_TFPlayer", "m_hRagdoll") + 4
local voicespamplayers = {}

function voicespamenabled(playerIndex)
  local enabled = voicespamplayers[tostring(playerIndex)]
  return enabled != nil
end

function adminvoicespam(pPlayer, command, arguments)
  local msgplayer
  if(pPlayer) then
    msgplayer = function(txt) pPlayer:SayText(txt) end
  else
    msgplayer = Msg
  end

  local playerlist = player.Find(arguments[1])
  // make sure there is exactly one player match
  if(#playerlist == 1) then
    local playerIndex = playerlist[1]:EntIndex()
    if(voicespamenabled(playerIndex)) then
      voicespamplayers[tostring(playerIndex)] = nil
      msgplayer("Voice spam abilities disabled for " .. playerlist[1]:GetName() .. "\n")
    else
      voicespamplayers[tostring(playerIndex)] = playerlist[1]
      msgplayer("Voice spam abilities enabled for " .. playerlist[1]:GetName() .. "\n")
    end
  // if no players found, print the not found message
  elseif(#playerlist < 1) then
    msgplayer("Unknown player '" .. arguments[1] .. "'.\n")
  // otherwise, more than one player was found
  else
    msgplayer("The name '" .. arguments[1] .. "' is ambiguous.\n")
  end
end

function resetlastvoicetime()
  // make sure we found an offset before trying to change anything
  if(voicespamoffset > 4) then
    for _, pPlayer in pairs(voicespamplayers) do
      if(pPlayer:IsPlaying()) then
        pPlayer:SetOffsetFloat(voicespamoffset, 0)
      end
    end
  end
end

function resetspamdisconnected(pPlayer)
  local playerIndex = pPlayer:EntIndex()
  if(voicespamenabled(playerIndex)) then
    voicespamplayers[tostring(playerIndex)] = nil
  end
end

if(sourceop.FeatureStatus(FEAT_ADMINCOMMANDS) && game:IsTF2()) then
  admincommand.Add("voicespam", 8192, 1, "<player>", adminvoicespam)
  hook.Add("Think", "resetlastvoicetime", resetlastvoicetime)
  hook.Add("PlayerDisconnected", "resetvoicespam", resetspamdisconnected)
end
