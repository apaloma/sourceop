// make offset relative to a nearby variable
local arrowlightoffset = sourceop.GetPropOffset("DT_WeaponCompoundBow", "m_bArrowAlight")
local permalightplayers = {}

function permalightenabled(playerIndex)
  local enabled = permalightplayers[tostring(playerIndex)]
  return enabled != nil
end

function adminpermalight(pPlayer, command, arguments)
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
    if(permalightenabled(playerIndex)) then
      permalightplayers[tostring(playerIndex)] = nil
      msgplayer("Permanent arrow light disabled for " .. playerlist[1]:GetName() .. "\n")
    else
      permalightplayers[tostring(playerIndex)] = playerlist[1]
      msgplayer("Permanent arrow light enabled for " .. playerlist[1]:GetName() .. "\n")
    end
  // if no players found, print the not found message
  elseif(#playerlist < 1) then
    msgplayer("Unknown player '" .. arguments[1] .. "'.\n")
  // otherwise, more than one player was found
  else
    msgplayer("The name '" .. arguments[1] .. "' is ambiguous.\n")
  end
end

function adminpermalightself(pPlayer, command, arguments)
  if(pPlayer == nil) then
    return
  end

  local playerIndex = pPlayer:EntIndex()
  if(permalightenabled(playerIndex)) then
    permalightplayers[tostring(playerIndex)] = nil
    pPlayer:SayText("Permanent arrow light disabled for " .. pPlayer:GetName() .. "\n")
  else
    permalightplayers[tostring(playerIndex)] = pPlayer
    pPlayer:SayText("Permanent arrow light enabled for " .. pPlayer:GetName() .. "\n")
  end

end

function permalightcrossbows()
  // make sure we found an offset before trying to change anything
  if(arrowlightoffset > 0) then
    for _, pPlayer in pairs(permalightplayers) do
      if(pPlayer:IsPlaying() && pPlayer:IsAlive() && pPlayer:GetPlayerClass() == TF2_CLASS_SNIPER) then
        local pWeapon = pPlayer:GetActiveWeapon()

        if(pWeapon != nil && pWeapon:GetClassname() == "tf_weapon_compound_bow") then
          if(pWeapon:GetOffsetByte(arrowlightoffset) == 0) then
            pWeapon:SetOffsetByte(arrowlightoffset, 1)
          end
        end
      end
    end
  end
end

function resetpermalightdisconnected(pPlayer)
  local playerIndex = pPlayer:EntIndex()
  if(permalightenabled(playerIndex)) then
    permalightplayers[tostring(playerIndex)] = nil
  end
end

if(sourceop.FeatureStatus(FEAT_ADMINCOMMANDS) && game:IsTF2()) then
  admincommand.Add("permalight", 8192, 1, "<player>", adminpermalight)
  admincommand.Add("permalight_self", 8192, 0, "", adminpermalightself)
  hook.Add("Think", "permalightcrossbows", permalightcrossbows)
  hook.Add("PlayerDisconnected", "resetpermalight", resetpermalightdisconnected)
end
