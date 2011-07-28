function adminskipnextmap( pPlayer, command, arguments )
  gamerules.AdvanceMapCycle()
end

if(sourceop.FeatureStatus(FEAT_ADMINCOMMANDS)) then
  admincommand.Add("skipnextmap", 2, 0, "", adminskipnextmap)
end
