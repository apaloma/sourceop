
/** Checks if the command to be executed needs special handling.
 *
 *  @param pPlayer The player to execute on.
 *  @param arguments Array or arguments.
 *  @return True if any special handling was done.
*/
function hlxexec_checkspecial( pPlayer, arguments )
  if(arguments[2] == "say" && string.sub(arguments[3],1,11) == "hlx_browse ") then
    pPlayer:ShowWebsite("HLstatsX", string.sub(arguments[3],12))
    return true
  end
  return false
end

function hlxexec( pPlayer, command, arguments )
  if(!pPlayer) then
    if(#arguments >= 2) then
      local playerid = tonumber(arguments[1])
      local pExecPlayer = player.GetByUserID(playerid)
      if(pExecPlayer) then
        if(!hlxexec_checkspecial(pExecPlayer, arguments)) then
          local execstring = ""
          for k,v in pairs(arguments) do
            if(k > 2) then
              execstring = execstring .. "\"" .. v .. "\"" .. " "
            elseif(k > 1) then
              execstring = execstring .. v .. " "
            end
          end
          Msg(execstring .. "\n")
          pExecPlayer:FakeConCommand(execstring)
        end
      end
    end
  end
end

concommand.Add("hlx_exec", hlxexec, "Executes a command on a player given a UserID.")
