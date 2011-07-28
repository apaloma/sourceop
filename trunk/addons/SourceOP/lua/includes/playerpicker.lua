local PLAYERS_PER_PAGE = 7

PlayerPicker = class(function(picker, pPlayer, choicecallback)
  picker.player = pPlayer
  picker.menu = nil
  picker.players = nil
  picker.choicecallback = choicecallback or (function(pPlayer, choice) end)

  picker:Show(1)

  return picker
end)

function PlayerPicker:Show(curpage)
  local players = player.GetAll()
  local pages = math.ceil(table.Count(players) / PLAYERS_PER_PAGE)

  if(curpage > pages) then
    curpage = pages
  end
  self.curpage = curpage

  // destroy any existing menu
  if(self.menu != nil) then
    self.menu:Expire()
  end

  local buttons = 1
  local menutext = "Choose a player:\n \n"
  local skipplayers = (curpage-1) * PLAYERS_PER_PAGE
  local numplayers = 0
  self.players = {}
  // the players aren't necessarily sequential, so pick them out
  // i.e. the keys may skip numbers
  // e.g. players could possibly contain keys 1,3,9,16
  for _,v in pairs(players) do
    if(skipplayers > 0) then
      skipplayers = skipplayers - 1
    else
      table.insert(self.players, v)
      numplayers = numplayers + 1
      // shift the 1 over left
      // subtract 1 from it later
      // effectively giving us the correct mask
      // i.e. 2^numplayers - 1
      buttons = bit.lshift(buttons, 1)
      menutext = menutext .. "->" .. numplayers .. ". " .. v:GetName() .. "\n"
      if(numplayers >= PLAYERS_PER_PAGE) then
        break
      end
    end
  end
  buttons = buttons - 1
  menutext = menutext .. "\n \n"
  if(curpage > 1) then
    buttons = buttons + 128
    menutext = menutext .. "->8. Previous\n"
  end
  if(curpage < pages) then
    buttons = buttons + 256
    menutext = menutext .. "->9. Next\n"
  end
  buttons = buttons + 512
  menutext = menutext .. "->0. Exit\n"
  self.menu = Menu(self.player, buttons, 999999, menutext, function(pPlayer, choice) self:Choose(choice) end)
end

function PlayerPicker:Choose(choice)
  if(choice == 8) then
    self:Show(self.curpage - 1)
  elseif(choice == 9) then
    self:Show(self.curpage + 1)
  elseif(choice == 0) then
    self.choicecallback(self.player, nil)
  else
    self.choicecallback(self.player, self.players[choice])
  end
end

/*function testpicker(pPlayer)
  local testpicker_func = (function(pPlayer, choice)
    if(choice == nil) then
      pPlayer:SayText("No player was chosen.\n")
    else
      pPlayer:SayText("Player " .. choice:GetName() .. " was chosen.\n")
    end
  end)
  PlayerPicker(pPlayer, testpicker_func)
end

concommand.Add("testpicker", testpicker, "Tests the Lua player picker.")*/
