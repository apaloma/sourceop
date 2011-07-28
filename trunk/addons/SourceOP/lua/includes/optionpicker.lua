local OPTIONS_PER_PAGE = 7

OptionPicker = class(function(picker, options, pPlayer, choicecallback)
  picker.player = pPlayer
  picker.menu = nil
  picker.options = nil
  picker.alloptions = options or { }
  picker.choicecallback = choicecallback or (function(pPlayer, choice) end)

  picker:Show(1)

  return picker
end)

function OptionPicker:Show(curpage)
  local pages = math.ceil(table.Count(self.alloptions) / OPTIONS_PER_PAGE)

  if(curpage > pages) then
    curpage = pages
  end
  self.curpage = curpage

  // destroy any existing menu
  if(self.menu != nil) then
    self.menu:Expire()
  end

  local buttons = 1
  local menutext = "Choose an option:\n \n"
  local skipoptions = (curpage-1) * OPTIONS_PER_PAGE
  local numoptions = 0
  self.options = {}
  // the players aren't necessarily sequential, so pick them out
  // i.e. the keys may skip numbers
  // e.g. players could possibly contain keys 1,3,9,16
  for k,v in pairs(self.alloptions) do
    if(skipoptions > 0) then
      skipoptions = skipoptions - 1
    else
      table.insert(self.options, k)
      numoptions = numoptions + 1
      // shift the 1 over left
      // subtract 1 from it later
      // effectively giving us the correct mask
      // i.e. 2^numoptions - 1
      buttons = bit.lshift(buttons, 1)
      menutext = menutext .. "->" .. numoptions .. ". " .. tostring(v) .. "\n"
      if(numoptions >= OPTIONS_PER_PAGE) then
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

function OptionPicker:Choose(choice)
  if(choice == 8) then
    self:Show(self.curpage - 1)
  elseif(choice == 9) then
    self:Show(self.curpage + 1)
  elseif(choice == 0) then
    self.choicecallback(self.player, nil)
  else
    self.choicecallback(self.player, self.options[choice])
  end
end

/*
function testpicker(pPlayer)
  local options = {"Test1", "Test2", "Test3", "Test4", "Test5", "Test6", "Test7", "Test8", "Test9", "Test10", "Test11", "Test12", "Test13", "Test14", "Test15", "Test16"}
  local testpicker_func = (function(pPlayer, choice)
    if(choice == nil) then
      pPlayer:SayText("No option was chosen.\n")
    else
      pPlayer:SayText("Option " .. options[choice] .. " was chosen.\n")
    end
  end)

  OptionPicker(options, pPlayer, testpicker_func)
end

concommand.Add("testpicker", testpicker, "Tests the Lua option picker.")
*/
