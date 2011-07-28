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

#ifndef SOPDOWNLOAD_H
#define SOPDOWNLOAD_H

typedef enum
{
    NOCONERROR, HOSTERR, CONSOCKERR, CONERROR,
    CONREFUSED, NEWLOCATION, NOTENOUGHMEM, CONPORTERR,
    BINDERR, BINDOK, LISTENERR, ACCEPTERR, ACCEPTOK,
    CONCLOSED, FTPOK, FTPLOGINC, FTPLOGREFUSED, FTPPORTERR,
    FTPNSFOD, FTPRETROK, FTPUNKNOWNTYPE, FTPRERR,
    FTPREXC, FTPSRVERR, FTPRETRINT, FTPRESTFAIL,
    URLOK, URLHTTP, URLFTP, URLFILE, URLUNKNOWN, URLBADPORT,
    URLBADHOST, FOPENERR, FWRITEERR, HOK, HLEXC, HEOF,
    HERR, RETROK, RECLEVELEXC, FTPACCDENIED, WRONGCODE,
    FTPINVPASV, FTPNOPASV,
    RETRFINISHED, READERR, TRYLIMEXC, URLBADPATTERN,
    FILEBADFILE, RANGEERR, RETRBADPATTERN, RETNOTSUP,
    ROBOTSOK, NOROBOTS, PROXERR, AUTHFAILED, QUOTEXC, WRITEFAILED
} uerr_t;

extern uerr_t ParseUserFile(void);
extern bool UpdateSourceOPMaster(const char *pszCurLevel = NULL);

#endif
