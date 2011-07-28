/*
 * Universal memory patcher class to patch memory of any process under Win32
 * Windows 95/98/ME/2k compatible.
 * Copyright (c) Vasily Pupkin & Tim Yunaev 2001
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: mempatcher.cpp,v 1.5 2001/11/21 12:41:11 tim Exp $
 */

#include "fixdebug.h"
//#include "stdafx.h"
#include <stdio.h>
#include <errno.h>

#include "mempatcher.h"

const unsigned long PAGE_SIZE = 4096;

// uncomment to debug
// #define PATCHER_DEBUG
#ifdef PATCHER_DEBUG
#include "tier0/dbg.h"
#endif

#include "tier0/memdbgon.h"

CMemoryPatcher::CMemoryPatcher()
{
    hKernel32 = 0;

    pfnCreateToolhelp32Snapshot = 0;
    pfnProcess32First = 0;
    pfnProcess32Next = 0;
    pfnModule32First = 0;
    pfnModule32Next = 0;
    pfnCloseToolhelp32Snapshot = 0;
}



CMemoryPatcher::~CMemoryPatcher()
{
    if ( hKernel32 )
        FreeLibrary (hKernel32);
}



void CMemoryPatcher::Error (const char *fmt, ...)
{
    va_list va_alist;
    char buf[512];

    va_start (va_alist, fmt);
    _vsnprintf (buf, sizeof(buf), fmt, va_alist);
    va_end (va_alist);

    strncpy(szErrorMsg, buf, sizeof(buf));
    buf[sizeof(buf)-1] = '\0';
}



bool CMemoryPatcher::Init()
{
    if(hKernel32) // already inited
        return true;

    if ( (hKernel32 = LoadLibrary("kernel32.dll")) == 0 )
    {
        Error ("Could not open kernel32.dll: %s", strerror(errno));
        return false;
    }

    if ( (pfnCreateToolhelp32Snapshot = (HANDLE (WINAPI*) (DWORD, DWORD)) GetProcAddress(hKernel32, "CreateToolhelp32Snapshot")) == 0 )
    {
        Error ("Could not get CreateToolhelp32Snapshot address\nIf you use Windows NT 4, you need libloader for NT.");
        return false;
    }

    if ( (pfnProcess32First = (BOOL (WINAPI*) (HANDLE, LPPROCESSENTRY32)) GetProcAddress(hKernel32, "Process32First")) == 0 )
    {
        Error ("Could not get Process32First address");
        return false;
    }

    if ( (pfnProcess32Next = (BOOL (WINAPI*) (HANDLE, LPPROCESSENTRY32)) GetProcAddress(hKernel32, "Process32Next")) == 0 )
    {
        Error ("Could not get Process32Next address");
        return false;
    }

    if ( (pfnModule32First = (BOOL (WINAPI*) (HANDLE, LPMODULEENTRY32)) GetProcAddress(hKernel32, "Module32First")) == 0 )
    {
        Error ("Could not get Module32First address");
        return false;
    }

    if ( (pfnModule32Next = (BOOL (WINAPI*) (HANDLE, LPMODULEENTRY32)) GetProcAddress(hKernel32, "Module32Next")) == 0 )
    {
        Error ("Could not get Process32Next address");
        return false;
    }

    // can be NULL on non-Win2k systems
    // HACK: Disabled this. SourceOP doesn't use it and it was causing issues in Visual Studio 2005.
    //pfnVirtualAllocEx = (LPVOID (WINAPI *) (HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect)) GetProcAddress(hKernel32, "VirtualAllocEx");

    // can be NULL, we use CloseHandle in this case
    pfnCloseToolhelp32Snapshot = (void (WINAPI*) (HANDLE)) GetProcAddress(hKernel32, "CloseToolhelp32Snapshot");
    return true;
}


void CMemoryPatcher::FreeProcessHandle(HANDLE hSnapShot)
{
    if ( hSnapShot != INVALID_HANDLE_VALUE )
    {
        if ( pfnCloseToolhelp32Snapshot )
            (*pfnCloseToolhelp32Snapshot)(hSnapShot);
        else
            CloseHandle(hSnapShot);
    }
}

