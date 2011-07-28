
include('info.lua')

function ENT:Initialize()
  self:AddEffects(EF_NODRAW)
  //self.SetSize(Vector(0,0,0), Vector(0,0,0))
  self:SetCollisionGroup(COLLISION_GROUP_NONE)
  self:SetMoveType(MOVETYPE_NONE)
end

function ENT:Use( activator, caller )
end

function ENT:Think()
  local players = player.GetAll()
  if(table.Count(players) > 0) then
    local rocket = ents.Create("sop_dodgeball_rocket")
    rocket:SetAbsOrigin(self:GetAbsOrigin())
    rocket:Spawn()
    rocket:Activate()
  end

  if(convar.GetBool("DF_dodgeball_noammopacks")) then
    local ammopacks = ents.FindByClass("tf_ammo_pack")
    for _, v in pairs(ammopacks) do
      v:Kill()
    end
  end

  self:NextThink(CurTime() + convar.GetNumber("DF_dodgeball_spawnrate"))
  collectgarbage("step")
  return true
end

function ENT:Touch(other)
end

convar.Add("DF_dodgeball_maxrockets", "30", 0, "Maximum number of rockets.")
convar.Add("DF_dodgeball_spawnrate", "1", 0, "Rocket spawn rate in seconds.")
convar.Add("DF_dodgeball_noammopacks", "1", 0, "Removes ammo packs.")
