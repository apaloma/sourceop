
include('info.lua')

function ENT:Precache()
  util.PrecacheModel("models/props_c17/furniturebathtub001a.mdl")
end

function ENT:Initialize()
  //PrintTable(self)
  //Msg("\n\n")
  //PrintTable(getmetatable(self))
  //Msg("\n\n")
  Msg(self.PrintName .. ": Hi2. Spawned at (" .. tostring(self:GetAbsOrigin()) .. ")\n")
  self:SetModel("models/props_c17/furniturebathtub001a.mdl")

  //self:PhysicsInit(SOLID_VPHYSICS)
  self:SetMoveType(MOVETYPE_FLY)
  self:SetVelocity(Vector(10, 0, 0))
end

function ENT:Use( activator, caller )
end

function ENT:Think()
  //Msg(self.PrintName .. ": Think2.\n")
  return false
end

function ENT:Touch(other)
end
