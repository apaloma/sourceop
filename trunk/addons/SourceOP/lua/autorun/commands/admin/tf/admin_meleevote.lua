/** \file admin_meleevote.lua
 *  \brief Allows admins to start a vote for melee mode.
 *
 * @author Tony Paloma
 * @version 1/18/2008
*/

/** Called when the vote is over.
 *
 *  @param vote The vote that ended.
*/
function meleevoteover( vote )
  local yesvotes    = vote.picks[1]
  local novotes     = vote.picks[2]
  local abstentions = vote.picks[3]

  // check which won and perform the corresponding action
  if(yesvotes >= novotes) then
    wintext = "Yes wins:  "
    game.ConsoleCommand("DF_tf2_meleeonly 1")
  else
    wintext = "No wins:  "
    game.ConsoleCommand("DF_tf2_meleeonly 0")
  end

  // say some text in chat to everyone about results
  wintext = wintext .. "Yes: " .. yesvotes .. ", No: " .. novotes .. ", Abstain: " .. abstentions .. "\n"
  sourceop.SayTextAll(wintext)
end

/** Starts a melee vote. */
function meleevote( pPlayer, command, arguments )
  AbstainableVote("Enable melee only mode?",
                  {"Yes", "No", "Abstain"}, 3, 30,
                  "Abstain is the default choice.\nAbstentions go to the majority.",
                  meleevoteover)
end

if(sourceop.FeatureStatus(FEAT_ADMINCOMMANDS) && game:IsTF2()) then
  admincommand.Add("meleevote", 8, 0, "", meleevote)
end
