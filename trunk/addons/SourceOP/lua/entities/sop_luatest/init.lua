
include('info.lua')

function ENT:Initialize()
  if(self.Test ~= nil) then
    self.Test = self.Test .. "!"
  else
    self.Test = "beans"
  end
  Msg(self.PrintName .. " initializing " .. self.Test .. "...\n")
  self.PrintName = self.PrintName .. "!"
  Msg(self.PrintName .. ": Hi. Spawned at (" .. tostring(self:GetAbsOrigin()) .. ")\n")
end

function ENT:Use( activator, caller )
end

function ENT:Think()
  Msg(self.PrintName .. ": Think.\n")
  return false
end

function ENT:Touch(other)
end
