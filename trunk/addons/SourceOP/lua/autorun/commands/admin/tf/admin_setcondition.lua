
function adminsetcondition(pPlayer, command, arguments)
  local msgplayer
  if(pPlayer) then
    msgplayer = function(txt) pPlayer:SayText(txt) end
  else
    msgplayer = Msg
  end


  local condition = tonumber(arguments[2])
  if(condition == nil || condition < 1 || condition > 22) then
    msgplayer("Condition " .. arguments[2] .. " is invalid. Try a number 1-22.\n")
    return
  end
  condition = condition - 1

  local enabled
  if(arguments[3] == '1' || arguments[3] == 'on') then
    enabled = true
  elseif(arguments[3] == '0' || arguments[3] == 'off') then
    enabled = false
  else
    msgplayer("Third argument must be 1, 0, on, or off.\n")
    return
  end

  local playerlist = player.Find(arguments[1])
  // make sure there is exactly one player match
  if(#playerlist == 1) then
    local pCondPlayer = playerlist[1]
    local prevcond, newcond
    prevcond, newcond = pCondPlayer:SetCondition(condition, enabled)
    if(enabled) then
      msgplayer("Condition add: " .. tostring(prevcond) .. "->" .. tostring(newcond) .. "\n")
    else
      msgplayer("Condition remove: " .. tostring(prevcond) .. "->" .. tostring(newcond) .. "\n")
      pCondPlayer:SetCondition(condition, enabled)
    end
  // if no players found, print the not found message
  elseif(#playerlist < 1) then
    msgplayer("Unknown player '" .. arguments[1] .. "'.\n")
  // otherwise, more than one player was found
  else
    msgplayer("The name '" .. arguments[1] .. "' is ambiguous.\n")
  end
end

if(sourceop.FeatureStatus(FEAT_ADMINCOMMANDS) && game:IsTF2()) then
  admincommand.Add("setcondition", 8192, 3, "<player> <condition> <1/0>", adminsetcondition)
end
