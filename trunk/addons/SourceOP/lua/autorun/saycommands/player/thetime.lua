function say_thetime(pPlayer, text, public)
  local ltext = string.lower(text)
  if(ltext == "thetime") then
    DelayedSayToAll("The current server time is: " .. os.date("%I:%M:%S %p") .. ".\n")
  end
end

if(sourceop.FeatureStatus(FEAT_PLAYERSAYCOMMANDS)) then
  hook.Add("PlayerSay", "thetimesayhook", say_thetime)
end