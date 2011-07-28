#ifdef _WIN32
/*
 * Universal memory patcher class to patch memory of any process under Win32
 * Windows 95/98/NT/ME/2k compatible.
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
 * $Id: mempatcher.h,v 1.7 2001/11/21 12:41:11 tim Exp $
 */

#ifndef FILE_PATCHER_H
#define FILE_PATCHER_H

#include <windows.h>
#include <tlhelp32.h>
#include <winnt.h>


class CMemoryPatcher
{
    public:
        CMemoryPatcher();
        ~CMemoryPatcher();
        bool    Init();
        const char  *   GetErrorText();

        // Finding functions

        // returns module entry if module is found
        bool    FindProcess (const char * module_name, PROCESSENTRY32 * procentry);

        // returns module entry if module is found
        bool    FindModuleInProcess (const char * module_name, DWORD pid, MODULEENTRY32 * modentry);

        // returns module entry if module is found
        bool    FindModuleInProcessLast (const char * module_name, DWORD pid, MODULEENTRY32 * modentry);

        // returns module entry if module is found
        bool    FindModule (const char * module_name, MODULEENTRY32 * modentry);

        // returns module entry if module is found
        bool    FindModule (DWORD pid, DWORD address, MODULEENTRY32 * modentry);

        // returns memory address where the buffer is found or 0
        DWORD   FindPattern (DWORD pid, BYTE * pattern, DWORD pattern_length, DWORD start_addr, DWORD process_len);

        // returns memory address where the buffer is found or 0
        DWORD   FindPatternWithMask (DWORD pid, BYTE * pattern, BYTE * mask, DWORD pattern_length, DWORD start_addr, DWORD process_len);

        // Finds and returns appropriate PE data directory entry
        // IMAGE_DIRECTORY_ENTRY_EXPORT - exports
        // IMAGE_DIRECTORY_ENTRY_IAT - import table
        bool    FindPeDirectory (MODULEENTRY32 * modentry, DWORD dir, IMAGE_DATA_DIRECTORY * pedir);

        // Finds and returns a memory region that seems to be an import table
        bool    FindImportTable (MODULEENTRY32 * modentry, DWORD funcaddr, DWORD * start, DWORD * end);

        // Patching functions

        // returns number of read bytes of 0 if no bytes were read
        DWORD   ReadProcess (DWORD pid, void * addr, BYTE * buf, DWORD length);

        // returns number of read bytes of 0 if no bytes were written
        DWORD   WriteProcess (DWORD pid, void * addr, BYTE * buf, DWORD length);

        // returns number of read bytes of 0 if no bytes were written
        bool    WriteProcessByte (DWORD pid, void * addr, BYTE value);

        // returns number of read bytes of 0 if no bytes were written
        bool    ReplaceByte (DWORD pid, void * addr, BYTE origvalue, BYTE newvalue);

        // Dump a memory region
        bool    Dump (const char * filename, DWORD pid, void * addr, DWORD length);

        // Dump a memory region
        void *  AllocProcessMemory (DWORD pid, DWORD size);

    private:
        void        Error (const char * fmt, ...);
        void        FreeProcessHandle(HANDLE hSnapShot);
        bool        ModulesLookup (DWORD pid, const char * name, DWORD addr, MODULEENTRY32 * modentry);
        bool        ModulesLookupLast (DWORD pid, const char * name, DWORD addr, MODULEENTRY32 * modentry);
        DWORD       ReadFromProcessToBuf (HANDLE hProcess, void * addr, BYTE * buf, DWORD length);
        bool        ReadPEdirectoryEntry (MODULEENTRY32 * modentry, DWORD entry, IMAGE_DATA_DIRECTORY * dentry);
        bool        CompareMemoryMask (BYTE * buf, BYTE * pattern, BYTE * mask, DWORD pattern_length);

        char        szErrorMsg[512];

        // Process information functions for Win95/98/2k
        HMODULE     hKernel32;
        HANDLE      (WINAPI *pfnCreateToolhelp32Snapshot) (DWORD, DWORD);
        BOOL        (WINAPI *pfnProcess32First) (HANDLE, LPPROCESSENTRY32);
        BOOL        (WINAPI *pfnProcess32Next)  (HANDLE, LPPROCESSENTRY32);
        BOOL        (WINAPI *pfnModule32First)  (HANDLE, LPMODULEENTRY32);
        BOOL        (WINAPI *pfnModule32Next)   (HANDLE, LPMODULEENTRY32);
        void        (WINAPI *pfnCloseToolhelp32Snapshot) (HANDLE);
        LPVOID      (WINAPI *pfnVirtualAllocEx) (HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
};


#endif // FILE_PATCHER_H
#endif // WIN32
