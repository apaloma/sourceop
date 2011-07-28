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

#ifndef SOPMYSQL_H
#define SOPMYSQL_H

#ifndef __linux__
#include <winsock.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <dlfcn.h>
#endif
#include <mysql.h>

#include "tier0/memdbgon.h"

typedef unsigned int (STDCALL* PtrFunction_MThreadSafe)(void);
typedef char (STDCALL* PtrFunction_MInit)(void);
typedef MYSQL* (STDCALL* PtrFunction_MSqlInit)(MYSQL *mysql);
typedef char (STDCALL* PtrFunction_MThreadInit)(void);
typedef void (STDCALL* PtrFunction_MThreadEnd)(void);
typedef int (STDCALL* PtrFunction_MOptions)(MYSQL *mysql, enum mysql_option option, const char *arg);
typedef MYSQL* (STDCALL* PtrFunction_MRC)(MYSQL *mysql, const char *host, const char *user, const char *passwd, const char *db, unsigned int port, const char *unix_socket, unsigned long clientflag);
typedef int (STDCALL* PtrFunction_MQuery)(MYSQL *mysql, const char *q);
typedef unsigned long (STDCALL* PtrFunction_mysql_real_escape_string)(MYSQL *mysql, char *to, const char *from, unsigned long length);
typedef MYSQL_RES* (STDCALL* PtrFunction_mysql_use_result)(MYSQL *mysql);
typedef MYSQL_ROW (STDCALL* PtrFunction_mysql_fetch_row)(MYSQL_RES *result);
typedef void (STDCALL* PtrFunction_mysql_free_result)(MYSQL_RES *result);
typedef void (STDCALL* PtrFunction_mysql_close)(MYSQL *sock);

class CSOPMySQLRow
{
public:
    CSOPMySQLRow(MYSQL_ROW data);
    const char* Column(int row);
    bool IsNull();
private:
    MYSQL_ROW rowdata;
};

class CSOPMySQL
{
    friend class CSOPMySQLConnection;
public:
    CSOPMySQL();
    ~CSOPMySQL();

    bool ThreadInit();
    void ThreadEnd();

protected:
    PtrFunction_MThreadSafe Ptr_MThreadSafe;
    PtrFunction_MInit Ptr_MInit;
    PtrFunction_MSqlInit Ptr_MSqlInit;
    PtrFunction_MThreadInit Ptr_MThreadInit;
    PtrFunction_MThreadEnd Ptr_MThreadEnd;
    PtrFunction_MOptions Ptr_MOptions;
    PtrFunction_MRC Ptr_MRC;
    PtrFunction_MQuery Ptr_MQuery;
    PtrFunction_mysql_real_escape_string Ptr_mysql_real_escape_string;
    PtrFunction_mysql_use_result Ptr_mysql_use_result;
    PtrFunction_mysql_fetch_row Ptr_mysql_fetch_row;
    PtrFunction_mysql_free_result Ptr_mysql_free_result;
    PtrFunction_mysql_close Ptr_mysql_close;

private:
#ifndef __linux__
    HINSTANCE DLLHandle;
#else
    void *DLLHandle;
#endif
};

class CSOPMySQLConnection
{
public:
    CSOPMySQLConnection();
    ~CSOPMySQLConnection();

    bool Connect(const char *host, const char *user, const char *passwd, const char *db, unsigned int port = 0, const char *unix_socket = NULL, unsigned long clientflag = 0);
    int Query(const char *q);
    unsigned long EscapeString(char *to, const char *from, unsigned long length);
    bool UseResult();
    CSOPMySQLRow NextRow();
    void FreeResult();
    void Close();

private:
    MYSQL mysql_db;
    bool connected;

    MYSQL_RES *result;
};

extern CSOPMySQL *g_pSOPMySQL;

#endif // SOPMYSQL_H
