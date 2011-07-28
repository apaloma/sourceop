/** \file admin_addtime.lua
 *  \brief Provides the admin_addtime admin command.
 *  admin_addtime finds the timer entity and adds (or subtracts) time to it.
 *
 * @author Tony Paloma
 * @version 6/26/2010
*/

function foreachtimer( func )
  local timers = ents.FindByClass("team_round_timer")
  for _, v in pairs(timers) do
    // check that the target name also contains timer
    local entname = v:GetName()
    if(entname:lower():find("timer") != nil) then
      func(v)
    end
  end
end

/** Adds time to all time entities. */
function adminaddtime( pPlayer, command, arguments )
  local msgplayer
  if(pPlayer) then
    msgplayer = function(txt) pPlayer:SayText(txt) end
  else
    msgplayer = Msg
  end

  local timertime = tonumber(arguments[1])
  local timers = 0
  foreachtimer(
    function(v)
      v:Fire("AddTime", timertime)
      timers = timers + 1
    end)
  local abstimertime = math.abs(timertime)
  local addorsubtract = "Added"
  local entitysuffix = "ies"
  local secondsuffix = "s"
  if(timertime < 0) then addorsubtract = "Subtracted" end
  if(timers == 1) then entitysuffix = "y" end
  if(abstimertime == 1) then secondsuffix = "" end
  msgplayer(addorsubtract .. " " .. abstimertime .. " second" .. secondsuffix .. " to " .. timers .. " timer entit" .. entitysuffix .. ".\n")
end

function adminsettime( pPlayer, command, arguments )
  local msgplayer
  if(pPlayer) then
    msgplayer = function(txt) pPlayer:SayText(txt) end
  else
    msgplayer = Msg
  end

  local timertime = tonumber(arguments[1])
  local timers = 0
  foreachtimer(
    function(v)
      v:Fire("SetTime", timertime)
      timers = timers + 1
    end)
  local entitysuffix = "ies"
  local secondsuffix = "s"
  if(timers == 1) then entitysuffix = "y" end
  if(timertime == 1) then secondsuffix = "" end
  msgplayer("Set time to " .. timertime .. " second" .. secondsuffix .. " for " .. timers .. " timer entit" .. entitysuffix .. ".\n")
end

if(sourceop.FeatureStatus(FEAT_ADMINCOMMANDS) && game:IsTF2()) then
  admincommand.Add("addtime", 8192, 1, "<time>", adminaddtime)
  admincommand.Add("settime", 8192, 1, "<time>", adminsettime)
end
