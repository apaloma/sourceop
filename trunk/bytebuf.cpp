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

#include "bytebuf.h"

#include <string.h>

#ifdef _WIN32
#include <Winsock2.h>
#else
#include <arpa/inet.h>
#endif
#include <string.h>

#include "tier0/memdbgon.h"


ByteBufRead::ByteBufRead(void *pData, int bytes)
{
    m_pData = (unsigned char *)pData;
    m_iMaxBytes = bytes;
    m_iPos = 0;

    m_bOverflow = false;
}

int ByteBufRead::ReadByte()
{
    if(m_iPos >= m_iMaxBytes)
    {
        m_bOverflow = true;
        return 0;
    }
    return m_pData[m_iPos++];
}

unsigned short ByteBufRead::PeekUShort()
{
    if(m_iPos+2 > m_iMaxBytes)
    {
        m_bOverflow = true;
        return 0;
    }

    unsigned short ret;
    unsigned char *pRet = (unsigned char *)&ret;
    pRet[0] = (m_pData[m_iPos]);
    pRet[1] = (m_pData[m_iPos+1]);
#if defined(NETWORK_ORDERED)
    ret = ntohs(ret);
#endif
    return ret;
}

unsigned short ByteBufRead::ReadUShort()
{
    if(m_iPos+2 > m_iMaxBytes)
    {
        m_bOverflow = true;
        return 0;
    }

    unsigned short ret;
    unsigned char *pRet = (unsigned char *)&ret;
    pRet[0] = (m_pData[m_iPos]);
    pRet[1] = (m_pData[m_iPos+1]);
#if defined(NETWORK_ORDERED)
    ret = ntohs(ret);
#endif
    m_iPos += 2;
    return ret;
}

short ByteBufRead::ReadShort()
{
    if(m_iPos+2 > m_iMaxBytes)
    {
        m_bOverflow = true;
        return 0;
    }

    unsigned short ret;
    unsigned char *pRet = (unsigned char *)&ret;
    pRet[0] = (m_pData[m_iPos]);
    pRet[1] = (m_pData[m_iPos+1]);
#if defined(NETWORK_ORDERED)
    ret = ntohs(ret);
#endif
    m_iPos += 2;
    return *(short *)(&ret);
}

int ByteBufRead::ReadLong()
{
    if(m_iPos+4 > m_iMaxBytes)
    {
        m_bOverflow = true;
        return 0;
    }

    int ret;
    unsigned char *pRet = (unsigned char *)&ret;
    pRet[0] = (m_pData[m_iPos]);
    pRet[1] = (m_pData[m_iPos+1]);
    pRet[2] = (m_pData[m_iPos+2]);
    pRet[3] = (m_pData[m_iPos+3]);
#if defined(NETWORK_ORDERED)
    ret = ntohl(ret);
#endif
    m_iPos += 4;
    return ret;
}

unsigned int ByteBufRead::ReadULong()
{
    if(m_iPos+4 > m_iMaxBytes)
    {
        m_bOverflow = true;
        return 0;
    }

    unsigned int ret;
    unsigned char *pRet = (unsigned char *)&ret;
    pRet[0] = (m_pData[m_iPos]);
    pRet[1] = (m_pData[m_iPos+1]);
    pRet[2] = (m_pData[m_iPos+2]);
    pRet[3] = (m_pData[m_iPos+3]);
#if defined(NETWORK_ORDERED)
    ret = ntohl(ret);
#endif
    m_iPos += 4;
    return ret;
}

unsigned long long ByteBufRead::ReadULongLong()
{
    if(m_iPos+8 > m_iMaxBytes)
    {
        m_bOverflow = true;
        return 0;
    }

    unsigned long long ret;
    unsigned char *pRet = (unsigned char *)&ret;
    pRet[0] = (m_pData[m_iPos]);
    pRet[1] = (m_pData[m_iPos+1]);
    pRet[2] = (m_pData[m_iPos+2]);
    pRet[3] = (m_pData[m_iPos+3]);
    pRet[4] = (m_pData[m_iPos+4]);
    pRet[5] = (m_pData[m_iPos+5]);
    pRet[6] = (m_pData[m_iPos+6]);
    pRet[7] = (m_pData[m_iPos+7]);
#if defined(NETWORK_ORDERED)
    ret = ntohll(ret);
#endif
    m_iPos += 8;
    return ret;
}

float ByteBufRead::ReadFloat()
{
    if(m_iPos+4 > m_iMaxBytes)
    {
        m_bOverflow = true;
        return 0;
    }

    float ret;
    unsigned char *pRet = (unsigned char *)&ret;
    pRet[0] = (m_pData[m_iPos]);
    pRet[1] = (m_pData[m_iPos+1]);
    pRet[2] = (m_pData[m_iPos+2]);
    pRet[3] = (m_pData[m_iPos+3]);

    m_iPos += 4;
    return ret;
}

double ByteBufRead::ReadDouble()
{
    if(m_iPos+8 > m_iMaxBytes)
    {
        m_bOverflow = true;
        return 0;
    }

    double ret;
    unsigned char *pRet = (unsigned char *)&ret;
    pRet[0] = (m_pData[m_iPos]);
    pRet[1] = (m_pData[m_iPos+1]);
    pRet[2] = (m_pData[m_iPos+2]);
    pRet[3] = (m_pData[m_iPos+3]);
    pRet[4] = (m_pData[m_iPos+4]);
    pRet[5] = (m_pData[m_iPos+5]);
    pRet[6] = (m_pData[m_iPos+6]);
    pRet[7] = (m_pData[m_iPos+7]);

    m_iPos += 8;
    return ret;
}

