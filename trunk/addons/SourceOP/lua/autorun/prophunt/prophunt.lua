/**
 * \file prophunt.lua
 * \brief The prophunt game mod. Blue team are the hunters and consist of only
          pyros and heavies. Red team are the props and consist of scouts. The
          props must go hide and the hunters must find all the props.
 */

require "timer"

local prophunt_offsetplayerlocal = sourceop.GetPropOffset("DT_LocalPlayerExclusive", "m_Local")
local prophunt_offsetobservermode = sourceop.GetPropOffset("DT_BasePlayer", "m_iObserverMode")
local prophunt_offsetplayerfov = sourceop.GetPropOffset("DT_BasePlayer", "m_iFOV")
local prophunt_offsetplayerdefaultfov = sourceop.GetPropOffset("DT_BasePlayer", "m_iDefaultFOV")
local prophunt_offsetdrawviewmodel = prophunt_offsetplayerlocal + sourceop.GetPropOffset("DT_Local", "m_bDrawViewmodel")
local prophunt_offsetragdoll = sourceop.GetPropOffset("DT_TFPlayer", "m_hRagdoll")

local prophunt_enabled = false
local prophunt_hasforcedprop = false
local prophunt_disableweaponstrip = false
local prophunt_damageonshoot = false
local prophunt_controlpointfound = false
local prophunt_controlpointpos = Vector(0,0,0)
local prophunt_onepropleft = false
local prophunt_currentconfig = {}
local prophunt_opendoors = false
local prophunt_roundtime = 175

-- http://www.gamingmasters.co.uk/prophunt/index.php?page=props
local prophunt_props = {
  {model="models/props_forest/tree_pine_stackedlogs.mdl", name="Stacked Logs"},
  {model="models/props_2fort/chimney003.mdl", name="Spinning Chimney Cowl"},
  {model="models/props_2fort/chimney005.mdl", name="Chimney"},
  {model="models/props_2fort/miningcrate001.mdl", name="Mining Crate"},
  {model="models/props_2fort/miningcrate002.mdl", name="Small Mining Crate"},
  {model="models/egypt/palm_tree/palm_tree.mdl", name="Palm Tree"},
  {model="models/egypt/stick_torch/stick_torch_medium.mdl", name="Medium Stick Torch"},
  {model="models/egypt/stick_torch/stick_torch_big.mdl", name="Big Stick Torch"},
  {model="models/props_gameplay/haybale.mdl", name="Hay Bale"},
  {model="models/props_2fort/frog.mdl", name="Frog"},
  {model="models/props_farm/metal_pile.mdl", name="Metal Pile"},
  {model="models/props_hydro/metal_barrier02.mdl", name="Metal Barrier"},
  {model="models/props_spytech/work_table001.mdl", name="Work Table"},
  {model="models/props_2fort/corrugated_metal001.mdl", name="Corrugated Metal"},
  {model="models/props_mining/rock006.mdl", name="Rocks"},
  {model="models/props_spytech/tv001.mdl", name="TV"},
  {model="models/props_granary/grain_sack.mdl", name="Grain Sack"},
  {model="models/props_farm/pallet001.mdl", name="Wooden Pallet"},
  {model="models/props_farm/wood_pile.mdl", name="Wood Pile"},
  {model="models/props_forest/wood_pile.mdl", name="Forest Wood Pile"},
  {model="models/props_forest/wood_pile_short.mdl", name="Short Forest Wood Pile"},
  {model="models/props_well/hand_truck01.mdl", name="Hand Truck"},
  {model="models/props_2fort/metalbucket001.mdl", name="Metal Bucket"},
  {model="models/props_2fort/trainwheel003.mdl", name="Train Wheel Pile"},
  {model="models/props_2fort/oildrum.mdl", name="Oil Drum"},
  {model="models/props_2fort/tire002.mdl", name="Tire Pile 2"},
  {model="models/props_2fort/tire003.mdl", name="Tire Pile 3"},
  {model="models/props_forest/sawmill_logs.mdl", name="Sawmill Logs"},
  {model="models/props_farm/haypile001.mdl", name="Hay Pile"},
  {model="models/props_spytech/terminal_chair.mdl", name="Terminal Chair"},
  {model="models/props_spytech/control_room_console01.mdl", name="Console 1"},
  {model="models/props_spytech/control_room_console02.mdl", name="Console 2"},
  {model="models/props_spytech/control_room_console03.mdl", name="Console 3"},
  {model="models/props_spytech/computer_wall02.mdl", name="Computer Wall 2"},
  {model="models/props_spytech/computer_wall03.mdl", name="Computer Wall 3"},
  {model="models/props_spytech/computer_wall05.mdl", name="Computer Wall 5"},
  {model="models/props_foliage/shrub_03a.mdl", name="Shrub 1"},
  {model="models/props_foliage/shrub_03b.mdl", name="Shrub 2"},
  {model="models/props_foliage/shrub_03c.mdl", name="Shrub 3"},
  {model="models/props_forest/shrub_03c.mdl", name="Forest Shrub"},
  {model="models/props_forest/shrub_03_cluster.mdl", name="Forest Shrub Cluster"},
  {model="models/props_2fort/propane_tank_tall01.mdl", name="Propane Tank"},
  {model="models/props_foliage/cactus01.mdl", name="Cactus"},
  {model="models/props_mining/sign001.mdl", name="Skull and Crossbones Sign"},
  {model="models/props_2fort/milkjug001.mdl", name="Milk Jug"},
  {model="models/props_well/computer_cart01.mdl", name="Computer Cart"},
  {model="models/props_badlands/barrel01.mdl", name="Large Yellow Barrel"},
  {model="models/props_badlands/barrel02.mdl", name="Large Blue Barrel"},
  {model="models/props_badlands/barrel03.mdl", name="Large Radioactive Barrel"},
  {model="models/props_gameplay/orange_cone001.mdl", name="Orange Cone"},
  {model="models/props_gameplay/cap_point_base.mdl", name="Capture Point"},
  {model="models/props_vehicles/mining_cart_supplies001.mdl", name="Mining Cart"},
  {model="models/props_trainyard/portable_stairs001.mdl", name="Portable Stairs"},
  {model="models/props_farm/concrete_block001.mdl", name="Concrete Block"},
  {model="models/props_farm/concrete_pipe001.mdl", name="Concrete Pipe"},
  {model="models/props_farm/welding_machine01.mdl", name="Welding Machine"}
}

