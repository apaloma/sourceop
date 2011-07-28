function adminsuddendeath( pPlayer, command, arguments )
  gamerules.SetStalemate(STALEMATE_TIMER, true)
  sourceop.SayTextAll("Sudden death has been forced.\n")
end

if(sourceop.FeatureStatus(FEAT_ADMINCOMMANDS) && game:IsTF2()) then
  admincommand.Add("suddendeath", 2048, 0, "", adminsuddendeath)
end
