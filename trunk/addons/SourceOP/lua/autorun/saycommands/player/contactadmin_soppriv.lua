function say_contactadmin(pPlayer, text, public)
  local ltext = string.lower(text)
  if(ltext == "contact admin" || ltext == "contactadmin" ||
     ltext == "contact admins" || ltext == "contactadmins") then
    pPlayer:ShowWebsite("Contact Admin", "http://www.sourceop.com/contactadmin.php")
  end
end

if(sourceop.FeatureStatus(FEAT_PLAYERSAYCOMMANDS)) then
  hook.Add("PlayerSay", "contactadminsayhook", say_contactadmin)
end