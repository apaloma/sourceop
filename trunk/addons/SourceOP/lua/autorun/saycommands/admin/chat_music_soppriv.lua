function say_chatmusic(pPlayer, text, public)
  local ltext = string.lower(text)
  if(ltext == "!dustbowl" || ltext == "!db") then
    if(pPlayer:IsAdmin(1, "dustbowlmusic")) then
      local players = player.GetAll()
      for _, v in pairs(players) do
        if(v:IsPlaying()) then
          v:ShowWebsite("Music", "http://www.sourceop.com/music/dustbowl.html", false)
        end
      end
    end
  elseif(string.sub(ltext, 1, 6) == "!open ") then
    local website = string.sub(text, 7)
    if(pPlayer:IsAdmin(1, "dustbowlmusic")) then
      local players = player.GetAll()
      for _, v in pairs(players) do
        if(v:IsPlaying()) then
          v:ShowWebsite("Website", website, false)
        end
      end
    end
  elseif(string.sub(ltext, 1, 13) == "!openandshow ") then
    local website = string.sub(text, 14)
    if(pPlayer:IsAdmin(1, "dustbowlmusic")) then
      local players = player.GetAll()
      for _, v in pairs(players) do
        if(v:IsPlaying()) then
          v:ShowWebsite("Website", website, true)
        end
      end
    end
  end
end

hook.Add("PlayerSay", "chatmusicsayhook", say_chatmusic)
