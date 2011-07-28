
function testcmd( player, command, arguments )
  player = player or "<console>"
  Msg("player " .. tostring(player) .. " called " .. command .. "\n");
  for k,v in pairs(arguments) do
    Msg("  arg " .. k .. ": " .. v .. "\n")
  end
end

function testcmd2( player, command, arguments )
  local tbl = { string.strsplit(" ", "a b c") }
  for k,v in pairs(tbl) do
    Msg("  arg " .. k .. ": " .. v .. "\n")
  end

  Msg("strtrimtest: " .. string.strtrim("   hi   \n") .. "\n")
end


function removecommands( player, command, arguments )
  concommand.Remove("testconcommand")
  concommand.Remove("testconcommand2")
  // concommand.Remove("removeconcommand")
end

function addcommands()
  // C++ style comments
  concommand.Add("testconcommand", testcmd)
  concommand.Add("testconcommand2", testcmd2)
  concommand.Add("removeconcommand", removecommands)

  convar.Add("testcvar", "0")
  convar.Add("testcvar2", "2", 0, "Test var with ranges 1-3", true, 1, true, 3)
  convar.Add("testcvar3", "4", 0, "Test var with ranges > 1", true, 1, false, 3)
end

addcommands()
