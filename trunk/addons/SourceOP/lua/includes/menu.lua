/** \file menu.lua
 *  \brief Provides a menu interface to players.
 *
 * @author Tony Paloma
 * @version 1/18/2008
*/

require "timer"
require "playercommand"

local menurefreshtime = 4
local menunum = 0


Menu = class(function(menu, pPlayer, buttons, showtime, menutext, choicecallback)
  menu.player = pPlayer
  menu.buttons = buttons
  menu.showtime = showtime
  menu.menutext = menutext
  menu.choicecallback = choicecallback or (function(pPlayer, choice) end)
  menu.name = "menutimer" .. menunum
  menu.valid = true

  local timercalls = math.floor(menu.showtime/menurefreshtime)
  if(timercalls > 0) then
    // create a timer with a unique name (incremented each time)
    // the timer calls a closure that simply calls the menu's Call function
    timer.Create(menu.name, menurefreshtime, timercalls, (function() menu:Call() end))
    timer.Create("menureset" .. pPlayer:EntIndex(), menu.showtime+1, 1, (function() menu:Expire() end))
  end
  menu:Call()

  menunum = menunum + 1

  return menu
end)

function Menu:Call()
  local menutime = self.showtime

  // skip when small or zero time remaining
  if(menutime < 0.01) then return end

  // show with two seconds extra when menu there are refreshes still to come
  // this effectively prevents flickering of menu when it refreshes
  if(menutime > menurefreshtime) then menutime = menurefreshtime + 2 end

  self.player:SetMenu(self)
  self.player:ShowMenu(self.buttons, menutime, self.menutext)
  // do not subtract the 7 seconds above, only the length between refreshes
  if(menutime > menurefreshtime) then menutime = menurefreshtime end
  self.showtime = self.showtime - menutime
end

function Menu:Choose(choice)
  self.choicecallback(self.player, choice)
  self:Expire()
end

function Menu:Expire()
  if(self.valid) then
    self.valid = false
    self.player:SetMenu(nil)
    timer.Destroy(self.name)
    timer.Destroy("menureset" .. self.player:EntIndex())
  end
end

function menuselect(pPlayer, command, arguments)
  local menu = pPlayer:GetMenu()
  if(menu) then
    menu:Choose(tonumber(arguments[1]))
    return true
  end

  return false
end

playercommand.Add("menuselect", menuselect)
