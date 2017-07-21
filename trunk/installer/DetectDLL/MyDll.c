#include <windows.h>

int __stdcall GetSteamPath(char *pszReturn)
{
    long RegResult;
    static char steamexe[255];
    ULONG lpType;
    ULONG lpSize = sizeof(steamexe);
    HKEY hCurKey;
    unsigned int pos;

    steamexe[0] = '\0';

    RegResult = RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Valve\\Steam", 0, KEY_QUERY_VALUE, &hCurKey);
    RegResult = RegQueryValueEx(hCurKey, "SteamExe", NULL, &lpType, (LPBYTE)&steamexe, &lpSize);

    if(RegResult == ERROR_SUCCESS)
    {
        pos = strlen(steamexe) - 1;

        // scan backwards till first directory separator...
        while ((pos) && (steamexe[pos] != '/') && (steamexe[pos] != '\\'))
            pos--;

        steamexe[pos] = '\0';

        pos = 0;
        while(pos<=strlen(steamexe))
        {
            if(steamexe[pos] == '/') steamexe[pos] = '\\';
            pos++;
        }

        strncpy(pszReturn, steamexe, 255);
        pszReturn[254] = '\0';

        //MessageBox(NULL, steamexe, "Returning...", MB_OK);
        return strlen(steamexe);
    }
    else
    {
        //MessageBox(NULL, steamexe, "Returning nothin", MB_OK);
        steamexe[0] = '\0';
        return 0;
    }
}

int __stdcall GetSteamUserPath(char *pszReturn)
{
    long RegResult;
    static char steamexe[255];
    ULONG lpType;
    ULONG lpSize = sizeof(steamexe);
    HKEY hCurKey;
    unsigned int pos;

    steamexe[0] = '\0';

    RegResult = RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\Valve\\Steam", 0, KEY_QUERY_VALUE, &hCurKey);
    RegResult = RegQueryValueEx(hCurKey, "ModInstallPath", NULL, &lpType, (LPBYTE)&steamexe, &lpSize);

    if(RegResult == ERROR_SUCCESS && steamexe[0])
    {
        pos = strlen(steamexe) - 1;

        // scan backwards till first directory separator...
        while ((pos) && (steamexe[pos] != '/') && (steamexe[pos] != '\\'))
            pos--;

        steamexe[pos] = '\0';

        pos = 0;
        while(pos<=strlen(steamexe))
        {
            if(steamexe[pos] == '/') steamexe[pos] = '\\';
            pos++;
        }

        strncpy(pszReturn, steamexe, 255);
        pszReturn[254] = '\0';

        return strlen(steamexe);
    }
    else
    {
        steamexe[0] = '\0';
        return 0;
    }
}
