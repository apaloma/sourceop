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

#ifndef TF2ITEMS_H
#define TF2ITEMS_H

class ISave;
class IRestore;

#define MAX_ITEM_DESCRIPTION_LENGTH 96

#pragma pack(push, 4)
class CEconItemAttribute
{
public:
    unsigned short m_iAttribDef;
    float m_flVal;
    wchar_t m_szDescription[MAX_ITEM_DESCRIPTION_LENGTH];

    virtual void NetworkStateChanged() { }
    virtual void NetworkStateChanged( void *pVar ) { }

    // Save / Restore
    virtual void Save( ISave &pSave ) { }
    virtual void Restore( IRestore &pRestore ) { }

    // Unknown return types
    virtual void UpdateDescription()
    {
        m_szDescription[0] = '\0';
    }
    virtual const wchar_t *GetDescription()
    {
        return m_szDescription;
    }
};

/*
Got this from the "extra data" on the send props:
_ST_m_Attributes_15\lengthproxy: 3520  204
first number is the offset
second number is the element stride
*/
class CEconItemView
{
public:
    DECLARE_CLASS_NOBASE( CEconItemView );

    //int     m_Padding1;               //4
    unsigned short m_iItemDefinitionIndex;     //8   4
    int     m_iEntityQuality;           //12  8
    int     m_iEntityLevel;             //16  12
    //int     m_Padding2;               //20
    unsigned long long m_iGlobalIndex;  //24  16
    int     m_iGlobalIndexHigh;         //32  24
    int     m_iGlobalIndexLow;          //36  28
    unsigned int m_iAccountID;          // new as of 4/28/2010
    unsigned int m_iPosition;           //40  32
    wchar_t m_szWideName[128];          //44  36
    char    m_szName[128];              //300 164
    char    m_szUnk[24];
    wchar_t m_szAttributeDescription[MAX_ITEM_DESCRIPTION_LENGTH * 16]; //428 292
    void *m_pLocalizationProvider;      // new as of uber update?
    CNetworkVarEmbedded( CUtlVector<CEconItemAttribute>, m_attributes ); // 3520

    bool    m_bInitialized;

public:
    virtual void            NetworkStateChanged         ( ) { }
    virtual void            NetworkStateChanged         ( void *pVar ) { }

private:
    DECLARE_DATADESC();

public:
    virtual const char      *BuildAttributeDescription  ( bool bVar ) { return NULL; }
};
#pragma pack(pop)

class CTF2ItemsLocalizationProvider
{
    virtual wchar_t *Find ( char const *tokenName ) { return NULL; }
    virtual void ConstructString ( wchar_t *, int, wchar_t *, int, ... ) { return; }
    virtual int ConvertLoccharToANSI ( wchar_t const *, char *, int ) { return 0; }
    virtual int ConvertLoccharToUnicode ( wchar_t const *, wchar_t *, int ) { return 0; }
    virtual int ConvertUTF8ToLocchar ( char const *, wchar_t *, int ) { return 0; }
};

#endif
