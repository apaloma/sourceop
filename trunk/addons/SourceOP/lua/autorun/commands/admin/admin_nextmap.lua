function adminnextmap( pPlayer, command, arguments )
  gamerules.ChangeLevel()
end

if(sourceop.FeatureStatus(FEAT_ADMINCOMMANDS)) then
  admincommand.Add("nextmap", 2, 0, "", adminnextmap)
end
