/*
    This file is part of SourceOP.

    SourceOP is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SourceOP is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with SourceOP.  If not, see <http://www.gnu.org/licenses/>.
*/

// HACK HACK HACK : BASEENTITY ACCESS
#define GAME_DLL
#include "fixdebug.h"
#ifdef _L4D_PLUGIN
#include "convar_l4d.h"
#endif
#include "cbase.h"
#include "baseentity.h"
#include "player.h"
// HACK HACK HACK : BASEENTITY ACCESS

#include "icvar.h"
#include "vmatrix.h"
#include "util.h"
#include "vphysics_interface.h"
#include "ivoiceserver.h"
#include "tier0/icommandline.h"
#ifdef __linux__
#include "datacache/imdlcache.h"
#endif
#include "toolframework/itoolentity.h"

#include "tier3/tier3.h"

#include <stdio.h>

#include "AdminOP.h"
#include "sourcehooks.h"
#include "adminopplayer.h"
#include "sourceopadmin.h"
#include "recipientfilter.h"
#include "bitbuf.h"
#include "download.h"
#include "isopgamesystem.h"
#include "reservedslots.h"

#include "tier0/memdbgon.h"

//#define PLUGIN_PHYSICS
#define FCVAR_SOP (1<<31)

//---------------------------------------------------------------------------------
// Purpose: server cvars (static required in those not also defined as external in cvars.h
//---------------------------------------------------------------------------------
void DFVersionChangeCallback( IConVar *var, const char *pOldValue, float flOldValue );
void DFDatadirChangeCallback( IConVar *var, const char *pOldValue, float flOldValue );
void DFMeleeOnlyCallback( IConVar *var, const char *pOldValue, float flOldValue );
void DFSleepTimeCallback( IConVar *var, const char *pOldValue, float flOldValue );
void DFRSlotsCallback( IConVar *var, const char *pOldValue, float flOldValue );
void DFSteamIDPrefixNumberCallback( IConVar *var, const char *pOldValue, float flOldValue );
ConVar DF_sopversion("DF_sop_version", SourceOPVersion, FCVAR_NOTIFY | FCVAR_SOP, "The current version of SourceOP running", &DFVersionChangeCallback);

ConVar DF_datadir("DF_datadir", "addons/SourceOP", FCVAR_SOP, "The data directory for SourceOP. This must be changed from the command line.", &DFDatadirChangeCallback);

// I have found that this must be 0 for mani/hlstatsx compatibility, therefore that is the default.
ConVar modify_gamedescription("DF_modify_gamedescription", "0", FCVAR_SOP, "Adds 'SourceOP - ' before the game description. This change will be visible in the server browser.");

ConVar command_prefix("DF_command_prefix", "admin_", FCVAR_SOP, "The prefix for all admin commands in SourceOP");
ConVar feature_credits("DF_feature_credits", "1", FCVAR_SOP, "Enables or disables credits", true, 0, true, 1);
ConVar credits_announce("DF_credits_announce", "1", FCVAR_SOP, "Announces certain purchases and announces when they run out", true, 0, true, 1);
ConVar credits_newcreditmsg("DF_credits_newcreditmsg", "1", FCVAR_SOP, "Tells players when they get new credits", true, 0, true, 1);
ConVar credits_maxpermap("DF_credits_maxpermap", "50", FCVAR_SOP, "The maximum amount of credits that a player is allowed to earn in one map.");
ConVar credits_maxpointsatonce("DF_credits_maxpointsatonce", "65", FCVAR_SOP, "The maximum amount of points/frags a player can get at once and still earn credits for it.");
ConVar credits_cheatprotect_on("DF_credits_cheatprotect_on", "1", FCVAR_SOP, "If on, a protection mechanism will be activated that punishes players if they earn credits unusually fast.", true, 0, true, 1);
ConVar credits_cheatprotect_amount("DF_credits_cheatprotect_amount", "16", FCVAR_SOP, "The amount of credits a player must earn in the set time period to get punished.", true, 2, false, 0);
ConVar credits_cheatprotect_time("DF_credits_cheatprotect_time", "24", FCVAR_SOP, "If a player earns the set amount of credits within this many seconds, they will be punished.", true, 0, false, 0);
ConVar creditgap("DF_creditgap", "5", FCVAR_SOP, "How many points/frags it takes to get a credit");
ConVar feature_showcredits("DF_feature_showcredits", "1", FCVAR_SOP, "Shows how many credits a player has on their screen", true, 0, true, 1);
ConVar feature_showcredits_x("DF_feature_showcredits_x", "1.0", FCVAR_SOP, "Controls the position of the credit counter (X) (ex. 1 = far right, 0 = far left, 0.5 = middle)");
ConVar feature_showcredits_y("DF_feature_showcredits_y", "0.0", FCVAR_SOP, "Controls the position of the credit counter (Y) (ex. 1 = bottom, 0 = top, 0.5 = middle)");

ConVar time_god("DF_time_god", "120", FCVAR_SOP, "Sets the time god lasts when purchased");
ConVar time_noclip("DF_time_noclip", "200", FCVAR_SOP, "Sets the time noclip lasts when purchased");

ConVar price_noclip("DF_price_noclip", "150", FCVAR_SOP, "Sets the price to purchase noclip with credits");
ConVar price_god("DF_price_god", "100", FCVAR_SOP, "Sets the price to purchase god with credits");
ConVar price_gun("DF_price_gun", "5", FCVAR_SOP, "Sets the price to purchase a gun with credits");
ConVar price_allguns("DF_price_allguns", "30", FCVAR_SOP, "Sets the price to purchase all guns with credits");
ConVar price_ammo("DF_price_ammo", "2", FCVAR_SOP, "Sets the price to purchase ammo with credits");
ConVar price_health("DF_price_health", "25", FCVAR_SOP, "Sets the price to purchase health with credits");
ConVar price_slay("DF_price_slay", "75", FCVAR_SOP, "Sets the price to purchase slaying another player with credits");
ConVar price_explosive("DF_price_explosive", "10", FCVAR_SOP, "Sets the price to purchase an explosive can with credits");

ConVar hook_on("DF_hook_on", "1", FCVAR_NOTIFY | FCVAR_SOP, "Enables or disables the grapple hook", true, 0, true, 1);
ConVar hook_speed("DF_hook_speed", "300", FCVAR_SOP, "Sets the speed that the hook pulls");
ConVar hook_pullmode("DF_hook_pullmode", "2", FCVAR_SOP, "Sets the hook pull mode. Mode 1 averages old pull direction with the new direction but can cause problems against walls. Mode 2 strictly pulls in one direction.", true, 1, true, 2);

ConVar jetpack_on("DF_jetpack_on", "1", FCVAR_NOTIFY | FCVAR_SOP, "Enables or disables the jetpack", true, 0, true, 1);
ConVar jetpack_team("DF_jetpack_team", "0", FCVAR_SOP, "If non-zero, will limit jetpack to that team only. This is mainly for maps like z_umizuri_p that force people onto a specific team.", true, 0, true, 5);
ConVar jetpack_show_disabledmsg("DF_jetpack_show_disabledmsg", "1", FCVAR_SOP, "Controls whether or not a message is printed to a player's console if that player tries to use the jetpack but the jetpack is turned off.", true, 0, true, 1);
ConVar jetpack_show_accessdenymsg("DF_jetpack_show_accessdenymsg", "1", FCVAR_SOP, "Controls whether or not a message is printed to a player's console if that player tries to use the jetpack but is denied access.", true, 0, true, 1);
ConVar jetpack_show_chargemsg("DF_jetpack_show_chargemsg", "1", FCVAR_SOP, "Controls whether or not to show messages about the jetpack's charge.", true, 0, true, 1);
ConVar jetpack_infinite_charge("DF_jetpack_infinite_charge", "0", FCVAR_SOP, "Controls whether or not the jetpack has infinite charge.", true, 0, true, 1);

ConVar radio_on("DF_radio_on", "1", FCVAR_NOTIFY | FCVAR_SOP, "Enables or disables the radio", true, 0, true, 1);
ConVar radio_model("DF_radio_model", "models/sourceop/radio.mdl", FCVAR_SOP, "The radio's model relative to the game dir");
ConVar radio_passable("DF_radio_passable", "1", FCVAR_SOP, "When enabled, the radio isn't solid and players can move through the radio", true, 0, true, 1);

ConVar feature_mapvote("DF_feature_mapvote", "1", FCVAR_SOP, "Enables or disables map voting", true, 0, true, 1);
ConVar vote_freq("DF_vote_freq", "180", FCVAR_SOP, "The time (in seconds) that must pass between map votes");
ConVar vote_maxextend("DF_vote_maxextend", "2", FCVAR_SOP, "How many times a map can be extended by vote. Set to 0 to disable extend", true, 0, false, 0);
ConVar vote_extendtime("DF_vote_extendtime", "30", FCVAR_SOP, "How long a map will be extended for (in minutes) if extend wins the map vote", true, 1, false, 0);
ConVar vote_ratio("DF_vote_ratio", "50", FCVAR_SOP, "The percentage of players required to agree on a map for a map vote to pass", true, 0, true, 100);
ConVar vote_menu("DF_vote_menu", "1", FCVAR_SOP, "Enables the vote menu instead of text only voting", true, 0, true, 1);

ConVar jetpackvote_enable("DF_jetpackvote_enable", "1", FCVAR_SOP, "Enables or disables voting for jetpack", true, 0, true, 1);
ConVar jetpackvote_show_disabledmsg("DF_jetpackvote_show_disabledmsg", "1", FCVAR_SOP, "Controls whether or not a message is printed to a player's console if that player tries to use the jetpack vote but the jetpack vote is disabled.", true, 0, true, 1);
ConVar jetpackvote_freq("DF_jetpackvote_freq", "120", FCVAR_SOP, "The time (in seconds) that must pass between jetpack votes");
ConVar jetpackvote_ratio("DF_jetpackvote_ratio", "40", FCVAR_SOP, "The percentage of players required to agree on an option for a jetpack vote to pass", true, 0, true, 100);
ConVar jetpackvote_duration("DF_jetpackvote_duration", "90", FCVAR_SOP, "The time it takes for a jetpack vote to end after being started (in seconds)", true, 0, false, 0);
ConVar jetpackvote_printstandings("DF_jetpackvote_printstandings", "1", FCVAR_SOP, "Prints the choice that is currently winning in chat after somebody votes", true, 0, true, 1);
ConVar jetpackvote_printvoters("DF_jetpackvote_printvoters", "1", FCVAR_SOP, "Shows when a player makes a vote and what he/she voted for in chat", true, 0, true, 1);

ConVar playerchat_motd("DF_playerchat_motd", "1", FCVAR_SOP, "If enabled, this CVAR allows players to say motd to bring up the message of the day window.", true, 0, true, 1);
ConVar playerchat_info("DF_playerchat_info", "1", FCVAR_SOP, "If enabled, this CVAR allows players to query a player's credits and rank using the info chat command.", true, 0, true, 1);
ConVar playerchat_names("DF_playerchat_names", "1", FCVAR_SOP, "If enabled, this CVAR allows players to query a player's previous names using the names chat command.", true, 0, true, 1);
ConVar playerchat_time("DF_playerchat_time", "1", FCVAR_SOP, "If enabled, this CVAR allows players to query a player's time connected using the time chat command.", true, 0, true, 1);

ConVar gag_notify("DF_gag_notify", "1", FCVAR_SOP, "If enabled, players will be notified that they are gagged.", true, 0, true, 1);

ConVar allow_noclipping("DF_allow_noclipping", "0", FCVAR_SOP, "Allows players to ask SourceOP for noclip", true, 0, true, 1);

ConVar entmove_changecolor("DF_entmove_changecolor", "1", FCVAR_SOP, "Controls whether or not entity moving changes the color of the entity while it's being moved.", true, 0, true, 1);

ConVar spawn_removeondisconnect("DF_spawn_removeondisconnect", "1", FCVAR_SOP, "Controls whether or not entities admins have spawned are deleted when the admin disconnects.", true, 0, true, 1);
ConVar spawnlimit_admin("DF_spawnlimit_admin", "300", FCVAR_SOP, "Imposes a limit on how many entities an admin can have spawned at a time. -1 removes the limit, 0 disables spawning.");
ConVar spawnlimit_server("DF_spawnlimit_server", "600", FCVAR_SOP, "Imposes a limit on how many entities can be spawned at a time on the server. -1 removes the limit, 0 disables spawning.");

ConVar feature_killsounds("DF_feature_killsounds", "1", FCVAR_SOP, "Plays sounds on firstblood, doublekill, multikill, etc.", true, 0, true, 1);

