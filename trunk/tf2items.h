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

#pragma pack(push, 4)
class CEconItemAttribute
{
    DECLARE_CLASS_NOBASE( CEconItemAttribute );

public:
    DECLARE_EMBEDDED_NETWORKVAR();

    CNetworkVar( unsigned short, m_iAttribDef );
    CNetworkVar( float, m_flVal );

    CNetworkVar( float, m_flInitialValue );
    CNetworkVar( int, m_nRefundableCurrency );

    CNetworkVar( bool,	m_bSetBonus );
};

class CAttributeList
{
    DECLARE_CLASS_NOBASE( CAttributeList );

public:
    DECLARE_EMBEDDED_NETWORKVAR();
    DECLARE_DATADESC();

    CUtlVector<CEconItemAttribute> m_Attributes;
    void *m_pManager;
};

/*
Got this from the "extra data" on the send props:
_ST_m_Attributes_15\lengthproxy: 3520  204
first number is the offset
second number is the element stride
*/
class CEconItemView
{
    DECLARE_CLASS_NOBASE( CEconItemView );

public:
    DECLARE_EMBEDDED_NETWORKVAR();
    DECLARE_DATADESC();

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

    void *m_pUnk;
    bool    m_bInitialized;

    CNetworkVarEmbedded( CAttributeList, m_attributes );
};
#pragma pack(pop)

#endif
