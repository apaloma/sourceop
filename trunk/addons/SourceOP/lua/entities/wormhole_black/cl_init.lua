include("shared.lua")

ENT.lastEmd = 0

local mainSpr = Material("sprites/bh-glow")
local refMat = Material("effects/strider_pinch_dudv")
local smGlo = Material("sprites/bh-smoothglow")

function ENT:Initialize()
	refMat:SetMaterialFloat("$refractamount",0.05)
	refMat:SetMaterialInt("$forcerefract",1)
	refMat:SetMaterialInt("$nofog",1)
end

function ENT:Think()
	if CurTime() > self.lastEmd + 0.1 then
		local ePos = self.Entity:GetPos()
		local em = ParticleEmitter(ePos,false)
		local particle
		
		for i=0,2 do
			local pPos = VectorRand() * 50
			particle = em:Add("sprites/bh-smoothglow",ePos + pPos)
			if particle then
				particle:SetLifeTime(0)
				particle:SetDieTime(0.5)
				particle:SetVelocity(pPos * -0.1)
				particle:SetGravity(pPos * -4)
				particle:SetEndSize(1 + math.random() * 1)
				particle:SetStartSize(0)
				particle:SetEndAlpha(180 + math.random() * 60)
				particle:SetStartAlpha(160)
				particle:SetColor(0,0,0,255)
			end
		end
		self.lastEmd = CurTime()
	end
end

function ENT:Draw()
	local cPos = self.Entity:GetPos()
	local pSin = math.sin(CurTime() * 3)
	render.UpdateRefractTexture()
	refMat:SetMaterialFloat("$refractamount",0.03 + pSin * 0.03)
	render.SetMaterial(refMat)
	render.DrawSprite(cPos,100,100,Color(255,255,255,255))
	render.SetMaterial(smGlo)
	render.DrawSprite(cPos,70 - pSin * 10,70 - pSin * 10,Color(0,0,0,255))
	mainSpr:SetMaterialFloat("$rotMul",-15.0)
	render.SetMaterial(mainSpr)
	render.DrawSprite(cPos,40,40,Color(255,255,255,255))
	mainSpr:SetMaterialFloat("$rotMul",18.0)
	render.SetMaterial(mainSpr)
	local sz = 36 + 8 * pSin
	render.DrawSprite(cPos,sz,sz,Color(255,255,255,255))
end
