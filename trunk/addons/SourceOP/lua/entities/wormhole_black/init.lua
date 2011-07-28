//AddCSLuaFile("cl_init.lua")
//AddCSLuaFile("shared.lua")
include("shared.lua")

ENT.LastSucked = 0

ENT.White = nil

ENT.Owner = nil

ENT.Suckeds = {}

ENT.LinkedTime = 0

ENT.LastExTime = 0
ENT.LastExOrigin = 0

function ENT:SpawnFunction(ply,tr)
	local wb = ents.Create("wormhole_black")
	wb:SetPos(tr.HitPos + tr.HitNormal * 30)
	wb:Spawn()
	ply.LastBlackHole = wb
	wb.Owner = ply
	return wb
end

function ENT:Precache()
  util.PrecacheModel("models/combine_helicopter/helicopter_bomb01.mdl")
end

function ENT:Initialize()
	self:SetModel("models/combine_helicopter/helicopter_bomb01.mdl")
	self:PhysicsInit(SOLID_VPHYSICS)
	self:SetCollisionGroup(COLLISION_GROUP_NONE)

	self:SetTrigger(true)
	local phy = self:GetPhysicsObject()
	if phy:IsValid() then
		phy:EnableGravity(false)
		phy:SetMass(500)
	end
	/*--[[local ed = EffectData()
	ed:SetOrigin(self:GetPos())
	ed:SetEntity(self)
	util.Effect("wormhole_black",ed)]]*/
	self:DrawShadow(false)
end

function ENT:Link(wh)
	self.White = wh
	self.LinkedTime = CurTime()
end

function ENT:Ex()
	local sk = self.Suckeds[1]
	self.LastExOrigin = sk.Time
	self.LastExTime = CurTime()
	local e = ents.Create(sk.Class)
	e:SetPos(self.White:GetPos() + sk.Offset)
	e:SetModel(sk.Model)
	e:SetSkin(sk.Skin)
	--e:SetColor(sk.Color)
	if sk.Class == "npc_grenade_frag" then
		e:SetOwner(sk.Own)
		e:Fire("settimer","2",0)
	end
	e:Spawn()
	e:Activate()

	/*local ed = EffectData()
	ed:SetEntity(self.White)
	util.Effect("wormhole_burp",ed)

	ed = EffectData()
	ed:SetEntity(e)
	util.Effect("wormhole_trail",ed)*/

	table.remove(self.Suckeds,1)
end

function ENT:Think()
	if CurTime() > self.LastSucked + 0.05 then
		local sPos = self:GetPos()
		for _,e in pairs(ents.FindInSphere(sPos,700,true)) do
			if e && e:IsValid() then
        local classname = e:GetClass()
        if classname != "wormhole_white" && classname != "wormhole_black" then
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
              //local num_a = (eOff:Length() + 1) / 720
              //local num_b = fMul*phy:GetMass()
              //local num_c = num_b / num_a
              //local normalvec = eOff:GetNormal()
              //local forcevec = normalvec * num_c
              phy:ApplyForceCenter(eOff:GetNormal() * ((fMul*phy:GetMass())/((eOff:Length() + 1) / 720)))
            end
          end
        end
			end
		end
	end
	if self.White && self.White:IsValid() then
		if #(self.Suckeds) > 0 && CurTime() - self.LastExTime > self.Suckeds[1].Time - self.LastExOrigin && CurTime() > self.LinkedTime + 1 then
			self:Ex()
		end
	else
		self.LastExTime = 0
		self.LastExOrigin = 0
	end
	self:NextThink(CurTime())
	return true
end

function ENT:Touch(other)
  if true then return end
	if !(other && other:IsValid()) then return end

	if other:GetClass() == "wormhole_white" then self:Link(other) end

	if other:IsPlayer() || other == self.White then return end

	--if !(self.White && self.White:IsValid()) || other:IsPlayer() || other == self.White then return end

	local phy = other:GetPhysicsObject()
	if !phy:IsValid() then return end

	local eOff = other:GetPos() - self:GetPos()

	//if eOff:Length() < (math.max(other:BoundingRadius() * 0.4,25)) then
  if true then
		-- do effect
		/*local ed = EffectData()
		ed:SetEntity(self)
		util.Effect("wormhole_swallow",ed)



		ed = EffectData()
		ed:SetEntity(other)
		ed:SetStart(other:LocalToWorld(other:OBBCenter()))
		ed:SetOrigin(self:GetPos())
		util.Effect("wormhole_dissolve",ed)*/

		-- teleport
		//constraint.RemoveAll(other)

		if ValidEntity(self.White) then
			-- teleport
			if other:GetClass() != "prop_ragdoll" then
				other:SetPos(self.White:GetPos() + eOff)
			else
				-- move all the bones more-or-less at once
				for b=0,other:GetPhysicsObjectCount() do
					local phy = other:GetPhysicsObjectNum(b)
					if phy && phy:IsValid() then
						phy:SetPos(self.White:GetPos() + phy:GetPos() - self:GetPos())
					end
				end
			end

			/*ed = EffectData()
			ed:SetEntity(self.White)
			util.Effect("wormhole_burp",ed)

			ed = EffectData()
			ed:SetEntity(other)
			util.Effect("wormhole_trail",ed)*/
		else
			-- store it for later
			local sk = {}
			sk.Class = other:GetClass()
			//sk.Model = other:GetModel()
			//sk.Skin = other:GetSkin()
			//sk.Color = other:GetColor()
			//sk.Own = other:GetOwner()
			sk.Time = CurTime()
			sk.Offset = eOff

			-- will fix this later - currently crashes the game when spitting these out
			if sk.Class != "prop_vehicle_jeep" || sk.Class == "prop_combine_ball" then self.Suckeds[#(self.Suckeds) + 1] = sk end

			local phy = other:GetPhysicsObject()
			if phy:IsValid() then
				//phy:EnableMotion(false)
				//phy:EnableCollisions(false)
			end
			timer.Simple(0.01,other.Remove,other)
		end
	end
end
