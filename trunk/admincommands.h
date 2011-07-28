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

#ifndef ADMINCOMMANDS_H
#define ADMINCOMMANDS_H

PLUGIN_RESULT DFAdminCommands(const CCommand &args, edict_t *pEntity);

//1
int DFAdminLookingAt(const CCommand &args, edict_t *pEntity);

//2
int DFAdminTimeLimit(const CCommand &args, edict_t *pEntity);
int DFAdminMap(const CCommand &args, edict_t *pEntity);
int DFAdminReload(const CCommand &args, edict_t *pEntity);

//4
//8
//16
int DFAdminPass(const CCommand &args, edict_t *pEntity);

//32
int DFAdminGodSelf(const CCommand &args, edict_t *pEntity);
int DFAdminNoclipSelf(const CCommand &args, edict_t *pEntity);
int DFAdminGiveWeaponsSelf(const CCommand &args, edict_t *pEntity);
int DFAdminAmmoSelf(const CCommand &args, edict_t *pEntity);

//64
int DFAdminBSay(const CCommand &args, edict_t *pEntity);
int DFAdminCSay(const CCommand &args, edict_t *pEntity);
int DFAdminCTSay(const CCommand &args, edict_t *pEntity);
int DFAdminVoiceTeam(const CCommand &args, edict_t *pEntity);

//128
int DFAdminSlap(const CCommand &args, edict_t *pEntity);
int DFAdminSlay(const CCommand &args, edict_t *pEntity);
int DFAdminKick(const CCommand &args, edict_t *pEntity);

//256
int DFAdminBan(const CCommand &args, edict_t *pEntity);
int DFAdminUnban(const CCommand &args, edict_t *pEntity);

//512
int DFAdminNoclip(const CCommand &args, edict_t *pEntity);
int DFAdminGiveWeapons(const CCommand &args, edict_t *pEntity);

//1024
int DFAdminKeyValue(const CCommand &args, edict_t *pEntity);

//2048

//4096
int DFAdminAI(const CCommand &args, edict_t *pEntity);
//8192
int DFAdminAmmo(const CCommand &args, edict_t *pEntity);
int DFAdminGod(const CCommand &args, edict_t *pEntity);
int DFAdminRender(const CCommand &args, edict_t *pEntity);
int DFAdminExec(const CCommand &args, edict_t *pEntity);
int DFAdminExecAll(const CCommand &args, edict_t *pEntity);
int DFAdminSend(const CCommand &args, edict_t *pEntity);
int DFAdminFillUber(const CCommand &args, edict_t *pEntity);
int DFAdminAlltalk(const CCommand &args, edict_t *pEntity);
int DFAdminPlayerSpray(const CCommand &args, edict_t *pEntity);
int DFAdminGag(const CCommand &args, edict_t *pEntity);
int DFAdminNumBuildings(const CCommand &args, edict_t *pEntity);
int DFAdminAwardAchievement(const CCommand &args, edict_t *pEntity);
int DFAdminAwardAllAchievements(const CCommand &args, edict_t *pEntity);

//16384
//32768
int DFAdminAddAdmin(const CCommand &args, edict_t *pEntity);

//65536
int DFAdminRcon(const CCommand &args, edict_t *pEntity);

#endif
