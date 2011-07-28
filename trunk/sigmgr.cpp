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

#include "AdminOP.h"
#include "sigmgr.h"

#include <ctype.h>

#include "tier0/memdbgon.h"

extern IFileSystem      *filesystem; // file I/O 
#define SIGNATURE_FILE  "DF_funcsigs.txt"

FuncSigMgr::FuncSigMgr()
{
    int slashpos;
    char gamedir[512];

    engine->GetGameDir(gamedir, sizeof(gamedir));
    for(slashpos = strlen(gamedir) - 1; slashpos >= 0 && gamedir[slashpos] != '\\' && gamedir[slashpos] != '/'; slashpos--);
    slashpos++;
    strncpy(gameDir, &gamedir[slashpos], sizeof(gameDir));
    gameDir[sizeof(gameDir)-1] = '\0';

    kv = NULL;

    kv = new KeyValues( "signatures" );
    if ( kv )
    {
        if ( !kv->LoadFromFile( filesystem, UTIL_VarArgs("%s/"SIGNATURE_FILE, pAdminOP.DataDir()), "MOD" ) )
        {
            kv->deleteThis();
            kv = NULL;
        }
    }
}

FuncSigMgr::~FuncSigMgr()
{
    if(kv) kv->deleteThis();
}

const char *FuncSigMgr::GetSignature(const char *pszSigName, unsigned int *len)
{
    KeyValues *sigs, *thissig;

    if(!kv) return "";
    if(sigs = kv->FindKey(gameDir))
    {
        if(thissig = sigs->FindKey(pszSigName))
        {
            const char *pszSig = thissig->GetString( "Signature", NULL );
            if(pszSig) return DeEscape(pszSig, len);
        }
    }
    if(sigs = kv->FindKey("default"))
    {
        if(thissig = sigs->FindKey(pszSigName))
        {
            const char *pszSig = thissig->GetString( "Signature", NULL );
            if(pszSig) return DeEscape(pszSig, len);
        }
    }
    return "";
}

const char *FuncSigMgr::GetMatchString(const char *pszSigName)
{
    KeyValues *sigs, *thissig;

    if(!kv) return "";
    if(sigs = kv->FindKey(gameDir))
    {
        if(thissig = sigs->FindKey(pszSigName))
        {
            const char *pszSigMatch = thissig->GetString( "Match", NULL );
            if(pszSigMatch) return pszSigMatch;
        }
    }
    if(sigs = kv->FindKey("default"))
    {
        if(thissig = sigs->FindKey(pszSigName))
        {
            const char *pszSigMatch = thissig->GetString( "Match", NULL );
            if(pszSigMatch) return pszSigMatch;
        }
    }
    return "";
}

const char *FuncSigMgr::GetExtra(const char *pszSigName)
{
    KeyValues *sigs, *thissig;

    if(sigs = kv->FindKey(gameDir))
    {
        if(thissig = sigs->FindKey(pszSigName))
        {
            return thissig->GetString( "Extra", "" );
        }
    }
    if(sigs = kv->FindKey("default"))
    {
        if(thissig = sigs->FindKey(pszSigName))
        {
            return thissig->GetString( "Extra", "" );
        }
    }
    return "";
}

const char *FuncSigMgr::DeEscape(const char *pszStr, unsigned int *len)
{
    static char returnMe[1024];
    int isHex = 0;
    unsigned char hexChar = 0;
    unsigned int pos = 0;

    returnMe[pos] = '\0';
    for(int i = 0; pszStr[i] != '\0'; i++)
    {
        if(isHex)
        {
            // convert '0'-'9' to 0-9
            int num = toupper(pszStr[i]) - 0x30;
            // covert 'A'-'F' to 10-15
            if(num >= 17) num -= 7;
            if(isHex == 2)
            {
                // multiply by 16
                num = (num << 4);
            }
            hexChar += num;
            isHex--;
            if(isHex == 0)
            {
                returnMe[pos] = hexChar;
                hexChar = 0;
                pos++;
            }
        }
        else
        {
            switch(pszStr[i])
            {
                case '\\':
                {
                    if(pszStr[i+1] == 'x' || pszStr[i+1] == 'X')
                    {
                        i++;
                        isHex = 2;
                        break;
                    }
                }
                default:
                {
                    returnMe[pos] = pszStr[i];
                    pos++;
                }
            }
        }
    }
    if(len) *len = pos;
    return returnMe;
}

bool FuncSigMgr::IsMatch(const char *pCheck, unsigned int addr, unsigned int len, const char *pszSig, const char *pszMatch)
{
    for(unsigned int i = 0; i<len; i++)
    {
        if(pCheck[addr+i] != pszSig[i] && (pszMatch[i] == 'x' || pszMatch[i] == 'X'))
        {
            return 0;
        }
    }
    return 1;
}