ConVar remote_port("DF_remote_port", "2375", FCVAR_SOP, "Controls the port the SourceOP remote listens on.");
ConVar remote_logchattoplayers("DF_remote_logchattoplayers", "0", FCVAR_SOP, "Controls whether or not a log entry is created in remoteadminlog.log when an admin says something to all players.", true, 0, true, 1);
ConVar remote_logchattoadmins("DF_remote_logchattoadmins", "1", FCVAR_SOP, "Controls whether or not a log entry is created in remoteadminlog.log when an admin says something to other admins.", true, 0, true, 1);
ConVar remote_mapupdaterate("DF_remote_mapupdaterate", "0.05", FCVAR_SOP, "Sets the update rate, in seconds, of the map window for the remote administration panel.", true, 0.01, true, 5);

ConVar help_show_loading("DF_help_show_loading", "0", FCVAR_SOP, "Shows a loading message while waiting for the !help or !buy stats page to load.", true, 0, true, 1);
ConVar ignore_chat("DF_ignore_chat", "0", FCVAR_SOP, "Tells SourceOP to ignore anything said in chat i.e. votemap, info, time, whois, etc. Does not include !help, !jetpack, !pc, etc.", true, 0, true, 1);

ConVar highpingkicker_on("DF_highpingkicker_on", "0", FCVAR_SOP, "Enables SourceOP's high ping kicker.", true, 0, true, 1);
ConVar highpingkicker_maxping("DF_highpingkicker_maxping", "400", FCVAR_SOP, "The maximum a player's average ping can be without being kicked. If any player has an average ping higher than this, then that player will be kicked.", true, 5, true, 4000);
ConVar highpingkicker_samples("DF_highpingkicker_samples", "40", FCVAR_SOP, "The number of ping samples that are obtained before they are averaged to make a decision on whether or not to kick the player.", true, 1, true, 900);
ConVar highpingkicker_sampletime("DF_highpingkicker_sampletime", "1.5", FCVAR_SOP, "The time between each ping sample in seconds.", true, 0.2, true, 30);
ConVar highpingkicker_message("DF_highpingkicker_message", "You were kicked because your ping is too high.", FCVAR_SOP, "The message to be displayed in a player's console if the player's ping is too high");

ConVar tf2_fastrespawn("DF_tf2_fastrespawn", "0", FCVAR_SOP, "Enables fast respawn in TF2 games. Setting to 2 makes truly instant respawn.", true, 0, true, 2);
ConVar tf2_meleeonly("DF_tf2_meleeonly", "0", FCVAR_SOP, "Forces players to use their melee weapon. Sentries are disabled.", true, 0, true, 1, &DFMeleeOnlyCallback);
ConVar tf2_roundendalltalk("DF_tf2_roundendalltalk", "0", FCVAR_SOP, "Turns on all talk after a round ends and turns it back off when the next round starts.", true, 0, true, 1);
ConVar tf2_customitems("DF_tf2_customitems", "1", FCVAR_SOP, "Enables overriding of real items with custom items loaded from database.", true, 0, true, 2);
ConVar tf2_arenateamsize("DF_tf2_arenateamsize", "0", FCVAR_SOP, "Sets the maximum team size in arena mode. Set to zero to disable interfering with TF2's logic.");
ConVar tf2_disable_voicemenu("DF_tf2_disable_voicemenu", "0", FCVAR_SOP, "0 - Nothing, 1 - Disables calls for medic, 2 - Disables all voicemenu commands.");
ConVar tf2_disable_voicemenu_message("DF_tf2_disable_voicemenu_message", "1", FCVAR_SOP, "Enables printing a message to players when they try to use voicemenu but it's disabled.");
ConVar tf2_disable_fish("DF_tf2_disable_fish", "0", FCVAR_SOP, "Enables disabling The Holy Mackerel and will replace the weapon with the standard bat.");
ConVar tf2_disable_witcher("DF_tf2_disable_witcher", "0", FCVAR_SOP, "Enables disabling witcher items.");

ConVar unvacban_enabled("DF_unvacban_enabled", "1", FCVAR_SOP, "If non-zero, allows the server to ignore the VAC status of specific SteamIDs.");

#ifdef OFFICIALSERV_ONLY
ConVar maxplayers_force("DF_maxplayers_force", "0", FCVAR_SOP, "If non-zero, forces the maxplayers to this number.", true, 0, true, 255);
#endif

ConVar lua_gc_stepeveryframe("DF_lua_gc_stepeveryframe", "1", FCVAR_SOP, "If enabled, a garbage-collection step will be performed every frame.", true, 0, true, 1);
ConVar lua_attack_hooks("DF_lua_attack_hooks", "0", FCVAR_SOP, "If non-zero, PrimaryAttack and SecondaryHooks will be available.", true, 0, true, 1);

ConVar serverquery_fakemaster("DF_serverquery_fakemaster", "1", FCVAR_SOP, "Changes the maxplayer portion of the query response to master servers to be no more than 24.", true, 0, true, 1);
ConVar serverquery_a2sinfo_override("DF_serverquery_a2sinfo_override", "0", FCVAR_SOP, "Overrides handling of A2S_INFO queries. Helps prevent query spam.", true, 0, true, 1);
ConVar serverquery_a2sinfo_override_serverversion("DF_serverquery_a2sinfo_override_serverversion", "1.1.1.0", FCVAR_SOP, "The server version when overriding queries.");
ConVar serverquery_block_alla2cprint("DF_serverquery_block_alla2cprint", "1", FCVAR_SOP, "Enables blocking of all A2C_PRINT packets", true, 0, true, 1);
#ifdef OFFICIALSERV_ONLY
ConVar serverquery_visibleplayers("DF_serverquery_visibleplayers", "-1", FCVAR_SOP, "Changes the number of visible connected players.", true, -1, true, 255);
ConVar serverquery_visibleplayers_min("DF_serverquery_visibleplayers_min", "-1", FCVAR_SOP, "The minimum amount of players to show connected.", true, -1, true, 255);
ConVar serverquery_visiblemaxplayers("DF_serverquery_visiblemaxplayers", "-1", FCVAR_SOP, "Changes the visible max players to any value no matter what the real maxplayers is.", true, -1, true, 255);
ConVar serverquery_addplayers("DF_serverquery_addplayers", "0", FCVAR_SOP, "Adds this many players to the real player count and maxplayers. Used for making 31/31 servers appear to be 32/32.", false, 0, false, 0);
ConVar serverquery_fakeplayers("DF_serverquery_fakeplayers", "0", FCVAR_SOP, "If enabled, fake players will be added to the A2S_PLAYER query so that it matches the number of players specified in DF_serverquery_visibleplayers or DF_serverquery_visibleplayers_min CVAR.", true, 0, true, 1);
ConVar serverquery_fakeaddplayersonly("DF_serverquery_fakeaddplayersonly", "0", FCVAR_SOP, "If enabled, fake players only applies to DF_serverquery_addplayers.", true, 0, true, 1);
ConVar serverquery_showconnecting("DF_serverquery_showconnecting", "1", FCVAR_SOP, "If enabled, players who are connecting will show on A2S_PLAYER queries.", true, 0, true, 1);
ConVar serverquery_hidereplay("DF_serverquery_hidereplay", "1", FCVAR_SOP, "If enabled and replay is on, one bot will be reported to account for the replay bot.", true, 0, true, 1);
ConVar serverquery_replacefirsthostnamechar("DF_serverquery_replacefirsthostnamechar", "0", FCVAR_SOP, "Replaces the first char of the hostname with 0x01.", true, 0, true, 1);
ConVar serverquery_tagsoverride("DF_serverquery_tagsoverride", "0", FCVAR_SOP, "If non-zero, this CVAR's value will be used instead of sv_tags's value.");
ConVar serverquery_debugprints("DF_serverquery_debugprints", "0", FCVAR_SOP, "Prints debug messages about queries. A value of 2 toggles verbose mode.", true, 0, true, 2);
ConVar serverquery_log("DF_serverquery_log", "0", FCVAR_SOP, "Logs server queries to DF_querylog.txt.", true, 0, true, 1);
ConVar serverquery_faketohost_host("DF_serverquery_faketohost_host", "0"); // expects ip in format 000.000.000.000 (3 digits in each octet). 0 means disable
ConVar serverquery_faketohost_passworded("DF_serverquery_faketohost_passworded", "0");
ConVar serverquery_faketohost_maxplayers("DF_serverquery_faketohost_maxplayers", "0");
ConVar serverquery_maxconnectionless("serverquery_maxconnectionless", "15", FCVAR_SOP, "Maximum number of connectionless packets per second.");

ConVar servermoved("DF_servermoved", "0", FCVAR_SOP, "Set to an ip of a new server so the server will display server moved messages.");
#endif

ConVar rslots_enabled("DF_rslots_enabled", "0", FCVAR_SOP, "Enables reserved slots.", true, 0, true, 1);
ConVar rslots_slots("DF_rslots_slots", "4", FCVAR_SOP, "Number of reserved slots.", true, 0, true, 255, &DFRSlotsCallback);
ConVar rslots_block_after_visiblemaxplayers("DF_rslots_block_after_visiblemaxplayers", "1", FCVAR_SOP, "If enabled, players cannot join past the visiblemaxplayers unless they have a reserved slot.", true, 0, true, 1);
ConVar mysql_database_addr("DF_mysql_database_addr", "localhost", FCVAR_SOP, "IP Address of the MySQL database. Used for reserved slot, player database, bans, and special items. See also: DF_bans_mysql");
ConVar mysql_database_user("DF_mysql_database_user", "", FCVAR_SOP, "Username for the MySQL database.");
ConVar mysql_database_pass("DF_mysql_database_pass", "", FCVAR_SOP, "Password for the MySQL database.");
ConVar mysql_database_name("DF_mysql_database_name", "", FCVAR_SOP, "Name of the database to connect to.");
ConVar rslots_minimum_donation("rslots_minimum_donation", "5", FCVAR_SOP, "The minimum value in whole dollars donators must have donated to get a slot.", true, 0, false, 0);
ConVar rslots_database2_addr("DF_rslots_database2_addr", "localhost", FCVAR_SOP, "IP Address of the reserved slot system's second MySQL database.");
ConVar rslots_database2_user("DF_rslots_database2_user", "", FCVAR_SOP, "Username for the second MySQL database.");
ConVar rslots_database2_pass("DF_rslots_database2_pass", "", FCVAR_SOP, "Password for the second MySQL database.");
ConVar rslots_database2_name("DF_rslots_database2_name", "", FCVAR_SOP, "Name of the second database to connect to.");
ConVar rslots_databases("DF_rslots_databases", "1", FCVAR_SOP, "1 - Use first database for reserved slots, 2 - Use second database for reserved slots, 3 - Use both.");
ConVar rslots_refresh("DF_rslots_refresh", "0", FCVAR_SOP, "If non-zero, reserved slots will be refreshed every interval specified. The value is in seconds.", true, 0, false, 0);

ConVar phys_gunmass("DF_phys_gunmass", "200", FCVAR_SOP, "Specifies the maximum mass of objects the physgun can pickup. The physgun will have a harder time moving objects with masses greater than this.");
ConVar phys_gunvel("DF_phys_gunvel", "400", FCVAR_SOP, "Specifies the maximum velocity at which the physgun moves object.");
ConVar phys_gunforce("DF_phys_gunforce", "5e5", FCVAR_SOP, "Specifies the force that is applied to objects when being moved by the physgun.");
ConVar phys_guntorque("DF_phys_guntorque", "100", FCVAR_SOP, "Currently unused.");

ConVar grenades("DF_grenades", "0", FCVAR_SOP, "Enables grenades (+gren command).");
ConVar grenades_perlife("DF_grenades_perlife", "4", FCVAR_SOP, "How many grenades a player spawns with.");

ConVar bans_bancfg("DF_bans_bancfg", "1", FCVAR_SOP, "If enabled, bans will be written to the banned_user.cfg and banned_ip.cfg files.");
ConVar bans_mysql("DF_bans_mysql", "0", FCVAR_SOP, "If enabled, bans will be loaded from and written to the MySQL database.");
ConVar bans_require_reason("DF_bans_require_reason", "0", FCVAR_SOP, "If enabled, a reason is required to ban a player.");

ConVar sql_playerdatabase("DF_sql_playerdatabase", "1", FCVAR_SOP, "Enables the SQL player database.");

ConVar sk_snark_health("DF_sk_snark_health", "2", FCVAR_SOP, "How much health snarks have.", true, 0, false, 0);
ConVar sk_snark_dmg_bite("DF_sk_snark_dmg_bite", "10", FCVAR_SOP, "How much damage snarks do when they bite.", true, 0, false, 0);
ConVar sk_snark_dmg_pop("DF_sk_snark_dmg_pop", "5", FCVAR_SOP, "How much base damage snarks do when they die (pop). Snarks will do more damage when they pop for each player they bite.", true, 0, false, 0);

ConVar damage_multiplier("DF_damage_multiplier", "1", FCVAR_SOP, "Multiplier for all damage.", true, 0, false, 0);

ConVar r_visualizetraces("DF_false", "0", FCVAR_SOP, "Always zero.", true, 0, true, 0);

ConVar nostats("DF_nostats", "0", FCVAR_SOP, "Blocks stats uploading to Valve servers.", true, 0, true, 1);
ConVar blockfriendlyheavy("DF_blockfriendlyheavy", "0", FCVAR_SOP, "Blocks spies from disguising as friendly heavy.", true, 0, true, 1);

