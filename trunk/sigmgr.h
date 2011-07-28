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

#ifndef SIGMGR_H
#define SIGMGR_H

class FuncSigMgr
{
public:
    FuncSigMgr();
    ~FuncSigMgr();
    const char *GetSignature(const char *pszSigName, unsigned int *len);
    const char *GetMatchString(const char *pszSigName);
    const char *GetExtra(const char *pszSigName);
    static const char *DeEscape(const char *pszStr, unsigned int *len);
    static bool IsMatch(const char *pCheck, unsigned int addr, unsigned int len, const char *pszSig, const char *pszMatch);

private:
    KeyValues *kv;
    char gameDir[256];
};

#endif