bool ByteBufRead::ReadString(char *pStr, int maxLen)
{
    bool bTooSmall = false;
    int iChar = 0;
    while(1)
    {
        char val = ReadByte();
        if(val == 0)
            break;

        if(iChar < (maxLen-1))
        {
            pStr[iChar] = val;
            iChar++;
        }
        else
        {
            bTooSmall = true;
        }
    }

    pStr[iChar] = 0;
    return !IsOverflowed() && !bTooSmall;
}

bool ByteBufRead::ReadBytes(void *pOut, int nBytes)
{
    if(m_iPos + nBytes > m_iMaxBytes)
    {
        m_bOverflow = true;
        return 0;
    }
    memcpy(pOut, &m_pData[m_iPos], nBytes);
    m_iPos += nBytes;

    return !IsOverflowed();
}


ByteBufWrite::ByteBufWrite(void *pData, int maxLen)
{
    m_pData = (unsigned char *)pData;
    m_iMaxBytes = maxLen;
    m_iPos = 0;

    m_bOverflow = false;
}

void ByteBufWrite::WriteByte(int val)
{
    if(m_iPos >= m_iMaxBytes)
    {
        m_bOverflow = true;
        return;
    }
    m_pData[m_iPos++] = val;
}

void ByteBufWrite::WriteUShort(unsigned short val)
{
    if(m_iPos+2 > m_iMaxBytes)
    {
        m_bOverflow = true;
        return;
    }

#if defined(NETWORK_ORDERED)
    val = htons(val);
#endif

    unsigned char *pVal = (unsigned char *)&val;
    m_pData[m_iPos++] = pVal[0];
    m_pData[m_iPos++] = pVal[1];
}

void ByteBufWrite::WriteShort(short val)
{
    if(m_iPos+2 > m_iMaxBytes)
    {
        m_bOverflow = true;
        return;
    }

    unsigned short tmp;
#if defined(NETWORK_ORDERED)
    tmp= htons(*(unsigned short *)(&val));
#endif

    unsigned char *pVal = (unsigned char *)&tmp;
    m_pData[m_iPos++] = pVal[0];
    m_pData[m_iPos++] = pVal[1];
}

void ByteBufWrite::WriteLong(int val)
{
    if(m_iPos+4 > m_iMaxBytes)
    {
        m_bOverflow = true;
        return;
    }

#if defined(NETWORK_ORDERED)
    val = htonl(val);
#endif

    unsigned char *pVal = (unsigned char *)&val;
    m_pData[m_iPos++] = pVal[0];
    m_pData[m_iPos++] = pVal[1];
    m_pData[m_iPos++] = pVal[2];
    m_pData[m_iPos++] = pVal[3];
}

void ByteBufWrite::WriteULong(unsigned int val)
{
    if(m_iPos+4 > m_iMaxBytes)
    {
        m_bOverflow = true;
        return;
    }

#if defined(NETWORK_ORDERED)
    val = htonl(val);
#endif

    unsigned char *pVal = (unsigned char *)&val;
    m_pData[m_iPos++] = pVal[0];
    m_pData[m_iPos++] = pVal[1];
    m_pData[m_iPos++] = pVal[2];
    m_pData[m_iPos++] = pVal[3];
}

void ByteBufWrite::WriteULongLong(unsigned long long val)
{
    if(m_iPos+8 > m_iMaxBytes)
    {
        m_bOverflow = true;
        return;
    }

#if defined(NETWORK_ORDERED)
    val = htonll(val);
#endif

    unsigned char *pVal = (unsigned char *)&val;
    m_pData[m_iPos++] = pVal[0];
    m_pData[m_iPos++] = pVal[1];
    m_pData[m_iPos++] = pVal[2];
    m_pData[m_iPos++] = pVal[3];
    m_pData[m_iPos++] = pVal[4];
    m_pData[m_iPos++] = pVal[5];
    m_pData[m_iPos++] = pVal[6];
    m_pData[m_iPos++] = pVal[7];
}

void ByteBufWrite::WriteFloat(float val)
{
    if(m_iPos+4 > m_iMaxBytes)
    {
        m_bOverflow = true;
        return;
    }

    unsigned char *pVal = (unsigned char *)&val;
    m_pData[m_iPos++] = pVal[0];
    m_pData[m_iPos++] = pVal[1];
    m_pData[m_iPos++] = pVal[2];
    m_pData[m_iPos++] = pVal[3];
}

void ByteBufWrite::WriteDouble(double val)
{
    if(m_iPos+8 > m_iMaxBytes)
    {
        m_bOverflow = true;
        return;
    }

    unsigned char *pVal = (unsigned char *)&val;
    m_pData[m_iPos++] = pVal[0];
    m_pData[m_iPos++] = pVal[1];
    m_pData[m_iPos++] = pVal[2];
    m_pData[m_iPos++] = pVal[3];
    m_pData[m_iPos++] = pVal[4];
    m_pData[m_iPos++] = pVal[5];
    m_pData[m_iPos++] = pVal[6];
    m_pData[m_iPos++] = pVal[7];
}

bool ByteBufWrite::WriteString(const char *pStr)
{
	if(pStr)
	{
		do
		{
			WriteByte(*pStr);
		}
        while(*(pStr++) != 0);
	}
	else
	{
		WriteByte(0);
	}

	return !IsOverflowed();
}

bool ByteBufWrite::WriteBytes(const void *pBuf, int nBytes)
{
    unsigned char *pData = (unsigned char *)pBuf;
    while(nBytes)
    {
        WriteByte(*(pData++));
        nBytes--;
    }

    return !IsOverflowed();
}