ConVar extra_heartbeat("DF_extra_heartbeat", "0", FCVAR_SOP, "If non-zero, an extra heartbeat will be executed every interval specified. The value is in seconds.", true, 0, false, 0);

int g_steamIDPrefix = 0;
ConVar steamid_stringprefix("DF_steamid_stringprefix", "STEAM_0", FCVAR_SOP);
ConVar steamid_prefixnumber("DF_steamid_prefixnumber", "0", FCVAR_SOP, "STEAM_#", false, 0, false, 0, &DFSteamIDPrefixNumberCallback);
ConVar steamid_customnetworkidvalidated("DF_steamid_customnetworkidvalidated", "1", FCVAR_SOP, "Call SourceOP NetworkIDValidated code from within SourceOP instead of by the engine.");

#ifdef OFFICIALSERV_ONLY
ConVar disablethink("DF_disablethink", "0", FCVAR_SOP, "Disables entities from thinking.");
ConVar disabletouch("DF_disabletouch", "0", FCVAR_SOP, "Disables entity touching.");

ConVar debug_log("DF_debug_log", "0", FCVAR_SOP, "Debug logging.");
#endif

#ifdef _WIN32
ConVar sleep_time("DF_sleep_time", "1", FCVAR_SOP, "Changes the Sleep time of srcds's main loop. Use -1 to disable Sleep call.", true, -1, true, 127, &DFSleepTimeCallback);
#else
ConVar sleep_time("DF_sleep_time", "1000", FCVAR_SOP, "Changes the usleep time of srcds's main loop. Use -1 to disable usleep call.", true, -1, true, 1000000);
#endif

ConVar *hostname = NULL;
ConVar *sv_cheats = NULL;
ConVar *nextlevel = NULL;
ConVar *mapcyclefile = NULL;
ConVar *sv_gravity = NULL;
ConVar *mp_teamplay = NULL;
ConVar *phys_pushscale_game = NULL;
ConVar *sv_logecho = NULL;
ConVar *mp_respawnwavetime = NULL;
ConVar *timelimit = NULL;
ConVar *mp_fraglimit = NULL;
ConVar *mp_winlimit = NULL;
ConVar *mp_maxrounds = NULL;
ConVar *mp_friendlyfire = NULL;
ConVar *sv_alltalk = NULL;
ConVar *srv_ip = NULL;
ConVar *srv_hostport = NULL;
ConVar *tv_enable = NULL;
ConVar *tv_name = NULL;
ConVar *sv_tags = NULL;
ConVar *replay_enable = NULL;

ConCommand *maxplayers = NULL;

#include "cvars.h"

static CreateInterfaceFn g_interfaceFactory;
static CreateInterfaceFn g_gameServerFactory;

// Interfaces from the engine
IVEngineServer          *engine = NULL; // helper functions (messaging clients, loading content, making entities, running commands, etc)
IVoiceServer            *g_pVoiceServer = NULL;
IFileSystem             *filesystem = NULL; // file I/O 
IGameEventManager       *gameeventmanager_old = NULL; // game events interface
IGameEventManager2      *gameeventmanager = NULL; // game events interface
IDataCache              *datacache = NULL;
IPlayerInfoManager      *playerinfomanager = NULL; // game dll interface to interact with players
IServerPluginHelpers    *helpers = NULL; // special 3rd party plugin helpers from the engine
IVModelInfo             *modelinfo = NULL;
INetworkStringTableContainer *networkstringtable = NULL;
IServerGameDLL          *servergame = NULL;
IServerGameEnts         *servergameents = NULL;
IServerGameClients      *servergameclients = NULL;
IEffects                *effects = NULL;
IEngineSound            *enginesound = NULL;
IEngineTrace            *enginetrace = NULL;
IStaticPropMgrServer    *staticpropmgr = NULL;
IUniformRandomStream    *random = NULL;
ICommandLine            *commandline = NULL;
IServerTools            *servertools = NULL;
#ifdef __linux__
// no tier2/3 libs
IMDLCache               *mdlcache = NULL;
#else
ICvar *cvar = NULL;
ICvar *g_pCVar = NULL;
#endif

CSharedEdictChangeInfo *g_pSharedChangeInfo = NULL;

char g_szA2SInfoCache[1024];
int g_iA2SInfoCacheSize = -1;
#ifdef OFFICIALSERV_ONLY
unsigned int g_iConnectionlessThisFrame = 0;
unsigned int g_iConsecutiveConnectionlessOverLimit = 0;
#endif
bool g_bShouldWriteOverLimitLog = 1;

SourceHook::Impl::CSourceHookImpl g_SourceHook;
SourceHook::ISourceHook *g_SHPtr;
SourceHook::Plugin g_PLID = 1;

bool g_bIsVsp = false;
IMetamodOverrides *g_metamodOverrides = NULL;
IConCommandBaseAccessor *pConCommandAccessor = NULL;

SourceHook::CallClass<IServerGameDLL> *servergame_cc = NULL;
//SourceHook::CallClass<IServerGameClients> *servergameclients_cc;
SourceHook::CallClass<IVEngineServer> *engine_cc = NULL;

SH_DECL_HOOK0(IServerGameDLL, GetGameDescription, SH_NOATTRIB, 0, char const *);
//SH_DECL_HOOK1_void(IServerGameClients, SetCommandClient, SH_NOATTRIB, 0, int);
SH_DECL_HOOK1(IVEngineServer, CreateEdict, SH_NOATTRIB, 0, edict_t *, int);
SH_DECL_HOOK1_void(IVEngineServer, RemoveEdict, SH_NOATTRIB, 0, edict_t *);
SH_DECL_HOOK1_void(IVEngineServer, FreeEntPrivateData, SH_NOATTRIB, 0, void *);
SH_DECL_HOOK1_void(IVEngineServer, LogPrint, SH_NOATTRIB, 0, const char *);
SH_DECL_HOOK2_void(IVEngineServer, ChangeLevel, SH_NOATTRIB, 0, const char *, const char *);
SH_DECL_HOOK1_void(IServerGameEnts, FreeContainingEntity, SH_NOATTRIB, 0, edict_t *);
SH_DECL_HOOK2_void(IServerGameEnts, MarkEntitiesAsTouching, SH_NOATTRIB, 0, edict_t *, edict_t *);
SH_DECL_HOOK3_void(IServerGameClients, GetPlayerLimits, const, 0, int &, int &, int &);
SH_DECL_HOOK3(IVoiceServer, SetClientListening, SH_NOATTRIB, 0, bool, int, int, bool);
SH_DECL_HOOK2(ICommandLine, ParmValue, const, 0, int, const char *, int);
SH_DECL_HOOK1_void(ConCommand, Dispatch, SH_NOATTRIB, 0, const CCommand &);
SH_DECL_HOOK2(IGameEventManager2, FireEvent, SH_NOATTRIB, 0, bool, IGameEvent*, bool);

SH_DECL_MANUALHOOK4(CGameStatsUploader_UploadGameStats, 0, 0, 0, unsigned int, char const *, unsigned int, unsigned int, void const *);
void *gamestats = NULL;

CUtlVector<int> g_serverHookIDS;

PlantBombFunc PlantBomb = NULL;
_CreateEntityByNameFunc _CreateEntityByName = NULL;
_CreateCombineBallFunc _CreateCombineBall = NULL;
_ClearMultiDamageFunc _ClearMultiDamage = NULL;
_ApplyMultiDamageFunc _ApplyMultiDamage = NULL;
_RadiusDamageFunc _RadiusDamage = NULL;
_SetMoveTypeFunc _SetMoveType = NULL;
_ResetSequenceFunc _ResetSequence = NULL;
_UtilPlayerByIndexFunc _UtilPlayerByIndex = NULL;
_ReceiveDatagramFunc _NET_ReceiveDatagram = NULL;
int (*VCR_Hook_recvfrom)(int s, char *buf, int len, int flags, struct sockaddr *from, int *fromlen) = NULL;
_SendPacketFunc _NET_SendPacket = NULL;
_SV_BVDFunc _SV_BroadcastVoiceData = NULL;
#ifdef __linux__
_usleepfunc _dedicated_usleep = NULL;
#endif
PFNSteam_BGetCallback _BGetCallback = NULL;
PFNSteam_FreeLastCallback _FreeLastCallback = NULL;
PFNSteam_GameServer_InitSafe _SteamGameServer_InitSafe = NULL;

//CSysModule *g_OurDLL = NULL;
#ifdef PLUGIN_PHYSICS
CSysModule *g_PhysicsDLL = NULL;
CreateInterfaceFn physicsFactory;
IPhysics *physics = NULL; // physics functions
IPhysicsSurfaceProps *physprops = NULL;
IPhysicsCollision *physcollision = NULL;
IPhysicsEnvironment *physenv = NULL;
IVPhysicsDebugOverlay *physdebugoverlay = NULL;
IPhysicsObjectPairHash *g_EntityCollisionHash = NULL;
#endif

int g_IgnoreColorMessages = 0;

static void WriteDWord(FileHandle_t fp, unsigned long val) 
{
    filesystem->Write(&val, 4, fp);
}

static void WriteWord(FileHandle_t fp, unsigned short val) 
{
    filesystem->Write(&val, 2, fp);
}

bool WriteWaveFile(
    const char *pFilename, 
    const char *pData, 
    int nBytes, 
    int wBitsPerSample, 
    int nChannels, 
    int nSamplesPerSec)
{
    FileHandle_t fp = filesystem->Open(pFilename, "wb");
    if(!fp)
        return false;

    // Write the RIFF chunk.
    filesystem->Write("RIFF", 4, fp);
    WriteDWord(fp, 0);
    filesystem->Write("WAVE", 4, fp);
    

    // Write the FORMAT chunk.
    filesystem->Write("fmt ", 4, fp);
    
    WriteDWord(fp, 0x10);
    WriteWord(fp, 1);	// WAVE_FORMAT_PCM
    WriteWord(fp, (unsigned short)nChannels);	
    WriteDWord(fp, (unsigned long)nSamplesPerSec);
    WriteDWord(fp, (unsigned long)((wBitsPerSample / 8) * nChannels * nSamplesPerSec));
    WriteWord(fp, (unsigned short)((wBitsPerSample / 8) * nChannels));
    WriteWord(fp, (unsigned long)wBitsPerSample);

    // Write the DATA chunk.
    filesystem->Write("data", 4, fp);
    WriteDWord(fp, (unsigned long)nBytes);
    filesystem->Write(pData, nBytes, fp);


    // Go back and write the length of the riff file.
    unsigned long dwVal = filesystem->Tell(fp) - 8;
    filesystem->Seek(fp, 4, FILESYSTEM_SEEK_HEAD);
    WriteDWord(fp, dwVal);

    filesystem->Close(fp);
    return true;
}


#ifdef _DEBUG
void DumpClientFunction( void *userPortion, size_t blockSize )
{
/*
{39562} normal block at 0x1654F4C8, 8 bytes long.
 Data: <  T   M > 10 F5 54 16 08 F0 4D 16 
*/
    printf("Data: %i\n", blockSize);
}
#endif

CAdminOP pAdminOP;
CGlobalVars *gpGlobals = NULL;

bool g_ignoreShutdown = 1;
bool g_bUseNetworkVars = true;

// useful helper func
/*inline bool FStrEq(const char *sz1, const char *sz2)
{
    return(Q_stricmp(sz1, sz2) == 0);
}*/

class CVspMetamodOverrides : public IMetamodOverrides
{
    virtual void UnregisterConCommand(ConCommandBase *pConCmd)
    {
        cvar->UnregisterConCommand(pConCmd);
    }
};

static CVspMetamodOverrides s_vspMetamodOverrides;

//---------------------------------------------------------------------------------
// Purpose: a 3rd party plugin class
//---------------------------------------------------------------------------------


// 
// The plugin is a static singleton that is exported as an interface
//
CEmptyServerPlugin g_ServerPlugin;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CEmptyServerPlugin, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, g_ServerPlugin );

//---------------------------------------------------------------------------------
// Purpose: constructor/destructor
//---------------------------------------------------------------------------------
CEmptyServerPlugin::CEmptyServerPlugin()
{
    m_iClientCommandIndex = 0;
    pAdminOP.m_iClientCommandIndex = 0;
}

CEmptyServerPlugin::~CEmptyServerPlugin()
{
}

