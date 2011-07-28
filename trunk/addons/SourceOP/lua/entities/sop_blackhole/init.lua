include("info.lua")

ENT.LastSucked = 0
ENT.Owner = nil

function ENT:SpawnFunction(ply,tr)
  local wb = ents.Create("sop_blackhole")
  wb:SetPos(tr.HitPos + tr.HitNormal * 30)
  wb:Spawn()
  wb.Owner = ply
  return wb
end

function ENT:Precache()
  util.PrecacheModel("models/combine_helicopter/helicopter_bomb01.mdl")
end

function ENT:Initialize()
  self:SetModel("models/combine_helicopter/helicopter_bomb01.mdl")
  self:PhysicsInit(SOLID_VPHYSICS)
  self:SetCollisionGroup(COLLISION_GROUP_DEBRIS_TRIGGER)

  self:SetTrigger(true)
  local phy = self:GetPhysicsObject()
  if phy:IsValid() then
    phy:EnableGravity(false)
    phy:SetMass(500)
  end

  // TODO: Look into adding effects similar to gmod util.Effect

  self:DrawShadow(false)
end

function ENT:Think()
  if CurTime() > self.LastSucked + 0.05 then
    local sPos = self:GetPos()
    for _,e in pairs(ents.FindInSphere(sPos,700,true)) do
      if e && e:IsValid() then
        local classname = e:GetClass()
        local phy = e:GetPhysicsObject()
        if phy:IsValid() then
          local ePos = e:LocalToWorld(e:OBBCenter())
          local eOff = sPos - ePos
          local fMul = 8
          local noForce = false
          if classname == "prop_combine_ball" then fMul = 18 end
          if classname == "prop_ragdoll" then fMul = 40 end
          if e:IsNPC() then
            fMul = 10
            if eOff:Length() < 120 && classname != "npc_rollermine" && classname != "npc_cscanner" && classname != "npc_manhack" && classname != "combine_mine" && classname != "npc_turret_floor" then
              e:TakeDamage(e:Health() + 1,self.Owner)
            end
          end
          if !noForce then
            local num_a = ((eOff:Length() + 1) / 720)
            local num_b = fMul*phy:GetMass()
            local num_c = num_b / num_a
            local normalvec = eOff:GetNormal()
            local forcevec = normalvec * num_c
            phy:ApplyForceCenter(forcevec)
          end
        end
      end
    end
    collectgarbage("step", 150)
  end

  self:NextThink(CurTime())
  return true
end

function ENT:Touch(other)
end