// returns module entry if module is found
bool CMemoryPatcher::FindModule (const char * module_name, MODULEENTRY32 * modentry)
{
    return FindModuleInProcess (module_name, 0, modentry);
}


// returns module entry if module is found
bool CMemoryPatcher::FindModule (DWORD pid, DWORD address, MODULEENTRY32 * modentry)
{
    return ModulesLookup(pid, 0, address, modentry);
}


// returns module entry if module is found in specified process
bool CMemoryPatcher::FindModuleInProcess (const char * module_name, DWORD pid, MODULEENTRY32 * modentry)
{
    if ( pid )
        return ModulesLookup(pid, module_name, 0, modentry);

    HANDLE hAllProcesses = INVALID_HANDLE_VALUE;
    PROCESSENTRY32 hProcEntry;
    hProcEntry.dwSize = sizeof(PROCESSENTRY32);

    // Get a handle to a Toolhelp snapshot of the systems processes.
    if ( (hAllProcesses = (*pfnCreateToolhelp32Snapshot) (TH32CS_SNAPPROCESS, 0)) == INVALID_HANDLE_VALUE )
    {
        Error ("CreateToolhelp32Snapshot: %s", strerror(errno));
        return false;
    }

    if ( (*pfnProcess32First) (hAllProcesses, &hProcEntry) )
    {
        // Look in processes
        for ( ; ; )
        {
            if ( hProcEntry.th32ProcessID != 0 )
            {
#if defined (PATCHER_DEBUG)
                Msg ("Process: %d, %s\n", hProcEntry.th32ProcessID, hProcEntry.szExeFile);
#endif
                if ( ModulesLookup (hProcEntry.th32ProcessID, module_name, 0, modentry) )
                {
#if defined (PATCHER_DEBUG)
                    Msg ("Found module: %s, base addr %08X, length %08X\n",
                        modentry->szModule,
                        modentry->modBaseAddr,
                        modentry->modBaseSize );
#endif
                    FreeProcessHandle (hAllProcesses);
                    return true;
                }
            }

            hProcEntry.dwSize = sizeof(PROCESSENTRY32);

            if ( !(*pfnProcess32Next) (hAllProcesses, &hProcEntry) )
                break;
        }
    }

    FreeProcessHandle (hAllProcesses);
    return false;
}

// returns module entry if module is found in specified process
bool CMemoryPatcher::FindModuleInProcessLast (const char * module_name, DWORD pid, MODULEENTRY32 * modentry)
{
    if ( pid )
        return ModulesLookupLast(pid, module_name, 0, modentry);

    return false;
}


// returns process entry if found
bool CMemoryPatcher::FindProcess (const char * process_name, PROCESSENTRY32 * procentry)
{
    char needed_name[256], module_name[256];
    HANDLE hAllProcesses = INVALID_HANDLE_VALUE;

    strcpy (needed_name, process_name);
    strupr (needed_name);
    procentry->dwSize = sizeof(PROCESSENTRY32);

    // Get a handle to a Toolhelp snapshot of the systems processes.
    if ( (hAllProcesses = (*pfnCreateToolhelp32Snapshot) (TH32CS_SNAPPROCESS, 0)) == INVALID_HANDLE_VALUE )
    {
        Error ("CreateToolhelp32Snapshot: %s", strerror(errno));
        return false;
    }

    if ( (*pfnProcess32First) (hAllProcesses, procentry) )
    {
        // Look in processes
        for ( ; ; )
        {
            if ( procentry->th32ProcessID != 0 )
            {
#if defined (PATCHER_DEBUG)
                Msg ("Process: %d, %s\n", procentry->th32ProcessID, procentry->szExeFile);
#endif
                strcpy (module_name, procentry->szExeFile);
                strupr (module_name);

                if ( strstr(module_name, needed_name) )
                {
                    FreeProcessHandle (hAllProcesses);
                    return true;
                }
            }

            procentry->dwSize = sizeof(PROCESSENTRY32);

            if ( !(*pfnProcess32Next) (hAllProcesses, procentry) )
                break;
        }
    }

    FreeProcessHandle (hAllProcesses);
    return false;
}


