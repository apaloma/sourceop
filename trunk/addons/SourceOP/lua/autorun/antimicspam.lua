local spamoffenses = {}

function micspam_handleplayer(pPlayer)
  local dealmethod = convar.GetInteger("DF_micspam_dealmethod")

  if(dealmethod == 1) then
    if(!pPlayer:IsGagged()) then
      pPlayer:Gag(true)
      pPlayer:SayText("You were automatically gagged for mic spamming.\n")
    end
  elseif(dealmethod == 2) then
    pPlayer:Kick()
  elseif(dealmethod == 3) then
    pPlayer:BanByID(convar.GetInteger("DF_micspam_bantime"), "Automatic ban for mic spam")
  else
    pPlayer:BanByID(0, "Automatic ban for mic spam")
  end
end

function micspam_querycomplete(pPlayer, estatus, value)
  local eQueryCvarValueStatus_ValueIntact = 0
  if(estatus == eQueryCvarValueStatus_ValueIntact && tonumber(value) != 0) then
    if(spamoffenses[pPlayer:EntIndex()] == nil) then
      spamoffenses[pPlayer:EntIndex()] = 1
    else
      spamoffenses[pPlayer:EntIndex()] = spamoffenses[pPlayer:EntIndex()] + 1
    end

    if(spamoffenses[pPlayer:EntIndex()] > convar.GetInteger("DF_micspam_threshold")) then
      micspam_handleplayer(pPlayer)
    end
  else
    spamoffenses[pPlayer:EntIndex()] = 0
  end
end

function micspam_query()
  if(convar.GetInteger("DF_micspam_enabled") != 0) then
    local players = player.GetAll()
    for _, v in pairs(players) do
      if(!v:IsBot()) then
        v:QueryConVar("voice_inputfromfile", function(estatus, name, value) micspam_querycomplete(v, estatus, value) end)
      end
    end
  end
end

function micspam_playeractivate( pPlayer )
  spamoffenses[pPlayer:EntIndex()] = 0
end

function micspam_levelinit()
  spamoffenses = {}
  // old timers by the same name are automatically removed
  timer.Create("antimicspam", 2, 0, micspam_query)
end

convar.Add("DF_micspam_enabled", "0", 0, "Enable automatic kicking, banning, or gagging of mic spammers.", true, 0, true, 1)
convar.Add("DF_micspam_threshold", "2", 0, "Number of consecutive occurences a player can be detected as playing audio from file before action is taken.", true, 0, false, 1)
convar.Add("DF_micspam_dealmethod", "1", 0, "Punishment for mic spammers. 1: Gag, 2: Kick, 3: Temporary ban, 4: Permanent ban", true, 1, true, 4)
convar.Add("DF_micspam_bantime", "5", 0, "Number of minutes to ban players if the temporary ban deal method is selected.")
hook.Add("PlayerActivate", "micspamactivate", micspam_playeractivate)
hook.Add("LevelInit", "micspaminit", micspam_levelinit)
micspam_levelinit()
