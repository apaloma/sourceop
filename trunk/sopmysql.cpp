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

#include "fixdebug.h"
#include <stdio.h>

#include "interface.h"
#include "eiface.h"

#include "sopmysql.h"

#include "tier0/memdbgon.h"

#ifdef __linux__
#define GetProcAddress dlsym
#else
#include <strsafe.h>
#endif

static CSOPMySQL s_sopMySQL;
CSOPMySQL *g_pSOPMySQL = &s_sopMySQL;

CSOPMySQLRow::CSOPMySQLRow(MYSQL_ROW data)
{
    rowdata = data;
}

const char *CSOPMySQLRow::Column(int row)
{
    if(!rowdata) return NULL;
    return rowdata[row];
}

bool CSOPMySQLRow::IsNull()
{
    return rowdata == NULL;
}

CSOPMySQL::CSOPMySQL()
{
#ifndef __linux__
    DLLHandle = LoadLibrary("libmySQL.dll");
#else
    DLLHandle = dlopen("libmysql.so", RTLD_NOW);
    if(!DLLHandle)
        DLLHandle = dlopen("libmysqlclient.so", RTLD_NOW);
#endif
    if(DLLHandle)
    {
        Ptr_MThreadSafe = (PtrFunction_MThreadSafe)GetProcAddress(DLLHandle,"mysql_thread_safe");
        Ptr_MInit = (PtrFunction_MInit)GetProcAddress(DLLHandle,"my_init");
        Ptr_MSqlInit = (PtrFunction_MSqlInit)GetProcAddress(DLLHandle,"mysql_init");
        Ptr_MThreadInit = (PtrFunction_MThreadInit)GetProcAddress(DLLHandle,"mysql_thread_init");
        Ptr_MThreadEnd = (PtrFunction_MThreadEnd)GetProcAddress(DLLHandle,"mysql_thread_end");
        Ptr_MOptions = (PtrFunction_MOptions)GetProcAddress(DLLHandle,"mysql_options");
        Ptr_MRC = (PtrFunction_MRC)GetProcAddress(DLLHandle,"mysql_real_connect");
        Ptr_MQuery = (PtrFunction_MQuery)GetProcAddress(DLLHandle,"mysql_query");
        Ptr_mysql_real_escape_string = (PtrFunction_mysql_real_escape_string)GetProcAddress(DLLHandle,"mysql_real_escape_string");
        Ptr_mysql_use_result = (PtrFunction_mysql_use_result)GetProcAddress(DLLHandle,"mysql_use_result");
        Ptr_mysql_fetch_row = (PtrFunction_mysql_fetch_row)GetProcAddress(DLLHandle,"mysql_fetch_row");
        Ptr_mysql_free_result = (PtrFunction_mysql_free_result)GetProcAddress(DLLHandle,"mysql_free_result");
        Ptr_mysql_close = (PtrFunction_mysql_close)GetProcAddress(DLLHandle,"mysql_close");

        if(!Ptr_MThreadSafe || !Ptr_MThreadInit || !Ptr_MThreadEnd || !Ptr_MThreadSafe())
            Error("[SOURCEOP] libmySQL has not been compiled as thread safe.\n");

        Ptr_MInit();
    }
    else
    {
        // TODO: It would be nice if this just prevented any SQL operations from occuring rather than bringing down srcds.
#ifndef __linux__
        LPVOID lpMsgBuf;
        DWORD dw = GetLastError();

        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR) &lpMsgBuf,
            0,
            NULL );

        Error("[SOURCEOP] Failed to load libmySQL with error %d: %s\n", dw, lpMsgBuf);
        LocalFree(lpMsgBuf);
#else
        Error("[SOURCEOP] Failed to load libmySQL: %s\n", dlerror());
#endif
    }
}

CSOPMySQL::~CSOPMySQL()
{
    if(DLLHandle)
    {
#ifndef __linux__
        FreeLibrary(DLLHandle);
#else
        dlclose(DLLHandle);
#endif
    }
}

bool CSOPMySQL::ThreadInit()
{
    return Ptr_MThreadInit() == 0;
}

void CSOPMySQL::ThreadEnd()
{
    Ptr_MThreadEnd();
}

CSOPMySQLConnection::CSOPMySQLConnection()
{
    connected = false;
    result = NULL;

    unsigned int timeout = 5;
    g_pSOPMySQL->Ptr_MSqlInit(&mysql_db);
    if(g_pSOPMySQL->Ptr_MOptions(&mysql_db, MYSQL_READ_DEFAULT_GROUP, "SourceOP"))
        Msg("[SOURCEOP] Failed to set MySQL option MYSQL_READ_DEFAULT_GROUP.\n");
    if(g_pSOPMySQL->Ptr_MOptions(&mysql_db, MYSQL_OPT_CONNECT_TIMEOUT, (const char *)&timeout))
        Msg("[SOURCEOP] Failed to set MySQL option MYSQL_OPT_CONNECT_TIMEOUT.\n");

}

CSOPMySQLConnection::~CSOPMySQLConnection()
{
    Close();
}

bool CSOPMySQLConnection::Connect(const char *host, const char *user, const char *passwd, const char *db, unsigned int port, const char *unix_socket, unsigned long clientflag)
{
    connected = g_pSOPMySQL->Ptr_MRC(&mysql_db, host, user, passwd, db, port, unix_socket, clientflag) != NULL;
    return connected;
}

int CSOPMySQLConnection::Query(const char *q)
{
    return g_pSOPMySQL->Ptr_MQuery(&mysql_db, q);
}

unsigned long CSOPMySQLConnection::EscapeString(char *to, const char *from, unsigned long length)
{
    return g_pSOPMySQL->Ptr_mysql_real_escape_string(&mysql_db, to, from, length);
}

bool CSOPMySQLConnection::UseResult()
{
    if(result) FreeResult();
    result = g_pSOPMySQL->Ptr_mysql_use_result(&mysql_db);
    return result != NULL;
}

CSOPMySQLRow CSOPMySQLConnection::NextRow()
{
    if(!result) UseResult();
    if(!result) return CSOPMySQLRow(NULL);
    return CSOPMySQLRow(g_pSOPMySQL->Ptr_mysql_fetch_row(result));
}

void CSOPMySQLConnection::FreeResult()
{
    if(result)
    {
        g_pSOPMySQL->Ptr_mysql_free_result(result);
        result = NULL;
    }
}

void CSOPMySQLConnection::Close()
{
    if(connected)
    {
        FreeResult();
        connected = false;
    }

    g_pSOPMySQL->Ptr_mysql_close(&mysql_db);
}
