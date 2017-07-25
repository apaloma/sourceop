# SourceOP
SourceOP is a Source engine plugin that works for TF2, HL2 Deathmatch, and Counter-Strike: Source.

# History
SourceOP was created in 2004 after the release of Half-Life 2 to bring some of the features of my TFC plugin, AdminOP, to the source engine. Active development ceased around 2010-2011, but it still functions for TF2 as of 2017.

# Features
All features optional.
- Lua scripting engine
  - Including a [custom implementation of the prophunt game](trunk/addons/SourceOP/lua/autorun/prophunt/prophunt.lua)
  - Allows custom entities to be implemented like dodgeball [rockets](trunk/addons/SourceOP/lua/entities/sop_dodgeball_rocket) and [spawners](trunk/addons/SourceOP/lua/entities/sop_dodgeball_spawner) for use in [dodgeball](https://www.youtube.com/watch?v=TORA0GtrmLM) maps
  - Some documentation available on [the wiki](http://www.sourceop.com/wiki/Lua)
- [Remote management app](http://www.sourceop.com/screenshots/remote/mapwindow.png) for [Windows](http://www.sourceop.com/modules.php?name=Downloads&d_op=viewdownload&cid=1) or [Android](https://play.google.com/store/apps/details?id=com.sourceop.remoteadminclient)
  - Allows admins to hear voice chat and spectate players on a 2d overview without having to join the game
- Basic admin commands for managing the server and players
  - Some [implemented in Lua](sourceop/tree/master/trunk/addons/SourceOP/lua/autorun/commands/admin)
- [Synchronized bans](http://www.sourceop.com/modules.php?name=SOPBans) to a SQL server of your choosing
- Reserved slots
- Voting (map, kick)
- A point/credit system
  - Points can optionally be spent by players for things like ammo and health
- Entity manipulation
  - [Remote-controlled cars with sentry guns mounted to the top](https://www.youtube.com/watch?v=PPaxWGcwJw8), for example
- Grappling hook for players
- Jetpack for players
- Quake-like kill announements
- A radio that players can place in the game to play music
- [Snarks!](https://www.youtube.com/watch?v=u4KNCb6BD_g)

# Compiling
All the code within sourceop/trunk should be placed in a subdirectory in the utils directory of the SDK, like utils/SourceOP, for example. For building for use in TF2, use AlliedModders's version of the SDK [here](https://github.com/alliedmodders/hl2sdk/tree/tf2). The included Makefile should be able to build in Linux using GCC 4.8, and the included vcxproj should work on Windows using Visual Studio 2013.

There are some external dependencies that you will need as well. Create directories external/win32/mysql and external/win32/openssl. Inside of external/win32/mysql place the win32 version of [MySQL Connector/C](https://dev.mysql.com/downloads/connector/c/). Inside of external/win32/openssl place OpenSSL 1.0.2 libs and includes. You may use a [precompiled package](http://www.npcglib.org/~stathis/blog/precompiled-openssl).
