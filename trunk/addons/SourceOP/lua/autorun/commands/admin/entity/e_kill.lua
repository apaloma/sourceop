/** \file e_kill.lua
 *  \brief Provides the e_kill admin command.
 *  e_kill kills the entity in front of the player.
 *  If it is run from the server console, it will kill the given list of
 *  entity IDs.
 *
 *
 * @author Tony Paloma
 * @version 12/3/2007
*/

/** Draws the e_kill beam effect.
 *
 *  @param startpos The starting position of the beam.
 *  @param endpos The Ending poistion of the beam.
*/
function drawkillbeam( startpos, endpos )
  local beamindex = util.PrecacheModel("effects/laser1.vmt");
  effects.Beam(startpos, endpos, beamindex, 0, 0, 0, 0.2, 5, 5, 0, 5, math.random(200,255), math.random(0,15), math.random(0,15), 255, 20)
  effects.Beam(startpos, endpos, beamindex, 0, 0, 0, 0.2, 5, 5, 0, 1, math.random(200,255), math.random(100,146), math.random(32,96), 255, 20)
end

/** Checks an entity to see if it is valid for killing.
 *
 *  @param ent Entity to check.
 *  @return true if the entity is OK to delete.
*/
function validkillent( ent )
  if(ent == nil) then return false end

  local classname = ent:GetClassname()
  if(string.sub(classname, 1, 12) == "prop_vehicle") then
    return false
  end
  return true
end

/** Kills a specified entity and draws a beam to the entity.
 *
 *  @param ent The entity to kill.
 *  @param startpos The starting position of the beam.
 *  @param endpos The Ending poistion of the beam.
*/
function ekill_killent( ent, startpos, endpos )
  if(validkillent(ent)) then
    local classname = ent:GetClassname()
    if(startpos && endpos) then drawkillbeam(startpos, endpos) end
    if(classname != "player") then
      // kill the entity with explosion sound
      ent:EmitSound("^weapons/explode" .. math.random(3,5) .. ".wav", 60, math.random(90,110))
      ent:Kill()
    else
      // slay the player
      local pDeadPlayer = player.GetByID(ent:EntIndex())
      pDeadPlayer:Slay()
    end
    return true
  else
    return false
  end
end

/** Kills the entity in front of the player.
 *  Kills entities from command line if run from server console.
*/
function ekill( pPlayer, command, arguments )
  if(pPlayer) then
    if(pPlayer:IsAdmin(1024, "e_kill")) then
      if(!pPlayer:IsEntMoving()) then
        local ent, endpos = pPlayer:FindEntityForward(MASK_ALL)
        ekill_killent(ent, pPlayer:GetAbsOrigin(), endpos)
      else
        pPlayer:SayText("You are currently moving an entity and cannot run " .. command .. ".\n")
      end
    else
      pPlayer:SayText("You do not have access to the command " .. command .. ".\n", HUD_PRINTCONSOLE)
    end
  else
    // command run at server console
    // kill each entity id specified on command line
    for k,v in pairs(arguments) do
      if k >= 1 then
        ekill_killent(ents.GetByIndex(tonumber(v)), nil, nil)
      end
    end
  end
end

if(sourceop.FeatureStatus(FEAT_ENTCOMMANDS)) then
  concommand.Add("e_kill", ekill, "Kills the entity in front of the player that runs the command.")
end
