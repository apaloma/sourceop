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

#ifndef CDETOURPLATFORM_H
#define CDETOURPLATFORM_H

#ifndef __linux__

#include <windows.h>

#else

#include "basetypes.h"
#include <string.h>
#include <dlfcn.h>
#include <sys/mman.h>

int VirtualProtect(const void *addr, size_t len, int prot, int *prev);
#define FlushInstructionCache(proc, addr, size) true
#define GetCurrentProcess
#define GetTickCount() 1
#define LoadLibrary(x) dlopen((x), RTLD_NOW)
#define GetProcAddress dlsym
#define SetLastError(x)
#define CopyMemory(dest, src, size) memcpy(dest, src, size)

#define ERROR_INVALID_DATA 13L

#ifndef PAGESIZE
#define PAGESIZE 4096
#endif
#define PAGE_EXECUTE_READWRITE (PROT_READ | PROT_WRITE | PROT_EXEC)

#define ADDRTYPE unsigned long
#define VOID void
typedef void *PVOID;
typedef char CHAR;
typedef char *PCHAR;
typedef long LONG;
typedef long *PLONG;
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef short SHORT;
typedef short *PSHORT;
typedef signed int INT32;
typedef unsigned int UINT32;
typedef unsigned char BYTE;
typedef BYTE *PBYTE;
typedef const char *LPCSTR;

#if !defined(HINSTANCE)
typedef void * HINSTANCE;
#endif
#if !defined(HMODULE)
typedef void * HMODULE;
#endif

#define __cdecl
#define __stdcall __attribute__ ((__stdcal__))

#define WINAPI __stdcall

#endif

#endif // CDETOURPLATFORM_H
