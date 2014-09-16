
local mapveto_shouldskipnextmap = false
local mapveto_votedone = false
local mapveto_reqpercents = {}

function mapveto_voteover(vote)
  local yesvotes = vote.picks[1]
  local total = vote.picks[1] + vote.picks[2]
  local nonvoters = table.Count(player.GetAll()) - total
  local yespercentage = (yesvotes / total) * 100
  local percentyes = tostring(math.floor(yespercentage)) .. "%"
  local nextmap = game.GetMapNext()
  local defpercent = convar.GetInteger("DF_mapveto_defaultpercent")
  local maxpercent = convar.GetInteger("DF_mapveto_maxpercent")
  local stepsize = convar.GetInteger("DF_mapveto_percentstep")
  local requiredpercent = mapveto_reqpercents[nextmap] or defpercent

  mapveto_votedone = true

  sourceop.SayTextAll("The veto vote got " .. yesvotes .. "/" .. total .. " (" .. percentyes .. ") yes votes.\n")
  if(yespercentage >= requiredpercent) then
    local newnextmap = game.GetLevelFromCycle(1)
    sourceop.SayTextAll("The map " .. nextmap .. " will be skipped. " .. newnextmap .. " is the new nextmap.\n")
    // set nextlevel to the winning map
    // this is only to make "nextmap" display the correct nextmap.
    // the nextlevel cvar will be erased later
    convar.SetString("nextlevel", newnextmap)
    mapveto_shouldskipnextmap = true

    // adjust required percentage for next time
    if(mapveto_reqpercents[nextmap] == nil) then
      mapveto_reqpercents[nextmap] = defpercent + stepsize
    else
      mapveto_reqpercents[nextmap] = mapveto_reqpercents[nextmap] + stepsize
    end
  else
    sourceop.SayTextAll("The map " .. nextmap .. " will not be skipped and remains the nextmap.\n")

    // adjust required percentage for next time
    if(mapveto_reqpercents[nextmap] == nil) then
      // do not subtract stepsize because it can't get lower than defpercent
      mapveto_reqpercents[nextmap] = defpercent
    else
      mapveto_reqpercents[nextmap] = mapveto_reqpercents[nextmap] - stepsize
    end
  end
  sourceop.SayTextAll(tostring(nonvoters) .. " players did not vote.\n")

  // check bounds
  if(mapveto_reqpercents[nextmap] > maxpercent) then
    mapveto_reqpercents[nextmap] = maxpercent
  elseif(mapveto_reqpercents[nextmap] < defpercent) then
    mapveto_reqpercents[nextmap] = defpercent
  end
end

function mapveto_checkmapstatus()
  if(mapveto_votedone == false) then
    if(game.GetTimeRemaining() < 360 && game.GetTimeLimit() != 0) then
      if(convar.GetBool("DF_mapveto_enabled")) then
        mapveto_startvote()
      else
        timer.Destroy("mapvetocheckmapstatus")
      end
    end
  end
end

function mapveto_intermission()
  if(mapveto_shouldskipnextmap) then
    mapveto_shouldskipnextmap = false
    convar.SetString("nextlevel", "")
    // advance the mapcycle after intermission has started
    timer.Simple(0.05, (function() gamerules.AdvanceMapCycle() end))
  end
end

function mapveto_startvote()
  local nextmap = game.GetMapNext()
  local nextnextmap = game.GetLevelFromCycle(1)
  local requiredpercent = mapveto_reqpercents[nextmap] or convar.GetInteger("DF_mapveto_defaultpercent")
  local requiredpercentstring = tostring(math.floor(requiredpercent)) .. "%"

  mapveto_votedone = true
  Vote(nextmap .. " is next.\nWould you like to skip it?",
       {"Yes, skip it and play " .. nextnextmap, "No, don't skip and play " .. nextmap}, 30,

       "If the veto succeeds, the next map would be:\n" ..
       nextnextmap .. "\n \n" ..

       "The results will be displayed in 30 seconds.\n" ..
       requiredpercentstring .. " of the votes must be yes\nfor the veto to succeed.\n",
       mapveto_voteover)
end

/**
 * Sets up the timers on map init.
 */
function mapveto_levelinit()
  mapveto_shouldskipnextmap = false
  mapveto_votedone = false
  // old timers by the same name are automatically removed
  if(convar.GetBool("DF_mapveto_enabled")) then
    timer.Create("mapvetocheckmapstatus", 5, 0, mapveto_checkmapstatus)
  end
end

function mapveto_levelshutdown()
  mapveto_savetable()
end

function mapveto_loadtable()
  local keytables = file.Read("DF_vetodata.txt")
  mapveto_reqpercents = util.KeyValuesToTable(keytables)
end

function mapveto_savetable()
  local vetodata = util.TableToKeyValues(mapveto_reqpercents)
  file.Write("DF_vetodata.txt", vetodata)
end

function mapveto_override( pPlayer, command, arguments )
  local override = tonumber(arguments[1])
  if(override != 0) then
    local newnextmap = game.GetLevelFromCycle(1)
    convar.SetString("nextlevel", newnextmap)
    mapveto_shouldskipnextmap = true
    sourceop.SayTextAll("An admin has overriden the map veto to skip the next map.\n")
  else
    convar.SetString("nextlevel", "")
    mapveto_shouldskipnextmap = false
    sourceop.SayTextAll("An admin has overriden the map veto to not skip the next map.\n")
  end
end

convar.Add("DF_mapveto_enabled", "0", 0, "Whether or not the map veto system is enabled.", true, 0, true, 1)
convar.Add("DF_mapveto_defaultpercent", "50", 0, "The default percentage of yes votes needed to veto a map.", true, 0, true, 100)
convar.Add("DF_mapveto_maxpercent", "100", 0, "The maximum percentage that will ever be reached.", true, 0, true, 100)
convar.Add("DF_mapveto_percentstep", "10", 0, "The percentage that the required percentage will change on success or failure.", true, 0, true, 100)
hook.Add("IntermissionStarting", "mapvetointermission", mapveto_intermission)
hook.Add("LevelInit", "mapvetolevelinit", mapveto_levelinit)
hook.Add("LevelShutdown", "mapvetolevelshutdown", mapveto_levelshutdown)

admincommand.Add("testveto", 32768, 0, "", mapveto_startvote)
admincommand.Add("testvetosave", 32768, 0, "", mapveto_savetable)
admincommand.Add("veto_override", 2, 1, "<override>", mapveto_override)

mapveto_levelinit()
mapveto_loadtable()
