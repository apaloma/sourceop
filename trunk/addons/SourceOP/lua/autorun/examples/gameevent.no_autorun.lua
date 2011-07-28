function gameeventtest(eventName, eventKeyVals)
  Msg(eventName .. "\n")
  PrintTable(eventKeyVals)
end

hook.Add("GameEvent", "gameeventtest", gameeventtest)

// Hook an event to test
// SourceOP always hooks the following events:
// server_spawn, player_say, player_changename, player_death, player_spawn, player_disconnect, server_cvar, round_start, and player_team
// In TF2, it also hooks:
// player_changeclass, teamplay_map_time_remaining, teamplay_teambalanced_player, teamplay_round_win, teamplay_round_start, teamplay_flag_event, arena_round_start, post_inventory_application

sourceop.AddGameEventListener("teamplay_round_selected")
