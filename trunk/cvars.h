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

#ifndef CVARS_H
#define CVARS_H

extern ConVar modify_gamedescription;

extern ConVar command_prefix;
extern ConVar feature_credits;
extern ConVar credits_announce;
extern ConVar credits_newcreditmsg;
extern ConVar credits_maxpermap;
extern ConVar credits_maxpointsatonce;
extern ConVar credits_cheatprotect_on;
extern ConVar credits_cheatprotect_amount;
extern ConVar credits_cheatprotect_time;
extern ConVar creditgap;
extern ConVar feature_showcredits;
extern ConVar feature_showcredits_x;
extern ConVar feature_showcredits_y;

extern ConVar time_god;
extern ConVar time_noclip;

extern ConVar price_noclip;
extern ConVar price_god;
extern ConVar price_gun;
extern ConVar price_allguns;
extern ConVar price_ammo;
extern ConVar price_health;
extern ConVar price_slay;
extern ConVar price_explosive;

extern ConVar hook_on;
extern ConVar hook_speed;
extern ConVar hook_pullmode;

extern ConVar jetpack_on;
extern ConVar jetpack_team;
extern ConVar jetpack_show_disabledmsg;
extern ConVar jetpack_show_accessdenymsg;
extern ConVar jetpack_show_chargemsg;
extern ConVar jetpack_infinite_charge;

extern ConVar radio_on;
extern ConVar radio_model;
extern ConVar radio_passable;

extern ConVar feature_mapvote;
extern ConVar vote_freq;
extern ConVar vote_maxextend;
extern ConVar vote_extendtime;
extern ConVar vote_ratio;
extern ConVar vote_menu;

extern ConVar jetpackvote_enable;
extern ConVar jetpackvote_show_disabledmsg;
extern ConVar jetpackvote_freq;
extern ConVar jetpackvote_ratio;
extern ConVar jetpackvote_duration;
extern ConVar jetpackvote_printstandings;
extern ConVar jetpackvote_printvoters;

extern ConVar playerchat_motd;
extern ConVar playerchat_info;
extern ConVar playerchat_names;
extern ConVar playerchat_time;

extern ConVar gag_notify;

extern ConVar allow_noclipping;

extern ConVar entmove_changecolor;

extern ConVar spawn_removeondisconnect;
extern ConVar spawnlimit_admin;
extern ConVar spawnlimit_server;

extern ConVar feature_killsounds;

extern ConVar override_execblock;
extern ConVar override_execblock_silent;
extern ConVar override_execblock_overwrite;

extern ConVar remote_port;
extern ConVar remote_logchattoplayers;
extern ConVar remote_logchattoadmins;
extern ConVar remote_mapupdaterate;

extern ConVar help_show_loading;
extern ConVar ignore_chat;

extern ConVar highpingkicker_on;
extern ConVar highpingkicker_maxping;
extern ConVar highpingkicker_samples;
extern ConVar highpingkicker_sampletime;
extern ConVar highpingkicker_message;

extern ConVar tf2_fastrespawn;
extern ConVar tf2_meleeonly;
extern ConVar tf2_roundendalltalk;
extern ConVar tf2_customitems;
extern ConVar tf2_arenateamsize;
extern ConVar tf2_disable_voicemenu;
extern ConVar tf2_disable_voicemenu_message;
extern ConVar tf2_disable_fish;
extern ConVar tf2_disable_witcher;
extern ConVar tf2_disable_mvmprecache;

extern ConVar unvacban_enabled;

extern ConVar maxplayers_force;

extern ConVar lua_gc_stepeveryframe;
extern ConVar lua_attack_hooks;

extern ConVar serverquery_fakemaster;
extern ConVar serverquery_a2sinfo_override;
extern ConVar serverquery_a2sinfo_override_serverversion;
extern ConVar serverquery_visibleplayers;
extern ConVar serverquery_visibleplayers_min;
extern ConVar serverquery_visiblemaxplayers;
extern ConVar serverquery_addplayers;
extern ConVar serverquery_fakeplayers;
extern ConVar serverquery_fakeaddplayersonly;
extern ConVar serverquery_showconnecting;
extern ConVar serverquery_hidereplay;
extern ConVar serverquery_replacefirsthostnamechar;
extern ConVar serverquery_tagsoverride;
extern ConVar serverquery_debugprints;
extern ConVar serverquery_log;
extern ConVar serverquery_faketohost_host;
extern ConVar serverquery_faketohost_passworded;
extern ConVar serverquery_faketohost_maxplayers;
extern ConVar serverquery_maxconnectionless;
extern ConVar serverquery_block_alla2cprint;

extern ConVar servermoved;

extern ConVar rslots_enabled;
extern ConVar rslots_slots;
extern ConVar rslots_block_after_visiblemaxplayers;
extern ConVar mysql_database_addr;
extern ConVar mysql_database_user;
extern ConVar mysql_database_pass;
extern ConVar mysql_database_name;
extern ConVar rslots_minimum_donation;
extern ConVar rslots_database2_addr;
extern ConVar rslots_database2_user;
extern ConVar rslots_database2_pass;
extern ConVar rslots_database2_name;
extern ConVar rslots_databases;
extern ConVar rslots_refresh;

extern ConVar phys_gunmass;
extern ConVar phys_gunvel;
extern ConVar phys_gunforce;
extern ConVar phys_guntorque;

extern ConVar grenades;
extern ConVar grenades_perlife;

extern ConVar bans_bancfg;
extern ConVar bans_mysql;
extern ConVar bans_require_reason;

extern ConVar sql_playerdatabase;

extern ConVar sk_snark_health;
extern ConVar sk_snark_dmg_bite;
extern ConVar sk_snark_dmg_pop;

extern ConVar damage_multiplier;

extern ConVar blockfriendlyheavy;

extern ConVar extra_heartbeat;

extern ConVar steamid_customnetworkidvalidated;

#ifdef OFFICIALSERV_ONLY
extern ConVar disablethink;
extern ConVar disabletouch;
#endif

extern ConVar debug_log;

extern ConVar sleep_time;

extern ConVar *hostname;
extern ConVar *sv_cheats;
extern ConVar *nextlevel;
extern ConVar *mapcyclefile;
extern ConVar *sv_gravity;
extern ConVar *mp_teamplay;
extern ConVar *phys_pushscale_game;
extern ConVar *sv_logecho;
extern ConVar *mp_respawnwavetime;
extern ConVar *timelimit;
extern ConVar *mp_fraglimit;
extern ConVar *mp_winlimit;
extern ConVar *mp_maxrounds;
extern ConVar *mp_friendlyfire;
extern ConVar *sv_alltalk;
extern ConVar *srv_ip;
extern ConVar *srv_hostport;
extern ConVar *tv_enable;
extern ConVar *tv_name;
extern ConVar *sv_tags;
extern ConVar *replay_enable;
extern ConVar *sv_use_steam_voice;

#endif