---------------------------------------------------------------------
-- Helper functions
---------------------------------------------------------------------

function prophunt_SetProp(pPlayer, pProp)
  -- this is a hack and needs to work for Player as well as Entity
  local pEnt = ents.GetByIndex(pPlayer:EntIndex())
  if(pEnt == nil) then return end

  pEnt.pCurrentProp = pProp
end

function prophunt_GetProp(pPlayer)
  local pEnt = ents.GetByIndex(pPlayer:EntIndex())
  if(pEnt == nil) then return nil end

  return pEnt.pCurrentProp
end

function prophunt_SetLastButtons(pPlayer, flag)
  local pEnt = ents.GetByIndex(pPlayer:EntIndex())
  if(pEnt == nil) then return end

  pEnt.iPropLastButtons = flag
end

function prophunt_GetLastButtons(pPlayer)
  local pEnt = ents.GetByIndex(pPlayer:EntIndex())
  if(pEnt == nil) then return 0 end

  return pEnt.iPropLastButtons
end

function prophunt_SetRealLastButtons(pPlayer, flag)
  local pEnt = ents.GetByIndex(pPlayer:EntIndex())
  if(pEnt == nil) then return end

  pEnt.iPropRealLastButtons = flag
end

function prophunt_GetRealLastButtons(pPlayer)
  local pEnt = ents.GetByIndex(pPlayer:EntIndex())
  if(pEnt == nil) then return 0 end

  return pEnt.iPropRealLastButtons
end

function prophunt_SetPropLockFlag(pPlayer, flag)
  local pEnt = ents.GetByIndex(pPlayer:EntIndex())
  if(pEnt == nil) then return end

  pEnt.iPropLockFlag = flag
end

function prophunt_GetPropLockFlag(pPlayer)
  local pEnt = ents.GetByIndex(pPlayer:EntIndex())
  if(pEnt == nil) then return 0 end

  return pEnt.iPropLockFlag
end

function prophunt_SetPropLockAngleFlag(pPlayer, flag)
  local pEnt = ents.GetByIndex(pPlayer:EntIndex())
  if(pEnt == nil) then return end

  pEnt.iPropLockAngleFlag = flag
end

function prophunt_GetPropLockAngleFlag(pPlayer)
  local pEnt = ents.GetByIndex(pPlayer:EntIndex())
  if(pEnt == nil) then return 0 end

  return pEnt.iPropLockAngleFlag
end

function prophunt_SetPropFirstPersonFlag(pPlayer, flag)
  local pEnt = ents.GetByIndex(pPlayer:EntIndex())
  if(pEnt == nil) then return end

  pEnt.iPropFirstPersonFlag = flag
end

function prophunt_GetPropFirstPersonFlag(pPlayer)
  local pEnt = ents.GetByIndex(pPlayer:EntIndex())
  if(pEnt == nil) then return 0 end

  return pEnt.iPropFirstPersonFlag
end

function prophunt_SetLastDamageTime(pPlayer, flDamageTime)
  local pEnt = ents.GetByIndex(pPlayer:EntIndex())
  if(pEnt == nil) then return end

  pEnt.flLastDamageSelfTime = flDamageTime
end

function prophunt_GetLastDamageTime(pPlayer)
  local pEnt = ents.GetByIndex(pPlayer:EntIndex())
  if(pEnt == nil) then return 0 end

  local ret = pEnt.flLastDamageSelfTime

  if(ret == nil) then
    return 0.0
  else
    return ret
  end
end

function prophunt_SetCPBonusFlag(pPlayer, flag)
  local pEnt = ents.GetByIndex(pPlayer:EntIndex())
  if(pEnt == nil) then return end

  pEnt.iCPBonusFlag = flag
end

function prophunt_GetCPBonusFlag(pPlayer)
  local pEnt = ents.GetByIndex(pPlayer:EntIndex())
  if(pEnt == nil) then return 0 end

  return pEnt.iCPBonusFlag
end

function prophunt_SetPlayerFoundFlag(pPlayer, flag)
  local pEnt = ents.GetByIndex(pPlayer:EntIndex())
  if(pEnt == nil) then return end

  pEnt.iPlayerFoundFlag = flag
end

function prophunt_GetPlayerFoundFlag(pPlayer)
  local pEnt = ents.GetByIndex(pPlayer:EntIndex())
  if(pEnt == nil) then return 0 end

  return pEnt.iPlayerFoundFlag
end

function prophunt_SetPropFirstPerson(pPlayer, firstPerson)
  if(firstPerson) then
    prophunt_SetPropFirstPersonFlag(pPlayer, true)

    pPlayer:Fire("SetForcedTauntCam", "0")
    pPlayer:SetOffsetByte(prophunt_offsetdrawviewmodel, 1)
    pPlayer:SetOffsetInt(prophunt_offsetplayerfov, 90)
  else
    prophunt_SetPropFirstPersonFlag(pPlayer, false)

    pPlayer:Fire("SetForcedTauntCam", "1")
    pPlayer:SetOffsetByte(prophunt_offsetdrawviewmodel, 0)
    pPlayer:SetOffsetInt(prophunt_offsetplayerfov, 100)

    -- kill any existing ragdolls
    local ragdoll = pPlayer:GetOffsetEntity(prophunt_offsetragdoll)
    if(ragdoll != nil) then
      ragdoll:Kill()
    end
  end
end

