local cheatcvars = {
{"sv_cheats"               , "replicated"},
// {"sv_pure"                 , "replicated"}, //sv_pure is not a cvar and thus not queryable
{"mat_fullbright"          , "0"},
{"mat_fillrate"            , "0"},
{"r_drawothermodels"       , "1"},
{"host_timescale"          , "replicated"},
{"mat_drawwater"           , "1"},
{"snd_show"                , "0"},
// {"host_framerate"          , "0"}, // changeable even when cheats are off
{"mat_wireframe"           , "0"},
{"cam_idealyaw"            , "0"},
{"mat_reversedepth"        , "0"},
{"mat_luxels"              , "0"},
{"r_drawopaquerenderables" , "1"}
}
local cheatcvarsindex = 1
local cheatcvarcheckinterval = 3


function cvarcheck_handleplayer(pPlayer, name, value)
  cvarcheck_handleplayer_message(pPlayer, "Cheating: " .. name .. " set to " .. tostring(value))
end

function cvarcheck_handleplayer_message(pPlayer, message)
  local dealmethod = convar.GetInteger("DF_cvarcheck_dealmethod")

  if(dealmethod == 1) then
    pPlayer:Kick()
  elseif(dealmethod == 2) then
    pPlayer:BanByID(convar.GetInteger("DF_cvarcheck_bantime"), message)
  else
    pPlayer:BanByID(0, message)
  end
end

function cvarcheck_querycomplete(pPlayer, estatus, name, value, servervalue, cvarInfo)
  local cvarChecked = cvarInfo[1]
  local expectedCvarValue = cvarInfo[2]
  local eQueryCvarValueStatus_ValueIntact = 0
  local playerIndex = pPlayer:EntIndex()
  local sv_cheats_value = convar.GetInteger("sv_cheats")

  ////Msg(name .. " = " .. tostring(value) .. "\n")

  if(sv_cheats_value != 0) then
    ////Msg("Aborting query due to cheats\n")
    // all bets are off
    return
  end

  if(cvarChecked != name) then
    //cvarcheck_handleplayer_message(pPlayer, "Cheating: Queried CVAR " .. cvarChecked .. " but received " .. name .. "=" .. tostring(value))
    // TODO: Apparently this can be hit accidently. Fix it.
    local timestamp = os.date("%x %I:%M:%S %p")
    local fileMessage = v:GetName() .. " queried CVAR " .. cvarChecked .. " but received " .. name .. "=" .. tostring(value) .. "\n"
    Msg(fileMessage)
    file.Append("cvarcheck_mismatch.txt", "[" .. timestamp .. "] " .. fileMessage)
    return
  end

  if(estatus != eQueryCvarValueStatus_ValueIntact) then
    cvarcheck_handleplayer_message(pPlayer, "Cheating: Blocked query of " .. name)
    return
  end

  if(expectedCvarValue == "replicated") then
    local currentServerValue = convar.GetString(cvarChecked)
    ////Msg(" - replicated, checking for " .. currentServerValue .. "\n")
    if(value != servervalue && value != currentServerValue) then
      cvarcheck_handleplayer(pPlayer, name, value)
    end
  else
    if(value != expectedCvarValue) then
      cvarcheck_handleplayer(pPlayer, name, value)
    end
  end

  ////Msg("\n")
end

function cvarcheck_query()
  if(convar.GetInteger("DF_cvarcheck_enabled") != 0) then
    local sv_cheats_value = convar.GetInteger("sv_cheats")

    if(sv_cheats_value != 0) then
      ////Msg("Aborting startquery due to cheats\n")
      // all bets are off
      return
    end

    local maxpendingqueries = math.max(5, (convar.GetInteger("sv_timeout") * 4) / cheatcvarcheckinterval)
    ////Msg("maxpendingqueries: " .. tostring(maxpendingqueries) .. "\n")
    local players = player.GetAll()
    for _, v in pairs(players) do
      if(!v:IsBot()) then
        local pendingQueries = v:PendingConVarQueries()
        ////Msg("Pending: " .. pendingQueries .. "\n")
        local cvarInfo = cheatcvars[cheatcvarsindex]
        local cvarToCheck = cvarInfo[1]
        local expectedCvarValue = cvarInfo[2]
        local serverValue = ""

        // TODO: Need a better way to detect this. What happens if queries get dropped and how do we account for other scripts that may be querying?
        if(pendingQueries > maxpendingqueries) then
          local timestamp = os.date("%x %I:%M:%S %p")
          local fileMessage = v:GetName() .. " blocking CVAR queries. " .. tostring(pendingQueries) .. " pending.\n"
          Msg(fileMessage)
          file.Append("queryblocklog.txt", "[" .. timestamp .. "] " .. fileMessage)
          //cvarcheck_handleplayer_message(v, "Cheating: Blocking CVAR queries. " .. tostring(pendingQueries) .. " pending.")
        end

        cheatcvarsindex = cheatcvarsindex + 1
        if(cheatcvarsindex > #cheatcvars) then
          cheatcvarsindex = 1
        end

        if(expectedCvarValue == "replicated") then
          serverValue = convar.GetString(cvarToCheck)
        end

        ////Msg("Checking " .. expectedCvarValue .. "\n")

        v:QueryConVar(cvarToCheck, function(estatus, name, value) cvarcheck_querycomplete(v, estatus, name, value, servervalue, cvarInfo) end)
      end
    end
  end
end

function cvarcheck_levelinit()
  // old timers by the same name are automatically removed
  timer.Create("anticvarcheat", cheatcvarcheckinterval, 0, cvarcheck_query)
end

convar.Add("DF_cvarcheck_enabled", "0", 0, "Enable automatic kicking or banning of players with certain cheat CVARs forced on.", true, 0, true, 1)
convar.Add("DF_cvarcheck_dealmethod", "3", 0, "Punishment for offenders. 1: Kick, 2: Temporary ban, 3: Permanent ban", true, 1, true, 3)
convar.Add("DF_cvarcheck_bantime", "5", 0, "Number of minutes to ban players if the temporary ban deal method is selected.")
hook.Add("LevelInit", "cvarcheckinit", cvarcheck_levelinit)
cvarcheck_levelinit()
