
function msayvoteover( vote )
  local yesvotes    = vote.picks[1]
  local total = table.Count(player.GetAll())
  local yespercentage = (yesvotes / total) * 100
  local percentyes = tostring(math.floor(yespercentage)) .. "%"

  sourceop.SayTextAll("The vote got " .. yesvotes .. "/" .. total .. " (" .. percentyes .. ") yes votes.\n")
end

function msayhandler( pPlayer, command, arguments )
  local votetext = string.gsub(arguments[1], "\\n", "\n")
  Vote(votetext,
       {"Yes", "No"}, 30,
       "The results will be displayed in 30 seconds.",
       msayvoteover)
end

if(sourceop.FeatureStatus(FEAT_ADMINCOMMANDS)) then
  admincommand.Add("msay", 64, 0, "", msayhandler)
end
