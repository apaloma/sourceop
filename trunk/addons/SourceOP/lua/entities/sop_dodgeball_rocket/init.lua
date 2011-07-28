
include('info.lua')

local forceOwner = nil
local m_bCriticalOffset = sourceop.GetPropOffset("DT_TFProjectile_Rocket", "m_bCritical")
local m_iReflectedOffset = sourceop.GetPropOffset("DT_TFBaseRocket", "m_iDeflected")
local m_flDamageOffset = m_iReflectedOffset + 4

function ENT:Initialize()
  self.Rocket = ents.Create("tf_projectile_rocket")
  self.Rocket:KeyValue("classname", "tf_projectile_rocket")
  self.Rocket:SetAbsOrigin(self:GetAbsOrigin())

  self:AddEffects(EF_NODRAW)
  //self.SetSize(Vector(0,0,0), Vector(0,0,0))
  self:SetCollisionGroup(COLLISION_GROUP_NONE)
  self:SetMoveType(MOVETYPE_NONE)

  self.Rocket:SetOffsetFloat(m_flDamageOffset, 90)
  self.Rocket:SetOffsetInt(m_bCriticalOffset, 1)
  //self.Rocket:SetOffsetInt(m_iRefelctedOffset, 0)

  self.Owner = nil
  if(self:FindRandomOwner() == false) then
    self.Rocket:Kill()
    self:Kill()
  end
  local team = tostring(self.Team)
  self.Rocket:Fire("SetTeam", team)
  self.Speed = convar.GetInteger("DF_dodgeball_rocket_speed")
  self.Velocity = Vector(0,0,-100)
  self.Rocket:SetVelocity(self.Velocity)
  self:FindNewTarget()

  self.Rocket:Spawn()
  self.Rocket:Activate()
end

function ENT:FindRandomOwner()
  if(forceOwner != nil) then
    self:SetOwnerPlayer(forceOwner, true)
    return
  end

  local players = player.GetAll()
  local validplayers= {}

  for _, v in pairs(players) do
    if(v:GetTeam() >= 2) then
      table.insert(validplayers, v)
    end
  end


  local total = table.Count(validplayers)
  if(total > 0) then
    local player_num = math.random(total)
    local owner = validplayers[player_num]

    self:SetOwnerPlayer(owner, true)
    return true
  end
  return false
end

function ENT:SetOwnerPlayer(pOwner, setRocketOwner)
  if(setRocketOwner) then self.Rocket:SetOwner(pOwner) end
  self.Owner = pOwner
  if(pOwner != nil) then
    self.Team = pOwner:GetTeam()
  end
end

function ENT:FindNewTarget()
  if(self.Owner == nil || !self.Owner:IsValid()) then
    return
  end

  local players = player.GetAll()
  local otherteam = {}
  local visibleotherteam = {}

  for _, v in pairs(players) do
    if(v:GetTeam() >= 2 && v:GetTeam() != self.Owner:GetTeam()) then
      table.insert(otherteam, v)
    end
  end

  for _, v in pairs(otherteam) do
    if(self.Rocket:Visible(v)) then
      table.insert(visibleotherteam, v)
    end
  end

  local otherteamcount = table.Count(otherteam)
  local visibleotherteamcount = table.Count(visibleotherteam)

  // choose a visible player if possible, otherwise choose any player on the
  // other team
  if(visibleotherteamcount > 0) then
    local player_num = math.random(visibleotherteamcount)
    self.Target = visibleotherteam[player_num]
  elseif(otherteamcount > 0) then
    local player_num = math.random(otherteamcount)
    self.Target = otherteam[player_num]
  else
    self.Target = nil
  end
end

function ENT:Use( activator, caller )
end

function ENT:Think()
  if(self.Rocket:IsValid() && self.Rocket:GetClassname() == "tf_projectile_rocket") then
    self.Rocket:SetAngles(self.Velocity:Angle())
  else
    self:Kill()
    return false
  end

  local realOwner = self.Rocket:GetOwner()
  if(realOwner == nil) then
      self:Kill()
      return false
  end

  // check for new owner (airblasted for example)
  if(self.Owner == nil || !self.Owner:IsValid() || self.Owner:EntIndex() != realOwner:EntIndex()) then
    if(realOwner == nil) then
      self:Kill()
      return false
    end
    self:SetOwnerPlayer(player.GetByID(realOwner:EntIndex()), false)
    self.Target = nil
    self.Speed = self.Speed * 1.5
    if(self.Speed > 2000) then self.Speed = 2000 end
  end

  if(self.Target == nil) then
    self:FindNewTarget()
  end

  if(self.Target != nil) then
    self.Velocity = self.Velocity * 0.4 + ((self.Target:EyePos() - self.Rocket:GetAbsOrigin()):GetNormal() * (self.Speed*0.6))
    //if(self.Velocity:Length() > convar.GetInteger("DF_dodgeball_rocket_speed")) then
      self.Velocity = self.Velocity:GetNormal() * self.Speed
    //end
    self.Rocket:SetVelocity(self.Velocity)
  end

  self:NextThink(CurTime() + 0.05)
  collectgarbage("step")
  return true
end

function ENT:Touch(other)
end

function admindodegeballrocket( pPlayer, command, arguments )
  if(pPlayer) then
    forceOwner = player.GetByID(pPlayer:EntIndex())
    local rocket = ents.Create("sop_dodgeball_rocket")
    rocket:SetAbsOrigin(pPlayer:EyePos())
    rocket:Spawn()
    rocket:Activate()
    forceOwner = nil
  end
end

convar.Add("DF_dodgeball_rocket_speed", "300", 0, "Rocket speed.")
admincommand.Add("dodgeballrocket", 8192, 0, "", admindodegeballrocket)