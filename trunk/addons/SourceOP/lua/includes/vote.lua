/** \file vote.lua
 *  \brief Utilizes the menu class to display and take votes.
 *
 * @author Tony Paloma
 * @version 1/18/2008
*/

require "hook"
require "timer"

local votenum = 0

Vote = class(function(vote, votetext, choices, showtime, append, voteover)
  append = append or ""
  vote.votetext = votetext
  vote.choices = choices
  vote.showtime = showtime
  vote.endtime = CurTime() + showtime
  vote.name = "votetimer" .. votenum
  vote.hookname = "votedisconnect" .. votenum
  vote.sayhookname = "voteplayersay" .. votenum
  vote.valid = true
  vote.menutext = votetext .. " \n \n"
  vote.buttons = 0
  vote.menus = {}
  vote.picks = {}
  vote.playervotes = {}
  vote.voteover = voteover or (function(vt) end)
  hook.Add( "PlayerDisconnected", vote.hookname, (function(ply) vote:PlayerDisconnected(ply) end) )
  hook.Add( "PlayerSay", vote.sayhookname, (function(pPlayer, text, public) vote:PlayerSay(pPlayer, text, public) end) )

  for k,v in pairs(choices) do
    vote.menutext = vote.menutext .. "->" .. k .. ". " .. v .. "\n"
    vote.buttons = vote.buttons + bit.lshift(1,k-1)
    vote.picks[k] = 0
  end
  vote.menutext = vote.menutext .. " \n" .. append

  for k,v in pairs(player.GetAll()) do
    table.insert(vote.menus, Menu(v, vote.buttons, showtime, vote.menutext, (function(pPlayer, choice) vote:PlayerVoted(pPlayer, choice) end)))
  end
  timer.Create(vote.name, showtime, 1, (function() vote:Finish() end))

  votenum = votenum + 1

  return vote
end)

function Vote:Finish()
  self.voteover(self)
  timer.Destroy(self.name)
  hook.Remove("PlayerDisconnected", self.hookname)
  hook.Remove("PlayerSay", self.sayhookname)
end

function Vote:PlayerVoted(pPlayer, choice)
  local playerindex = pPlayer:EntIndex()

  local oldchoice = self.playervotes[playerindex]
  if(oldchoice ~= nil) then
    if(self.picks[oldchoice] ~= nil) then
      self.picks[oldchoice] = self.picks[oldchoice] - 1
      self.playervotes[playerindex] = nil
    end
  end

  if(self.picks[choice] ~= nil) then
    self.picks[choice] = self.picks[choice] + 1
    self.playervotes[playerindex] = choice
  end
end

function Vote:PlayerDisconnected(pPlayer)
  local playerindex = pPlayer:EntIndex()

  if(self.playervotes[playerindex]) then
    local playerchoice = self.playervotes[playerindex]
    self.picks[playerchoice] = self.picks[playerchoice] - 1
    self.playervotes[playerindex] = nil
  end
end

function Vote:PlayerSay(pPlayer, text, public)
  local ltext = string.lower(text)

  local showtime = self.endtime - CurTime()
  if(ltext == "!revote") then
    table.insert(self.menus, Menu(pPlayer, self.buttons, showtime, self.menutext, (function(pVotePlayer, choice) self:PlayerVoted(pVotePlayer, choice) end)))
  end
end