unsigned int HookUploadGameStats(const char *a, unsigned int b, unsigned int c, const void *d)
{
    if(nostats.GetInt())
    {
        Msg("[SOURCEOP] Blocking stats upload.\n");
        RETURN_META_VALUE(MRES_SUPERCEDE, 1);
    }
    RETURN_META_VALUE(MRES_IGNORED, 1);
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is loaded, load the interface we need from the engine
//---------------------------------------------------------------------------------
bool CEmptyServerPlugin::Load( CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory )
{
    g_bIsVsp = true;

    CAdminOP::ColorMsg(CONCOLOR_CYAN, "[SOURCEOP] SourceOP loading...\n");

    if(interfaceFactory == NULL || gameServerFactory == NULL)
    {
        CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Failed to load because at least one factory was NULL.\n");
        return false;
    }

    if(!VspLoad(interfaceFactory, gameServerFactory))
    {
        return false;
    }

    return MainLoad(interfaceFactory, gameServerFactory);
}

bool CEmptyServerPlugin::MainLoad( CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory )
{
    if(!g_bIsVsp)
    {
        // Tier1 won't have been connected yet. Go connect it.
        ConnectTier1Libraries( &interfaceFactory, 1 );
    }

#if defined(_DEBUG) && defined(_WIN32)
    int tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    tmpDbgFlag |= _CRTDBG_DELAY_FREE_MEM_DF;
    tmpDbgFlag |= _CRTDBG_LEAK_CHECK_DF;
    _CrtSetDbgFlag(tmpDbgFlag);
    _CrtSetDumpClient(DumpClientFunction);
#endif

    g_interfaceFactory = interfaceFactory;
    g_gameServerFactory = gameServerFactory;

    g_serverHookIDS.Purge();

    playerinfomanager = (IPlayerInfoManager *)gameServerFactory(INTERFACEVERSION_PLAYERINFOMANAGER,NULL);
    if ( !playerinfomanager )
    {
        CAdminOP::ColorMsg(CONCOLOR_LIGHTRED,"[SOURCEOP] Failed to retrieve playerinfomanager.\n");
        return false;
        //Warning( "Unable to load playerinfomanager, ignoring\n" ); // this isn't fatal, we just won't be able to access specific player data
    }

    // get the interfaces we want to use
    if( !(engine = (IVEngineServer*)interfaceFactory(INTERFACEVERSION_VENGINESERVER, NULL)) )
    {
        CAdminOP::ColorMsg(CONCOLOR_LIGHTCYAN, "[SOURCEOP] Warning: Attempting to use engine server version 22.\n");
        if( !(engine = (IVEngineServer*)interfaceFactory("VEngineServer022", NULL)))
        {
            CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Failed getting engine interface.\n");
            return false;
        }
    }
    if( !(g_pVoiceServer = (IVoiceServer*)interfaceFactory(INTERFACEVERSION_VOICESERVER, NULL)) )
    {
        CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Failed getting voice interface.\n");
        return false;
    }
    if( !(staticpropmgr = (IStaticPropMgrServer *)interfaceFactory(INTERFACEVERSION_STATICPROPMGR_SERVER,NULL)) ||
        !(networkstringtable = (INetworkStringTableContainer *)interfaceFactory(INTERFACENAME_NETWORKSTRINGTABLESERVER,NULL)) ||
        !(random = (IUniformRandomStream *)interfaceFactory(VENGINE_SERVER_RANDOM_INTERFACE_VERSION, NULL)) ||
        !(enginesound = (IEngineSound*)interfaceFactory(IENGINESOUND_SERVER_INTERFACE_VERSION, NULL)) ||
        !(enginetrace = (IEngineTrace *)interfaceFactory(INTERFACEVERSION_ENGINETRACE_SERVER,NULL)) )
    {
        CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Failed getting first set of interfaces.\n");
        return false;
    }
    if( !(modelinfo = (IVModelInfoClient *)interfaceFactory(VMODELINFO_SERVER_INTERFACE_VERSION, NULL )) )
    {
        if( !(modelinfo = (IVModelInfoClient *)interfaceFactory("VModelInfoServer003", NULL )) )
        {
            if( !(modelinfo = (IVModelInfoClient *)interfaceFactory("VModelInfoServer002", NULL )) )
            {
                CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Failed getting modelinfo interface.\n");
                return false;
            }
        }
    }
    if( !(datacache = (IDataCache*)interfaceFactory(DATACACHE_INTERFACE_VERSION, NULL )) )
    {
        CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Failed getting datacache interface.\n");
        return false;
    }
    if( !(filesystem = (IFileSystem*)interfaceFactory(FILESYSTEM_INTERFACE_VERSION, NULL)) )
    {
        CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Failed getting filesystem interface.\n");
        return false;
    }

    if( !(gameeventmanager = (IGameEventManager2 *)interfaceFactory(INTERFACEVERSION_GAMEEVENTSMANAGER2,NULL)) ||
        !(helpers = (IServerPluginHelpers*)interfaceFactory(INTERFACEVERSION_ISERVERPLUGINHELPERS, NULL))
        )
    {
        CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Failed getting second set of interfaces.\n");
        return false;
    }
    if( !(gameeventmanager_old = (IGameEventManager *)interfaceFactory(INTERFACEVERSION_GAMEEVENTSMANAGER,NULL)))
    {
        CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Old game event manager interface is not available.\n           SourceOP will not be able to send \"GameEvent\" to Lua scripts.");
    }
    if( !(servergame = (IServerGameDLL*)gameServerFactory("ServerGameDLL007", NULL)))
    {
        if( !(servergame = (IServerGameDLL*)gameServerFactory("ServerGameDLL008", NULL)))
        {
             CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Failed getting servergame interface.\n");
             return false;
        }
    }
    if( !(servergameents = (IServerGameEnts*)gameServerFactory(INTERFACEVERSION_SERVERGAMEENTS, NULL)) )
    {
        CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Failed getting servergameents interface.\n");
        return false;
    }
    if( !(servergameclients = (IServerGameClients*)gameServerFactory("ServerGameClients004", NULL)) )
    {
        if( !(servergameclients = (IServerGameClients*)gameServerFactory(INTERFACEVERSION_SERVERGAMECLIENTS, NULL)) )
        {
             CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Failed getting servergameclients interface.\n");
             return false;
        }
    }
    if( !(effects = (IEffects*)gameServerFactory(IEFFECTS_INTERFACE_VERSION, NULL))
#ifdef __linux__
        || !(mdlcache = (IMDLCache*)interfaceFactory(MDLCACHE_INTERFACE_VERSION, NULL))
#endif
        )
    {
        CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Failed getting third set of interfaces.\n");
        return false; // we require all these interface to function
    }
    if(!(servertools = (IServerTools*)gameServerFactory(VSERVERTOOLS_INTERFACE_VERSION, NULL)))
    {
        Warning("[SOURCEOP] Failed to get servertools interface.\n");
    }

    gamestats = interfaceFactory("ServerUploadGameStats001", NULL);

    if(!(commandline = CommandLine_Tier0()))
    {
        CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Failed getting commandline interface.\n");
        return false;
    }

    MathLib_Init( 2.2f, 2.2f, 0.0f, 2.0f );

    gpGlobals = playerinfomanager->GetGlobalVars();

    pAdminOP.LoadFeatures();

    int r = 0;
    r = SH_ADD_HOOK_MEMFUNC(IServerGameDLL, GetGameDescription, servergame, &pAdminOP, &CAdminOP::GetGameDescription, false);
    if(r) g_serverHookIDS.AddToTail(r);
    //r = SH_ADD_HOOK_MEMFUNC(IServerGameDLL, LevelInit, servergame, &pAdminOP, &CAdminOP::ServerGameDLLLevelInit, true);
    //r = SH_ADD_HOOK_MEMFUNC(IServerGameClients, SetCommandClient, servergameclients, &pAdminOP, &CAdminOP::SetCommandClient, false);
    // needed for FakeClientCommand
    //r = SH_ADD_HOOK_MEMFUNC(IVEngineServer, CreateEdict, engine, &pAdminOP, &CAdminOP::CreateEdictPre, false);
    //if(r) g_serverHookIDS.AddToTail(r);
    //r = SH_ADD_HOOK_MEMFUNC(IVEngineServer, CreateEdict, engine, &pAdminOP, &CAdminOP::CreateEdict, true);
    //if(r) g_serverHookIDS.AddToTail(r);
    r = SH_ADD_HOOK_MEMFUNC(IVEngineServer, RemoveEdict, engine, &pAdminOP, &CAdminOP::RemoveEdictPre, false);
    if(r) g_serverHookIDS.AddToTail(r);
    r = SH_ADD_HOOK_MEMFUNC(IVEngineServer, FreeEntPrivateData, engine, &pAdminOP, &CAdminOP::FreeEntPrivateData, false);
    if(r) g_serverHookIDS.AddToTail(r);
    r = SH_ADD_HOOK_MEMFUNC(IVEngineServer, LogPrint, engine, &pAdminOP, &CAdminOP::LogPrint, false);
    if(r) g_serverHookIDS.AddToTail(r);
    r = SH_ADD_HOOK_MEMFUNC(IVEngineServer, ChangeLevel, engine, &pAdminOP, &CAdminOP::ChangeLevel, false);
    if(r) g_serverHookIDS.AddToTail(r);
    r = SH_ADD_HOOK_MEMFUNC(IServerGameEnts, FreeContainingEntity, servergameents, &pAdminOP, &CAdminOP::FreeContainingEntity, false);
    if(r) g_serverHookIDS.AddToTail(r);
    r = SH_ADD_HOOK_MEMFUNC(IServerGameEnts, MarkEntitiesAsTouching, servergameents, &pAdminOP, &CAdminOP::MarkEntitiesAsTouching, false);
    if(r) g_serverHookIDS.AddToTail(r);
    r = SH_ADD_HOOK_MEMFUNC(IServerGameClients, GetPlayerLimits, servergameclients, &pAdminOP, &CAdminOP::GetPlayerLimits, true);
    if(r) g_serverHookIDS.AddToTail(r);
    r = SH_ADD_VPHOOK(IVoiceServer, SetClientListening, g_pVoiceServer, SH_MEMBER(&pAdminOP, &CAdminOP::SetClientListening), false);
    if(r) g_serverHookIDS.AddToTail(r);
    r = SH_ADD_HOOK_MEMFUNC(ICommandLine, ParmValue, commandline, &pAdminOP, &CAdminOP::ParmValue, false);
    if(r) g_serverHookIDS.AddToTail(r);

    if(gamestats)
    {
        r = SH_ADD_MANUALHOOK(CGameStatsUploader_UploadGameStats, gamestats, SH_STATIC(HookUploadGameStats), false);
        if(r) g_serverHookIDS.AddToTail(r);
    }

    maxplayers = cvar->FindCommand("maxplayers");
#ifndef _L4D_PLUGIN
#ifdef OFFICIALSERV_ONLY
    /*if(maxplayers)
    {
        // FIXME: This causes crash if plugin unloaded. I don't know why.
        r = SH_ADD_HOOK_MEMFUNC(ConCommand, Dispatch, maxplayers, &pAdminOP, &CAdminOP::MaxPlayersDispatch, false);
        if(r) g_serverHookIDS.AddToTail(r);
    }*/
#endif
#endif

    servergame_cc = SH_GET_CALLCLASS(servergame);
    //servergameclients_cc = SH_GET_CALLCLASS(servergameclients);
    engine_cc = SH_GET_CALLCLASS(engine);

    //g_OurDLL = filesystem->LoadModule( "sourceop" );
    // Load the dependent components
#ifdef PLUGIN_PHYSICS
    g_PhysicsDLL = filesystem->LoadModule( "vphysics" , NULL, false );
    physicsFactory = Sys_GetFactory( g_PhysicsDLL );
    if ( !physicsFactory )
    {
        CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Failed to load vphysics factory.\n");
        //Error( "Sys_LoadPhysicsDLL:  Failed to load vphysics\n" );
        return false;
    }

    if ((physics = (IPhysics *)physicsFactory( VPHYSICS_INTERFACE_VERSION, NULL )) == NULL ||
        (physprops = (IPhysicsSurfaceProps *)physicsFactory( VPHYSICS_SURFACEPROPS_INTERFACE_VERSION, NULL )) == NULL ||
        (physcollision = (IPhysicsCollision *)physicsFactory( VPHYSICS_COLLISION_INTERFACE_VERSION, NULL )) == NULL )
    {
        CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Failed to load vphysics interfaces.\n");
        //Error( "SOURCEOP: Physics failed to load, unloading plugin...\n" );
        return false;
    }
#endif

    // pConCommandAccessor will be NULL for VSP load (and thus use default accessor) and be the metamod accessor for metamod load
    ConVar_Register(0, pConCommandAccessor); // register any cvars we have defined

    hostname = cvar->FindVar("hostname");
    sv_cheats = cvar->FindVar("sv_cheats");
    sv_gravity = cvar->FindVar("sv_gravity");
    mp_teamplay = cvar->FindVar("mp_teamplay");
    phys_pushscale_game = cvar->FindVar("phys_pushscale");
    sv_logecho = cvar->FindVar("sv_logecho");
    mp_respawnwavetime = cvar->FindVar("mp_respawnwavetime");
    timelimit = cvar->FindVar("mp_timelimit");
    mp_fraglimit = cvar->FindVar("mp_fraglimit");
    mp_winlimit = cvar->FindVar("mp_winlimit");
    mp_maxrounds = cvar->FindVar("mp_maxrounds");
    mp_friendlyfire = cvar->FindVar("mp_friendlyfire");
    sv_alltalk = cvar->FindVar("sv_alltalk");
    srv_ip = cvar->FindVar("ip");
    srv_hostport = cvar->FindVar("hostport");
    tv_enable = cvar->FindVar("tv_enable");
    tv_name = cvar->FindVar("tv_name");
    sv_tags = cvar->FindVar("sv_tags");
    replay_enable = cvar->FindVar("replay_enable");

    // temporary fix for new TF2 server exploit
    ConCommand *removeme = cvar->FindCommand("sv_benchmark_force_start");
    if(removeme)
    {
        cvar->UnregisterConCommand(removeme);
    }
    removeme = cvar->FindCommand("sv_soundemitter_filecheck");
    if(removeme)
    {
        cvar->UnregisterConCommand(removeme);
    }

    pAdminOP.InitCommandLineCVars();

#ifndef _L4D_PLUGIN
    g_pSharedChangeInfo = engine->GetSharedEdictChangeInfo();
#endif

    pAdminOP.Load();
    // if the plugin is loaded mid-game, do some init stuff
    if(gpGlobals->curtime >= 0.01)
    {
        //HACKHACKHACK this hack is so that server activate is called with proper params if plugin loaded mid-game

        CBaseEntity *pTest = CreateEntityByName("info_target");
        edict_t *pListBase = NULL;
        if(pTest)
        {
#ifndef _L4D_PLUGIN
            int entindex = engine->IndexOfEdict(servergameents->BaseEntityToEdict(pTest));
            pListBase = servergameents->BaseEntityToEdict(pTest);
            pListBase -= entindex;
#else
            // no way to find entity base in l4d since ENTINDEX and INDEXENT are unavailable
            CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Cannot load mid-game on this engine.\n");
            Unload();
            return false;
#endif

            // do some init stuff
            LevelInit(STRING(gpGlobals->mapname));
            // the second parameter should be set correctly, but it is not used by ServerActivate anyways
            ServerActivate(pListBase, 0, gpGlobals->maxClients);
            // set up all currently connected players
            for(int i = 1; i <= gpGlobals->maxClients; i++)
            {
                IPlayerInfo *info = playerinfomanager->GetPlayerInfo(pListBase+i);
                bool connected = info && info->IsConnected();
                // it's possible IsConnected is false but player is connected
                if(connected || (pAdminOP.pAOPPlayers[i-1].baseclient))
                {
                    pAdminOP.pAOPPlayers[i-1].Connect();
                }
                // only activate if IsConnected was true
                if(connected)
                {
                    pAdminOP.pAOPPlayers[i-1].Activate();
                }
            }
            UTIL_Remove(pTest);
        }
        else
        {
            CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] SourceOP tried to load mid-game but for some reason it couldn't.\n");
            Unload();
            return false;
        }
        CAdminOP::ColorMsg(CONCOLOR_CYAN, "[SOURCEOP] SourceOP was loaded in the middle of a game.\n");
        CAdminOP::ColorMsg(CONCOLOR_CYAN, "[SOURCEOP] Some functionality such as +copyent might not work until a new map\n");
        CAdminOP::ColorMsg(CONCOLOR_CYAN, "           is loaded.\n");
    }

    IGameSystem::Add( GameStringSystem() );

    if ( !IGameSystem::InitAllSystems() )
    {
        CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Failed to init all systems.\n");
        Unload();
        return false;
    }
    if ( !ISOPGameSystem::InitAllSystems() )
    {
        CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Failed to init all SourceOP systems.\n");
        Unload();
        return false;
    }

    // TODO: L4D, put back event listeners