const char * CMemoryPatcher::GetErrorText()
{
    return szErrorMsg;
}



bool CMemoryPatcher::ModulesLookup (DWORD pid, const char * name, DWORD addr, MODULEENTRY32 * modentry)
{
    char needed_name[256], module_name[256];
    HANDLE hAllModules = INVALID_HANDLE_VALUE;
    bool fFound = false;

    if ( name )
    {
        strcpy (needed_name, name);
        strupr (needed_name);
    }

    if ( (hAllModules = (*pfnCreateToolhelp32Snapshot) (TH32CS_SNAPMODULE, pid)) == INVALID_HANDLE_VALUE )
        return false;

    modentry->dwSize = sizeof(MODULEENTRY32);

    if ( (*pfnModule32First) (hAllModules, modentry) )
    {
        // Look in all the modules
        for ( ; fFound == false; )
        {
#if defined (PATCHER_DEBUG)
            Msg ("  Module: %08X, %08X, %s\n", modentry->hModule, modentry->modBaseAddr, modentry->szModule);
#endif
            modentry->dwSize = sizeof(MODULEENTRY32);
    
            if ( name )
            {
                strcpy (module_name, modentry->szModule);
                strupr (module_name);

                //if ( strstr(module_name, needed_name) )
                if(!stricmp(module_name, needed_name) )
                {
                    FreeProcessHandle (hAllModules);
                    return true;
                }
            }
            else
            {
                if ( addr >= (DWORD) modentry->modBaseAddr 
                &&  addr <= ((DWORD) modentry->modBaseAddr + modentry->modBaseSize) )
                {
                    FreeProcessHandle (hAllModules);
                    return true;
                }
            }

            if ( !(*pfnModule32Next) (hAllModules, modentry) )
                break;
        }
    }

    FreeProcessHandle (hAllModules);    
    return false;
}

bool CMemoryPatcher::ModulesLookupLast (DWORD pid, const char * name, DWORD addr, MODULEENTRY32 * modentry)
{
    char needed_name[256], module_name[256];
    HANDLE hAllModules = INVALID_HANDLE_VALUE;
    bool fFound = false;

    if ( name )
    {
        strcpy (needed_name, name);
        strupr (needed_name);
    }

    if ( (hAllModules = (*pfnCreateToolhelp32Snapshot) (TH32CS_SNAPMODULE, pid)) == INVALID_HANDLE_VALUE )
        return false;

    modentry->dwSize = sizeof(MODULEENTRY32);

    MODULEENTRY32 cur;
    if ( (*pfnModule32First) (hAllModules, &cur) )
    {
        // Look in all the modules
        for ( ;; )
        {
#if defined (PATCHER_DEBUG)
            Msg ("  Module: %08X, %08X, %s\n", modentry->hModule, modentry->modBaseAddr, modentry->szModule);
#endif
            cur.dwSize = sizeof(MODULEENTRY32);

            if ( name )
            {
                strcpy (module_name, cur.szModule);
                strupr (module_name);

                //if ( strstr(module_name, needed_name) )
                if(!stricmp(module_name, needed_name) )
                {
                    memcpy(modentry, &cur, sizeof(MODULEENTRY32));
                    fFound = true;
                }
            }
            else
            {
                if ( addr >= (DWORD) cur.modBaseAddr 
                &&  addr <= ((DWORD) cur.modBaseAddr + cur.modBaseSize) )
                {
                    memcpy(modentry, &cur, sizeof(MODULEENTRY32));
                    fFound = true;
                }
            }

            if ( !(*pfnModule32Next) (hAllModules, &cur) )
                break;
        }
    }

    FreeProcessHandle (hAllModules);
    return fFound;
}