function prophunt_ForcePlayerClass(pPlayer, playerclass)
  pPlayer:SetPlayerClass(playerclass)
  pPlayer:SetDesiredPlayerClass(playerclass)
  pPlayer:ForceRespawn()
  /*if(playerclass == TF2_CLASS_SCOUT) then
    pPlayer:FakeConCommand("joinclass scout")
  elseif(playerclass == TF2_CLASS_PYRO) then
    pPlayer:FakeConCommand("joinclass pyro")
  end*/
end

function prophunt_MaxHeavies()
  return convar.GetInteger("DF_prophunt_maxheavies")
end

function prophunt_IsHeavyAllowedNow()
  local numheavies = 0
  local maxheavies = prophunt_MaxHeavies()

  if(maxheavies == 0) then
    return false
  end

  local players = player.GetAll()
  for _, pPlayer in pairs(players) do
    if(pPlayer:GetTeam() == 3 && pPlayer:GetPlayerClass() == TF2_CLASS_HEAVY) then
      numheavies = numheavies + 1
      if(numheavies >= maxheavies) then
        return false
      end
    end
  end

  return true
end

function prophunt_DestroyTimers()
  timer.Destroy("prophunt_StartIn30")
  timer.Destroy("prophunt_StartIn20")
  timer.Destroy("prophunt_StartIn10")
  timer.Destroy("prophunt_StartIn5")
  timer.Destroy("prophunt_StartIn4")
  timer.Destroy("prophunt_StartIn3")
  timer.Destroy("prophunt_StartIn2")
  timer.Destroy("prophunt_StartIn1")

  timer.Destroy("prophunt_HuntStart")
  timer.Destroy("prophunt_HuntOver")
  timer.Destroy("prophunt_DamageOnShoot")
  timer.Destroy("prophunt_RechargeCPBonus")
end

function prophunt_SwitchTeams()
  local players = player.GetAll()
  for _, pPlayer in pairs(players) do
    prophunt_RemovePlayerProp(pPlayer)
  end

  gamerules:HandleSwitchTeams()
end

---------------------------------------------------------------------
-- Mod functions
---------------------------------------------------------------------

function prophunt_LoadMapConfig()
  local cfgName = prophunt_GetMapConfigName() .. ".cfg"
  Msg("Loading prophunt config file: " .. cfgName .. "\n")

  local keytables = file.Read("lua/autorun/prophunt/data/maps/" .. cfgName)
  if(keytables == "") then
    Msg("Failed to load prophunt map config! Trying to load default config.\n")
    keytables = file.Read("lua/autorun/prophunt/data/maps/default.cfg")
  end

  prophunt_currentconfig = util.KeyValuesToTable(keytables)
  if(prophunt_currentconfig == nil) then
    Msg("Failed to load any prophunt config.\n")
    return
  end

  prophunt_props = {}
  for model, _ in pairs(prophunt_currentconfig["Props"]) do
    local thisProp = { }
    thisProp["model"] = model
    thisProp["name"] = model

    table.insert(prophunt_props, thisProp)
    Msg("model: " .. model .. "\n")
  end

  prophunt_roundtime = prophunt_currentconfig["Settings"]["round"] or 175
  prophunt_opendoors = (prophunt_currentconfig["Settings"]["doors"] == 1)
  convar.SetInteger("DF_prophunt_hunterfreeze", prophunt_currentconfig["Settings"]["freeze"] or 1)

  Msg("Round time: " .. tostring(prophunt_roundtime) .. "\n")
  Msg("Open doors: " .. tostring(prophunt_opendoors) .. "\n")
end

function prophunt_GetMapConfigName()
  local mapName = game.GetMap()
  local firstUnderscore = string.find(mapName, "_")
  if(firstUnderscore == nil) then
    return mapName
  end

  local secondUnderscore = string.find(mapName, "_", firstUnderscore + 1)
  if(secondUnderscore == nil) then
    return mapName
  end

  return mapName:sub(1, secondUnderscore - 1)

end

function prophunt_FindControlPoint()
  local props = ents.FindByClass("prop_dynamic")
  for _, pProp in pairs(props) do
    if(pProp:GetModel() == "models/props_gameplay/cap_point_base.mdl") then
      prophunt_controlpointfound = true
      prophunt_controlpointpos = pProp:GetAbsOrigin()
      return
    end
  end

  props = ents.FindByClass("prop_static")
  for _, pProp in pairs(props) do
    if(pProp:GetModel() == "models/props_gameplay/cap_point_base.mdl") then
      prophunt_controlpointfound = true
      prophunt_controlpointpos = pProp:GetAbsOrigin()
      return
    end
  end
end

function prophunt_GetNewPropModel()
  local iPossibleProps = table.Count(prophunt_props)
  local choice = math.random(1, iPossibleProps)
  local prop = prophunt_props[choice]
  --Msg("Chose " .. tostring(choice) .. ": " .. prop["name"] .. "\n")

  return prop
end

function prophunt_SetPlayerAsProp(pPlayer)
  local propinfo = prophunt_GetNewPropModel()
  prophunt_SetPlayerAsPropWithInfo(pPlayer, propinfo)
end