#ifndef _L4D_PLUGIN
    gameeventmanager->AddListener( &g_eventListener, "server_spawn", true );
    gameeventmanager->AddListener( &g_eventListener, "player_say", true );
    gameeventmanager->AddListener( &g_eventListener, "player_changename", true );
    gameeventmanager->AddListener( &g_eventListener, "player_death", true );
    gameeventmanager->AddListener( &g_eventListener, "player_spawn", true );
    gameeventmanager->AddListener( &g_eventListener, "player_disconnect", true );
    gameeventmanager->AddListener( &g_eventListener, "server_cvar", true );
    gameeventmanager->AddListener( &g_eventListener, "round_start", true );
    gameeventmanager->AddListener( &g_eventListener, "round_end", true );
    //gameeventmanager->AddListener( &g_eventListener, "player_shoot", true );
    gameeventmanager->AddListener( &g_eventListener, "player_team", true);

    if(pAdminOP.isTF2)
    {
        gameeventmanager->AddListener( &g_eventListener, "player_changeclass", true);
        gameeventmanager->AddListener( &g_eventListener, "teamplay_map_time_remaining", true );
        gameeventmanager->AddListener( &g_eventListener, "teamplay_teambalanced_player", true );
        gameeventmanager->AddListener( &g_eventListener, "teamplay_round_win", true );
        gameeventmanager->AddListener( &g_eventListener, "teamplay_round_start", true );
        gameeventmanager->AddListener( &g_eventListener, "teamplay_flag_event", true );
        gameeventmanager->AddListener( &g_eventListener, "arena_round_start", true );
        gameeventmanager->AddListener( &g_eventListener, "post_inventory_application", true );
    }
#else
    r = SH_ADD_HOOK_MEMFUNC(IGameEventManager2, FireEvent, gameeventmanager, &g_eventListener, &CGameEventListener::FireEventHook, false);
    if(r) g_serverHookIDS.AddToTail(r);
#endif

    IGameSystem::PostInitAllSystems();
    ISOPGameSystem::PostInitAllSystems();

    return true;
}