DWORD CMemoryPatcher::ReadProcess (DWORD pid, void * addr, BYTE * buf, DWORD length)
{
    DWORD returnlen = 0;
    HANDLE hProcess = OpenProcess (PROCESS_VM_OPERATION|PROCESS_VM_READ|PROCESS_VM_WRITE, FALSE, pid);

    if ( hProcess )
    {
        returnlen = ReadFromProcessToBuf(hProcess, (void*) addr, buf, length);
        CloseHandle (hProcess);
    }   
        
    return returnlen;
}


DWORD CMemoryPatcher::WriteProcess (DWORD pid, void * addr, BYTE * buf, DWORD length)
{
    DWORD old_attributes, old_attributes2, writelen, returnlen = 0;
    HANDLE hProcess = OpenProcess (PROCESS_VM_OPERATION|PROCESS_VM_READ|PROCESS_VM_WRITE, FALSE, pid);

    if ( hProcess )
    {
        if ( VirtualProtectEx(hProcess, addr, length, PAGE_READWRITE, &old_attributes) )
        {
            if ( WriteProcessMemory (hProcess, addr, buf, length, &writelen) )
            {
                // Restore old page protection mask
                if ( VirtualProtectEx(hProcess, addr, length, old_attributes, &old_attributes2) )
                {
                    returnlen = writelen;
                }
                else
                    Error ("VirtualProtectEx error: %d", GetLastError());
            }
            else
                Error ("WriteProcessMemory error: %d", GetLastError());
        }
        else
            Error ("VirtualProtectEx error: %d", GetLastError());
    
        CloseHandle (hProcess);
    }
    else
        Error ("OpenProcess error: %s", strerror(errno));

    return returnlen;
}


DWORD CMemoryPatcher::FindPattern (DWORD pid, BYTE * pattern, DWORD pattern_length, DWORD start_addr, DWORD process_len)
{
    DWORD found_addr = 0;

    HANDLE hProcess = OpenProcess (PROCESS_VM_OPERATION|PROCESS_VM_READ|PROCESS_VM_WRITE, FALSE, pid);
    BYTE * tempbuf = new BYTE [PAGE_SIZE];

    if ( hProcess )
    {
        for ( DWORD current = 0; current < process_len && found_addr == 0; )
        {
            DWORD amount = process_len - current > PAGE_SIZE ? PAGE_SIZE : process_len - current;

            if ( !ReadFromProcessToBuf(hProcess, (void*) (start_addr + current), tempbuf, amount) )
            {
                break;
            }

            for ( unsigned int i = 0; i < amount - pattern_length; i++ )
            {
                if ( !memcmp (tempbuf+i, pattern, pattern_length) )
                {
                    found_addr = start_addr + current + i;
                    break;
                }
            }

            current += amount;
        }
    
        CloseHandle (hProcess);
    }
    else
        Error ("OpenProcess error: %s", strerror(errno));

    delete tempbuf;
    return found_addr;
}



DWORD CMemoryPatcher::ReadFromProcessToBuf (HANDLE hProcess, void * addr, BYTE * buf, DWORD length)
{
    DWORD returnlen = 0, readlen, old_attributes, old_attributes2;

    if ( VirtualProtectEx(hProcess, addr, length, PAGE_READWRITE, &old_attributes) )
    {
        if ( ReadProcessMemory (hProcess, addr, buf, length, &readlen) )
        {
            // Restore old page protection mask
            if ( VirtualProtectEx(hProcess, addr, length, old_attributes, &old_attributes2) )
            {
                returnlen = readlen;
            }
            else
                Error ("VirtualProtectEx error: %d", GetLastError());
        }
        else
            Error ("ReadProcessMemory error: %d", GetLastError());
    }
    else
        Error ("VirtualProtectEx error: %d", GetLastError());

    return returnlen;
}


bool CMemoryPatcher::WriteProcessByte (DWORD pid, void * addr, BYTE value)
{
    BYTE buf[1];

    buf[0] = value;
    return (WriteProcess (pid, addr, buf, 1) == 1 ? true : false);
}


