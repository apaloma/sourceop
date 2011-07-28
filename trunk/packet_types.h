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

#ifndef PACKET_TYPES_H
#define PACKET_TYPES_H

#define svc_statusmessage       '!'
#define svc_authcomplete        '0'
#define svc_adminchat           'A'
#define svc_playerchat          'C'
#define svc_playerdisconnect    'D'
#define svc_mapplayerupdate     'F'
#define svc_logdata             'L'
#define svc_mapplayerupdate_old 'M'
#define svc_mapchange           'N'
#define svc_playerupdate        'P'
#define svc_maplist             'Q'
#define svc_pingresponse        'R'
#define svc_serverinfo          'S'
#define svc_admindisconnect     'T'
#define svc_adminupdate         'U'
#define svc_voicedata           'V'

#define clc_auth                '0'
#define clc_modeselection       '1'
#define clc_adminchat           'A'
#define clc_banplayer           'B'
#define clc_playerchat          'C'
#define clc_gagplayer           'G'
#define clc_kickplayer          'K'
#define clc_ping                'P'
#define clc_requestmaplist      'Q'
#define clc_rcon                'R'
#define clc_setplayerteam       'T'

#endif
