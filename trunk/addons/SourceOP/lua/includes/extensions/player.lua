/** \file player.lua
 *  \brief Provides additional functions for use in the Player class.
 *
 * @author Tony Paloma
 * @version 9/10/2009
*/

require "hook"

playermenus = {}

local playercondoffset = sourceop.GetPropOffset("DT_TFPlayer", "m_Shared") + sourceop.GetPropOffset("DT_TFPlayerShared", "m_nPlayerCond")
local playercondsbaseoffset = playercondoffset + 6

function Player:GetMenu()
  return playermenus[self:EntIndex()]
end

function Player:SetMenu(themenu)
  local index = self:EntIndex()
  if playermenus[index] != themenu then
    local rmenu = playermenus[index]
    if(rmenu) then
      rmenu:Expire()
    end
    //Msg("menu set to: " .. tostring(themenu) .. "\n")
    playermenus[index] = themenu
  end
end

function Player:ChatPrint(text)
  self:SayText(text)
end

function Player:GetLogString()
  return self:GetName() ..
      "<" .. tostring(self:UserID()) .. "><" ..
      self:SteamIDRender() .. "><" ..
      gamerules.GetIndexedTeamName(self:Team()) .. ">"
end

if(playercondoffset <= 8 && game:IsTF2()) then
  Msg("Invalid offsets for Player:SetCondition.\n")
end

function Player:SetCondition(iCondition, bSet)
  if(!game:IsTF2() || playercondoffset <= 8) then
    return 0,0
  end

  local prevcond = self:GetOffsetInt(playercondoffset)
  local newcond
  local condadd = bit.lshift(1, iCondition)
  if(bSet) then
    newcond = bit.bor(prevcond, condadd)
    self:SetOffsetInt(playercondoffset, newcond)
    self:SetOffsetInt(playercondsbaseoffset + (iCondition * 4), 49024) // magic number
  else
    newcond = bit.band(prevcond, bit.bnot(condadd))
    self:SetOffsetInt(playercondoffset, newcond)
  end

  return prevcond, newcond
end

hook.Add( "PlayerDisconnected", "resetmenuondisconnect", (function(ply) ply:SetMenu(nil) end) )