bool CMemoryPatcher::ReplaceByte (DWORD pid, void * addr, BYTE origvalue, BYTE newvalue)
{
    BYTE buf[1];

    if ( ReadProcess (pid, addr, buf, 1) != 1 )
        return false;

    if ( buf[0] != origvalue )
    {
        Error ("ReplaceByte: wrong original byte: %02X (%02X expected)", buf[0], origvalue);
        return false;
    }

    buf[0] = newvalue;
    return (WriteProcess (pid, addr, buf, 1) == 1 ? true : false);
}


bool CMemoryPatcher::Dump (const char * filename, DWORD pid, void * addr, DWORD length)
{
    BYTE tmpbuf[PAGE_SIZE];
    HANDLE hProcess = OpenProcess (PROCESS_VM_OPERATION|PROCESS_VM_READ, FALSE, pid);

    if ( !hProcess )
    {
        Error ("OpenProcess error: %s", strerror(errno));
        return false;
    }

    FILE * fp = fopen (filename, "wb");

    if ( !fp )
    {
        Error ("Couldn't open file %s: %s", filename, strerror(errno));
        return false;
    }

    for ( DWORD current = 0; current < length; )
    {
        DWORD amount = length - current > PAGE_SIZE ? PAGE_SIZE : length - current;

        if ( !ReadFromProcessToBuf(hProcess, (void*) ((DWORD) addr + current), tmpbuf, amount) )
            break;

        fwrite (tmpbuf, 1, amount, fp);
        current += amount;
    }

    CloseHandle (hProcess);
    fclose (fp);

    return true;        
}


bool CMemoryPatcher::FindPeDirectory (MODULEENTRY32 * modentry, DWORD entry, IMAGE_DATA_DIRECTORY * dentry)
{
    IMAGE_DOS_HEADER dosHeader;
    IMAGE_NT_HEADERS NTHeader;
    bool bFound = false;

    HANDLE hProcess = OpenProcess (PROCESS_VM_OPERATION|PROCESS_VM_READ, FALSE, modentry->th32ProcessID);

    if ( !hProcess )
    {
        Error ("OpenProcess error: %s", strerror(errno));
        return false;
    }

    if ( ReadFromProcessToBuf (hProcess, modentry->modBaseAddr, (BYTE *) &dosHeader, sizeof(IMAGE_DOS_HEADER)) == sizeof(IMAGE_DOS_HEADER)      // Read MSDOS header
    && ReadFromProcessToBuf (hProcess, modentry->modBaseAddr + dosHeader.e_lfanew, (BYTE *) &NTHeader, sizeof(IMAGE_NT_HEADERS)) == sizeof(IMAGE_NT_HEADERS)    // Read PE header and optional header
    && NTHeader.OptionalHeader.DataDirectory[entry].VirtualAddress
    && NTHeader.OptionalHeader.DataDirectory[entry].Size )
    {
        dentry->VirtualAddress = NTHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress;
        dentry->Size = NTHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size;
        bFound = true;
    }

    CloseHandle (hProcess);
    return bFound;
}


bool CMemoryPatcher::FindImportTable (MODULEENTRY32 * modentry, DWORD funcaddr, DWORD * start, DWORD * end)
{
    MODULEENTRY32       modfind;
    IMAGE_DATA_DIRECTORY imptable;

    *start = 0;
    *end = 0;

    DWORD search_addr = (DWORD) modentry->modBaseAddr;
    DWORD search_size = modentry->modBaseSize;
    DWORD import_start = 0, import_end = 0;

    FindPeDirectory (modentry, IMAGE_DIRECTORY_ENTRY_IAT, &imptable);

    if ( imptable.Size )
    {
        import_start = (DWORD) modentry->modBaseAddr + imptable.VirtualAddress;
        import_end = import_start + imptable.Size;
    }

    while ( search_size > 0 )
    {
        // Try to found the import address
        DWORD pattern = FindPattern (modentry->th32ProcessID, (BYTE *) &funcaddr, sizeof(DWORD), search_addr, search_size);

        // No pattern at all
        if ( !pattern )
        {
            Error ("Address is not found in module address space");
            return false;
        }

        // Does the address belong to the original import table?
        if ( import_start
        && pattern >= import_start
        && pattern < import_end )
        {
            // Skip it
            search_size = pattern + 4 - search_addr;
            search_addr = pattern + 4;
            continue;
        }

        // Scan down while the address belongs to a loaded module
        DWORD * iatptr, iataddr;

        for ( iatptr = (DWORD *) pattern; ReadProcess (modentry->th32ProcessID, iatptr, (BYTE*) &iataddr, sizeof(iataddr)); iatptr-- )
        {
            if ( !iataddr )
                continue;

            if ( !FindModule (GetCurrentProcessId(), iataddr, &modfind) )
                break;

            *start = (DWORD) iatptr;
        }

        for ( iatptr = (DWORD *) pattern; ReadProcess (modentry->th32ProcessID, iatptr, (BYTE*) &iataddr, sizeof(iataddr)); iatptr++ )
        {
            if ( !iataddr )
                continue;

            if ( !FindModule (GetCurrentProcessId(), iataddr, &modfind) )
                break;

            *end = (DWORD) iatptr;
        }
        
        return true;
    }

    return false;
}