bool CEmptyServerPlugin::VspLoad( CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory )
{
    ConnectTier1Libraries( &interfaceFactory, 1 );
#ifndef __linux__
    // no tier2/3 libs in linux
    ConnectTier2Libraries( &interfaceFactory, 1 );
    ConnectTier3Libraries( &interfaceFactory, 1 );
#endif

    g_SHPtr = static_cast<SourceHook::ISourceHook *>(&g_SourceHook);

    if(!cvar)
    {
        ////CAdminOP::ColorMsg(CONCOLOR_LIGHTCYAN, "[SOURCEOP] Warning: Manually hooking up cvar interface.\n");
        if( !(cvar = (ICvar*)interfaceFactory(CVAR_INTERFACE_VERSION, NULL)) )
        {
            CAdminOP::ColorMsg(CONCOLOR_LIGHTCYAN, "[SOURCEOP] Warning: Trying cvar interface version 7.\n");
            if( !(cvar = (ICvar*)interfaceFactory("VEngineCvar007", NULL)) )
            {
                CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Failed getting CVAR interface.\n");
                return false;
            }
        }
    }
    if(!g_pCVar)
    {
        g_pCVar = cvar;
        ////CAdminOP::ColorMsg(CONCOLOR_LIGHTCYAN, "[SOURCEOP] Warning: Manually tieing g_pCvar to cvar.\n");
    }

    g_metamodOverrides = &s_vspMetamodOverrides;

    return true;
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unloaded (turned off)
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::Unload( void )
{
    pAdminOP.PreShutdown();
    IGameSystem::ShutdownAllSystems();
    ISOPGameSystem::ShutdownAllSystems();

    CAdminOP::ColorMsg(CONCOLOR_DARKGRAY, "[SOURCEOP] Removing server hooks.");
    for(int i = 0; i < g_serverHookIDS.Count(); i++)
    {
        if(SH_REMOVE_HOOK_ID(g_serverHookIDS.Element(i)))
        {
            CAdminOP::ColorMsg(CONCOLOR_DARKGRAY, ".");
        }
        else
        {
            CAdminOP::ColorMsg(CONCOLOR_DARKGRAY, "F");
        }
    }
    Msg("\n");
    SH_RELEASE_CALLCLASS(servergame_cc);
    //SH_RELEASE_CALLCLASS(servergameclients_cc);
    SH_RELEASE_CALLCLASS(engine_cc);

    // When server quits, disconnect doesn't seem to be called
    // and unload comes after all info stored about player
    // in the engine is lost. So save like this:
    pAdminOP.ShuttingDown = 1;
    for(int i = 0; i < pAdminOP.GetMaxClients(); i++)
    {
        if(pAdminOP.pAOPPlayers[i].GetPlayerState() == 2)
        {
            pAdminOP.pAOPPlayers[i].PluginUnloading();
        }
    }

#ifdef PLUGIN_PHYSICS
    if(g_PhysicsDLL)
    {
        filesystem->UnloadModule(g_PhysicsDLL);
        g_PhysicsDLL = NULL;
    }
#endif

    if(gameeventmanager) gameeventmanager->RemoveListener( &g_eventListener ); // make sure we are unloaded from the event system
    pAdminOP.Unload();

    if(g_bIsVsp)
    {
        if(cvar) ConVar_Unregister();

        g_SourceHook.CompleteShutdown();

#ifndef __linux__
        DisconnectTier3Libraries( );
        DisconnectTier2Libraries( );
#endif

        //filesystem->UnloadModule(g_OurDLL);
        //filesystem->UnloadModule(g_OurDLL);
    }

    DisconnectTier1Libraries( );
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is paused (i.e should stop running but isn't unloaded)
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::Pause( void )
{
}

//---------------------------------------------------------------------------------
// Purpose: called when the plugin is unpaused (i.e should start executing again)
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::UnPause( void )
{
}

//---------------------------------------------------------------------------------
// Purpose: the name of this plugin, returned in "plugin_print" command
//---------------------------------------------------------------------------------
const char *CEmptyServerPlugin::GetPluginDescription( void )
{
    return "SourceOP, www.sourceop.com";
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::LevelInit( char const *pMapName )
{
#ifdef PLUGIN_PHYSICS
    g_EntityCollisionHash = physics->CreateObjectPairHash();
#endif
    g_ignoreShutdown = 0;
    //Msg( "Level \"%s\" has been loaded\n", pMapName );
    pAdminOP.LevelInit(pMapName);
    IGameSystem::LevelInitPreEntityAllSystems(pMapName);
    ISOPGameSystem::LevelInitPreEntityAllSystems(pMapName);
    //gameeventmanager->AddListener( this, true );
}

//---------------------------------------------------------------------------------
// Purpose: called on level start, when the server is ready to accept client connections
//      edictCount is the number of entities in the level, clientMax is the max client count
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::ServerActivate( edict_t *pEdictList, int edictCount, int clientMax )
{
    // There could probably be a better place for this
    // LevelInit seemed to be too early
#ifdef PLUGIN_PHYSICS
    if(!physenv)
    {
        if(physics)
        {
            physenv = physics->GetActiveEnvironmentByIndex(0);
            physdebugoverlay = physenv->GetDebugOverlay();
        }
    }
#endif

    pAdminOP.ServerActivate(pEdictList, edictCount, clientMax);
    IGameSystem::LevelInitPostEntityAllSystems();
    ISOPGameSystem::LevelInitPostEntityAllSystems();
}

//---------------------------------------------------------------------------------
// Purpose: called once per server frame, do recurring work here (like checking for timeouts)
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::GameFrame( bool simulating )
{
    IGameSystem::FrameUpdatePreEntityThinkAllSystems();
    IGameSystem::PreClientUpdateAllSystems(); // this doesn't go here
    ISOPGameSystem::FrameUpdatePreEntityThinkAllSystems();
    ISOPGameSystem::PreClientUpdateAllSystems(); // this doesn't go here

    pAdminOP.GameFrame( simulating );

    IGameSystem::FrameUpdatePostEntityThinkAllSystems();
    ISOPGameSystem::FrameUpdatePostEntityThinkAllSystems();
}

//---------------------------------------------------------------------------------
// Purpose: called on level end (as the server is shutting down or going to a new map)
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::LevelShutdown( void ) // !!!!this can get called multiple times per map change
{
    if(g_ignoreShutdown) return;
#ifdef PLUGIN_PHYSICS
    physenv = NULL; // Needed so that physenv is retrieved next map
#endif
    IGameSystem::LevelShutdownPreEntityAllSystems();
    ISOPGameSystem::LevelShutdownPreEntityAllSystems();
    pAdminOP.LevelShutdown();
    IGameSystem::LevelShutdownPostEntityAllSystems();
    ISOPGameSystem::LevelShutdownPostEntityAllSystems();
    //gameeventmanager->RemoveListener( this );
    g_ignoreShutdown = 1;
}

//---------------------------------------------------------------------------------
// Purpose: called when a client spawns into a server (i.e as they begin to play)
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::ClientActive( edict_t *pEntity )
{
    pAdminOP.ClientActive(pEntity);
}

//---------------------------------------------------------------------------------
// Purpose: called when a client leaves a server (or is timed out)
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::ClientDisconnect( edict_t *pEntity )
{
    ISOPGameSystem::ClientDisconnectAllSystems(pEntity);
    pAdminOP.ClientDisconnect(pEntity);
}

//---------------------------------------------------------------------------------
// Purpose: called on 
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::ClientPutInServer( edict_t *pEntity, char const *playername )
{
    /*KeyValues *kv = new KeyValues( "msg" );
    kv->SetString( "title", "Hello" );
    kv->SetString( "msg", "Hello there" );
    kv->SetColor( "color", Color( 255, 0, 0, 255 ));
    kv->SetInt( "level", 5);
    kv->SetInt( "time", 10);
    helpers->CreateMessage( pEntity, DIALOG_MSG, kv, this );
    kv->deleteThis();*/
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::SetCommandClient( int index )
{
    m_iClientCommandIndex = index;
    pAdminOP.m_iClientCommandIndex = index;
}

//---------------------------------------------------------------------------------
// Purpose: called on level start
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::ClientSettingsChanged( edict_t *pEdict )
{
/*  if ( playerinfomanager )
    {
        IPlayerInfo *playerinfo = playerinfomanager->GetPlayerInfo( pEdict );

        const char * name = engine->GetClientConVarValue( engine->IndexOfEdict(pEdict), "name" );

        if ( playerinfo && name && playerinfo->GetName() && 
             Q_stricmp( name, playerinfo->GetName()) ) // playerinfo may be NULL if the MOD doesn't support access to player data 
                                                       // OR if you are accessing the player before they are fully connected
        {
            char msg[128];
            Q_snprintf( msg, sizeof(msg), "Your name changed to \"%s\" (from \"%s\")\n", name, playerinfo->GetName() ); 
            engine->ClientPrintf( pEdict, msg ); // this is the bad way to check this, the better option it to listen for the "player_changename" event in FireGameEvent()
                                                // this is here to give a real example of how to use the playerinfo interface
        }
    }*/
}

//---------------------------------------------------------------------------------
// Purpose: called when a client joins a server
//---------------------------------------------------------------------------------
PLUGIN_RESULT CEmptyServerPlugin::ClientConnect( bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
{
    return pAdminOP.ClientConnect(bAllowConnect, pEntity, pszName, pszAddress, reject, maxrejectlen);
    //return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a client types in a command (only a subset of commands however, not CON_COMMAND's)
//---------------------------------------------------------------------------------
PLUGIN_RESULT CEmptyServerPlugin::ClientCommand( edict_t *pEntity, const CCommand &args )
{
    const char *pcmd = args[0];

    if ( !pEntity || pEntity->IsFree() ) 
    {
        return PLUGIN_CONTINUE;
    }

    /*if ( FStrEq( pcmd, "menu" ) )
    {
        KeyValues *kv = new KeyValues( "menu" );
        kv->SetString( "title", "You've got options, hit ESC" );
        kv->SetInt( "level", 1 );
        kv->SetColor( "color", Color( 255, 0, 0, 255 ));
        kv->SetInt( "time", 20 );
        kv->SetString( "msg", "Pick an option" );
        
        for( int i = 1; i < atoi(engine->Cmd_Argv(1)); i++ )
        {
            char num[10], msg[10], cmd[10];
            Q_snprintf( num, sizeof(num), "%i", i );
            Q_snprintf( msg, sizeof(msg), "Option %i", i );
            Q_snprintf( cmd, sizeof(cmd), "option%i", i );

            KeyValues *item1 = kv->FindKey( num, true );
            item1->SetString( "msg", msg );
            item1->SetString( "command", cmd );
        }

        helpers->CreateMessage( pEntity, DIALOG_MENU, kv, this );
        kv->deleteThis();
        return PLUGIN_STOP; // we handled this function
    }
    else if ( FStrEq( pcmd, "rich" ) )
    {
        KeyValues *kv = new KeyValues( "menu" );
        kv->SetString( "title", "A rich message" );
        kv->SetInt( "level", 1 );
        kv->SetInt( "time", 20 );
        kv->SetString( "msg", "This is a long long long text string.\n\nIt also has line breaks." );
        
        helpers->CreateMessage( pEntity, DIALOG_TEXT, kv, this );
        kv->deleteThis();
        return PLUGIN_STOP; // we handled this function
    }
    else if ( FStrEq( pcmd, "msg" ) )
    {
        KeyValues *kv = new KeyValues( "menu" );
        kv->SetString( "title", "Just a simple hello" );
        kv->SetInt( "level", 1 );
        kv->SetInt( "time", 20 );
        
        helpers->CreateMessage( pEntity, DIALOG_MSG, kv, this );
        kv->deleteThis();
        return PLUGIN_STOP; // we handled this function
    }
    else if ( FStrEq( pcmd, "admin" ) )
    {
        char msg[1024];
        sprintf(msg, "Your DF_password is %s\n", engine->GetClientConVarValue(engine->IndexOfEdict(pEntity), "DF_password"));

        engine->ClientPrintf(pEntity, msg);
    }
    else if ( FStrEq( pcmd, "sayall" ) )
    {
        char msg[256];
        sprintf(msg, "%s\n", engine->Cmd_Args());

        bf_write *pWrite;
        CReliableBroadcastRecipientFilter filter;
        filter.AddAllPlayers();
        
        pWrite = engine->UserMessageBegin(&filter, 4);
        pWrite->WriteByte(HUD_PRINTTALK);
        pWrite->WriteString(msg);
        //pWrite->WriteByte(1);
        engine->MessageEnd();
        
    }
    else if ( FStrEq( pcmd, "dumpmsgs" ) )
    {
        for(int i = 0; i < USR_MSGS_MAX; i++)
        {
            char msg[1024];
            sprintf(msg, "Msg %i: %s\n", i, pAdminOP.GetMessageName(i));
            engine->ClientPrintf(pEntity, msg);
        }
    }
    else if ( FStrEq( pcmd, "hudmsg" ) )
    {
        bf_write *pWrite;
        CReliableBroadcastRecipientFilter filter;
        filter.AddAllPlayers();

        pWrite = engine->UserMessageBegin(&filter, 5);
            pWrite->WriteByte( 1 & 0xFF );  // channel
            pWrite->WriteFloat( 0 );        // x
            pWrite->WriteFloat( 0 );        // y
            pWrite->WriteByte( 100 );       // r1
            pWrite->WriteByte( 100 );       // g1
            pWrite->WriteByte( 0 );         // b1
            pWrite->WriteByte( 0 );         // a1
            pWrite->WriteByte( 255  );      // r2
            pWrite->WriteByte( 255 );       // g2
            pWrite->WriteByte( 250 );       // b2
            pWrite->WriteByte( 0 );         // a2
            pWrite->WriteByte( 0 );         // effect
            pWrite->WriteFloat( 0.1 );      // fade in time
            pWrite->WriteFloat( 0.1 );      // fade out time
            pWrite->WriteFloat( 20.0 );     // hold time
            pWrite->WriteFloat( 3.5 );      // fx time
            pWrite->WriteString( "ogm this works?\n" );
        engine->MessageEnd();
    }*/


    if(ISOPGameSystem::ClientCommandAllSystems(servergameents->EdictToBaseEntity(pEntity), args) > COMMAND_IGNORED)
    {
        return PLUGIN_STOP;
    }
    return pAdminOP.ClientCommand(pEntity, args);


    //return PLUGIN_CONTINUE;
}

//---------------------------------------------------------------------------------
// Purpose: called when a client is authenticated
//---------------------------------------------------------------------------------
PLUGIN_RESULT CEmptyServerPlugin::NetworkIDValidated( const char *pszUserName, const char *pszNetworkID )
{
    if(!steamid_customnetworkidvalidated.GetBool())
    {
        pAdminOP.NetworkIDValidated(pszUserName, pszNetworkID);
    }
    return PLUGIN_CONTINUE;
}

void CEmptyServerPlugin::OnQueryCvarValueFinished( QueryCvarCookie_t iCookie, edict_t *pPlayerEntity, EQueryCvarValueStatus eStatus, const char *pCvarName, const char *pCvarValue )
{
    pAdminOP.OnQueryCvarValueFinished(iCookie, pPlayerEntity, eStatus, pCvarName, pCvarValue);
}

//---------------------------------------------------------------------------------
// Purpose: called when an event is fired
//---------------------------------------------------------------------------------
void CEmptyServerPlugin::FireGameEvent( KeyValues * event )
{/*
    //const char * name = event->GetName();
    //Msg( "CEmptyServerPlugin::FireGameEvent: Got event \"%s\"\n", name );
#ifdef OFFICIALSERV_ONLY
    //Msg("[EVENT] %s\n", event->GetName());
#endif
    KeyValues* kv;
    kv=event->GetFirstValue();
    if(FStrEq(event->GetName(), "server_spawn"))
    {
        pAdminOP.serverPort = event->GetInt("port", 0);
        if(!pAdminOP.hasDownloadUsers)
        {
            for(int i=0;i<5;i++)
            {
                Msg("Connecting to SourceOP.com ...\n");
                if(ParseUserFile() == 0) break;
            }
            pAdminOP.hasDownloadUsers = 1;
        }
    }
    else if(FStrEq(event->GetName(), "player_say"))
    {
        int playerList[128];
        int playerCount = 0;
        int userid = kv->GetInt();
        char text[256];
        int priority;

        kv = kv->GetNextValue();
        Q_snprintf(text, sizeof(text), "%s", kv->GetString());
        kv = kv->GetNextValue();
        priority = kv->GetInt();

        playerCount = pAdminOP.FindPlayerByUserID(playerList, userid);
        if(playerCount == 1)
        {
            pAdminOP.PlayerSpeak(playerList[0], userid, text);
        }
    }
    else if(FStrEq(event->GetName(), "player_changename"))
    {
        int playerList[128];
        int playerCount = 0;
        int userid = kv->GetInt();
        char oldname[128];
        char newname[128];

        kv = kv->GetNextValue();
        strncpy(oldname, kv->GetString(), sizeof(oldname));
        kv = kv->GetNextValue();
        strncpy(newname, kv->GetString(), sizeof(newname));

        playerCount = pAdminOP.FindPlayerByUserID(playerList, userid);
        if(playerCount == 1)
        {
            pAdminOP.pAOPPlayers[playerList[0]-1].NameChanged(oldname, newname);
        }
    }
    else if(FStrEq(event->GetName(), "player_death"))
    {
        int userid = kv->GetInt();
        kv = kv->GetNextValue();
        int attacker = kv->GetInt();
        char weapon[128];

        kv = kv->GetNextValue();
        strncpy(weapon, kv->GetString(), sizeof(weapon));

        pAdminOP.PlayerDeath(userid, attacker, weapon);
    }
    else if(FStrEq(event->GetName(), "player_spawn"))
    {
        int playerList[128];
        int playerCount = 0;
        int userid = kv->GetInt();
        kv = kv->GetNextValue();

        playerCount = pAdminOP.FindPlayerByUserID(playerList, userid);
        if(playerCount == 1)
        {
            pAdminOP.pAOPPlayers[playerList[0]-1].OnSpawn();
        }
    }
    else if(FStrEq(event->GetName(), "server_cvar"))
    {
        char cvarname[256];
        char cvarvalue[256];

        strncpy(cvarname, kv->GetString(), sizeof(cvarname));
        kv = kv->GetNextValue();
        strncpy(cvarvalue, kv->GetString(), sizeof(cvarvalue));

        pAdminOP.CvarChanged(cvarname, cvarvalue);
    }*/
}

char * CheckChatText( char *text )
{
    char *p = text;

    // invalid if NULL or empty
    if ( !text || !text[0] )
        return NULL;

    int length = Q_strlen( text );

    // remove quotes (leading & trailing) if present
    if (*p == '"')
    {
        p++;
        length -=2;
        p[length] = 0;
    }

    // cut off after 127 chars
    if ( length > 127 )
        text[127] = 0;

    return p;
}

#ifdef OFFICIALSERV_ONLY
class CMaxPlayerHook : public ConCommand
{
    // This will hold the pointer original gamedll say command
    ConCommand *m_pGameDLLSayCommand;
public:
    CMaxPlayerHook() : ConCommand("maxplayers", (ICommandCallback *)NULL, "Change the maximum number of players allowed on this server.", FCVAR_GAMEDLL), m_pGameDLLSayCommand(NULL)
    {
#ifdef __linux__
        // to be honest, i'm not even sure how this works since cvar isn't initialized at this point
        // but i suppose it must be somehow

        // this is required since the overrided Init() doesn't seem to be getting called
        HookOrig();
#endif
    }

    void HookOrig()
    {
        // Try to find the gamedll say command
        ConCommandBase *pPtr = pAdminOP.GetCommands();
        m_pGameDLLSayCommand = NULL;
        while (pPtr)
        {
            if (pPtr != this && pPtr->IsCommand() && strcmp(pPtr->GetName(), "maxplayers") == 0)
            {
                m_pGameDLLSayCommand = (ConCommand*)pPtr;
                return;
            }
            // Nasty
            pPtr = const_cast<ConCommandBase*>(pPtr->GetNext());
        }
        CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Warning: SourceOP cannot hook say command!");
        pAdminOP.TimeLog("SourceOPErrors.log", "SourceOP was unable to hook the say command.\n");
    }

    // Override Init
    void Init()
    {
        HookOrig();
        // Call base class' init function
        ConCommand::Init();
    }

    void Dispatch(const CCommand &args)
    {
        CCommand *ret = pAdminOP.MaxPlayersDispatchHandler(args);

        // Forward to gamedll
        if(m_pGameDLLSayCommand)
        {
            if(ret)
            {
                m_pGameDLLSayCommand->Dispatch(*ret);
            }
            else
            {
                m_pGameDLLSayCommand->Dispatch(args);
            }
        }
    }
};
//TODO: L4D, put back maxplayers hook
#ifndef _L4D_PLUGIN
CMaxPlayerHook g_MaxPlayersHook;
#endif
#endif // OFFICIALSERV_ONLY

//---------------------------------------------------------------------------------
// Purpose: hooks the say command so we can kill it and what not
// Thanks to PM and the SourceMod forum community
//---------------------------------------------------------------------------------
class CSayHook : public ConCommand
{
    // This will hold the pointer original gamedll say command
    ConCommand *m_pGameDLLSayCommand;
public:
    CSayHook() : ConCommand("say", (ICommandCallback *)NULL, "Display player message", FCVAR_GAMEDLL), m_pGameDLLSayCommand(NULL)
    {
#ifdef __linux__
        // to be honest, i'm not even sure how this works since cvar isn't initialized at this point
        // but i suppose it must be somehow

        // this is required since the overrided Init() doesn't seem to be getting called
        HookOrig();
#endif
    }

    void HookOrig()
    {
        // Try to find the gamedll say command
        ConCommandBase *pPtr = pAdminOP.GetCommands();
        m_pGameDLLSayCommand = NULL;
        while (pPtr)
        {
            if (pPtr != this && pPtr->IsCommand() && strcmp(pPtr->GetName(), "say") == 0)
            {
                m_pGameDLLSayCommand = (ConCommand*)pPtr;
                return;
            }
            // Nasty
            pPtr = const_cast<ConCommandBase*>(pPtr->GetNext());
        }
        CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Warning: SourceOP cannot hook say command!");
        pAdminOP.TimeLog("SourceOPErrors.log", "SourceOP was unable to hook the say command.\n");
    }

    // Override Init
    void Init()
    {
        HookOrig();
        // Call base class' init function
        ConCommand::Init();
    }

    void Dispatch(const CCommand &args)
    {
        char *p;
        char text[256];
        if(args.ArgC() > 1)
        {
            strncpy(text, args.ArgS(), sizeof(text));
            text[sizeof(text)-1] = '\0';
        }
        else
        {
            memset(&text, 0, sizeof(text));
        }

        if(!text[0]) return;
        p = CheckChatText(text);
        if(!p || !p[0]) return;
        // Do the normal stuff, return if you want to override the say
        //Msg("SayHook heard %i say: %s\n", pAdminOP.m_iClientCommandIndex, p);
        if(pAdminOP.m_iClientCommandIndex >= 0)
        {
            if(pAdminOP.PlayerSpeakPre(pAdminOP.m_iClientCommandIndex+1, p))
                return;
        }
        else
        {
            if(remoteserver) remoteserver->ConsoleChat(p);
        }

        // Forward to gamedll
        if(m_pGameDLLSayCommand)
            m_pGameDLLSayCommand->Dispatch(args);
    }
};
//TODO: L4D, put back say hook
#ifndef _L4D_PLUGIN
CSayHook g_SayHook;
#endif

class CSayTeamHook : public ConCommand
{
    // This will hold the pointer original gamedll say command
    ConCommand *m_pGameDLLSayCommand;
public:
    CSayTeamHook() : ConCommand("say_team", (ICommandCallback *)NULL, "Display player message to team", FCVAR_GAMEDLL), m_pGameDLLSayCommand(NULL)
    {
#ifdef __linux__
        HookOrig();
#endif
    }

    void HookOrig()
    {
        // Try to find the gamedll say command
        ConCommandBase *pPtr = pAdminOP.GetCommands();
        m_pGameDLLSayCommand = NULL;
        while (pPtr)
        {
            if (pPtr != this && pPtr->IsCommand() && strcmp(pPtr->GetName(), "say_team") == 0)
            {
                m_pGameDLLSayCommand = (ConCommand*)pPtr;
                return;
            }
            // Nasty
            pPtr = const_cast<ConCommandBase*>(pPtr->GetNext());
        }
        CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Warning: SourceOP cannot hook say_team command!");
        pAdminOP.TimeLog("SourceOPErrors.log", "SourceOP was unable to hook the say_team command.\n");
    }

    // Override Init
    void Init()
    {
        HookOrig();
        // Call base class' init function
        ConCommand::Init();
    }

    void Dispatch(const CCommand &args)
    {
        char *p;
        char text[256];
        if(args.ArgC() > 1)
        {
            strncpy(text, args.ArgS(), sizeof(text));
            text[127] = '\0';
        }
        else
        {
            memset(&text, 0, sizeof(text));
        }

        if(!text[0]) return;
        p = CheckChatText(text);
        if(!p || !p[0]) return;
        // Do the normal stuff, return if you want to override the say
        //Msg("SayTeamHook heard %i say: %s\n", pAdminOP.m_iClientCommandIndex, p);
        if(pAdminOP.m_iClientCommandIndex >= 0)
        {
            if(pAdminOP.PlayerSpeakTeamPre(pAdminOP.m_iClientCommandIndex+1, p))
                return;
        }

        // Forward to gamedll
        if(m_pGameDLLSayCommand)
            m_pGameDLLSayCommand->Dispatch(args);
    }
};
//TODO: L4D, put back say team hook
#ifndef _L4D_PLUGIN
CSayTeamHook g_SayTeamHook;
#endif

CGameEventListener g_eventListener;

bool CGameEventListener::FireEventHook( IGameEvent *event, bool bDontBroadcast )
{
    FireGameEvent(event);
    RETURN_META_VALUE(MRES_HANDLED, true);
}

void CGameEventListener::FireGameEvent( IGameEvent * event )
{
    const char *eventName = event->GetName();
    //Msg( "CEmptyServerPlugin::FireGameEvent: Got event \"%s\"\n", name );
#ifdef OFFICIALSERV_ONLY
    //Msg("[EVENT] %s\n", event->GetName());
#endif

    pAdminOP.GameEvent(event);

    if(FStrEq(eventName, "server_spawn"))
    {
        pAdminOP.serverPort = event->GetInt("port", 0);
        if(!pAdminOP.hasDownloadUsers)
        {
#ifndef OFFICIALSERV_ONLY
            for(int i=0;i<3;i++)
            {
                CAdminOP::ColorMsg(CONCOLOR_DARKGRAY, "[SOURCEOP] Connecting to SourceOP.com ...\n");
                if(ParseUserFile() == 0) break;
            }
#endif
            pAdminOP.hasDownloadUsers = 1;
        }
    }
    else if(FStrEq(eventName, "player_say"))
    {
        int playerList[128];
        int playerCount = 0;
        int userid = event->GetInt("userid");
        char text[256];
        //int priority;

        Q_snprintf(text, sizeof(text), "%s", event->GetString("text"));
        //priority = kv->GetInt();

        playerCount = pAdminOP.FindPlayerByUserID(playerList, userid);
        if(playerCount == 1)
        {
            pAdminOP.PlayerSpeak(playerList[0], userid, text);
        }
    }
    else if(FStrEq(eventName, "player_changename"))
    {
        int playerList[128];
        int playerCount = 0;
        int userid = event->GetInt("userid");
        char oldname[128];
        char newname[128];

        strncpy(oldname, event->GetString("oldname"), sizeof(oldname));
        strncpy(newname, event->GetString("newname"), sizeof(newname));

        playerCount = pAdminOP.FindPlayerByUserID(playerList, userid);
        if(playerCount == 1)
        {
            pAdminOP.pAOPPlayers[playerList[0]-1].NameChanged(oldname, newname);
        }
    }
    else if(FStrEq(eventName, "player_death"))
    {
        int userid = event->GetInt("userid");
        int attacker = event->GetInt("attacker");
        bool bIsFake = false;

        if(pAdminOP.isTF2)
        {
            int death_flags = event->GetInt("death_flags");
            bIsFake = ((death_flags & 32) == 32);
        }

        if(!bIsFake)
        {
            char weapon[128];
            strncpy(weapon, event->GetString("weapon"), sizeof(weapon));

            pAdminOP.PlayerDeath(userid, attacker, weapon);
        }
    }
    else if(FStrEq(eventName, "player_spawn"))
    {
        int playerList[128];
        int playerCount = 0;
        int userid = event->GetInt("userid");

        playerCount = pAdminOP.FindPlayerByUserID(playerList, userid);
        if(playerCount == 1)
        {
            pAdminOP.pAOPPlayers[playerList[0]-1].OnSpawn();
        }
    }
    else if(FStrEq(eventName, "player_disconnect"))
    {
        const char *reason = event->GetString("reason" );
        const char *name = event->GetString("name" );
        //const char *networkid = event->GetString("networkid" );

        char connectMsg[256];
        V_snprintf(connectMsg, sizeof(connectMsg), "%s left the game (%s)", name, reason);
        if(remoteserver) remoteserver->GameChat(0, connectMsg);
    }
    else if(FStrEq(eventName, "server_cvar"))
    {
        char cvarname[256];
        char cvarvalue[256];

        strncpy(cvarname, event->GetString("cvarname"), sizeof(cvarname));
        strncpy(cvarvalue, event->GetString("cvarvalue"), sizeof(cvarvalue));

        pAdminOP.CvarChanged(cvarname, cvarvalue);
    }
    else if(FStrEq(eventName, "round_start"))
    {
        if(!pAdminOP.isCstrike)
            pAdminOP.RoundStart();
    }
    else if(FStrEq(eventName, "round_end"))
    {
        pAdminOP.RoundEnd();
    }
    else if(FStrEq(eventName, "player_team"))
    {
        int playerList[128];
        int playerCount = 0;
        int userid = event->GetInt("userid");
        int newteam = event->GetInt("team");
        bool disconnect = event->GetBool("disconnect");
        if(!disconnect)
        {
            playerCount = pAdminOP.FindPlayerByUserID(playerList, userid);
            if(playerCount == 1)
            {
                pAdminOP.pAOPPlayers[playerList[0]-1].OnChangeTeam(newteam);
            }
        }
    }
    else if(FStrEq(eventName, "player_changeclass"))
    {
        int playerList[128];
        int playerCount = 0;
        int userid = event->GetInt("userid");
        int newclass = event->GetInt("class");

        playerCount = pAdminOP.FindPlayerByUserID(playerList, userid);
        if(playerCount == 1)
        {
            pAdminOP.pAOPPlayers[playerList[0]-1].OnChangeClass(newclass);
        }
    }
    else if(FStrEq(eventName, "teamplay_map_time_remaining"))
    {
        int timeleft = event->GetInt("seconds");

        pAdminOP.OverrideMapTimeRemaining(timeleft);
        //Msg("teamplay_map_time_remaining: %i\n", event->GetInt("seconds"));
    }
    else if(FStrEq(eventName, "teamplay_teambalanced_player"))
    {
        // TODO: Add player function that calls lua hook
    }
    else if(FStrEq(eventName, "teamplay_round_start"))
    {
        pAdminOP.RoundStart();
    }
    else if(FStrEq(eventName, "teamplay_round_win"))
    {
        pAdminOP.RoundEnd();
    }
    else if(FStrEq(eventName, "teamplay_flag_event"))
    {
        int player = event->GetInt("player");
        int carrier = event->GetInt("carrier");
        int type = event->GetInt("eventtype"); // 1. pickup, 2. capture, 3. defend, 4. dropped

        if(player > 0)
        {
            pAdminOP.pAOPPlayers[player-1].FlagEvent((EFlagEvent)type);
        }
    }
    else if(FStrEq(eventName, "arena_round_start"))
    {
        pAdminOP.ArenaRoundStart();
    }
    else if(FStrEq(eventName, "post_inventory_application"))
    {
        int playerList[128];
        int playerCount = 0;

        int userid = event->GetInt("userid");

        playerCount = pAdminOP.FindPlayerByUserID(playerList, userid);
        if(playerCount == 1)
        {
            pAdminOP.pAOPPlayers[playerList[0]-1].OnPostInventoryApplication();
        }
    }
    /*else if(FStrEq(eventName, "player_shoot"))
    {
        Msg("player_shoot: %i %i %i\n", event->GetInt("userid"), event->GetInt("weapon"), event->GetInt("mode"));
    }*/
#ifdef OFFICIALSERV_ONLY
    /*else
    {       
        while(kv)
        {
            Msg("[VALUE] %s : %s\n", kv->GetName(), kv->GetString());
            kv = kv->GetNextValue();
        }
    }*/
#endif
}

//---------------------------------------------------------------------------------
// Purpose: server commands
//---------------------------------------------------------------------------------
CON_COMMAND_F( DF_savecredits, "saves credits to file manually", FCVAR_SOP )
{
    if(pAdminOP.FeatureStatus(FEAT_CREDITS) && UTIL_IsCommandIssuedByServerAdmin() && pAdminOP.creditList.Count())
    {
        bool hasQuery = false;
        LARGE_INTEGER liFrequency,liStart,liStop;

        if(QueryPerformanceFrequency(&liFrequency)) hasQuery = true;
        if(hasQuery) QueryPerformanceCounter(&liStart);
        pAdminOP.SaveCreditsToFile();
        if(hasQuery) QueryPerformanceCounter(&liStop);
        engine->LogPrint(UTIL_VarArgs("[SOURCEOP] \"Console<0><Console><Console>\" saved credits to file in \"%f\" seconds\n", hasQuery ? (double)(liStop.QuadPart - liStart.QuadPart) / (double) liFrequency.QuadPart : -1));
    }
}

CON_COMMAND_F( DF_refreshpatcher, "Refreshes the generated file list that is used by the remote admin panel's patcher utility.", FCVAR_SOP )
{
    if(pAdminOP.FeatureStatus(FEAT_REMOTE) && UTIL_IsCommandIssuedByServerAdmin())
    {
        bool hasQuery = false;
        LARGE_INTEGER liFrequency,liStart,liStop;

        if(QueryPerformanceFrequency(&liFrequency)) hasQuery = true;
        if(hasQuery) QueryPerformanceCounter(&liStart);
        // TODO: seniorproj: Refresh remote server's patch file list
        if(hasQuery) QueryPerformanceCounter(&liStop);
        engine->LogPrint(UTIL_VarArgs("[SOURCEOP] \"Console<0><Console><Console>\" reinitialized _filelist_generated.txt in \"%f\" seconds\n", hasQuery ? (double)(liStop.QuadPart - liStart.QuadPart) / (double) liFrequency.QuadPart : -1));
    }
}

CON_COMMAND( DF_unhidecvars, "Unhides hidden CVARs." )
{
    if(!UTIL_IsCommandIssuedByServerAdmin()) return;

    if(!CAdminOP::UnhideDevCvars())
    {
        CAdminOP::ColorMsg(CONCOLOR_DARKGRAY, "[SOURCEOP] No CVARs to unhide.\n");
    }
}

#ifdef OFFICIALSERV_ONLY
#include "vfuncs.h"
#undef GetClassName
CON_COMMAND( DF_entlist, "Dumps entities to a file." )
{
    if(!UTIL_IsCommandIssuedByServerAdmin()) return;

    FILE *fp = fopen("allents.txt", "wt");
    if(fp)
    {
        CBaseEntity *pCur = NULL;
        while(pCur = gEntList.NextEnt(pCur))
        {
            IServerNetworkable *pNet = VFuncs::GetNetworkable(pCur);
            ServerClass *pServerClass = pNet ? pNet->GetServerClass() : NULL;
            fprintf(fp, "%-40s %s\n", VFuncs::GetClassname(pCur), pServerClass ? pServerClass->GetName() : "unknown");
        }

        fclose(fp);
    }
}
CON_COMMAND( DF_dumpcommands, "Dumps con commands to file." )
{
    if(!UTIL_IsCommandIssuedByServerAdmin()) return;

    FILE *fp = fopen("concommands.txt", "wt");
    ConCommandBase const *cmd = pAdminOP.GetCommands();

    for ( ; cmd; cmd = cmd->GetNext() )
    {
        if ( cmd->IsCommand() )
        {
            if( !cmd->IsFlagSet(FCVAR_CHEAT) && !cmd->IsFlagSet(FCVAR_DEVELOPMENTONLY) )
            {
                fputs(UTIL_VarArgs("%s\n", cmd->GetName()), fp);
            }
        }
    }

    fclose(fp);
}

CON_COMMAND( DF_testcolorprint, "Tests color print." )
{
    if(!UTIL_IsCommandIssuedByServerAdmin()) return;

    CAdminOP::ColorMsg(CONCOLOR_LIGHTBLUE, "Test1 %s\n", "%s");
    CAdminOP::ColorMsg(CONCOLOR_LIGHTBLUE, "Test2 %s\n", "%d");
    CAdminOP::ColorMsg(CONCOLOR_LIGHTBLUE, "Test3 %s\n", "Hello i have a %s in my name");
    CAdminOP::ColorMsg(CONCOLOR_LIGHTBLUE, "Test2 %s\n", "Hello i have only a % in my name");
}
#endif

CON_COMMAND( DF_luareload, "Reloads Lua." )
{
    if(pAdminOP.FeatureStatus(FEAT_LUA) && UTIL_IsCommandIssuedByServerAdmin())
    {
        pAdminOP.CloseLua();
        pAdminOP.LoadLua();
    }
}

CON_COMMAND( DF_printcommandline, "Prints the command line used to start srcds." )
{
    if(UTIL_IsCommandIssuedByServerAdmin())
    {
        Msg("%s\n", commandline->GetCmdLine());
    }
}

CON_COMMAND( DF_showstats, "Shows the last server stats received from Steam." )
{
    if(UTIL_IsCommandIssuedByServerAdmin())
    {
        Msg("Stats:\n m_eResult: %i\n m_nRank: %i\n m_unTotalConnects %u\n m_unTotalMinutesPlayed %u\n",
            pAdminOP.m_iStatsResult, pAdminOP.m_iStatsRank, pAdminOP.m_iStatsTotalConnects, pAdminOP.m_iStatsTotalMinutesPlayed);
    }
}

CON_COMMAND( DF_rslots_reload, "Reloads reserved slot database." )
{
    if(!UTIL_IsCommandIssuedByServerAdmin()) return;

    if(pAdminOP.rslots)
    {
        // update number of slots regardless of the enabled cvar
        pAdminOP.rslots->SetReservedSlots(rslots_slots.GetInt());
        if(rslots_enabled.GetBool())
        {
            if(!pAdminOP.rslots->LoadReservedUsersInThread())
            {
                CAdminOP::ColorMsg(CONCOLOR_LIGHTRED, "[SOURCEOP] Error starting reserve slot loader thread.\n");
            }
        }
        else
        {
            Msg("[SOURCEOP] Reserved slots are not enabled.\n");
        }
    }
}

CON_COMMAND( DF_unvacban_add, "Adds a SteamID to the un-vacban list." )
{
    if(!UTIL_IsCommandIssuedByServerAdmin()) return;

    if(args.ArgC() != 2)
    {
        Msg("[SOURCEOP] Usage: DF_unvacban_add <steamid>\n");
        return;
    }

    if(IsValidSteamID(args[1]))
    {
        CSteamID steamid = CSteamID(args[1], k_EUniversePublic);
        if(pAdminOP.AddVacAllowedPlayer(steamid))
        {
            Msg("[SOURCEOP] Added SteamID \"%s\".\n", args[1]);
        }
        else
        {
            Msg("[SOURCEOP] SteamID \"%s\" is already on the list.\n", args[1]);
        }
    }
    else
    {
        Msg("[SOURCEOP] Invalid SteamID \"%s\".\n", args[1]);
    }
}

CON_COMMAND( DF_unvacban_remove, "Removed a SteamID from the un-vacban list." )
{
    if(!UTIL_IsCommandIssuedByServerAdmin()) return;

    if(args.ArgC() != 2)
    {
        Msg("[SOURCEOP] Usage: DF_unvacban_remove <steamid>\n");
        return;
    }

    if(IsValidSteamID(args[1]))
    {
        CSteamID steamid = CSteamID(args[1], k_EUniversePublic);
        if(pAdminOP.RemoveVacAllowedPlayer(steamid))
        {
            Msg("[SOURCEOP] Removed SteamID \"%s\".\n", args[1]);
        }
        else
        {
            Msg("[SOURCEOP] SteamID \"%s\" is not on the list.\n", args[1]);
        }
    }
    else
    {
        Msg("[SOURCEOP] Invalid SteamID.\n");
    }
}

CON_COMMAND( DF_unvacban_remove_all, "Removes all SteamIDs from the un-vacban list." )
{
    if(!UTIL_IsCommandIssuedByServerAdmin()) return;

    pAdminOP.ClearVacAllowedPlayerList();
    Msg("[SOURCEOP] Un-vacban list cleared.\n");
}

CON_COMMAND( DF_unvacban_list, "Prints all SteamIDs on the un-vacban list." )
{
    if(!UTIL_IsCommandIssuedByServerAdmin()) return;

    pAdminOP.PrintVacAllowedPlayerList();
}

CON_COMMAND( DF_memstats, "Dumps g_pMemAlloc stats." )
{
    if(UTIL_IsCommandIssuedByServerAdmin())
    {
        // leak
        //malloc(123);
#if defined(_DEBUG) && defined(_WIN32)
        _CrtDumpMemoryLeaks();
#endif
        g_pMemAlloc->DumpStatsFileBase("sopmemstats");
    }
}

/*CON_COMMAND_F( DF_playertimefix, "Temporary command to fix player time.", FCVAR_SOP )
{
    Msg("Executing player time fix.\n");
    // if lastseen >= 1188301575, time = 0
    for ( unsigned int i=pAdminOP.creditList.Head(); i != pAdminOP.creditList.InvalidIndex(); i = pAdminOP.creditList.Next( i ) )
    {
        creditsram_t *findCredits = &pAdminOP.creditList.Element(i);
        if(findCredits->lastsave >= 1188301575)
        {
            // estimate
            int newval = min(findCredits->credits * 100, 86400);
            Msg("Fixed %s %s (%i->%i)\n", findCredits->WonID, findCredits->CurrentName, findCredits->timeonserver, newval);
            findCredits->timeonserver = newval;
        }
    }
}*/

/*CON_COMMAND_F( DF_namefix, "Temp command to fix name.", FCVAR_SOP )
{
    // if lastseen >= 1188301575, time = 0
    for ( unsigned int i=pAdminOP.creditList.Head(); i != pAdminOP.creditList.InvalidIndex(); i = pAdminOP.creditList.Next( i ) )
    {
        creditsram_t *findCredits = &pAdminOP.creditList.Element(i);
        if(findCredits->FirstName[0] == '\0')
        {
            if(findCredits->CurrentName[0] != '\0')
            {
                Msg("Fixing first name for \"%s\" \"%s\".\n", findCredits->CurrentName, findCredits->WonID);
                strcpy(findCredits->FirstName, findCredits->CurrentName);
            }
            else
            {
                int previ = pAdminOP.creditList.Previous(i);
                Msg("Removing unnamed entry \"%s\" with time \"%i\" and credits \"%i\".\n", findCredits->WonID, findCredits->timeonserver, findCredits->credits);
                pAdminOP.creditList.Remove(i);
                i = previ;
            }
        }
        else if(findCredits->CurrentName[0] == '\0')
        {
            Msg("Fixing current name for \"%s\" \"%s\".\n", findCredits->FirstName, findCredits->WonID);
            strcpy(findCredits->CurrentName, findCredits->FirstName);
        }
    }
}*/

void DFVersionChangeCallback( IConVar *var, const char *pOldValue, float flOldValue )
{
    if(strcmp(DF_sopversion.GetString(), SourceOPVersion))
    {
        var->SetValue(SourceOPVersion);
    }
}

void DFDatadirChangeCallback( IConVar *var, const char *pOldValue, float flOldValue )
{
    if(strcmp(DF_datadir.GetString(), pAdminOP.DataDir()))
    {
        var->SetValue(pAdminOP.DataDir());
    }
}

void DFMeleeOnlyCallback( IConVar *var, const char *pOldValue, float flOldValue )
{
    pAdminOP.SetMeleeMode(tf2_meleeonly.GetBool());
}

void DFSleepTimeCallback( IConVar *var, const char *pOldValue, float flOldValue )
{
    if(pAdminOP.sleepHack)
    {
        int newVal = sleep_time.GetInt();
        if(newVal == -1)
        {
            pAdminOP.sleepHack->RemoveSleepCall();
        }
        else
        {
            pAdminOP.sleepHack->RestoreSleepCall();
            pAdminOP.sleepHack->SetSleepParam(sleep_time.GetInt());
        }
    }
}

void DFRSlotsCallback( IConVar *var, const char *pOldValue, float flOldValue )
{
    if(pAdminOP.rslots)
    {
        pAdminOP.rslots->SetReservedSlots(rslots_slots.GetInt());
        Msg("[SOURCEOP] Number of reserved slots changed from %i to %i. %i remaining.\n", atoi(pOldValue), rslots_slots.GetInt(), pAdminOP.rslots->ReservedSlotsRemaining());
    }
}

void DFSteamIDPrefixNumberCallback( IConVar *var, const char *pOldValue, float flOldValue )
{
    g_steamIDPrefix = steamid_prefixnumber.GetInt();
}

IChangeInfoAccessor *CBaseEdict::GetChangeAccessor()
{
    return engine->GetChangeAccessor( (const edict_t *)this );
}

const IChangeInfoAccessor *CBaseEdict::GetChangeAccessor() const
{
    return engine->GetChangeAccessor( (const edict_t *)this );
}