function prophunt_SetPlayerAsPropWithInfo(pPlayer, propinfo)
  if(propinfo == nil) then
    Msg("prophunt_SetPlayerAsPropWithInfo got null propinfo!\n")
    return
  end

  -- create the prop temporarily so that we can get size from it
  local pProp = ents.Create("prop_dynamic_override")

  if(pProp == nil) then
    Msg("Error creating prop entity.\n")
    return
  end

  pProp:KeyValue("model",           propinfo["model"])
  pProp:KeyValue("rendermode",      "2")
  pProp:KeyValue("renderamt",       "0")
  pProp:KeyValue("disableshadows",  "1")
  pProp:KeyValue("physdamagescale", "0")
  pProp:KeyValue("ExplodeDamage",   "0")
  pProp:KeyValue("ExplodeRadius",   "0")
  pProp:KeyValue("spawnflags",      "256") // start with collision disabled
  pProp:PhysicsInit(SOLID_BBOX)
  pProp:Spawn()
  pProp:Activate()
  pProp:SetModel(propinfo["model"])
  pProp:KeyValue("solid",           "0")

  -- set the position such that the origin causes the prop to be touching
  -- the ground
  local propMins = pProp:GetMins()
  local propPosOffset = Vector(0, 0, -propMins:GetZ())
  pProp:Kill()

  pPlayer:Fire("SetCustomModel", propinfo["model"])
  pPlayer:Fire("SetCustomModelOffset", tostring(propPosOffset))
  pPlayer:Fire("SetCustomRotates", "1")
  pPlayer:Fire("SetCustomModelVisibleToSelf", "1")
  pPlayer:Fire("DisableShadow", "1")

  prophunt_SetProp(pPlayer, true)

  prophunt_SetPropFirstPerson(pPlayer, false)

  if(!prophunt_disableweaponstrip) then
    pPlayer:StripWeapons()
  end
  prophunt_HideWearables(pPlayer)
end

function prophunt_HideWearables(pPlayer)
  local wearables = ents.FindByClass("tf_wearable_item")
  for _, wearable in pairs(wearables) do
    local pOwner = wearable:GetOwner()

    if(pOwner != nil) then
      if(pOwner:EntIndex() == pPlayer:EntIndex()) then
        wearable:SetRenderMode(2)
        wearable:SetRenderAmt(0)
      end
    end
  end
end

function prophunt_ShowWearables(pPlayer)
  local wearables = ents.FindByClass("tf_wearable_item")
  for _, wearable in pairs(wearables) do
    local pOwner = wearable:GetOwner()

    if(pOwner != nil) then
      if(pOwner:EntIndex() == pPlayer:EntIndex()) then
        wearable:SetRenderMode(0)
        wearable:SetRenderAmt(255)
      end
    end
  end
end

function prophunt_RemovePlayerProp(pPlayer)
  local pProp = prophunt_GetProp(pPlayer)

  if(pProp == nil) then
    return
  end

  prophunt_SetProp(pPlayer, nil)

  pPlayer:Fire("SetForcedTauntCam", "0")
  pPlayer:SetOffsetByte(prophunt_offsetdrawviewmodel, 1)
  pPlayer:Fire("EnableShadow", "1")
  pPlayer:Fire("SetCustomModel", "")
  pPlayer:StopParticleEffects()

  -- reset player's fov back to his/her desired default
  local defaultFov = pPlayer:GetOffsetInt(prophunt_offsetplayerdefaultfov)
  pPlayer:SetOffsetInt(prophunt_offsetplayerfov, defaultFov)

  prophunt_ShowWearables(pPlayer)
end

