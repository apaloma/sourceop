/** \file abstainvote.lua
 *  \brief Uses the Vote class to create a vote in which players can abstain.
 *
 * @author Tony Paloma
 * @version 1/18/2008
*/

AbstainableVote = class(function(vote, votetext, choices, abstainchoice, showtime, append, voteover)
  voteover = voteover or (function(vt) end)
  vote.voteover = function(vt) vote:Finish() voteover(vote.rvote) end
  vote.rvote = Vote(votetext, choices, showtime, append, vote.voteover)
  vote.abstainchoice = abstainchoice
end)

function AbstainableVote:Finish()
  local winningchoice = 1
  local winningchoicepicks = 0
  local abstentions = table.Count(player.GetAll())

  for k,v in pairs(self.rvote.picks) do
    if(k ~= self.abstainchoice) then
      abstentions = abstentions - v
      if(v > winningchoicepicks) then
        winningchoice = k
        winningchoicepicks = v
      end
    end
  end

  self.rvote.picks[winningchoice] = self.rvote.picks[winningchoice] + abstentions
  self.rvote.picks[self.abstainchoice] = abstentions
end