DWORD CMemoryPatcher::FindPatternWithMask (DWORD pid, BYTE * pattern, BYTE * mask, DWORD pattern_length, DWORD start_addr, DWORD process_len)
{
    DWORD found_addr = 0;

    HANDLE hProcess = OpenProcess (PROCESS_VM_OPERATION|PROCESS_VM_READ|PROCESS_VM_WRITE, FALSE, pid);
    BYTE * tempbuf = new BYTE [PAGE_SIZE];

    if ( hProcess )
    {
        for ( DWORD current = 0; current < process_len && found_addr == 0; )
        {
            DWORD amount = process_len - current > PAGE_SIZE ? PAGE_SIZE : process_len - current;

            if ( !ReadFromProcessToBuf(hProcess, (void*) (start_addr + current), tempbuf, amount) )
                break;

            for ( unsigned int i = 0; i < amount - pattern_length; i++ )
            {
                if ( CompareMemoryMask (tempbuf+i, pattern, mask, pattern_length) )
                {
                    found_addr = start_addr + current + i;
                    break;
                }
            }

            current += amount;
        }
    
        CloseHandle (hProcess);
    }
    else
        Error ("OpenProcess error: %s", strerror(errno));

    delete tempbuf;
    return found_addr;
}


bool CMemoryPatcher::CompareMemoryMask (BYTE * buf, BYTE * pattern, BYTE * mask, DWORD pattern_length)
{
    for ( unsigned int i = 0; i < pattern_length; i++ )
        if ( pattern[i] != (buf[i] & mask[i]) )
            return false;

    return true;
}


// Don't change it unless you know what you're doing
#define CUSTOM_CODE_SIGNATURE   "unable to initialize heap"


void * CMemoryPatcher::AllocProcessMemory (DWORD pid, DWORD size)
{
    if ( pfnVirtualAllocEx )
    {
        HANDLE hProcess = OpenProcess (PROCESS_VM_OPERATION|PROCESS_VM_READ|PROCESS_VM_WRITE, FALSE, pid);

        if ( !hProcess )
        {
            Error ("OpenProcess error: %s", strerror(errno));
            return 0;
        }

        void * addr = pfnVirtualAllocEx (hProcess, 0, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
        CloseHandle (hProcess);
        
        return addr;
    }
    else
    {
        // In Windows 9X, create a small memory mapped file.  On this
        // platform, memory mapped files are above 2GB, and thus are
        // accessible by all processes.
        // Taken from DelayLoad (C) Matt Pietrek
        HANDLE hFileMapping = CreateFileMapping (INVALID_HANDLE_VALUE, 0,
                                    PAGE_READWRITE | SEC_COMMIT, 0,
                                    size, 0);
        if ( !hFileMapping )
        {
            Error ("CreateFileMapping error: %s", strerror(errno));
            return 0;
        }

        return (void *) MapViewOfFile (hFileMapping, FILE_MAP_WRITE, 0, 0, size);
    }

    return 0;
}