function prophunt_test(pPlayer, command, arguments)
  if(pPlayer == nil) then
    return
  end

  local pPropPlayer = nil
  if(#arguments >= 1) then
    local playerlist = player.Find(arguments[1])
    -- make sure there is exactly one player match
    if(#playerlist == 1) then
      pPropPlayer = playerlist[1]
    end
  else
    pPropPlayer = pPlayer
  end

  if(pPropPlayer == nil) then
    return
  end

  if(prophunt_GetProp(pPropPlayer) == nil) then
    prophunt_disableweaponstrip = true
    prophunt_SetPlayerAsProp(pPropPlayer)
    prophunt_disableweaponstrip = false
    prophunt_hasforcedprop = true
  else
    prophunt_RemovePlayerProp(pPropPlayer)
  end
end

function prophunt_menu(pPlayer, command, arguments)
  if(pPlayer == nil) then
    return
  end

  if(!prophunt_enabled) then
    return
  end

  local menufunc = (function(pPlayer, choice)
    if(choice != nil && pPlayer:IsAlive()) then
      if(prophunt_GetProp(pPlayer) != nil) then
        prophunt_RemovePlayerProp(pPlayer)
      end

      prophunt_SetPlayerAsPropWithInfo(pPlayer, prophunt_props[choice])
    end
  end)

  local options = { }
  for key, propinfo in pairs(prophunt_props) do
    local startpos, endpos, contents = string.find(propinfo["model"], ".*/([%w_]*).mdl")
    if(contents == nil) then
      Msg(propinfo["model"] .. " was null\n")
      contents = "null?"
    end
    options[key] = contents
  end

  OptionPicker(options, pPlayer, menufunc)
end

function prophunt_HuntStart()
  sourceop.PlaySoundAll("vo/announcer_am_gamestarting04.wav")
  local mapPrefix = string.sub(game.GetMap(), 1, 3)
  if(prophunt_opendoors) then
    local doors = ents.FindByClass("func_door")
    for _, door in pairs(doors) do
      door:Fire("Open", 0)
    end
  end

  local players = player.GetAll()
  for _, pPlayer in pairs(players) do
    if(pPlayer:GetTeam() == 3) then
      pPlayer:SetMoveType(MOVETYPE_WALK)
    end
  end

  timer.Create("prophunt_DamageOnShoot", 1, 1, (function() prophunt_damageonshoot = true end))
end

function prophunt_HuntOver()
  -- Kill all the hunters to cause prop team win
  local players = player.GetAll()
  for _, pPlayer in pairs(players) do
    if(pPlayer:GetTeam() == 3) then
      local damage = DamageInfo(pPlayer, pPlayer, 100000, DMG_GENERIC)
      pPlayer:DispatchTraceAttack(damage, pPlayer:GetAbsOrigin(), pPlayer:GetAbsOrigin())
    end
  end
end

function prophunt_RechargeCPBonus()
  local players = player.GetAll()
  for _, pPlayer in pairs(players) do
    if(prophunt_GetCPBonusFlag(pPlayer) != 0) then
      pPlayer:SayText("\003Control point bonus now available.\n")
      prophunt_SetCPBonusFlag(pPlayer, 0)
    end
  end
end

function prophunt_WeaponFired(pOwner, pWeapon)
  local weaponname = pWeapon:GetClassname()
  if(weaponname == "tf_weapon_flamethrower" || weaponname == "tf_weapon_minigun") then
    local lastDamageTime = prophunt_GetLastDamageTime(pOwner)
    if(CurTime() >= lastDamageTime + 0.05) then
      local damage = DamageInfo(pOwner, pOwner, 2, DMG_GENERIC)
      pOwner:DispatchTraceAttack(damage, pOwner:GetAbsOrigin(), pOwner:GetAbsOrigin())
      prophunt_SetLastDamageTime(pOwner, CurTime())
    end

    -- jetpack only on flamethrower
    if(weaponname == "tf_weapon_flamethrower") then
      local bIsOnGround = (bit.band(pOwner:Flags(), FL_ONGROUND) == FL_ONGROUND)
      if(!bIsOnGround && !pOwner:JetpackActive()) then
        pOwner:JetpackActivate()
      end
    end

  else
    local damage = DamageInfo(pOwner, pOwner, 10, DMG_GENERIC)
    pOwner:DispatchTraceAttack(damage, pOwner:GetAbsOrigin(), pOwner:GetAbsOrigin())
  end
end

function prophunt_CheckIsLastProp()
  -- count number of players alive on red
  local numPropsAlive = 0
  local lastPropChecked
  local players = player.GetAll()
  for _, pPlayer in pairs(players) do
    if(pPlayer:GetTeam() == 2 && pPlayer:IsAlive()) then
      lastPropChecked = pPlayer
      numPropsAlive = numPropsAlive + 1
    end
  end

  -- last remaining gets weapon
  if(numPropsAlive == 1) then
    prophunt_onepropleft = true
    sourceop.PlaySoundAll("prophunt/oneandonly.mp3")
    local pWeapon = lastPropChecked:GiveNamedItem("tf_weapon_scattergun")
    if(pWeapon != nil) then
      pWeapon:SetRenderMode(2)
      pWeapon:SetRenderAmt(0)
      lastPropChecked:Weapon_Equip(pWeapon)
    end
  end
end

function prophunt_SetCvars()
  convar.SetInteger("DF_lua_attack_hooks", 1)
  convar.SetInteger("sv_gravity", 500)
  convar.SetInteger("tf_arena_preround_time", 5)
  -- convar.SetInteger("tf_arena_round_time", 55) -- for testing
  convar.SetInteger("tf_arena_round_time", prophunt_roundtime)
  convar.SetInteger("mp_enableroundwaittime", 0)
  convar.SetInteger("tf_arena_override_cap_enable_time", 600)
  convar.SetInteger("mp_waitingforplayers_time", 40)
end

function prophunt_Precache()
  util.PrecacheSound("vo/announcer_begins_30sec.wav")
  util.PrecacheSound("vo/announcer_begins_20sec.wav")
  util.PrecacheSound("vo/announcer_begins_10sec.wav")
  util.PrecacheSound("vo/announcer_begins_5sec.wav")
  util.PrecacheSound("vo/announcer_begins_4sec.wav")
  util.PrecacheSound("vo/announcer_begins_3sec.wav")
  util.PrecacheSound("vo/announcer_begins_2sec.wav")
  util.PrecacheSound("vo/announcer_begins_1sec.wav")
  util.PrecacheSound("vo/announcer_am_gamestarting04.wav")
  util.PrecacheSound("vo/demoman_positivevocalization04.wav")
  util.PrecacheSound("prophunt/found.mp3")
  util.PrecacheSound("prophunt/oneandonly.mp3")

  sourceop.AddDownloadable("sound/prophunt/found.mp3")
  sourceop.AddDownloadable("sound/prophunt/oneandonly.mp3")

  for _, propinfo in pairs(prophunt_props) do
    util.PrecacheModel(propinfo["model"])
  end
end

---------------------------------------------------------------------
-- Game hooks
---------------------------------------------------------------------

function prophunt_LevelInit(mapName)
  prophunt_DestroyTimers()
  prophunt_damageonshoot = false
  prophunt_onepropleft = false
end

function prophunt_LevelShutdown(mapName)
  prophunt_DestroyTimers()
end

function prophunt_ServerActivate(iEdicts, iMaxClients)
  if(!convar.GetBool("DF_prophunt_enabled")) then
    prophunt_enabled = false
    return
  end

  prophunt_enabled = true
  prophunt_hasforcedprop = false
  Msg("Prophunt is enabled.\n")

  prophunt_LoadMapConfig()
  prophunt_SetCvars()
  prophunt_Precache()
end

function prophunt_PlayerDied(pPlayer, pAttacker, weapon)
  if(!prophunt_enabled && !prophunt_hasforcedprop) then
    return
  end

  prophunt_RemovePlayerProp(pPlayer)

  if(!prophunt_enabled) then
    return
  end

  -- Kind of a hack to use prophunt_damageonshoot as indication that the
  -- hunt is in progress.
  if(!prophunt_onepropleft && prophunt_damageonshoot) then
    timer.Simple(0.001, (function() prophunt_CheckIsLastProp() end))
  end
end

function prophunt_PlayerDisconnected(pPlayer)
  if(!prophunt_enabled && !prophunt_hasforcedprop) then
    return
  end

  if(pPlayer == nil) then
    return
  end

  pcall(function() prophunt_RemovePlayerProp(pPlayer) end)
end

function prophunt_GameFrame()
  if(!prophunt_enabled && !prophunt_hasforcedprop) then
    return
  end

  local players = player.GetAll()
  for _, pPlayer in pairs(players) do
    local iTeam = pPlayer:GetTeam()
    local buttons = pPlayer:Buttons()
    if(pPlayer:IsAlive() && (iTeam == 2 || iTeam == 3)) then
      local pProp = prophunt_GetProp(pPlayer)
      local lastButtons = prophunt_GetLastButtons(pPlayer)
      local realLastButtons = prophunt_GetRealLastButtons(pPlayer)
      local bWasInAttack = false
      local bWasInAttack2 = false

      if(lastButtons != nil) then
        bWasInAttack = (bit.band(lastButtons, IN_ATTACK) == IN_ATTACK)
      end

      if(realLastButtons != nil) then
        bWasInAttack2 = (bit.band(realLastButtons, IN_ATTACK2) == IN_ATTACK2)
      end

      local bIsInAttack = (bit.band(buttons, IN_ATTACK) == IN_ATTACK)

      -- If the player is on the control point, give a health bonus if
      -- it's available.
      if(prophunt_controlpointfound) then
        if(prophunt_GetCPBonusFlag(pPlayer) == 0 &&
          pPlayer:DistanceTo(prophunt_controlpointpos) < 80 &&
          pPlayer:Health() < pPlayer:GetMaxHealth()) then

          pPlayer:SayText("\003Control point bonus: health filled.\n")
          pPlayer:PlaySound("vo/demoman_positivevocalization04.wav")
          pPlayer:SetHealth(pPlayer:GetMaxHealth())
          pPlayer:Extinguish()
          pPlayer:Fire("ExtinguishPlayer", "1")
          prophunt_SetCPBonusFlag(pPlayer, 1)
        end
      end

      if(pProp != nil) then
        local bIsOnGround = (bit.band(pPlayer:Flags(), FL_ONGROUND) == FL_ONGROUND)
        local bIsInAttack2 = (bit.band(buttons, IN_ATTACK2) == IN_ATTACK2)
        local bWantsToMove = (bit.band(buttons, IN_JUMP) == IN_JUMP) ||
          (bit.band(buttons, IN_FORWARD) == IN_FORWARD) ||
          (bit.band(buttons, IN_BACK) == IN_BACK) ||
          (bit.band(buttons, IN_MOVELEFT) == IN_MOVELEFT) ||
          (bit.band(buttons, IN_MOVERIGHT) == IN_MOVERIGHT)

        if(bIsInAttack && !bWasInAttack) then
          if(prophunt_GetPropLockFlag(pPlayer)) then
            pPlayer:SayText("\003Prop-lock disabled.\n")
            prophunt_SetPropLockFlag(pPlayer, false)
          elseif(bIsOnGround && pPlayer:GetActiveWeapon() == nil) then
            pPlayer:SayText("\003Prop-lock enabled.\n")
            prophunt_SetPropLockFlag(pPlayer, true)
          end
        end

        if(bIsInAttack2 && !bWasInAttack2) then
          if(prophunt_GetPropFirstPersonFlag(pPlayer)) then
            pPlayer:SayText("\003First-person disabled.\n")
            prophunt_SetPropFirstPerson(pPlayer, false)

            pPlayer:Fire("SetCustomModelVisibleToSelf", "1")
          else
            pPlayer:SayText("\003First-person enabled.\n")
            prophunt_SetPropFirstPerson(pPlayer, true)

            pPlayer:Fire("SetCustomModelVisibleToSelf", "0")
          end
        end

        -- only remember the last buttons if we are on the ground
        -- this allows props to instantly get proplock as soon as they hit the
        -- ground if they are holding down the mouse button
        if(bIsOnGround) then
          prophunt_SetLastButtons(pPlayer, buttons)
        end

        prophunt_SetRealLastButtons(pPlayer, buttons)

        if(prophunt_GetPropLockFlag(pPlayer) && bWantsToMove && !bIsInAttack && !bIsInAttack2) then
          pPlayer:SayText("\003Prop-lock disabled due to movement.\n")
          prophunt_SetPropLockFlag(pPlayer, false)
        end

        -- If the player is holding the fire button, let's lock the player's
        -- angles so that he or she can look around without moving.
        if(prophunt_GetPropLockFlag(pPlayer)) then
          if(prophunt_GetPropLockAngleFlag(pPlayer) != 1) then
            if(bIsOnGround) then
              pPlayer:SetVelocity(Vector(0,0,0))
              pPlayer:SetMoveType(MOVETYPE_NONE)
              pPlayer:Fire("SetCustomModelRotates", "0")
              prophunt_SetPropLockAngleFlag(pPlayer, 1)
            end
          end
        else
          if(prophunt_GetPropLockAngleFlag(pPlayer) != 0) then
            pPlayer:SetMoveType(MOVETYPE_WALK)
            pPlayer:Fire("SetCustomModelRotates", "1")
            prophunt_SetPropLockAngleFlag(pPlayer, 0)
          end

          if(prophunt_GetPropLockFlag(pPlayer)) then
            prophunt_SetPropLockFlag(pPlayer, false)
          end
        end
      end -- pProp != nil

      if(pPlayer:JetpackActive()) then
        local pWeapon = pPlayer:GetActiveWeapon()
        local currentWeaponName = ""
        if(pWeapon != nil) then
          currentWeaponName = pWeapon:GetClassname()
        end

        if(!bIsInAttack || currentWeaponName != "tf_weapon_flamethrower") then
          pPlayer:JetpackOff()
        end
      end
    end -- team == 2 or team == 3
  end -- for loop
end

function prophunt_RoundStart()
  if(!prophunt_enabled) then
    return
  end

  prophunt_damageonshoot = false
  prophunt_onepropleft = false
  prophunt_FindControlPoint()

  local numheavies = 0
  local players = player.GetAll()
  for _, pPlayer in pairs(players) do
    prophunt_SetProp(pPlayer, nil)
    prophunt_SetLastButtons(pPlayer, 0)
    prophunt_SetPropLockFlag(pPlayer, false)
    prophunt_SetPropLockAngleFlag(pPlayer, 0)
    prophunt_SetCPBonusFlag(pPlayer, 0)
    prophunt_SetPlayerFoundFlag(pPlayer, 0)

    pPlayer:SetRenderMode(0)
    pPlayer:SetRenderAmt(255)

    -- Force red to scout, blue to pyro or heavy
    if(pPlayer:GetTeam() == 2) then
      prophunt_ForcePlayerClass(pPlayer, TF2_CLASS_SCOUT)
    elseif(pPlayer:GetTeam() == 3) then
      local bIsHeavy = pPlayer:GetPlayerClass() == TF2_CLASS_HEAVY
      if(bIsHeavy) then
        numheavies = numheavies + 1
        if(numheavies > prophunt_MaxHeavies()) then
          bIsHeavy = false
        end
      end

      -- If the player is not a heavy (or not allowed to be a heavy) or a pyro,
      -- then force to pyro
      if(!bIsHeavy &&
        pPlayer:GetPlayerClass() != TF2_CLASS_PYRO) then
        prophunt_ForcePlayerClass(pPlayer, TF2_CLASS_PYRO)
      end

      if(convar.GetBool("DF_prophunt_hunterfreeze")) then
        pPlayer:SetMoveType(MOVETYPE_NONE)
      end
    end
  end

  -- Remove all:
  --  team_round_timer
  --  team_control_point_master
  --  trigger_capture_area
  local timers = ents.FindByClass("team_round_timer")
  for _, pTimer in pairs(timers) do
    pTimer:Kill()
  end
  local cpms = ents.FindByClass("team_control_point_master")
  for _, pCpm in pairs(cpms) do
    pCpm:Kill()
  end
  local capareas = ents.FindByClass("trigger_capture_area")
  for _, pCapArea in pairs(capareas) do
    pCapArea:Kill()
  end
end

function prophunt_ArenaRoundStart()
  if(!prophunt_enabled) then
    return
  end

  -- Set all red to props and freeze all hunters again
  local players = player.GetAll()
  for _, pPlayer in pairs(players) do
    if(pPlayer:GetTeam() == 2) then
      prophunt_SetPlayerAsProp(pPlayer)
    elseif(pPlayer:GetTeam() == 3) then
      if(convar.GetBool("DF_prophunt_hunterfreeze")) then
        pPlayer:SetMoveType(MOVETYPE_NONE)
      end
    end
  end

  local hidetime = convar.GetInteger("DF_prophunt_hidetime")
  local roundtime = convar.GetInteger("tf_arena_round_time")
  local chargetime = convar.GetInteger("DF_prophunt_cpbonus_rechargetime")

  timer.Create("prophunt_StartIn30", hidetime - 30, 1, function() sourceop.PlaySoundAll("vo/announcer_begins_30sec.wav") end)
  timer.Create("prophunt_StartIn20", hidetime - 20, 1, function() sourceop.PlaySoundAll("vo/announcer_begins_20sec.wav") end)
  timer.Create("prophunt_StartIn10", hidetime - 10, 1, function() sourceop.PlaySoundAll("vo/announcer_begins_10sec.wav") end)
  timer.Create("prophunt_StartIn5", hidetime - 5, 1, function() sourceop.PlaySoundAll("vo/announcer_begins_5sec.wav") end)
  timer.Create("prophunt_StartIn4", hidetime - 4, 1, function() sourceop.PlaySoundAll("vo/announcer_begins_4sec.wav") end)
  timer.Create("prophunt_StartIn3", hidetime - 3, 1, function() sourceop.PlaySoundAll("vo/announcer_begins_3sec.wav") end)
  timer.Create("prophunt_StartIn2", hidetime - 2, 1, function() sourceop.PlaySoundAll("vo/announcer_begins_2sec.wav") end)
  timer.Create("prophunt_StartIn1", hidetime - 1, 1, function() sourceop.PlaySoundAll("vo/announcer_begins_1sec.wav") end)

  timer.Create("prophunt_HuntStart", hidetime, 1, prophunt_HuntStart)
  timer.Create("prophunt_HuntOver", roundtime - 0.05, 1, prophunt_HuntOver)
  timer.Create("prophunt_RechargeCPBonus", chargetime, 0, prophunt_RechargeCPBonus)
end

function prophunt_RoundEnd()
  prophunt_DestroyTimers()
  prophunt_damageonshoot = false

  if(!prophunt_enabled) then
    return
  end

  timer.Simple(14.4, (function() prophunt_SwitchTeams() end))
  --gamerules:SetSwitchTeams(true)
end

function prophunt_PlayerChangedClass(pPlayer, newClass)
  if(!prophunt_enabled || !pPlayer:IsAlive()) then
    return
  end

  -- Classes allowed specific to team
  if(pPlayer:GetTeam() == 2) then
    if(newClass != TF2_CLASS_SCOUT && newClass != "scout") then
      prophunt_ForcePlayerClass(pPlayer, TF2_CLASS_SCOUT)
    end
  elseif(pPlayer:GetTeam() == 3) then
    if(newClass != TF2_CLASS_HEAVY && newClass != "heavyweapons" &&
      newClass != TF2_CLASS_PYRO && newClass != "pyro") then
      prophunt_ForcePlayerClass(pPlayer, TF2_CLASS_PYRO)
    end
  end
end

function prophunt_PlayerCommand(pPlayer, command, arguments)
  if(!prophunt_enabled) then
    return
  end

  -- Classes allowed specific to team
  if(command == "joinclass" || command == "join_class") then
    if(pPlayer:GetTeam() == 2) then
      if(arguments[1] != TF2_CLASS_SCOUT && arguments[1] != "scout") then
        pPlayer:SayText("You must be a scout.\n")
        return true
      end
    elseif(pPlayer:GetTeam() == 3) then
      -- check for invalid class
      if((arguments[1] != TF2_CLASS_HEAVY && arguments[1] != "heavyweapons" &&
        arguments[1] != TF2_CLASS_PYRO && arguments[1] != "pyro")) then
        pPlayer:SayText("You must be a pyro or a heavy.\n")
        return true
      end

      -- or heavy not allowed right now
      if((arguments[1] == TF2_CLASS_HEAVY || arguments[1] == "heavyweapons") && !prophunt_IsHeavyAllowedNow()) then
        pPlayer:SayText("No more heavies allowed.\n")
        return true
      end
    end
  end

  -- allow dead props to enter free roaming mode
  if(command == "spec_mode" && pPlayer:GetTeam() == 2 && convar.GetBool("DF_prophunt_prop_spec_freeroam")) then
    if(pPlayer:GetOffsetInt(prophunt_offsetobservermode) == OBS_MODE_CHASE) then
      pPlayer:SetOffsetInt(prophunt_offsetobservermode, OBS_MODE_ROAMING)
      return true
    end
  end
end

function prophunt_PrimaryAttack(pWeapon)
  if(!prophunt_enabled) then
    return
  end

  if(!prophunt_damageonshoot) then
    return
  end

  local pOwner = pWeapon:GetOwner()
  if(pOwner == nil) then
    return
  end

  -- convert to Player class
  pOwner = player.GetByID(pOwner:EntIndex())

  -- damage on fire (-10 shotgun axe, -2 flame every 0.05 seconds)
  if(pOwner:GetTeam() == 3) then
    prophunt_WeaponFired(pOwner, pWeapon)
  end
end

function prophunt_SecondaryAttack(pWeapon)
  if(!prophunt_enabled) then
    return
  end

  local pOwner = pWeapon:GetOwner()
  if(pOwner == nil) then
    return
  end

  -- convert to Player class
  pOwner = player.GetByID(pOwner:EntIndex())

  -- Block airblast on pyros
  if(pOwner:GetTeam() == 3 && pOwner:GetPlayerClass() == TF2_CLASS_PYRO) then
    return true
  end

  if(!prophunt_damageonshoot) then
    return
  end

  -- SecondaryAttack is called when the minigun is fired while holding down attack2
  local bIsInAttack = (bit.band(pOwner:Buttons(), IN_ATTACK) == IN_ATTACK)
  if(pOwner:GetTeam() == 3 && bIsInAttack) then
    prophunt_WeaponFired(pOwner, pWeapon)
  elseif(pWeapon:GetClassname() == "tf_weapon_fists") then
    prophunt_WeaponFired(pOwner, pWeapon)
  end
end

function prophunt_OnTakeDamage(pPlayer, pInflictor, pAttacker, flDamage, iDamageType)
  if(!prophunt_enabled) then
    return
  end

  -- the player and attacker must both be present and be players
  if(pPlayer == nil || !pPlayer:IsPlayer() || pAttacker == nil || !pAttacker:IsPlayer()) then
    return
  end

  if(pPlayer == pAttacker) then
    return
  end

  -- Damaged player is prop and Attacker is hunter
  if(pPlayer:GetTeam() == 2 && pAttacker:GetTeam() == 3) then
    local health = pAttacker:Health()
    local maxHealth = pAttacker:GetMaxHealth()

    -- play a sound when player is first found
    if(prophunt_GetPlayerFoundFlag(pPlayer) == 0) then
      pPlayer:EmitSound("prophunt/found.mp3", 100, 100)
      pAttacker:PlaySound("prophunt/found.mp3")
      prophunt_SetPlayerFoundFlag(pPlayer, 1)
    end

    if(health < maxHealth) then
      local addHealth = flDamage
      if(addHealth > 10) then
        addHealth = 10
      end

      local newHealth = pAttacker:Health() + addHealth
      if(newHealth > maxHealth) then
        newHealth = maxHealth
      end

      pAttacker:SetHealth(newHealth)
    end
  end
end

convar.Add("DF_prophunt_enabled", "0", 0, "Whether or not prophunt is enabled.")
convar.Add("DF_prophunt_prop_spec_freeroam", "1", 0, "Whether or not to allow props to enter free roaming mode when dead.")
convar.Add("DF_prophunt_hunterfreeze", "1", 0, "Whether or not the hunters will be frozen while the props hide.")
-- set to 5 for testing
convar.Add("DF_prophunt_hidetime", "35", 0, "How long (in seconds) props have to hide.")
convar.Add("DF_prophunt_maxheavies", "2", 0, "Maximum number of heavies to allow on blue.")
-- set to 10 for testing
convar.Add("DF_prophunt_cpbonus_rechargetime", "55", 0, "How often (in seconds) the control point bonus is recharged.")

hook.Add("LevelInit", "prophunt_LevelInit", prophunt_LevelInit)
hook.Add("LevelShutdown", "prophunt_LevelShutdown", prophunt_LevelShutdown)
hook.Add("ServerActivate", "prophunt_ServerActivate", prophunt_ServerActivate)
hook.Add("PlayerDied", "prophunt_PlayerDied", prophunt_PlayerDied)
hook.Add("PlayerDisconnected", "prophunt_PlayerDisconnected", prophunt_PlayerDisconnected)
hook.Add("Think", "prophunt_GameFrame", prophunt_GameFrame)
hook.Add("RoundStart", "prophunt_RoundStart", prophunt_RoundStart)
hook.Add("ArenaRoundStart", "prophunt_ArenaRoundStart", prophunt_ArenaRoundStart)
hook.Add("RoundEnd", "prophunt_RoundEnd", prophunt_RoundEnd)
hook.Add("PlayerChangedClass", "prophunt_PlayerChangedClass", prophunt_PlayerChangedClass)
hook.Add("PlayerCommand", "prophunt_PlayerCommand", prophunt_PlayerCommand)
hook.Add("PrimaryAttack", "prophunt_PrimaryAttack", prophunt_PrimaryAttack)
hook.Add("SecondaryAttack", "prophunt_SecondaryAttack", prophunt_SecondaryAttack)
hook.Add("OnTakeDamage", "prophunt_OnTakeDamage", prophunt_OnTakeDamage)

admincommand.Add("testprop", 32768, 0, "", prophunt_test)
admincommand.Add("propmenu", 32768, 0, "", prophunt_menu)
