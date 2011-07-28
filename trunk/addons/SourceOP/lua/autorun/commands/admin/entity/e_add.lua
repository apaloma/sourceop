/** \file e_add.lua
 *  \brief Provides the e_add admin command.
 *  e_add adds an entity given a classname and key value pairs.
 *
 * @author Tony Paloma
 * @version 12/3/2007
*/

/** Adds an entity given a table of key value pairs.
 *
 *  @param keys Table (dictionary) of key value pairs.
 *  @return The new entity.
*/
function eadd_addent(keys)
  //there must be a valid classname
  if(!keys["classname"]) then return end

  local pEnt = ents.Create(keys["classname"])
  if(pEnt) then
    pEnt:KeyValue("classname", keys["classname"])
    for k,v in pairs(keys) do
      pEnt:KeyValue(k, v)
    end
    pEnt:Spawn()
    pEnt:Activate()
  end
  return pEnt
end

/** Converts the value of a key to the correct form.
 *  For example %HERE% will be replaced with the player's origin in the origin key.
 *
 *  @param pPlayer The player calling the command.
 *  @param key The key.
 *  @param v The value.
 *  @return Converted value.
*/
function eadd_convertkeyval(pPlayer, key, v)
  if(!pPlayer) then return v end

  if(key == "origin") then
    if(v == "%HERE%") then
      // set v to "x y z"
      v = tostring(pPlayer:GetAbsOrigin())
    elseif(v == "%FRONT%" || v == "%FORWARD%") then
      local forward = pPlayer:GetForward()
      v = tostring(pPlayer:EyePos() + Vector(forward:GetX() * 48, forward:GetY() * 48, 0))
    end
  end
  return v
end

/** Adds an entity to the game given a classname and list of key value pairs. */
function eadd( pPlayer, command, arguments )
  local msgplayer
  if(pPlayer) then
    msgplayer = function(txt) pPlayer:SayText(txt) end
  else
    msgplayer = Msg
  end

  if(!pPlayer || pPlayer:IsAdmin(1024, "e_add")) then
    if(#arguments > 0) then
      if( (pPlayer && pPlayer:CanSpawn()) || (!pPlayer && sourceop.CanSpawn()) ) then
        local key = nil
        local keys = {}
        // the entitiy's classname is the first argument
        keys["classname"] = arguments[1]
        for k,v in ipairs(arguments) do
          if(k > 1) then
            if(k % 2 == 0) then
              key = v
            else
              keys[key] = eadd_convertkeyval(pPlayer, key, v)
              key = nil
            end
          end
        end
        // if key is present, there must be one more key than there are values.
        if(key) then
          msgplayer("More keys were provided than values. The last key was ignored.\n")
        end
        local ent = eadd_addent(keys)
        if(ent) then
          if(pPlayer) then pPlayer:AddSpawnedEnt(ent) end
          sourceop.AddSpawnedEnt(ent)
        end
      else
        if(pPlayer) then
          msgplayer("Entity limit reached. Your limit: " .. pPlayer:GetSpawnedCount() .. "/" .. pPlayer:GetSpawnLimit() .. ". Server limit: " .. sourceop.GetSpawnedCount() .. "/" .. convar.GetNumber("DF_spawnlimit_server") .. ".\n")
        else
          msgplayer("Entity limit reached. Server limit: " .. sourceop.GetSpawnedCount() .. "/" .. convar.GetNumber("DF_spawnlimit_server") .. ".\n")
        end
      end
    else
      msgplayer("Incorrect amount of parameters for " .. command .. "\n")
      msgplayer("Usage: " .. command .. " classname key1 value1...\n")
      msgplayer("Include as many key/value pairs as you want.\n")
    end
  end
end


if(sourceop.FeatureStatus(FEAT_ENTCOMMANDS)) then
  // example usage:
  // e_add prop_physics model models/props_c17/oildrum001_explosive.mdl spawnflags 256 origin %FRONT%
  //   - spawns an explosive drum in front of the player
  concommand.Add("e_add", eadd)
end